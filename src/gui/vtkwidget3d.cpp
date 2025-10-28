/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget3d.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the VTK widget responsible for volume rendering in 3D layouts.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in
 *      all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 * ------------------------------------------------------------------------------------
 */

#include "vtkwidget3d.h"
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <algorithm>
#include <vtkWidgetRepresentation.h>
#include <vtkVolumeProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkContourValues.h>
#include <vtkImageData.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCamera.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkBoxRepresentation.h>
#include <vtkBoxWidget2.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <vtkDataSetAttributes.h>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QByteArray>
#include <QString>
#include <algorithm>
#include <cmath>
#include <cstring>

Q_LOGGING_CATEGORY(lcVtkWidget3D, "isis.gui.vtkwidget3d")

namespace
{
        constexpr double uniformRangeThreshold = 1e-5;

        struct GhostArrayInfo
        {
                bool present = false;
                vtkIdType tuples = 0;
                int minValue = 0;
                int maxValue = 0;
                bool anyHidden = false;
                bool allHidden = false;
        };

        GhostArrayInfo analyzeGhostArray(vtkUnsignedCharArray* array)
        {
                GhostArrayInfo info;
                if (!array)
                {
                        return info;
                }
                info.present = true;
                info.tuples = array->GetNumberOfTuples();
                double range[2] = {0.0, 0.0};
                array->GetRange(range, 0);
                info.minValue = static_cast<int>(std::lround(range[0]));
                info.maxValue = static_cast<int>(std::lround(range[1]));
                info.anyHidden = (info.maxValue & vtkDataSetAttributes::HIDDENCELL) != 0;
                info.allHidden = info.anyHidden && ((info.minValue & vtkDataSetAttributes::HIDDENCELL) != 0);
                return info;
        }

        bool isUniformRange(double minValue, double maxValue)
        {
                const double span = maxValue - minValue;
                return !std::isfinite(minValue) || !std::isfinite(maxValue) || std::abs(span) <= uniformRangeThreshold;
        }

        void logGhostArrays(vtkImageData* image, const char* context)
        {
                if (!image)
                {
                        qCInfo(lcVtkWidget3D) << context << "ghostCheck imageData=nullptr";
                        return;
                }

                auto describeArray = [context](vtkUnsignedCharArray* array, const char* label)
                {
                        const auto info = analyzeGhostArray(array);
                        if (!info.present)
                        {
                                qCInfo(lcVtkWidget3D) << context << label << "ghostArray absent";
                                return;
                        }
                        qCInfo(lcVtkWidget3D) << context
                                              << label
                                              << "ghostTuples" << info.tuples
                                              << "valueRange" << info.minValue << info.maxValue
                                              << "anyHidden" << info.anyHidden
                                              << "allHidden" << info.allHidden;
                };

                qCInfo(lcVtkWidget3D) << context << "imageHasGhostCells" << image->HasAnyGhostCells();
                auto* cellGhosts = image->GetCellData()
                        ? vtkUnsignedCharArray::SafeDownCast(
                                image->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()))
                        : nullptr;
                auto* pointGhosts = image->GetPointData()
                        ? vtkUnsignedCharArray::SafeDownCast(
                                image->GetPointData()->GetArray(vtkDataSetAttributes::GhostArrayName()))
                        : nullptr;
                describeArray(cellGhosts, "cell");
                describeArray(pointGhosts, "point");
        }

        bool mitigateBlankingGhostArrays(vtkImageData* image, const char* context)
        {
                if (!image)
                {
                        return false;
                }
                bool removed = false;
                auto removeIfAllHidden = [&](vtkDataSetAttributes* data, const char* label)
                {
                        if (!data)
                        {
                                return;
                        }
                        auto* array = vtkUnsignedCharArray::SafeDownCast(
                                data->GetArray(vtkDataSetAttributes::GhostArrayName()));
                        const auto info = analyzeGhostArray(array);
                        if (info.present && info.allHidden)
                        {
                                data->RemoveArray(vtkDataSetAttributes::GhostArrayName());
                                removed = true;
                                qCWarning(lcVtkWidget3D) << context
                                                         << label
                                                         << "ghost array removed (all tuples hidden)"
                                                         << info.tuples;
                        }
                };

                removeIfAllHidden(image->GetCellData(), "cell");
                removeIfAllHidden(image->GetPointData(), "point");
                return removed;
        }

        vtkDataArray* ensureActiveScalars(vtkImageData* image, const char* context)
        {
                if (!image)
                {
                        qCWarning(lcVtkWidget3D) << context << "ensureActiveScalars() imageData=nullptr";
                        return nullptr;
                }
                auto* const pointData = image->GetPointData();
                if (!pointData)
                {
                        qCWarning(lcVtkWidget3D) << context << "pointData unavailable while activating scalars.";
                        return nullptr;
                }
                auto* scalars = pointData->GetScalars();
                if (scalars)
                {
                        return scalars;
                }

                const int arrayCount = pointData->GetNumberOfArrays();
                for (int index = 0; index < arrayCount; ++index)
                {
                        auto* candidate = pointData->GetArray(index);
                        if (!candidate)
                        {
                                continue;
                        }
                        const char* candidateName = candidate->GetName();
                        if (candidateName && std::strcmp(candidateName, vtkDataSetAttributes::GhostArrayName()) == 0)
                        {
                                continue;
                        }
                        if (candidate->GetNumberOfComponents() <= 0)
                        {
                                continue;
                        }
                        pointData->SetScalars(candidate);
                        scalars = candidate;
                        qCWarning(lcVtkWidget3D)
                                << context
                                << "activated scalar array"
                                << (candidateName ? candidateName : "unnamed")
                                << "components"
                                << candidate->GetNumberOfComponents();
                        break;
                }

                if (!scalars)
                {
                        qCCritical(lcVtkWidget3D) << context << "no suitable scalar array available for activation.";
                }
                return scalars;
        }
}

std::string isis::gui::vtkWidget3D::seriesCacheKey() const
{
        if (!m_series)
        {
                return {};
        }
        return m_series->getUID();
}

isis::gui::TransferFunction::Preset isis::gui::vtkWidget3D::cachedPresetForSeries() const
{
        const auto key = seriesCacheKey();
        if (key.empty())
        {
                return TransferFunction::Preset::Automatic;
        }
        const auto it = m_seriesPresetCache.find(key);
        if (it != m_seriesPresetCache.end())
        {
                return it->second;
        }
        return TransferFunction::Preset::Automatic;
}

void isis::gui::vtkWidget3D::rememberPresetForSeries(const TransferFunction::Preset preset)
{
        const auto key = seriesCacheKey();
        if (key.empty())
        {
                return;
        }
        m_seriesPresetCache[key] = preset;
}

void isis::gui::vtkWidget3D::syncBoxWidgetWithVolume()
{
        if (!m_volume)
        {
                return;
        }

        const bool wasEnabled = m_boxWidget && m_boxWidget->GetEnabled();
        if (!m_boxWidget)
        {
                initBoxWidget();
        }
        if (!m_boxWidget)
        {
                return;
        }

        if (auto* representation = vtkBoxRepresentation::SafeDownCast(m_boxWidget->GetRepresentation()))
        {
                representation->PlaceWidget(m_volume->GetBounds());
        }
        else if (m_boxWidget->GetRepresentation())
        {
                m_boxWidget->GetRepresentation()->SetPlaceFactor(1.0);
                m_boxWidget->GetRepresentation()->PlaceWidget(m_volume->GetBounds());
        }

        if (m_boxWidgetCallback)
        {
                m_boxWidgetCallback->setVolume(m_volume);
        }

        if (wasEnabled)
        {
                m_boxWidget->SetEnabled(1);
        }
}

void isis::gui::vtkWidget3D::initWidget()
{
        m_mapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
        m_renderer = vtkSmartPointer<vtkRenderer>::New();
        m_volume = vtkSmartPointer<vtkVolume>::New();
        m_transferFunction = std::make_unique<TransferFunction>();
        m_transferFunction->initializeDefaultCurve();
        qCInfo(lcVtkWidget3D) << "Initialized vtkWidget3D components"
                              << "renderWindow" << static_cast<void*>(nullptr)
                              << "mapper" << m_mapper.GetPointer()
                              << "renderer" << m_renderer.GetPointer()
                              << "volume" << m_volume.GetPointer();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::initBoxWidget()
{
	if (!m_renderer || !m_volume)
	{
		return;
	}
        vtkRenderWindow* renderWindow = nullptr;
        if (m_renderWindows[0])
        {
                renderWindow = m_renderWindows[0].GetPointer();
        }
        else if (m_renderer->GetRenderWindow())
        {
                renderWindow = m_renderer->GetRenderWindow();
        }
        if (!renderWindow)
        {
                qCWarning(lcVtkWidget3D) << "initBoxWidget() aborted - render window unavailable.";
                return;
        }
        auto* interactor = renderWindow->GetInteractor();
        if (!interactor)
        {
                qCWarning(lcVtkWidget3D) << "initBoxWidget() aborted - interactor unavailable.";
                return;
        }
	m_boxWidget = vtkSmartPointer<vtkBoxWidget2>::New();
	m_boxWidget->SetInteractor(interactor);
	m_boxWidget->CreateDefaultRepresentation();
	m_boxWidget->GetRepresentation()->SetPlaceFactor(1);
	m_boxWidget->GetRepresentation()->PlaceWidget(m_volume->GetBounds());
	initBoxWidgetCallback();
}

void isis::gui::vtkWidget3D::onRenderWindowAssigned(const vtkSmartPointer<vtkRenderWindow>& window)
{
        if (!window)
        {
                return;
        }
        window->OffScreenRenderingOff();
        window->SetOffScreenRendering(0);
        window->SetUseOffScreenBuffers(false);
        window->SwapBuffersOn();
        window->SetDoubleBuffer(true);
        if (auto* genericWindow = vtkGenericOpenGLRenderWindow::SafeDownCast(window.GetPointer()))
        {
                genericWindow->SetUseOffScreenBuffers(false);
                genericWindow->SetOffScreenRendering(0);
        }
        qCInfo(lcVtkWidget3D)
                << "onRenderWindowAssigned() configured render window"
                << static_cast<void*>(window.GetPointer());
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::initBoxWidgetCallback()
{
	m_boxWidgetCallback = vtkSmartPointer<vtkBoxWidget3DCallback>::New();
	m_boxWidgetCallback->setVolume(m_volume);
	m_boxWidget->AddObserver(vtkCommand::InteractionEvent, m_boxWidgetCallback);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::initInteractorStyle()
{
	m_interactorStyle =
		vtkSmartPointer<vtkWidget3DInteractorStyle>::New();
	m_interactorStyle->setWidget(this);
	m_interactorStyle->setTransferFunction(m_transferFunction.get());
	m_interactor->SetInteractorStyle(m_interactorStyle);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::setVolumeMapperBlend() const
{
	m_mapper->SetBlendMode(vtkVolumeMapper::COMPOSITE_BLEND);
	
	// ✅ CORREÇÃO: Configurar mapper conforme VTK best practices
	m_mapper->SetAutoAdjustSampleDistances(1);  // Ajuste automático para qualidade/performance
	// Sample distance menor = melhor qualidade (0.5 a 1.0 é bom)
	// m_mapper->SetSampleDistance(0.5);  // Pode descomentar para forçar sample distance
}

//-----------------------------------------------------------------------------
std::shared_ptr<isis::gui::VtkDicomVolume> isis::gui::vtkWidget3D::acquireVolume(QString* failureReason) const
{
        QString localFailure;
        std::shared_ptr<VtkDicomVolume> volume;

        if (m_image && m_image->getIsMultiFrame())
        {
                volume = m_volumeLoader.loadFromImage(*m_image, &localFailure);
        }
        else if (m_series)
        {
                volume = m_volumeLoader.loadFromSeries(*m_series, &localFailure);
        }

        if (!volume || !volume->ImageData)
        {
                if (localFailure.isEmpty())
                {
                        localFailure = QStringLiteral("Volume data unavailable for the selected dataset.");
                }
        }
        else
        {
                localFailure.clear();
        }

        if (failureReason)
        {
                *failureReason = localFailure;
        }
        return volume;
}

std::tuple<int, int> isis::gui::vtkWidget3D::getWindowLevel(const std::shared_ptr<VtkDicomVolume>& volume) const
{
        if (!volume || !volume->ImageData)
        {
                return std::make_tuple(0, 0);
        }
        double window = volume->WindowWidth;
        double level = volume->WindowCenter;
        double range[2] = {0.0, 0.0};
        auto* const scalars = volume->ImageData->GetPointData()
                ? volume->ImageData->GetPointData()->GetScalars()
                : nullptr;
        if (scalars)
        {
                scalars->GetRange(range);
        }
        const bool windowInvalid = !std::isfinite(window) || window <= 0.0;
        const bool levelInvalid = !std::isfinite(level);
        if (windowInvalid || levelInvalid)
        {
                if (scalars)
                {
                        window = range[1] - range[0];
                        level = 0.5 * (range[1] + range[0]);
                        qCInfo(lcVtkWidget3D) << "getWindowLevel() derived from scalar range" << range[0] << range[1];
                }
        }
        qCInfo(lcVtkWidget3D) << "getWindowLevel()" << "center" << level << "width" << window << "range" << range[0]
                              << range[1] << "imageIdx" << (m_image ? m_image->getIndex() : -1);
        return std::make_tuple(static_cast<int>(std::round(window)), static_cast<int>(std::round(level)));
}

void isis::gui::vtkWidget3D::applyWindowLevelToTransferFunction(const int window, const int level)
{
        if (!m_transferFunction)
        {
                return;
        }
        m_transferFunction->updateWindowLevel(window, level);
        qCInfo(lcVtkWidget3D) << "applyWindowLevelToTransferFunction()" << "window" << window << "level" << level;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::setFilter(const QString& t_filePath)
{
        try
        {
                if (!m_volumeData)
                {
                        QString failure;
                        m_volumeData = acquireVolume(&failure);
                        m_lastVolumeError = failure;
                }
                if (!m_volumeData || !m_volumeData->ImageData)
                {
                        qCWarning(lcVtkWidget3D) << "setFilter() aborted - volume data unavailable." << m_lastVolumeError;
                        return;
                }
                m_lastVolumeError.clear();
                const auto [window, level] = getWindowLevel(m_volumeData);
                if (t_filePath == "MIP")
                {
                        m_transferFunction.reset();
                        m_transferFunction = std::make_unique<TransferFunction>();
                        m_transferFunction->initializeDefaultCurve();
                        m_mapper->SetBlendMode(vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND);
                        m_transferFunction->setMaximumIntensityProjectionFunction(0, 0);
                        applyWindowLevelToTransferFunction(window, level);
		qCInfo(lcVtkWidget3D) << "setFilter() applied MIP" << "imageIdx"
                                              << (m_image ? m_image->getIndex() : -1);
	}
	else
	{
		m_mapper->SetBlendMode(vtkVolumeMapper::COMPOSITE_BLEND);
		m_transferFunction->loadFilterFromFile(t_filePath);
		applyWindowLevelToTransferFunction(window, level);
		qCInfo(lcVtkWidget3D) << "setFilter() loaded filter" << t_filePath
                                              << "seriesUid"
                                              << (m_series ? QString::fromStdString(m_series->getUID()) : QStringLiteral("n/a"));
	}
	rememberPresetForSeries(m_transferFunction->currentPreset());
	updateFilter();
	m_interactorStyle->setTransferFunction(m_transferFunction.get());

        }
        catch (const std::exception& ex)
        {
                qCCritical(lcVtkWidget3D) << "setFilter() failed" << ex.what()
                                          << "filter" << t_filePath;
        }
}

//-----------------------------------------------------------------------------
bool isis::gui::vtkWidget3D::composeAndRenderVolume(const std::shared_ptr<VtkDicomVolume>& volume,
	const QString& initialFailureReason)
{
	const QByteArray disableOffscreenEnv = qgetenv("ISIS_3D_DISABLE_OFFSCREEN").trimmed().toLower();
	const bool disableOffscreen =
		disableOffscreenEnv == "1" || disableOffscreenEnv == "true" || disableOffscreenEnv == "yes" ||
		disableOffscreenEnv == "on";
	const QByteArray enableOffscreenEnv = qgetenv("ISIS_3D_ENABLE_OFFSCREEN").trimmed().toLower();
	const bool enableOffscreen =
		enableOffscreenEnv == "1" || enableOffscreenEnv == "true" || enableOffscreenEnv == "yes" ||
		enableOffscreenEnv == "on";
	const bool requestOffscreen = enableOffscreen && !disableOffscreen;

	if (m_renderWindows[0])
	{
		if (requestOffscreen)
		{
			qCInfo(lcVtkWidget3D)
				<< "render() enabling off-screen rendering (ISIS_3D_ENABLE_OFFSCREEN)"
				<< enableOffscreenEnv;
			m_renderWindows[0]->OffScreenRenderingOn();
		}
		else
		{
			if (disableOffscreen)
			{
				qCInfo(lcVtkWidget3D)
					<< "render() forcing on-screen rendering due to ISIS_3D_DISABLE_OFFSCREEN"
					<< disableOffscreenEnv;
			}
			m_renderWindows[0]->OffScreenRenderingOff();
		}
	}

        m_volumeData = volume;
        m_lastVolumeError = initialFailureReason;
        m_lastVolumeWarning.clear();
        const auto appendWarning = [this](const QString& warning)
        {
		if (warning.isEmpty())
		{
			return;
		}
		if (m_lastVolumeWarning.isEmpty())
		{
			m_lastVolumeWarning = warning;
		}
		else if (!m_lastVolumeWarning.contains(warning))
		{
			m_lastVolumeWarning.append(QStringLiteral("\n") + warning);
		}
	};
        if (!m_volumeData || !m_volumeData->ImageData)
        {
                if (m_lastVolumeError.isEmpty())
                {
                        m_lastVolumeError = QStringLiteral("Volume data unavailable for the selected dataset.");
                }
                qCCritical(lcVtkWidget3D) << "render() aborted - volume data unavailable." << m_lastVolumeError;
                return false;
        }

        // ✅ CORREÇÃO CRÍTICA: Garantir que scalars estão ativos ANTES de passar ao mapper
        vtkDataArray* activeScalars = ensureActiveScalars(m_volumeData->ImageData, "composeAndRenderVolume()");
        if (!activeScalars)
        {
                m_lastVolumeError = QStringLiteral("Failed to activate scalar array for volume rendering.");
                qCCritical(lcVtkWidget3D) << "render() aborted - no active scalars." << m_lastVolumeError;
                return false;
        }
        
        int extent[6] = {0, 0, 0, 0, 0, 0};
        m_volumeData->ImageData->GetExtent(extent);
        const int slicesFromExtent = extent[5] - extent[4] + 1;
        const int slicesFromFrames = m_volumeData->NumberOfFrames > 0 ? m_volumeData->NumberOfFrames : 0;
        const bool hasDepth = (slicesFromExtent >= 2) || (slicesFromFrames >= 2);
        if (!hasDepth)
        {
                const int detectedSlices = std::max(slicesFromExtent > 0 ? slicesFromExtent : 0,
                        slicesFromFrames > 0 ? slicesFromFrames : 0);
                QString message;
                if (detectedSlices <= 0)
                {
                        message = QStringLiteral(
                                "3D volume rendering requires at least two slices; this dataset does not provide volumetric depth.");
                }
                else
                {
                        const QString sliceLabel = detectedSlices == 1 ? QStringLiteral("slice") : QStringLiteral("slices");
                        message = QStringLiteral(
                                "3D volume rendering requires at least two slices; this dataset provides only %1 %2.")
                                                  .arg(detectedSlices)
                                                  .arg(sliceLabel);
                }

                m_lastVolumeError = message;
                qCWarning(lcVtkWidget3D) << "render() aborted - insufficient depth" << detectedSlices;
                m_volumeData.reset();
                return false;
        }

        double spacing[3] = {0.0, 0.0, 0.0};
        double origin[3] = {0.0, 0.0, 0.0};
        m_volumeData->ImageData->GetSpacing(spacing);
        m_volumeData->ImageData->GetOrigin(origin);
        const char* scalarName = activeScalars->GetName();
        const QString scalarNameLabel = scalarName ? QString::fromUtf8(scalarName) : QStringLiteral("<unnamed>");
        const int componentCount = activeScalars->GetNumberOfComponents();
        const vtkIdType tupleCount = activeScalars->GetNumberOfTuples();
        const int dataType = activeScalars->GetDataType();
        double activeRange[2] = {0.0, 0.0};
        activeScalars->GetRange(activeRange);
        qCInfo(lcVtkWidget3D)
                << "[Diag] composeAndRenderVolume input"
                << "extent" << extent[0] << extent[1] << extent[2] << extent[3] << extent[4] << extent[5]
                << "spacing" << spacing[0] << spacing[1] << spacing[2]
                << "origin" << origin[0] << origin[1] << origin[2]
                << "scalarRange" << activeRange[0] << activeRange[1]
                << "scalarName" << scalarNameLabel
                << "components" << componentCount
                << "tuples" << tupleCount
                << "dataType" << dataType;

        setVolumeMapperBlend();
        m_lastVolumeError.clear();
        vtkDataArray* scalars = ensureActiveScalars(m_volumeData->ImageData, "render()");
	if (!scalars)
	{
		m_lastVolumeError = QStringLiteral("Unable to access voxel intensities for 3D rendering.");
		qCCritical(lcVtkWidget3D) << "render() aborted - missing scalar data.";
		m_volumeData.reset();
		return false;
	}

	double scalarRange[2] = { 0.0, 0.0 };
	scalars->GetRange(scalarRange);
	if (isUniformRange(scalarRange[0], scalarRange[1]))
	{
		appendWarning(QStringLiteral(
			"Volume decoded but intensities are near-uniform; the 3D view may appear dim or black."));
		qCWarning(lcVtkWidget3D)
			<< "render() continuing with uniform scalar range" << scalarRange[0] << scalarRange[1];
		scalarRange[1] = scalarRange[0] + 1.0;
	}

	if (m_transferFunction && !m_transferFunction->isCustom())
	{
		auto requestedPreset = cachedPresetForSeries();
		if (requestedPreset == TransferFunction::Preset::Automatic)
		{
			const double dynamicRange = scalarRange[1] - scalarRange[0];
			requestedPreset = dynamicRange < 1500.0
				? TransferFunction::Preset::MRDefault
				: TransferFunction::Preset::CTSoftTissue;
		}
		m_transferFunction->applyPreset(requestedPreset);
		m_transferFunction->setGradientOpacityParameters(30.0, 0.2);
		rememberPresetForSeries(requestedPreset);
	}
	else if (m_transferFunction)
	{
		rememberPresetForSeries(m_transferFunction->currentPreset());
	}

	auto [window, level] = getWindowLevel(m_volumeData);
	if (window <= 0)
	{
		const int derivedWindow = static_cast<int>(std::round(std::max(scalarRange[1] - scalarRange[0], 1.0)));
		window = derivedWindow <= 0 ? 1 : derivedWindow;
		qCWarning(lcVtkWidget3D) << "render() adjusted window width fallback" << window;
	}
	qCInfo(lcVtkWidget3D) << "render() configuring pipeline"
		<< "seriesUid" << (m_series ? QString::fromStdString(m_series->getUID()) : QStringLiteral("n/a"))
		<< "imageIdx" << (m_image ? m_image->getIndex() : -1);
        logGhostArrays(m_volumeData->ImageData, "render()");
        const bool clearedGhosts = mitigateBlankingGhostArrays(m_volumeData->ImageData, "render()");
        if (clearedGhosts)
        {
                logGhostArrays(m_volumeData->ImageData, "render(afterCleanup)");
		scalars = ensureActiveScalars(m_volumeData->ImageData, "render(afterCleanup)");
		if (scalars)
		{
			scalars->GetRange(scalarRange);
			if (isUniformRange(scalarRange[0], scalarRange[1]))
			{
				appendWarning(QStringLiteral(
					"Volume decoded but intensities are near-uniform; the 3D view may appear dim or black."));
				scalarRange[1] = scalarRange[0] + 1.0;
			}
		}
        }
	if (!scalars)
	{
		m_lastVolumeError = QStringLiteral("Unable to access voxel intensities for 3D rendering.");
		qCCritical(lcVtkWidget3D) << "render() aborted - scalar data unavailable after cleanup.";
		m_volumeData.reset();
		return false;
	}
	m_mapper->SetInputData(m_volumeData->ImageData);
	applyWindowLevelToTransferFunction(window, level);
	m_volume->SetMapper(m_mapper);
	syncBoxWidgetWithVolume();
	updateFilter();
	if (m_volumeData->Direction)
	{
		m_volume->SetUserMatrix(m_volumeData->Direction);
		m_volume->SetOrigin(0.0, 0.0, 0.0);
	}
	else
	{
		const auto& origin = m_volumeData->Geometry.Origin;
		m_volume->SetOrigin(origin[0], origin[1], origin[2]);
	}

	if (m_renderer)
	{
		m_renderer->RemoveAllViewProps();
		m_renderer->AddVolume(m_volume);
		m_renderer->ResetCamera();
		if (auto* camera = m_renderer->GetActiveCamera())
		{
			double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
			m_volume->GetBounds(bounds);
			const double center[3] = {
				(bounds[0] + bounds[1]) * 0.5,
				(bounds[2] + bounds[3]) * 0.5,
				(bounds[4] + bounds[5]) * 0.5 };
			const double distance = camera->GetDistance();
			camera->SetFocalPoint(center);
			camera->SetPosition(center[0], center[1] - distance, center[2]);
			camera->SetViewUp(0.0, 0.0, 1.0);
			m_renderer->ResetCameraClippingRange();
		}
	}
        if (m_renderWindows[0] && m_renderer)
        {
                auto* rendererCollection = m_renderWindows[0]->GetRenderers();
                bool rendererAttached = false;
                if (rendererCollection)
                {
                        rendererCollection->InitTraversal();
                        while (auto* current = rendererCollection->GetNextItem())
                        {
                                if (current == m_renderer)
                                {
                                        rendererAttached = true;
                                        break;
                                }
                        }
                }
                if (!rendererAttached)
                {
                        m_renderWindows[0]->AddRenderer(m_renderer);
                }
        }

	double viewport[4] = { 0.0, 0.0, 0.0, 0.0 };
	if (m_renderer && m_renderer->GetViewport())
	{
		const double* rendererViewport = m_renderer->GetViewport();
		viewport[0] = rendererViewport[0];
		viewport[1] = rendererViewport[1];
		viewport[2] = rendererViewport[2];
		viewport[3] = rendererViewport[3];
	}

	double clippingRange[2] = { 0.0, 0.0 };
	if (auto* const camera = m_renderer ? m_renderer->GetActiveCamera() : nullptr)
	{
		camera->GetClippingRange(clippingRange);
	}

	const int* size = m_renderWindows[0] ? m_renderWindows[0]->GetSize() : nullptr;
	auto* const openGlWindow = m_renderWindows[0]
		? vtkOpenGLRenderWindow::SafeDownCast(m_renderWindows[0])
		: nullptr;
	if (!requestOffscreen && m_renderWindows[0] && openGlWindow)
	{
		if (openGlWindow->GetOffScreenRendering())
		{
			qCInfo(lcVtkWidget3D) << "render() enforcing on-screen rendering before frame composition.";
		}
		openGlWindow->OffScreenRenderingOff();
		openGlWindow->SetOffScreenRendering(0);
		openGlWindow->SetUseOffScreenBuffers(false);
	}
	const auto framebufferId = [](vtkOpenGLFramebufferObject* fbo) -> unsigned int
	{
		return fbo ? fbo->GetFBOIndex() : 0U;
	};
	const unsigned int frameBufferObject = openGlWindow ? framebufferId(openGlWindow->GetRenderFramebuffer()) : 0U;
	const unsigned int defaultFrameBufferId = openGlWindow ? framebufferId(openGlWindow->GetDisplayFramebuffer()) : 0U;
	qCInfo(lcVtkWidget3D) << "render() OpenGL state"
		<< "size" << (size ? size[0] : 0) << (size ? size[1] : 0)
		<< "viewport" << viewport[0] << viewport[1] << viewport[2] << viewport[3]
		<< "clipRange" << clippingRange[0] << clippingRange[1]
		<< "offscreen" << (m_renderWindows[0] ? m_renderWindows[0]->GetOffScreenRendering() : 0)
		<< "fbo" << frameBufferObject
		<< "defaultFbo" << defaultFrameBufferId;

	if (!m_renderWindows[0])
	{
		m_lastVolumeError = QStringLiteral("Render window unavailable for volume composition.");
		qCCritical(lcVtkWidget3D) << "render() aborted - render window unavailable.";
		return false;
	}

	QElapsedTimer timer;
	timer.start();
	m_renderWindows[0]->Render();
	qint64 renderDuration = timer.elapsed();
	bool retriedOnScreen = false;
	if (requestOffscreen && openGlWindow && openGlWindow->GetOffScreenRendering())
	{
		const unsigned int effectiveFbo = framebufferId(openGlWindow->GetRenderFramebuffer());
		if (effectiveFbo == 0U)
		{
			qCWarning(lcVtkWidget3D)
				<< "render() retrying on-screen due to unavailable off-screen framebuffer.";
			appendWarning(QStringLiteral("Off-screen rendering unavailable; falling back to on-screen mode."));
			m_renderWindows[0]->OffScreenRenderingOff();
			timer.restart();
			m_renderWindows[0]->Render();
			renderDuration += timer.elapsed();
			retriedOnScreen = true;
		}
	}

	const unsigned int finalFrameBufferObject = openGlWindow ? framebufferId(openGlWindow->GetRenderFramebuffer()) : 0U;
	qCInfo(lcVtkWidget3D) << "render() final OpenGL state"
		<< "offscreen" << (m_renderWindows[0] ? m_renderWindows[0]->GetOffScreenRendering() : 0)
		<< "fbo" << finalFrameBufferObject
		<< "defaultFbo" << defaultFrameBufferId;

	if (requestOffscreen && m_renderWindows[0] && !retriedOnScreen)
	{
		m_renderWindows[0]->OffScreenRenderingOff();
	}

	double bounds[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	if (m_volume)
	{
		m_volume->GetBounds(bounds);
	}
	initInteractorStyle();

	int dimensions[3] = { 0, 0, 0 };
	int finalExtent[6] = { 0, 0, 0, 0, 0, 0 };
	vtkIdType voxelCount = -1;
	if (m_volumeData && m_volumeData->ImageData)
	{
		m_volumeData->ImageData->GetDimensions(dimensions);
		m_volumeData->ImageData->GetExtent(finalExtent);
		voxelCount = static_cast<vtkIdType>(dimensions[0]) * dimensions[1] * dimensions[2];
	}

	qCInfo(lcVtkWidget3D)
		<< "[Telemetry] Volume render completed"
		<< "durationMs" << renderDuration
		<< "extent" << finalExtent[0] << finalExtent[1] << finalExtent[2]
		<< finalExtent[3] << finalExtent[4] << finalExtent[5]
		<< "bounds" << bounds[0] << bounds[1] << bounds[2] << bounds[3] << bounds[4] << bounds[5]
		<< "dimensions" << dimensions[0] << dimensions[1] << dimensions[2]
		<< "voxelCount" << voxelCount
		<< "scalarRange" << scalarRange[0] << scalarRange[1];

	return true;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::render()
{
	QString failureReason;
	const auto volume = acquireVolume(&failureReason);
	composeAndRenderVolume(volume, failureReason);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::activateBoxWidget(const bool& t_flag)
{
	if (!m_boxWidget)
	{
		initBoxWidget();
	}
	m_boxWidget->SetEnabled(t_flag);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget3D::updateFilter() const
{
	m_volume->GetProperty()->SetInterpolationTypeToLinear();
	m_volume->GetProperty()->SetScalarOpacity(m_transferFunction->getOpacityFunction());
	m_volume->GetProperty()->SetAmbient(m_transferFunction->getAmbient());
	m_volume->GetProperty()->SetColor(m_transferFunction->getColorFunction());
	m_volume->GetProperty()->SetDiffuse(m_transferFunction->getDiffuse());
	m_volume->GetProperty()->SetSpecular(m_transferFunction->getSpecular());
	m_volume->GetProperty()->SetSpecularPower(m_transferFunction->getSpecularPower());
	if (auto* gradient = m_transferFunction->getGradientFunction())
	{
		m_volume->GetProperty()->SetGradientOpacity(gradient);
	}
	else
	{
		m_volume->GetProperty()->SetGradientOpacity(nullptr);
	}
	m_volume->GetProperty()->GetIsoSurfaceValues()->SetValue(0, 0);
	(m_transferFunction->getHasShade()) ? m_volume->GetProperty()->ShadeOn() : m_volume->GetProperty()->ShadeOff();
}

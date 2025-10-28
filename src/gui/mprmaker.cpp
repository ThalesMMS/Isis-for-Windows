/*
 * ------------------------------------------------------------------------------------
 *  File: mprmaker.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Builds the data pipelines required to generate multiplanar reconstruction slices.
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

#include "mprmaker.h"
#include <vtkImageData.h>
#include <vtkDataArray.h>
#include <vtkRenderer.h>
#include <vtkImageProperty.h>
#include <vtkCamera.h>
#include <vtkInformation.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkCellData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkDataSetAttributes.h>
#include "image.h"
#include "series.h"
#include <vtkImageActor.h>
#include <vtkImageResliceToColors.h>
#include <vtkImageReslice.h>
#include <vtkImageReslice.h>
#include <vtkScalarsToColors.h>
#include <vtkMatrix4x4.h>
#include <vtkImageResliceMapper.h>
#include <vtkTransform.h>
#include <vtkWindowLevelLookupTable.h>
#include <vtkPointData.h>
#include <QString>
#include <QLoggingCategory>
#include <cmath>
#include <cstring>
#include <limits>

Q_LOGGING_CATEGORY(lcMprMaker, "isis.gui.mprmaker")

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

        vtkWindowLevelLookupTable* ensureWindowLevelLookupTable(vtkSmartPointer<vtkScalarsToColors>& colorMap)
        {
                auto* lookupTable = vtkWindowLevelLookupTable::SafeDownCast(colorMap.GetPointer());
                if (!lookupTable)
                {
                        auto table = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
                        table->SetNumberOfTableValues(256);
                        table->SetRampToLinear();
                        table->SetScaleToLinear();
                        table->SetHueRange(0.0, 0.0);
                        table->SetSaturationRange(0.0, 0.0);
                        table->SetValueRange(0.0, 1.0);
                        table->SetAlphaRange(1.0, 1.0);
                        table->SetWindow(1.0);
                        table->SetLevel(0.0);
                        colorMap = table;
                        lookupTable = table;
                }
                return lookupTable;
        }

        bool isUniformRange(const double* range)
        {
                if (!range)
                {
                        return true;
                }
                const double span = range[1] - range[0];
                return !std::isfinite(range[0]) || !std::isfinite(range[1]) || std::abs(span) <= uniformRangeThreshold;
        }

        void logGhostArrays(vtkImageData* image, const char* context)
        {
                if (!image)
                {
                        qCInfo(lcMprMaker) << context << "ghostCheck imageData=nullptr";
                        return;
                }

                auto describeArray = [context](vtkUnsignedCharArray* array, const char* label)
                {
                        const auto info = analyzeGhostArray(array);
                        if (!info.present)
                        {
                                qCInfo(lcMprMaker) << context << label << "ghostArray absent";
                                return;
                        }
                        qCInfo(lcMprMaker) << context
                                           << label
                                           << "ghostTuples" << info.tuples
                                           << "valueRange" << info.minValue << info.maxValue
                                           << "anyHidden" << info.anyHidden
                                           << "allHidden" << info.allHidden;
                };

                qCInfo(lcMprMaker) << context << "imageHasGhostCells" << image->HasAnyGhostCells();
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
                                qCWarning(lcMprMaker) << context
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
                        qCWarning(lcMprMaker) << context << "ensureActiveScalars() imageData=nullptr";
                        return nullptr;
                }
                auto* const pointData = image->GetPointData();
                if (!pointData)
                {
                        qCWarning(lcMprMaker) << context << "pointData unavailable while activating scalars.";
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
                        qCWarning(lcMprMaker)
                                << context
                                << "activated scalar array"
                                << (candidateName ? candidateName : "unnamed")
                                << "components"
                                << candidate->GetNumberOfComponents();
                        break;
                }

                if (!scalars)
                {
                        qCCritical(lcMprMaker) << context << "no suitable scalar array available for activation.";
                }
                return scalars;
        }
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::SetRenderWindows(const vtkSmartPointer<vtkRenderWindow>& t_sagittalWindow,
                                                const vtkSmartPointer<vtkRenderWindow>& t_coronalWindow,
                                                const vtkSmartPointer<vtkRenderWindow>& t_axialWindow)
{
        m_renderWindow[0] = t_sagittalWindow;
        m_renderWindow[1] = t_coronalWindow;
        m_renderWindow[2] = t_axialWindow;
}

void isis::gui::MPRMaker::overrideFailureMessage(const QString& failureMessage)
{
        m_lastFailure = failureMessage;
}

void isis::gui::MPRMaker::setVolumeForTesting(const std::shared_ptr<VtkDicomVolume>& volume)
{
        m_volume = volume;
}

//-----------------------------------------------------------------------------
vtkImageReslice* isis::gui::MPRMaker::getOriginalValueImageReslice(const int t_plane)
{
        m_originalValuesReslicer[t_plane]->SetResliceAxes(m_reslicer[t_plane]->GetResliceAxes());
        return m_originalValuesReslicer[t_plane];
}

//-----------------------------------------------------------------------------
double isis::gui::MPRMaker::getCenterSliceZPosition(const int t_plane) const
{
    if (!m_volume || !m_volume->ImageData)
    {
        return 0.0;
    }
    int extent[6] = {0};
    m_volume->ImageData->GetExtent(extent);
    return 0.5 * (extent[t_plane * 2] + extent[t_plane * 2 + 1]);
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::create3DMatrix()
{
	if (!m_series)
	{
		m_series = m_image->getParentObject();
	}

	m_lastFailure.clear();
	m_lastWarning.clear();
	resetWindowLevelCache();

    QString failureReason;
    qCInfo(lcMprMaker)
            << "create3DMatrix() invoked"
            << "seriesUid"
            << (m_series ? QString::fromStdString(m_series->getUID()) : QStringLiteral("n/a"))
            << "imageIdx"
            << (m_image ? m_image->getIndex() : -1)
            << "multiFrame"
            << (m_image ? m_image->getIsMultiFrame() : false);

    if (m_image && m_image->getIsMultiFrame())
    {
        m_volume = m_volumeLoader.loadFromImage(*m_image, &failureReason);
    }
    else if (m_series)
    {
        m_volume = m_volumeLoader.loadFromSeries(*m_series, &failureReason);
    }
    else
    {
        failureReason = QStringLiteral("Missing series context for multiplanar reconstruction.");
        m_volume.reset();
    }

    if (m_volume && m_volume->ImageData)
    {
        int dims[3] = {0, 0, 0};
        double spacing[3] = {0.0, 0.0, 0.0};
        double origin[3] = {0.0, 0.0, 0.0};
        double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        m_volume->ImageData->GetDimensions(dims);
        m_volume->ImageData->GetSpacing(spacing);
        m_volume->ImageData->GetOrigin(origin);
        m_volume->ImageData->GetBounds(bounds);
        vtkDataArray* scalars = ensureActiveScalars(m_volume->ImageData, "create3DMatrix()");
        double range[2] = {0.0, 0.0};
        if (scalars)
        {
            scalars->GetRange(range);
        }
        qCInfo(lcMprMaker)
                << "Volume ready"
                << "dims"
                << dims[0]
                << dims[1]
                << dims[2]
                << "spacing"
                << spacing[0]
                << spacing[1]
                << spacing[2]
                << "origin"
                << origin[0]
                << origin[1]
                << origin[2]
                << "bounds"
                << bounds[0]
                << bounds[1]
                << bounds[2]
                << bounds[3]
                << bounds[4]
                << bounds[5]
                << "range"
                << range[0]
                << range[1];
        logGhostArrays(m_volume->ImageData, "create3DMatrix()");
        const bool clearedGhosts = mitigateBlankingGhostArrays(m_volume->ImageData, "create3DMatrix()");
        if (clearedGhosts)
        {
                logGhostArrays(m_volume->ImageData, "create3DMatrix(afterCleanup)");
                scalars = ensureActiveScalars(m_volume->ImageData, "create3DMatrix(afterCleanup)");
                if (scalars)
                {
                        scalars->GetRange(range);
                }
        }
        if (!scalars)
        {
            failureReason = QStringLiteral("Unable to access voxel intensities for multiplanar reconstruction.");
            qCCritical(lcMprMaker) << "Scalar array missing for volume.";
            m_volume.reset();
        }
        else if (isUniformRange(range))
        {
            m_lastWarning = QStringLiteral(
                    "Volume decoded but intensities are near-uniform; multiplanar views may appear dim or black.");
            qCWarning(lcMprMaker) << "Uniform intensity detected for volume" << range[0] << range[1];
            failureReason.clear();
        }
        else
        {
            m_lastWarning.clear();
            failureReason.clear();
        }
    }

    if (!m_volume || !m_volume->ImageData)
    {
        if (failureReason.isEmpty())
        {
            failureReason = QStringLiteral("Volume data unavailable for multiplanar reconstruction.");
        }
        qCWarning(lcMprMaker) << "create3DMatrix() failed" << failureReason;
    }
    else
    {
        resetMatrixesToInitialPosition();
    }
    m_lastFailure = failureReason;
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::createMPR()
{
    if (!m_volume || !m_volume->ImageData)
    {
        if (m_lastFailure.isEmpty())
        {
            m_lastFailure = QStringLiteral("Multiplanar reconstruction unavailable: volume data missing or failed to load.");
        }
        qCWarning(lcMprMaker) << "createMPR() aborted" << m_lastFailure;
        return;
    }

    qCInfo(lcMprMaker) << "createMPR() starting";
    resetWindowLevel();
    createMprViews();
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::resetMatrixesToInitialPosition()
{
    setInitialMatrix();
    for (auto i = 0; i < 3; ++i)
    {
        setMiddleSlice(i);
        if (m_originalValuesReslicer[i])
        {
                m_originalValuesReslicer[i]->SetResliceAxes(m_reslicer[i]->GetResliceAxes());
        }
    }
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::resetWindowLevel()
{
    if (!m_volume || !m_volume->ImageData)
    {
        qCWarning(lcMprMaker) << "resetWindowLevel() skipped - volume missing.";
        return;
    }
    double level = m_volume->WindowCenter;
    double window = m_volume->WindowWidth;
    double range[2] = {0.0, 0.0};
    auto* const scalars = m_volume->ImageData->GetPointData()
            ? m_volume->ImageData->GetPointData()->GetScalars()
            : nullptr;
    if (scalars)
    {
        scalars->GetRange(range);
    }
    const bool windowInvalid = !std::isfinite(window) || window <= 0.0;
    const bool levelInvalid = !std::isfinite(level);
    if ((windowInvalid || levelInvalid) && scalars)
    {
        window = range[1] - range[0];
        level = 0.5 * (range[1] + range[0]);
        qCInfo(lcMprMaker)
                << "resetWindowLevel() derived from range"
                << range[0]
                << range[1];
    }
    updateColorMapWindowLevel(window, level);
    m_lastRenderedWindow = window;
    m_lastRenderedLevel = level;
    qCInfo(lcMprMaker)
            << "resetWindowLevel() applied"
            << "window"
            << window
            << "level"
            << level
            << "range"
            << range[0]
            << range[1];
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::updateColorMapWindowLevel(const double t_window, const double t_level)
{
    auto* lookupTable = ensureWindowLevelLookupTable(m_colorMap);
    constexpr double minimumWindow = 1e-5;
    const double clampedWindow = (t_window > minimumWindow) ? t_window : minimumWindow;
    lookupTable->SetWindow(clampedWindow);
    lookupTable->SetLevel(t_level);
    lookupTable->Build();
    lookupTable->Modified();
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::resetWindowLevelCache()
{
    m_lastRenderedWindow = std::numeric_limits<double>::quiet_NaN();
    m_lastRenderedLevel = std::numeric_limits<double>::quiet_NaN();
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::initialize()
{
	for (auto i = 0; i < 3; ++i)
	{
		m_reslicer[i] = vtkSmartPointer<vtkImageResliceToColors>::New();
		m_originalValuesReslicer[i] = vtkSmartPointer<vtkImageReslice>::New();
	}
	resetWindowLevelCache();
	setInitialMatrix();
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::setInitialMatrix()
{
        auto computeResliceAxes = [&](int planeIndex) -> vtkSmartPointer<vtkMatrix4x4>
        {
                const double* baseMatrixValues = nullptr;
                switch (planeIndex)
                {
                case 0:
                        baseMatrixValues = m_sagittalMatrix;
                        break;
                case 1:
                        baseMatrixValues = m_coronalMatrix;
                        break;
                case 2:
                        baseMatrixValues = m_axialMatrix;
                        break;
                default:
                        baseMatrixValues = m_axialMatrix;
                        break;
                }

                vtkNew<vtkMatrix4x4> baseMatrix;
                for (int row = 0; row < 4; ++row)
                {
                        for (int column = 0; column < 4; ++column)
                        {
                                baseMatrix->SetElement(row, column, baseMatrixValues[row * 4 + column]);
                        }
                }

                auto resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
                if (m_volume && m_volume->Direction)
                {
                        vtkMatrix4x4::Multiply4x4(m_volume->Direction, baseMatrix, resliceAxes);
                }
                else
                {
                        resliceAxes->DeepCopy(baseMatrix);
                }

                for (int row = 0; row < 3; ++row)
                {
                        resliceAxes->SetElement(row, 3, 0.0);
                }
                resliceAxes->SetElement(3, 0, 0.0);
                resliceAxes->SetElement(3, 1, 0.0);
                resliceAxes->SetElement(3, 2, 0.0);
                resliceAxes->SetElement(3, 3, 1.0);

                // Align the reslice output with the viewer's expected vertical orientation (VTK displays +Y upwards).
                if (planeIndex != 2)
                {
                        vtkNew<vtkMatrix4x4> flipYAxis;
                        flipYAxis->Identity();
                        flipYAxis->SetElement(1, 1, -1.0);
                        vtkNew<vtkMatrix4x4> adjustedAxes;
                        vtkMatrix4x4::Multiply4x4(resliceAxes, flipYAxis, adjustedAxes);
                        resliceAxes->DeepCopy(adjustedAxes);
                }
                // Axial view keeps the original orientation (no vertical flip).

                if (lcMprMaker().isInfoEnabled())
                {
                        qCInfo(lcMprMaker)
                                << "[Diag] resliceAxes base"
                                << "plane" << planeIndex
                                << "baseRow0" << baseMatrix->GetElement(0, 0) << baseMatrix->GetElement(0, 1) << baseMatrix->GetElement(0, 2) << baseMatrix->GetElement(0, 3)
                                << "baseRow1" << baseMatrix->GetElement(1, 0) << baseMatrix->GetElement(1, 1) << baseMatrix->GetElement(1, 2) << baseMatrix->GetElement(1, 3)
                                << "baseRow2" << baseMatrix->GetElement(2, 0) << baseMatrix->GetElement(2, 1) << baseMatrix->GetElement(2, 2) << baseMatrix->GetElement(2, 3);
                        if (m_volume && m_volume->Direction)
                        {
                                qCInfo(lcMprMaker)
                                        << "[Diag] volume direction"
                                        << "row0" << m_volume->Direction->GetElement(0, 0) << m_volume->Direction->GetElement(0, 1) << m_volume->Direction->GetElement(0, 2) << m_volume->Direction->GetElement(0, 3)
                                        << "row1" << m_volume->Direction->GetElement(1, 0) << m_volume->Direction->GetElement(1, 1) << m_volume->Direction->GetElement(1, 2) << m_volume->Direction->GetElement(1, 3)
                                        << "row2" << m_volume->Direction->GetElement(2, 0) << m_volume->Direction->GetElement(2, 1) << m_volume->Direction->GetElement(2, 2) << m_volume->Direction->GetElement(2, 3);
                        }
                        qCInfo(lcMprMaker)
                                << "[Diag] resliceAxes final"
                                << "plane" << planeIndex
                                << "row0" << resliceAxes->GetElement(0, 0) << resliceAxes->GetElement(0, 1) << resliceAxes->GetElement(0, 2) << resliceAxes->GetElement(0, 3)
                                << "row1" << resliceAxes->GetElement(1, 0) << resliceAxes->GetElement(1, 1) << resliceAxes->GetElement(1, 2) << resliceAxes->GetElement(1, 3)
                                << "row2" << resliceAxes->GetElement(2, 0) << resliceAxes->GetElement(2, 1) << resliceAxes->GetElement(2, 2) << resliceAxes->GetElement(2, 3);
                }

                return resliceAxes;
        };

        for (int plane = 0; plane < 3; ++plane)
        {
                auto resliceAxes = computeResliceAxes(plane);
                m_reslicer[plane]->SetResliceAxes(resliceAxes);
        }
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::createMprViews()
{
	for (auto i = 0; i < 3; i++)
	{
		renderPlaneOffScreen(i);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::setMiddleSlice(const int t_plane)
{
    if (!m_volume || !m_volume->ImageData)
    {
        return;
    }
    double spacing[3];
    double origin[3];
    double center[3];
    int extent[6] = {0};
    m_volume->ImageData->GetExtent(extent);
    m_volume->ImageData->GetSpacing(spacing);
    m_volume->ImageData->GetOrigin(origin);
    center[0] = origin[0] + spacing[0] * 0.5 * (extent[0] + extent[1]);
    center[1] = origin[1] + spacing[1] * 0.5 * (extent[2] + extent[3]);
    center[2] = origin[2] + spacing[2] * 0.5 * (extent[4] + extent[5]);
    m_reslicer[t_plane]->GetResliceAxes()->SetElement(0, 3, center[0]);
    m_reslicer[t_plane]->GetResliceAxes()->SetElement(1, 3, center[1]);
    m_reslicer[t_plane]->GetResliceAxes()->SetElement(2, 3, center[2]);
    if (lcMprMaker().isInfoEnabled())
    {
        auto* axes = m_reslicer[t_plane]->GetResliceAxes();
        qCInfo(lcMprMaker)
                << "[Diag] setMiddleSlice"
                << "plane" << t_plane
                << "center" << center[0] << center[1] << center[2]
                << "axesRow0" << axes->GetElement(0, 0) << axes->GetElement(0, 1) << axes->GetElement(0, 2) << axes->GetElement(0, 3)
                << "axesRow1" << axes->GetElement(1, 0) << axes->GetElement(1, 1) << axes->GetElement(1, 2) << axes->GetElement(1, 3)
                << "axesRow2" << axes->GetElement(2, 0) << axes->GetElement(2, 1) << axes->GetElement(2, 2) << axes->GetElement(2, 3);
    }
}

//-----------------------------------------------------------------------------
void isis::gui::MPRMaker::renderPlaneOffScreen(const int t_plane)
{
    if (!m_volume || !m_volume->ImageData)
    {
        qCWarning(lcMprMaker) << "renderPlaneOffScreen() skipped - volume missing" << "plane" << t_plane;
        return;
    }
    double window = (m_initialWindowWidth == 0)
            ? m_volume->WindowWidth
            : static_cast<double>(m_initialWindowWidth);
    double level = (m_initialWindowCenter == 0)
            ? m_volume->WindowCenter
            : static_cast<double>(m_initialWindowCenter);
    double range[2] = {0.0, 0.0};
    auto* const scalars = m_volume->ImageData->GetPointData()
            ? m_volume->ImageData->GetPointData()->GetScalars()
            : nullptr;
    if (scalars)
    {
        scalars->GetRange(range);
    }
    const bool windowInvalid = !std::isfinite(window) || window <= 0.0;
    if (windowInvalid && scalars)
    {
        window = range[1] - range[0];
    }
    if (window <= 0.0)
    {
        window = 1.0;
    }
    const bool levelInvalid = !std::isfinite(level);
    if (levelInvalid && scalars)
    {
        level = 0.5 * (range[1] + range[0]);
    }
    setMiddleSlice(t_plane);
    if (!std::isfinite(m_lastRenderedWindow)
            || !std::isfinite(m_lastRenderedLevel)
            || window != m_lastRenderedWindow
            || level != m_lastRenderedLevel)
    {
        updateColorMapWindowLevel(window, level);
        m_lastRenderedWindow = window;
        m_lastRenderedLevel = level;
    }
    vtkDataArray* activeScalars = ensureActiveScalars(m_volume->ImageData, "renderPlaneOffScreen()");
    if (activeScalars)
    {
        int extent[6] = {0, 0, 0, 0, 0, 0};
        double spacing[3] = {0.0, 0.0, 0.0};
        double origin[3] = {0.0, 0.0, 0.0};
        m_volume->ImageData->GetExtent(extent);
        m_volume->ImageData->GetSpacing(spacing);
        m_volume->ImageData->GetOrigin(origin);
        double activeRange[2] = {0.0, 0.0};
        activeScalars->GetRange(activeRange);
        const char* scalarName = activeScalars->GetName();
        qCInfo(lcMprMaker)
                << "[Diag] renderPlaneOffScreen input"
                << "plane" << t_plane
                << "extent" << extent[0] << extent[1] << extent[2] << extent[3] << extent[4] << extent[5]
                << "spacing" << spacing[0] << spacing[1] << spacing[2]
                << "origin" << origin[0] << origin[1] << origin[2]
                << "scalarRange" << activeRange[0] << activeRange[1]
                << "components" << activeScalars->GetNumberOfComponents()
                << "tuples" << activeScalars->GetNumberOfTuples()
                << "dataType" << activeScalars->GetDataType()
                << "scalarName" << (scalarName ? scalarName : "<unnamed>");
    }
    else
    {
        qCWarning(lcMprMaker) << "renderPlaneOffScreen()" << "plane" << t_plane << "active scalar array unavailable";
    }
    m_reslicer[t_plane]->SetInputData(m_volume->ImageData);
    m_reslicer[t_plane]->SetInformationInput(m_volume->ImageData);
    m_reslicer[t_plane]->SetOutputDimensionality(2);
    m_reslicer[t_plane]->SetSlabNumberOfSlices(0);
    m_reslicer[t_plane]->SetInterpolationModeToLinear();
    m_reslicer[t_plane]->SetWrap(0);
    m_reslicer[t_plane]->BypassOff();
    auto* lookupTable = ensureWindowLevelLookupTable(m_colorMap);
    if (!lookupTable)
    {
        qCCritical(lcMprMaker)
                << "renderPlaneOffScreen()"
                << "plane"
                << t_plane
                << "failed to acquire lookup table; skipping render.";
        return;
    }
    constexpr double minimumWindow = 1e-5;
    const double clampedWindow = (window > minimumWindow) ? window : minimumWindow;
    lookupTable->SetWindow(clampedWindow);
    lookupTable->SetLevel(level);
    lookupTable->Build();
    lookupTable->BuildSpecialColors();
    qCInfo(lcMprMaker)
            << "[Diag] renderPlaneOffScreen lookupTable"
            << "plane" << t_plane
            << "window" << lookupTable->GetWindow()
            << "level" << lookupTable->GetLevel()
            << "tableValues" << lookupTable->GetNumberOfTableValues();
    m_reslicer[t_plane]->SetLookupTable(lookupTable);
    m_reslicer[t_plane]->SetOutputFormatToRGB();
    m_reslicer[t_plane]->Update();
    if (auto* resliceOutput = m_reslicer[t_plane]->GetOutput())
    {
        double outputRange[2] = {0.0, 0.0};
        resliceOutput->GetScalarRange(outputRange);
        int outputExtent[6] = {0, 0, 0, 0, 0, 0};
        resliceOutput->GetExtent(outputExtent);
        double outputSpacing[3] = {0.0, 0.0, 0.0};
        resliceOutput->GetSpacing(outputSpacing);
        double outputOrigin[3] = {0.0, 0.0, 0.0};
        resliceOutput->GetOrigin(outputOrigin);
        vtkDataArray* outputScalars = resliceOutput->GetPointData()
                ? resliceOutput->GetPointData()->GetScalars()
                : nullptr;
        const int outputComponents = resliceOutput->GetNumberOfScalarComponents();
        const auto outputTuples = outputScalars ? static_cast<long long>(outputScalars->GetNumberOfTuples()) : 0LL;
        const int outputDataType = outputScalars ? outputScalars->GetDataType() : resliceOutput->GetScalarType();
        const char* outputScalarName = outputScalars ? outputScalars->GetName() : nullptr;
        qCInfo(lcMprMaker)
                << "[Diag] renderPlaneOffScreen output"
                << "plane" << t_plane
                << "extent" << outputExtent[0] << outputExtent[1] << outputExtent[2] << outputExtent[3] << outputExtent[4] << outputExtent[5]
                << "spacing" << outputSpacing[0] << outputSpacing[1] << outputSpacing[2]
                << "origin" << outputOrigin[0] << outputOrigin[1] << outputOrigin[2]
                << "components" << outputComponents
                << "scalarRange" << outputRange[0] << outputRange[1]
                << "dataType" << outputDataType
                << "tuples" << outputTuples
                << "scalarName" << (outputScalarName ? outputScalarName : "<unnamed>");
    }
    m_originalValuesReslicer[t_plane]->SetInputData(m_volume->ImageData);
    m_originalValuesReslicer[t_plane]->SetInformationInput(m_volume->ImageData);
    m_originalValuesReslicer[t_plane]->SetOutputDimensionality(2);
    m_originalValuesReslicer[t_plane]->SetSlabNumberOfSlices(0);
    m_originalValuesReslicer[t_plane]->SetInterpolationModeToLinear();
    m_originalValuesReslicer[t_plane]->SetWrap(0);
    m_originalValuesReslicer[t_plane]->SetResliceAxes(m_reslicer[t_plane]->GetResliceAxes());
    m_originalValuesReslicer[t_plane]->Update();
    if (auto* rawReslice = m_originalValuesReslicer[t_plane]->GetOutput())
    {
        double rawRange[2] = {0.0, 0.0};
        rawReslice->GetScalarRange(rawRange);
        int rawExtent[6] = {0, 0, 0, 0, 0, 0};
        rawReslice->GetExtent(rawExtent);
        double rawSpacing[3] = {0.0, 0.0, 0.0};
        rawReslice->GetSpacing(rawSpacing);
        double rawOrigin[3] = {0.0, 0.0, 0.0};
        rawReslice->GetOrigin(rawOrigin);
        vtkDataArray* rawScalars = rawReslice->GetPointData()
                ? rawReslice->GetPointData()->GetScalars()
                : nullptr;
        const int rawComponents = rawReslice->GetNumberOfScalarComponents();
        const auto rawTuples = rawScalars ? static_cast<long long>(rawScalars->GetNumberOfTuples()) : 0LL;
        const int rawDataType = rawScalars ? rawScalars->GetDataType() : rawReslice->GetScalarType();
        const char* rawScalarName = rawScalars ? rawScalars->GetName() : nullptr;
        qCInfo(lcMprMaker)
                << "[Diag] renderPlaneOffScreen reslice (pre-map)"
                << "plane" << t_plane
                << "extent" << rawExtent[0] << rawExtent[1] << rawExtent[2] << rawExtent[3] << rawExtent[4] << rawExtent[5]
                << "spacing" << rawSpacing[0] << rawSpacing[1] << rawSpacing[2]
                << "origin" << rawOrigin[0] << rawOrigin[1] << rawOrigin[2]
                << "components" << rawComponents
                << "scalarRange" << rawRange[0] << rawRange[1]
                << "dataType" << rawDataType
                << "tuples" << rawTuples
                << "scalarName" << (rawScalarName ? rawScalarName : "<unnamed>");
        qCInfo(lcMprMaker)
                << "[Diag] renderPlaneOffScreen raw"
                << "plane" << t_plane
                << "components" << rawComponents
                << "scalarRange" << rawRange[0] << rawRange[1];
    }
        vtkNew<vtkImageActor> actor;
        vtkNew<vtkRenderer> renderer;
        vtkNew<vtkImageResliceMapper> mapper;
        mapper->SeparateWindowLevelOperationOff();
        mapper->SetInputConnection(m_reslicer[t_plane]->GetOutputPort());
	actor->GetProperty()->SetInterpolationTypeToCubic();
	actor->SetMapper(mapper);
	renderer->AddActor(actor);
        renderer->SetBackground(0, 0, 0);
        renderer->GetActiveCamera()->SetParallelProjection(1);
        renderer->ResetCamera();
        m_renderWindow[t_plane]->AddRenderer(renderer);
        const int* size = m_renderWindow[t_plane]->GetSize();
        auto* const openGlWindow = vtkOpenGLRenderWindow::SafeDownCast(m_renderWindow[t_plane]);
        const auto framebufferId = [](vtkOpenGLFramebufferObject* fbo) -> unsigned int
        {
                return fbo ? fbo->GetFBOIndex() : 0U;
        };
        const unsigned int frameBufferObject = openGlWindow ? framebufferId(openGlWindow->GetRenderFramebuffer()) : 0U;
        const unsigned int defaultFrameBufferId = openGlWindow ? framebufferId(openGlWindow->GetDisplayFramebuffer()) : 0U;
        double actorBounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        actor->GetBounds(actorBounds);
        qCInfo(lcMprMaker)
                << "renderPlaneOffScreen()"
                << "plane"
                << t_plane
                << "window"
                << window
                << "level"
                << level
                << "range"
                << range[0]
                << range[1]
                << "size"
                << (size ? size[0] : 0)
                << (size ? size[1] : 0)
                << "offscreen"
                << m_renderWindow[t_plane]->GetOffScreenRendering()
                << "fbo"
                << frameBufferObject
                << "defaultFbo"
                << defaultFrameBufferId
                << "actorBounds"
                << actorBounds[0]
                << actorBounds[1]
                << actorBounds[2]
                << actorBounds[3]
                << actorBounds[4]
                << actorBounds[5];
        m_renderWindow[t_plane]->Render();
}


/*
 * ------------------------------------------------------------------------------------
 *  File: dicomviewcontroller.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements the controller orchestrating render window interactions, camera logic,
 *      and slice management for the DICOM VTK widget.
 * ------------------------------------------------------------------------------------
 */

#include "dicomviewcontroller.h"

#include "dicomcoresyncadapter.h"
#include "dicomvtkpipelinebuilder.h"

#include "dicomvolume.h"

#include <vtkCamera.h>
#include <vtkCoordinate.h>
#include <vtkDataObject.h>
#include <vtkImageActor.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkInteractorStyleImage.h>
#include <vtkInformation.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <QLoggingCategory>

#include <algorithm>
#include <cmath>
#include <limits>

Q_DECLARE_LOGGING_CATEGORY(lcWidgetDicom)

namespace
{
	constexpr double epsilon = 1e-8;
	constexpr double spacingChangeThreshold = 5e-4;
	constexpr double cameraChangeThreshold = 1e-5;

	bool isMagnitudeValid(double value)
	{
		constexpr double spacingUpperBound = 1e4;
		const double absValue = std::abs(value);
		return std::isfinite(value) && absValue > epsilon && absValue < spacingUpperBound;
	}
}

using namespace isis;

gui::DicomViewController::DicomViewController(
	DicomVTKPipelineBuilder& pipelineBuilder,
	DicomCoreSyncAdapter& coreAdapter)
	: m_pipelineBuilder(pipelineBuilder)
	, m_coreAdapter(coreAdapter)
{
}

void gui::DicomViewController::setRenderWindow(vtkRenderWindow* renderWindow)
{
	if (renderWindow == m_renderWindow)
	{
		ensureRendererAttached();
		return;
	}

	if (m_renderWindow)
	{
		auto* const renderer = m_pipelineBuilder.renderer();
		auto* const renderers = m_renderWindow->GetRenderers();
		if (renderers && renderer && renderers->IsItemPresent(renderer) != 0)
		{
			m_renderWindow->RemoveRenderer(renderer);
		}
	}

	m_renderWindow = renderWindow;
	ensureRendererAttached();

	if (!m_renderWindow)
	{
		return;
	}

	if (m_interactor && m_interactor->GetRenderWindow() != m_renderWindow)
	{
		m_interactor->SetRenderWindow(m_renderWindow);
	}
	if (m_renderWindow->GetInteractor() != m_interactor && m_interactor)
	{
		m_renderWindow->SetInteractor(m_interactor);
	}
}

void gui::DicomViewController::setupInteractor(vtkRenderWindowInteractor* interactor)
{
	m_interactor = interactor;
	if (!m_interactor || !m_renderWindow)
	{
		return;
	}
	if (m_interactor->GetRenderWindow() != m_renderWindow)
	{
		m_interactor->SetRenderWindow(m_renderWindow);
	}
	if (m_renderWindow->GetInteractor() != m_interactor)
	{
		m_renderWindow->SetInteractor(m_interactor);
	}
}

vtkRenderer* gui::DicomViewController::renderer() const
{
	return m_pipelineBuilder.renderer();
}

void gui::DicomViewController::render()
{
	updateDisplayExtent();
	if (!m_renderWindow)
	{
		return;
	}

	const int* size = m_renderWindow->GetSize();
	if (!size || size[0] <= 0 || size[1] <= 0)
	{
		m_renderWindow->SetSize(512, 512);
	}
	if (m_renderWindow->GetNeverRendered())
	{
		m_renderWindow->Start();
	}

	const int* resolvedSize = m_renderWindow->GetSize();
	double viewport[4] = {0.0, 0.0, 0.0, 0.0};
	if (const double* rendererViewport = renderer() ? renderer()->GetViewport() : nullptr)
	{
		viewport[0] = rendererViewport[0];
		viewport[1] = rendererViewport[1];
		viewport[2] = rendererViewport[2];
		viewport[3] = rendererViewport[3];
	}
	double clippingRange[2] = {0.0, 0.0};
	if (auto* const camera = renderer() ? renderer()->GetActiveCamera() : nullptr)
	{
		camera->GetClippingRange(clippingRange);
	}
	auto* const openGlWindow = vtkOpenGLRenderWindow::SafeDownCast(m_renderWindow);
	const auto framebufferId = [](vtkOpenGLFramebufferObject* fbo) -> unsigned int
	{
		return fbo ? fbo->GetFBOIndex() : 0U;
	};
	const unsigned int frameBufferObject = openGlWindow ? framebufferId(openGlWindow->GetRenderFramebuffer()) : 0U;
	const unsigned int defaultFrameBufferId = openGlWindow ? framebufferId(openGlWindow->GetDisplayFramebuffer()) : 0U;
	double actorBounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	if (auto* const actor = m_pipelineBuilder.imageActor())
	{
		actor->GetBounds(actorBounds);
	}
	qCInfo(lcWidgetDicom) << "Render() state"
	                      << "size" << (resolvedSize ? resolvedSize[0] : 0) << (resolvedSize ? resolvedSize[1] : 0)
	                      << "viewport" << viewport[0] << viewport[1] << viewport[2] << viewport[3]
	                      << "clipRange" << clippingRange[0] << clippingRange[1]
	                      << "offscreen" << m_renderWindow->GetOffScreenRendering()
	                      << "fbo" << frameBufferObject
	                      << "defaultFbo" << defaultFrameBufferId
	                      << "sliceMapper" << m_pipelineBuilder.isSliceMapperEnabled()
	                      << "actorBounds" << actorBounds[0] << actorBounds[1] << actorBounds[2] << actorBounds[3]
	                      << actorBounds[4] << actorBounds[5];
	m_renderWindow->Render();
}

void gui::DicomViewController::onVolumeCleared()
{
	m_sliceMin = 0;
	m_sliceMax = 0;
	m_currentSlice = 0;
	m_lastClippingRange = -1.0;
	m_lastAvgSpacing = -1.0;
	m_lastSliceOrientation = -1;
	m_cachedSpacing = {1.0, 1.0, 1.0};
	m_originalSpacing = {1.0, 1.0, 1.0};
	m_cachedCameraPosition = {std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()};
	m_cachedCameraFocalPoint = {std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()};
	m_hasCachedSpacing = false;
	m_hasOriginalSpacing = false;
	m_hasCachedCamera = false;
	if (auto* actor = m_pipelineBuilder.imageActor())
	{
		actor->SetDisplayExtent(0, -1, 0, -1, 0, -1);
		actor->Modified();
	}
}

void gui::DicomViewController::onVolumeLoaded()
{
	setSliceOrientation(SLICE_ORIENTATION_XY);
	updateSliceRange();
	setSlice(0);
	if (auto* const camera = renderer() ? renderer()->GetActiveCamera() : nullptr)
	{
		camera->ParallelProjectionOn();
	}
	m_cachedCameraPosition = {std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()};
	m_cachedCameraFocalPoint = {std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()};
	m_hasCachedCamera = false;
	m_lastSliceOrientation = -1;
}

void gui::DicomViewController::setSliceOrientation(const int orientation)
{
	if (orientation < SLICE_ORIENTATION_YZ || orientation > SLICE_ORIENTATION_XY)
	{
		return;
	}
	if (m_sliceOrientation == orientation)
	{
		return;
	}
	m_sliceOrientation = orientation;
	updateSliceRange();
	updateActorExtentWithInformation(
		m_pipelineBuilder.windowLevelColors() ? m_pipelineBuilder.windowLevelColors()->GetOutputInformation(0) : nullptr);
}

void gui::DicomViewController::setSlice(const int slice)
{
	const int clamped = std::clamp(slice, m_sliceMin, m_sliceMax);
	if (m_currentSlice == clamped)
	{
		return;
	}
	m_currentSlice = clamped;
	updateActorExtentWithInformation(
		m_pipelineBuilder.windowLevelColors() ? m_pipelineBuilder.windowLevelColors()->GetOutputInformation(0) : nullptr);
}

void gui::DicomViewController::updateSliceRange()
{
	int extent[6] = {0, -1, 0, -1, 0, -1};
	if (auto* const windowLevel = m_pipelineBuilder.windowLevelColors())
	{
		windowLevel->UpdateInformation();
		if (auto* info = windowLevel->GetOutputInformation(0))
		{
			if (info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
			{
				info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
			}
		}
	}

	const auto& volume = m_coreAdapter.volume();
	if ((extent[1] < extent[0] || extent[3] < extent[2] || extent[5] < extent[4]) && volume && volume->ImageData)
	{
		volume->ImageData->GetExtent(extent);
	}

	switch (m_sliceOrientation)
	{
	case SLICE_ORIENTATION_XY:
		m_sliceMin = extent[4];
		m_sliceMax = extent[5];
		break;
	case SLICE_ORIENTATION_XZ:
		m_sliceMin = extent[2];
		m_sliceMax = extent[3];
		break;
	case SLICE_ORIENTATION_YZ:
	default:
		m_sliceMin = extent[0];
		m_sliceMax = extent[1];
		break;
	}

	if (m_sliceMin > m_sliceMax)
	{
		std::swap(m_sliceMin, m_sliceMax);
	}
	m_currentSlice = std::clamp(m_currentSlice, m_sliceMin, m_sliceMax);
}

void gui::DicomViewController::updateActorExtentWithInformation(vtkInformation* info)
{
	auto* const actor = m_pipelineBuilder.imageActor();
	if (!actor)
	{
		return;
	}

	int extent[6] = {0, -1, 0, -1, 0, -1};
	auto isExtentValid = [](const int* candidate) -> bool
	{
		return candidate[1] >= candidate[0] &&
			candidate[3] >= candidate[2] &&
			candidate[5] >= candidate[4];
	};

	if (info && info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
	{
		info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
	}

	const auto& volume = m_coreAdapter.volume();
	if (!isExtentValid(extent) && volume && volume->ImageData)
	{
		volume->ImageData->GetExtent(extent);
	}

	const int safeSlice = std::clamp(m_currentSlice, m_sliceMin, m_sliceMax);
	switch (m_sliceOrientation)
	{
	case SLICE_ORIENTATION_XY:
		extent[4] = safeSlice;
		extent[5] = safeSlice;
		break;
	case SLICE_ORIENTATION_XZ:
		extent[2] = safeSlice;
		extent[3] = safeSlice;
		break;
	case SLICE_ORIENTATION_YZ:
	default:
		extent[0] = safeSlice;
		extent[1] = safeSlice;
		break;
	}

	actor->SetDisplayExtent(extent);
	actor->Modified();
}

void gui::DicomViewController::updateDisplayExtent()
{
	auto* const windowLevel = m_pipelineBuilder.windowLevelColors();
	auto* const actor = m_pipelineBuilder.imageActor();
	if (!windowLevel || !actor)
	{
		qCWarning(lcWidgetDicom) << "UpdateDisplayExtent aborted - missing filter or actor.";
		return;
	}

	windowLevel->UpdateInformation();
	auto* info = windowLevel->GetOutputInformation(0);

	double sanitizedSpacing[3] = {1.0, 1.0, 1.0};
	bool spacingSanitized = false;

	auto mergeSpacing = [&](const double* source)
	{
		if (!source)
		{
			return;
		}
		for (int axis = 0; axis < 3; ++axis)
		{
			if (isMagnitudeValid(source[axis]))
			{
				sanitizedSpacing[axis] = std::abs(source[axis]);
			}
		}
	};

	const auto& volume = m_coreAdapter.volume();
	if (volume)
	{
		mergeSpacing(volume->Geometry.Spacing);
	}

	auto* const inputImage = volume ? volume->ImageData.GetPointer() : nullptr;
	double originalImageSpacing[3] = {1.0, 1.0, 1.0};
	if (inputImage)
	{
		const double* currentSpacing = inputImage->GetSpacing();
		if (currentSpacing)
		{
			for (int axis = 0; axis < 3; ++axis)
			{
				originalImageSpacing[axis] = currentSpacing[axis];
			}
			mergeSpacing(currentSpacing);
		}
		m_originalSpacing = {originalImageSpacing[0], originalImageSpacing[1], originalImageSpacing[2]};
		m_hasOriginalSpacing = true;
	}
	else
	{
		m_originalSpacing = {1.0, 1.0, 1.0};
		m_hasOriginalSpacing = false;
	}

	bool needsOutInfoUpdate = (info == nullptr);
	if (info)
	{
		const double* infoSpacing = info->Get(vtkDataObject::SPACING());
		if (!infoSpacing)
		{
			needsOutInfoUpdate = true;
		}
		else
		{
			for (int axis = 0; axis < 3; ++axis)
			{
				if (!isMagnitudeValid(infoSpacing[axis]))
				{
					needsOutInfoUpdate = true;
				}
			}
			mergeSpacing(infoSpacing);
			for (int axis = 0; axis < 3; ++axis)
			{
				if (std::abs(std::abs(infoSpacing[axis]) - sanitizedSpacing[axis]) > epsilon)
				{
					needsOutInfoUpdate = true;
					break;
				}
			}
		}
	}
	if (needsOutInfoUpdate && info)
	{
		info->Set(vtkDataObject::SPACING(), sanitizedSpacing, 3);
		spacingSanitized = true;
	}

	if (inputImage)
	{
		bool requiresRepair = false;
		for (int axis = 0; axis < 3; ++axis)
		{
			if (!isMagnitudeValid(originalImageSpacing[axis]))
			{
				requiresRepair = true;
				break;
			}
		}

		if (requiresRepair)
		{
			inputImage->SetSpacing(sanitizedSpacing);
			spacingSanitized = true;
		}
	}

	if (spacingSanitized)
	{
		qCWarning(lcWidgetDicom)
			<< "Sanitized image spacing to"
			<< sanitizedSpacing[0]
			<< sanitizedSpacing[1]
			<< sanitizedSpacing[2];
	}

	bool spacingChangedSignificantly = !m_hasCachedSpacing;
	if (!spacingChangedSignificantly)
	{
		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(m_cachedSpacing[axis] - sanitizedSpacing[axis]) > spacingChangeThreshold)
			{
				spacingChangedSignificantly = true;
				break;
			}
		}
	}

	const bool spacingRequiresCameraReset = spacingChangedSignificantly || spacingSanitized;

	m_cachedSpacing = {sanitizedSpacing[0], sanitizedSpacing[1], sanitizedSpacing[2]};
	m_hasCachedSpacing = true;

	updateSliceRange();
	updateActorExtentWithInformation(info);

	windowLevel->Update();
	info = windowLevel->GetOutputInformation(0);
	if (!info)
	{
		return;
	}

	if (!info->Get(vtkDataObject::SPACING()))
	{
		info->Set(vtkDataObject::SPACING(), sanitizedSpacing, 3);
	}

	auto* const activeRenderer = renderer();
	if (!activeRenderer)
	{
		return;
	}

	auto* const camera = activeRenderer->GetActiveCamera();
	if (!camera)
	{
		return;
	}

	bool cameraInvalid = false;
	const double* cameraPosition = camera->GetPosition();
	const double* cameraFocalPoint = camera->GetFocalPoint();
	for (int axis = 0; axis < 3; ++axis)
	{
		if (!std::isfinite(cameraPosition[axis]) || !std::isfinite(cameraFocalPoint[axis]))
		{
			cameraInvalid = true;
			break;
		}
	}

	if (spacingRequiresCameraReset || cameraInvalid || m_lastSliceOrientation != m_sliceOrientation)
	{
		activeRenderer->ResetCamera();
		const double* resetPosition = camera->GetPosition();
		const double* resetFocalPoint = camera->GetFocalPoint();
		qCInfo(lcWidgetDicom)
			<< "Reset camera due to"
			<< (spacingChangedSignificantly
				    ? "spacing update"
				    : (spacingSanitized ? "sanitized spacing"
				                        : (cameraInvalid ? "invalid camera" : "orientation change")))
			<< "position"
			<< resetPosition[0]
			<< resetPosition[1]
			<< resetPosition[2]
			<< "focal"
			<< resetFocalPoint[0]
			<< resetFocalPoint[1]
			<< resetFocalPoint[2];
		camera->ParallelProjectionOn();
		m_lastSliceOrientation = m_sliceOrientation;
		m_cachedCameraPosition = {resetPosition[0], resetPosition[1], resetPosition[2]};
		m_cachedCameraFocalPoint = {resetFocalPoint[0], resetFocalPoint[1], resetFocalPoint[2]};
		m_hasCachedCamera = true;
	}

	if (!spacingChangedSignificantly && !spacingSanitized && m_hasCachedCamera)
	{
		bool cameraShifted = false;
		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(cameraPosition[axis] - m_cachedCameraPosition[axis]) > cameraChangeThreshold ||
				std::abs(cameraFocalPoint[axis] - m_cachedCameraFocalPoint[axis]) > cameraChangeThreshold)
			{
				cameraShifted = true;
				break;
			}
		}
		if (!cameraShifted)
		{
			camera->SetPosition(m_cachedCameraPosition.data());
			camera->SetFocalPoint(m_cachedCameraFocalPoint.data());
		}
	}

	auto* const interactorStyle = m_interactor
		                              ? vtkInteractorStyleImage::SafeDownCast(m_interactor->GetInteractorStyle())
		                              : nullptr;

	if (interactorStyle && interactorStyle->GetAutoAdjustCameraClippingRange())
	{
		activeRenderer->ResetCameraClippingRange();
	}
	else
	{
		double bounds[6] = {0.0};
		actor->GetBounds(bounds);
		const int* displayExtent = actor->GetDisplayExtent();
		qCInfo(lcWidgetDicom)
			<< "Actor bounds"
			<< bounds[0]
			<< bounds[1]
			<< bounds[2]
			<< bounds[3]
			<< bounds[4]
			<< bounds[5]
			<< "display extent"
			<< displayExtent[0]
			<< displayExtent[1]
			<< displayExtent[2]
			<< displayExtent[3]
			<< displayExtent[4]
			<< displayExtent[5];

		const int axis = (m_sliceOrientation == SLICE_ORIENTATION_XY)
			                 ? 2
			                 : (m_sliceOrientation == SLICE_ORIENTATION_XZ ? 1 : 0);
		const double spos = bounds[axis * 2];
		const double cpos = camera->GetPosition()[axis];
		const double range = std::fabs(spos - cpos);
		const double* spacing = info->Get(vtkDataObject::SPACING());
		double safeSpacing[3] = {sanitizedSpacing[0], sanitizedSpacing[1], sanitizedSpacing[2]};
		if (volume)
		{
			for (int index = 0; index < 3; ++index)
			{
				if (isMagnitudeValid(volume->Geometry.Spacing[index]))
				{
					safeSpacing[index] = std::abs(volume->Geometry.Spacing[index]);
				}
			}
		}
		if (spacing)
		{
			for (int index = 0; index < 3; ++index)
			{
				if (isMagnitudeValid(spacing[index]))
				{
					safeSpacing[index] = std::abs(spacing[index]);
				}
			}
		}

		double avgSpacing = (safeSpacing[0] + safeSpacing[1] + safeSpacing[2]) / 3.0;
		if (!std::isfinite(avgSpacing) || avgSpacing <= epsilon)
		{
			avgSpacing = 1.0;
		}
		const double halfThickness = std::max(avgSpacing * 3.0, epsilon);
		double nearPlane = range - halfThickness;
		double farPlane = range + halfThickness;

		const double minNearPlane = std::max(epsilon, halfThickness * 0.1);
		if (!std::isfinite(nearPlane) || nearPlane <= minNearPlane)
		{
			nearPlane = minNearPlane;
		}
		if (!std::isfinite(farPlane) || farPlane <= nearPlane)
		{
			farPlane = nearPlane + halfThickness;
		}

		camera->SetClippingRange(nearPlane, farPlane);
		double computedNear = 0.0;
		double computedFar = 0.0;
		camera->GetClippingRange(computedNear, computedFar);
		qCInfo(lcWidgetDicom)
			<< "Clipping range set to"
			<< computedNear
			<< computedFar
			<< "range baseline"
			<< range
			<< "axis spacing"
			<< avgSpacing
			<< "parallel scale"
			<< camera->GetParallelScale();
		m_lastClippingRange = range;
		m_lastAvgSpacing = avgSpacing;
	}

	const double* finalCameraPosition = camera->GetPosition();
	const double* finalCameraFocalPoint = camera->GetFocalPoint();
	m_cachedCameraPosition = {finalCameraPosition[0], finalCameraPosition[1], finalCameraPosition[2]};
	m_cachedCameraFocalPoint = {finalCameraFocalPoint[0], finalCameraFocalPoint[1], finalCameraFocalPoint[2]};
	m_hasCachedCamera = true;
}

double gui::DicomViewController::getZoomFactor() const
{
	auto* const actor = m_pipelineBuilder.imageActor();
	if (!actor)
	{
		qCWarning(lcWidgetDicom) << "getZoomFactor called without an image actor.";
		return 0.0;
	}
	const int* actorExtent = actor->GetDisplayExtent();
	if (!actorExtent)
	{
		qCWarning(lcWidgetDicom) << "Missing display extent for zoom computation.";
		return 0.0;
	}
	const auto [x, y] = getImageActorDisplayValue();
	const double denominator = static_cast<double>(actorExtent[1]) - static_cast<double>(actorExtent[0]) + 1.0;
	if (!std::isfinite(denominator) || denominator <= 0.0)
	{
		return 0.0;
	}
	const double numerator = static_cast<double>(std::abs(y - x));
	return numerator / denominator;
}

std::tuple<int, int> gui::DicomViewController::getImageActorDisplayValue() const
{
	auto* const actor = m_pipelineBuilder.imageActor();
	auto* const activeRenderer = renderer();
	if (!actor || !m_renderWindow || !activeRenderer)
	{
		return {0, 0};
	}

	double actorBounds[6] = {0.0};
	actor->GetBounds(actorBounds);
	vtkNew<vtkCoordinate> coordinate;
	coordinate->SetCoordinateSystemToWorld();
	coordinate->SetValue(actorBounds[0], 0, 0);
	const int x = coordinate->GetComputedDisplayValue(activeRenderer)[0];
	coordinate->SetValue(actorBounds[1], 0, 0);
	const int y = coordinate->GetComputedDisplayValue(activeRenderer)[0];
	return std::make_tuple(x, y);
}

void gui::DicomViewController::ensureRendererAttached()
{
	if (!m_renderWindow)
	{
		return;
	}
	auto* const rendererPtr = renderer();
	if (!rendererPtr)
	{
		return;
	}
	auto* const renderers = m_renderWindow->GetRenderers();
	if (!renderers || renderers->IsItemPresent(rendererPtr) == 0)
	{
		m_renderWindow->AddRenderer(rendererPtr);
	}
}

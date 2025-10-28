/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetdicom.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements the vtkWidgetDicom helper that connects data sources to VTK image pipelines.
 *
 * ------------------------------------------------------------------------------------
 */

#include "vtkwidgetdicom.h"

#include "dicomcoresyncadapter.h"
#include "dicomviewcontroller.h"
#include "dicomvtkpipelinebuilder.h"

#include "dicomvolume.h"

#include <QLoggingCategory>

#include <vtkImageActor.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

Q_LOGGING_CATEGORY(lcWidgetDicom, "isis.gui.vtkwidgetdicom")
vtkStandardNewMacro(isis::gui::vtkWidgetDICOM)

using namespace isis;

gui::vtkWidgetDICOM::vtkWidgetDICOM()
	: m_pipelineBuilder(std::make_unique<DicomVTKPipelineBuilder>())
	, m_coreAdapter(std::make_unique<DicomCoreSyncAdapter>(*m_pipelineBuilder))
	, m_viewController(std::make_unique<DicomViewController>(*m_pipelineBuilder, *m_coreAdapter))
{
}

gui::vtkWidgetDICOM::~vtkWidgetDICOM() = default;

void gui::vtkWidgetDICOM::SetRenderWindow(vtkRenderWindow* renderWindow)
{
	if (m_viewController)
	{
		m_viewController->setRenderWindow(renderWindow);
	}
}

void gui::vtkWidgetDICOM::SetupInteractor(vtkRenderWindowInteractor* interactor)
{
	if (m_viewController)
	{
		m_viewController->setupInteractor(interactor);
	}
}

void gui::vtkWidgetDICOM::Render()
{
	if (m_viewController)
	{
		m_viewController->render();
	}
}

void gui::vtkWidgetDICOM::setVolume(const std::shared_ptr<core::DicomVolume>& t_volume)
{
	if (!m_coreAdapter || !m_viewController)
	{
		return;
	}

	const auto state = m_coreAdapter->setVolume(t_volume);
	if (!state.HasVolume)
	{
		m_viewController->onVolumeCleared();
		return;
	}

	m_viewController->onVolumeLoaded();
}

void gui::vtkWidgetDICOM::setInvertColors(const bool t_flag)
{
	if (m_pipelineBuilder)
	{
		m_pipelineBuilder->setInvertColors(t_flag);
	}
}

void gui::vtkWidgetDICOM::setWindowWidthCenter(const int t_width, const int t_center)
{
	if (m_pipelineBuilder)
	{
		m_pipelineBuilder->setWindowWidthCenter(t_width, t_center);
	}
}

void gui::vtkWidgetDICOM::changeWindowWidthCenter(const int t_width, const int t_center)
{
	if (m_pipelineBuilder)
	{
		m_pipelineBuilder->changeWindowWidthCenter(t_width, t_center);
	}
}

void gui::vtkWidgetDICOM::setInitialWindowWidthCenter()
{
	if (!m_pipelineBuilder || !m_coreAdapter)
	{
		return;
	}

	const auto& volume = m_coreAdapter->volume();
	if (!volume)
	{
		qCWarning(lcWidgetDicom) << "Missing volume while initializing window/level.";
		return;
	}

	m_pipelineBuilder->resetWindowFromVolume(*volume);
}

void gui::vtkWidgetDICOM::SetSlice(const int slice)
{
	if (m_viewController)
	{
		m_viewController->setSlice(slice);
	}
}

void gui::vtkWidgetDICOM::SetSliceOrientation(const int orientation)
{
	if (m_viewController)
	{
		m_viewController->setSliceOrientation(orientation);
	}
}

void gui::vtkWidgetDICOM::SetSliceOrientationToXY()
{
	SetSliceOrientation(SLICE_ORIENTATION_XY);
}

void gui::vtkWidgetDICOM::SetSliceOrientationToXZ()
{
	SetSliceOrientation(SLICE_ORIENTATION_XZ);
}

void gui::vtkWidgetDICOM::SetSliceOrientationToYZ()
{
	SetSliceOrientation(SLICE_ORIENTATION_YZ);
}

void gui::vtkWidgetDICOM::UpdateDisplayExtent()
{
	if (m_viewController)
	{
		m_viewController->updateDisplayExtent();
	}
}

double gui::vtkWidgetDICOM::getZoomFactor()
{
	if (!m_viewController)
	{
		return 0.0;
	}
	return m_viewController->getZoomFactor();
}

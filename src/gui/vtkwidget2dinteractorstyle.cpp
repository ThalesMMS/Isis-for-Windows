/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget2dinteractorstyle.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements a custom interactor style that drives 2D slice navigation and tools.
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

#include "vtkwidget2dinteractorstyle.h"
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkObjectFactory.h>
#include "utils.h"

#include "dicomvolume.h"

#include <algorithm>

vtkStandardNewMacro(isis::gui::vtkWidget2DInteractorStyle);

void isis::gui::vtkWidget2DInteractorStyle::OnMouseMove()
{
        if (m_leftScrolling && m_widget2D)
        {
                const int deltaY = Interactor->GetEventPosition()[1] - Interactor->GetLastEventPosition()[1];
                m_scrollDragAccumulator += static_cast<double>(deltaY);
                constexpr double pixelsPerStep = 5.0;
                const int steps = static_cast<int>(m_scrollDragAccumulator / pixelsPerStep);
                if (steps != 0)
                {
                        int targetIndex = m_currentImageIndex + steps;
                        if (m_image && !m_image->getIsMultiFrame())
                        {
                                int total = 0;
                                if (m_series)
                                {
                                        total = static_cast<int>(m_series->getSinlgeFrameImages().size());
                                }
                                if (total > 0)
                                {
                                        targetIndex = std::clamp(targetIndex, 0, total - 1);
                                }
                                else
                                {
                                        targetIndex = 0;
                                }
                        }
                        else if (m_widget2D && m_widget2D->getDCMWidget())
                        {
                                const int minSlice = m_widget2D->getDCMWidget()->GetSliceMin();
                                const int maxSlice = m_widget2D->getDCMWidget()->GetSliceMax();
                                targetIndex = std::clamp(targetIndex, minSlice, maxSlice);
                        }
                        if (targetIndex != m_currentImageIndex)
                        {
                                changeImage(targetIndex);
                        }
                        m_scrollDragAccumulator -= static_cast<double>(steps) * pixelsPerStep;
                }
                updateOverlayHUValue();
                Interactor->Render();
                return;
        }

        switch (State)
        {
        case VTKIS_WINDOW_LEVEL:
                updateOverlayWindowLevelApply();
                return;
        default:
                updateOverlayHUValue();
                Interactor->Render();
                break;
        }
        vtkInteractorStyleImage::OnMouseMove();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnMouseWheelForward()
{
	try
	{
		if (m_image)
		{
			if (!m_image->getIsMultiFrame())
			{
				auto* const previousImage = m_series->getPreviousSingleFrameImage(m_image);
				if (!m_image->equal(previousImage))
				{
					int index = m_series->
						findImageIndex(m_series->getSinlgeFrameImages(), m_image);
					changeImage(--index);
				}
			}
			else
			{
				auto indexImage = m_widget2D->getDCMWidget()->GetSlice();
				if (indexImage != m_widget2D->getDCMWidget()->GetSliceMin())
				{
					changeImage(--indexImage);
				}
			}
		}
	}
	catch (const std::exception& ex)
	{
		//todo log
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnMouseWheelBackward()
{
	try
	{
		if (m_image)
		{
			if (!m_image->getIsMultiFrame())
			{
				auto* const nextImage = m_series->getNextSingleFrameImage(m_image);
				if (!m_image->equal(nextImage))
				{
					int index = m_series->
						findImageIndex(m_series->getSinlgeFrameImages(), m_image);
					changeImage(++index);
				}
			}
			else
			{
				auto indexImage = m_widget2D->getDCMWidget()->GetSlice();
				if (indexImage != m_widget2D->getDCMWidget()->GetSliceMax())
				{
					changeImage(++indexImage);
				}
			}
		}
	}
	catch (const std::exception& ex)
	{
		//todo log
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::changeImage(int& t_index)
{
	if (m_image)
	{
		if (!m_image->getIsMultiFrame())
		{
			m_image = m_series->getSingleFrameImageByIndex(t_index);
			refreshImage();
			if (m_image->getModality() == "MR")
			{
				m_widget2D->updateOverlayWindowLevelApply(m_image->getWindowWidth(),
					m_image->getWindowCenter(), false);
			}
		}
		else
		{
			m_widget2D->getDCMWidget()->SetSlice(t_index);
		}
		m_currentImageIndex = t_index;
		Interactor->GetInteractorStyle()->InvokeEvent(changeScrollValue);
		updateOvelayImageNumber(t_index);
	}
	else
	{
		throw std::runtime_error("Image is null!");
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnLeftButtonDown()
{
        m_leftScrolling = false;
        m_scrollDragAccumulator = 0.0;
        if (!m_widget2D)
        {
                vtkInteractorStyleImage::OnLeftButtonDown();
                return;
        }

        switch (m_widget2D->activeTool())
        {
        case InteractionTool::scroll:
                if (m_image || (m_widget2D && m_widget2D->getDCMWidget()))
                {
                        m_leftScrolling = true;
                        return;
                }
                break;
        case InteractionTool::window:
                StartWindowLevel();
                return;
        case InteractionTool::zoom:
                StartDolly();
                return;
        case InteractionTool::pan:
                StartPan();
                return;
        default:
                break;
        }
        vtkInteractorStyleImage::OnLeftButtonDown();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnLeftButtonUp()
{
        if (m_leftScrolling)
        {
                m_leftScrolling = false;
                m_scrollDragAccumulator = 0.0;
                updateOverlayHUValue();
                Interactor->Render();
                return;
        }
        StopState();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnRightButtonDown()
{
	StartDolly();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnRightButtonUp()
{
	StopState();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnMiddleButtonDown()
{
	StartPan();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::OnMiddleButtonUp()
{
	StopState();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::Dolly()
{
	vtkInteractorStyleImage::Dolly();
	m_widget2D->updateOverlayZoomFactor();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::refreshImage() const
{
	if (!m_image)
	{
		throw std::runtime_error("Image is null!");
	}
	if (!m_widget2D)
	{
		throw std::runtime_error("Widget2D is null!");
	}
	const auto volume = m_image->getDicomVolume();
	if (!volume || !volume->ImageData)
	{
		throw std::runtime_error("Unable to refresh image: missing DICOM volume.");
	}
	m_widget2D->setImage(m_image);
	m_widget2D->setVolume(volume);
	if (auto widgetDicom = m_widget2D->getDCMWidget())
	{
		widgetDicom->setVolume(volume);
	}
	m_widget2D->render();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::updateOverlayWindowLevelApply() const
{
	const auto x =
		Interactor->GetEventPosition()[0] - Interactor->GetLastEventPosition()[0];
	const auto y =
		Interactor->GetEventPosition()[1] - Interactor->GetLastEventPosition()[1];
	m_widget2D->updateOverlayWindowLevelApply(x, y, true);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::updateOverlayHUValue() const
{
	const auto x = Interactor->GetEventPosition()[0];
	const auto y = Interactor->GetEventPosition()[1];
	double world[4];
	ComputeDisplayToWorld(m_widget2D->getDCMWidget()->GetRenderWindow()
	                                ->GetRenderers()->GetFirstRenderer(), x, y, 0, world);
	m_widget2D->updateOverlayHUValue(world[0] / m_image->getPixelSpacingX(),
		world[1] / m_image->getPixelSpacingY());
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidget2DInteractorStyle::updateOvelayImageNumber(const int& t_index) const
{
	const auto max = !m_image->getIsMultiFrame()
		                 ? m_series->getSinlgeFrameImages().size()
		                 : m_image->getNumberOfFrames();
	m_widget2D->updateOvelayImageNumber(t_index, max,
	                                    std::stoi(m_series->getNumber()));
}

//-----------------------------------------------------------------------------
isis::core::Image* isis::gui::vtkWidget2DInteractorStyle::getNextImage() const
{
	return m_image ? m_series->getNextSingleFrameImage(m_image) : nullptr;
}

//-----------------------------------------------------------------------------
isis::core::Image* isis::gui::vtkWidget2DInteractorStyle::getPreviousImage() const
{
	return m_image ? m_series->getPreviousSingleFrameImage(m_image) : nullptr;
}


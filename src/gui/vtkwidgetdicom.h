/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetdicom.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkWidgetDicom helper that prepares VTK pipelines for displaying DICOM volumes.
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

#pragma once

#include <array>
#include <limits>
#include <memory>
#include <tuple>

#include <vtkObject.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include "dicomcoresyncadapter.h"
#include "dicomviewcontroller.h"
#include "dicomvtkpipelinebuilder.h"

class vtkImageActor;

namespace isis::core
{
	struct DicomVolume;
}

namespace isis::gui
{
	class vtkWidgetDICOM final : public vtkObject
	{
	public:
		static vtkWidgetDICOM* New();
		vtkTypeMacro(vtkWidgetDICOM, vtkObject);

		vtkWidgetDICOM();
		~vtkWidgetDICOM() override;

		[[nodiscard]] double getZoomFactor();
		[[nodiscard]] int getWindowWidth() const { return m_pipelineBuilder ? m_pipelineBuilder->windowWidth() : 0; }
		[[nodiscard]] int getWindowCenter() const { return m_pipelineBuilder ? m_pipelineBuilder->windowCenter() : 0; }

		void setVolume(const std::shared_ptr<core::DicomVolume>& t_volume);
		void setInvertColors(bool t_flag);
		void setWindowWidthCenter(int t_width, int t_center);
		void changeWindowWidthCenter(int t_width, int t_center);
		void setInitialWindowWidthCenter();

		void SetRenderWindow(vtkRenderWindow* renderWindow);
		[[nodiscard]] vtkRenderWindow* GetRenderWindow() const
		{
			return m_viewController ? m_viewController->renderWindow() : nullptr;
		}
                [[nodiscard]] vtkRenderer* GetRenderer() const
		{
			return m_viewController ? m_viewController->renderer() : nullptr;
		}
                [[nodiscard]] vtkImageActor* GetImageActor() const
		{
			return m_pipelineBuilder ? m_pipelineBuilder->imageActor() : nullptr;
		}
                [[nodiscard]] std::array<double, 3> getLastSanitizedSpacing() const
		{
			return m_viewController ? m_viewController->lastSanitizedSpacing() : std::array<double, 3>{1.0, 1.0, 1.0};
		}
                [[nodiscard]] std::array<double, 3> getOriginalSpacing() const
		{
			return m_viewController ? m_viewController->originalSpacing() : std::array<double, 3>{1.0, 1.0, 1.0};
		}
                [[nodiscard]] bool hasOriginalSpacing() const
		{
			return m_viewController ? m_viewController->hasOriginalSpacing() : false;
		}

                void SetupInteractor(vtkRenderWindowInteractor* interactor);
                [[nodiscard]] vtkRenderWindowInteractor* GetInteractor() const
		{
			return m_viewController ? m_viewController->interactor() : nullptr;
		}

		void Render();

		void SetSlice(int slice);
		[[nodiscard]] int GetSlice() const { return m_viewController ? m_viewController->currentSlice() : 0; }
		[[nodiscard]] int GetSliceMin() const { return m_viewController ? m_viewController->sliceMin() : 0; }
		[[nodiscard]] int GetSliceMax() const { return m_viewController ? m_viewController->sliceMax() : 0; }

		void SetSliceOrientation(int orientation);
		void SetSliceOrientationToXY();
		void SetSliceOrientationToXZ();
		void SetSliceOrientationToYZ();
		[[nodiscard]] int GetSliceOrientation() const
		{
			return m_viewController ? m_viewController->sliceOrientation() : SLICE_ORIENTATION_XY;
		}

		void UpdateDisplayExtent();

		static constexpr int SLICE_ORIENTATION_YZ = 0;
		static constexpr int SLICE_ORIENTATION_XZ = 1;
		static constexpr int SLICE_ORIENTATION_XY = 2;

	private:
		std::unique_ptr<DicomVTKPipelineBuilder> m_pipelineBuilder;
		std::unique_ptr<DicomCoreSyncAdapter> m_coreAdapter;
		std::unique_ptr<DicomViewController> m_viewController;
        };
}

/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetoverlay.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkWidgetOverlay class for managing VTK text overlays tied to DICOM metadata.
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

#include <memory>

#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "corneroverlay.h"
#include "dicomvolume.h"
#include "overlayinfo.h"

namespace isis::gui
{
	class vtkWidgetOverlay
	{
	public:
		vtkWidgetOverlay();
		~vtkWidgetOverlay();

		//getters
		[[nodiscard]] vtkRenderer* getRenderer() const { return m_render; }
		[[nodiscard]] vtkOpenGLTextActor* getOverlayInCorner(const int& t_corner)
		{ return m_cornersOfOverlay[t_corner]->getTextActor(); }

		//setters
		void setRenderer(vtkRenderer* t_renderer) { m_render = t_renderer; }
		void setOverlayInCorner(const int& t_corner) { m_cornersOfOverlay[t_corner]->initTextActor(); }

		void createOverlay(vtkRenderWindow* t_renderWindow, const core::DicomMetadata* t_metadata);
		void clearOverlay();
		void updateOverlayInCorner(const int& t_corner, const std::string& t_key,const std::string& t_value)
		{ m_cornersOfOverlay[t_corner]->setOverlayInfo(t_key, t_value); }
		void positionOverlay();

	private:
		vtkSmartPointer<vtkTextProperty> m_textProperty[4] = {};
		std::unique_ptr<CornerOverlay> m_cornersOfOverlay[4] = {};
		vtkRenderer* m_render = {};
		std::vector<std::unique_ptr<OverlayInfo>> m_overlays;
		unsigned long m_observerTag = 0;
		

		void createOverlayInCorners();
		void readOverlayInfo();
		void createOverlayInfo(const core::DicomMetadata* t_metadata);
		void setPositionOfOverlayInCorner(const int& x, const int& y, const int& nr);
		void createCallback(vtkRenderWindow* t_renderWindow);
		[[nodiscard]] std::string replaceInvalidCharactersInString(const std::string& t_string);
		[[nodiscard]] std::tuple<int, int> computeCurrentOverlayPosition(const int& i, const int& j, const int* size);
	};
}


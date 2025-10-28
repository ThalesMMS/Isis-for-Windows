/*
 * ------------------------------------------------------------------------------------
 *  File: widgetbase.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the WidgetBase abstraction shared by GUI widgets built around VTK render windows.
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

#include <QWidget>
#include <vtkRenderWindow.h>
#include "loadinganimation.h"
#include "vtkeventfilter.h"
#include "series.h"


namespace isis::gui
{
	class WidgetBase : public QWidget
	{
	Q_OBJECT
	public:
		explicit WidgetBase(QWidget* t_parent);
		~WidgetBase() = default;

		virtual void render() = 0;
			
		enum class WidgetType
		{
			none,
			widget2d,
			widgetmpr,
			widget3d
		};

		//getters
		[[nodiscard]] core::Series* getSeries() const { return m_series; }
		[[nodiscard]] core::Image* getImage() const { return m_image; }
		[[nodiscard]] bool getIsImageLoaded() const { return m_isImageLoaded; }
		[[nodiscard]] WidgetType getWidgetType() const { return m_widgetType; }
		[[nodiscard]] QWidget* getTabWidget() const { return m_tabWidget; }
		[[nodiscard]] vtkEventFilter* getvtkEventFilter() const { return m_vtkEvents.get(); }
		[[nodiscard]] int getPatientIndex() const { return m_patientIndex; }
		[[nodiscard]] int getStudyInex() const { return m_studyIndex; }
		[[nodiscard]] int getSeriesIndex() const { return m_seriesIndex; }
		[[nodiscard]] int getImageIndex() const { return m_imageIndex; }

		//setters
		void setSeries(core::Series* t_series) { m_series = t_series; }
		void setImage(core::Image* t_image) { m_image = t_image; }
		void setIsImageLoaded(const bool& t_flag) { m_isImageLoaded = t_flag; }
		void setWidgetType(const WidgetType& t_widgetType) { m_widgetType = t_widgetType; }
		void setIndexes(const int& t_patientIndex, const int& t_studyIndex, const int& t_seriesIndex,
		                const int& t_imageIndex);
		void setTabWidget(QWidget* t_tabWidget) { m_tabWidget = t_tabWidget; }
	protected:
		QWidget* m_tabWidget = {};
		core::Series* m_series = {};
		core::Image* m_image = {};
		vtkSmartPointer<vtkRenderWindow> m_renderWindow[3] = {};
		std::unique_ptr<vtkEventFilter> m_vtkEvents = {};
		std::unique_ptr<LoadingAnimation> m_loadingAnimation = {};
		bool m_isImageLoaded = false;
		WidgetType m_widgetType = WidgetType::none;
		int m_patientIndex = {};
		int m_studyIndex = {};
		int m_seriesIndex = {};
		int m_imageIndex = {};

		virtual void initView() = 0;
		virtual void initData() = 0;
		virtual void createConnections() = 0;
		virtual void resetView() {};
		virtual void setSliderValues(const int& t_min, const int& t_max, const int& t_value) {};
		virtual void startLoadingAnimation() = 0;
		void stopLoadingAnimation();
		void focusInEvent(QFocusEvent* event) override;
	};
}


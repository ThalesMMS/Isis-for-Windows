/*
 * ------------------------------------------------------------------------------------
 *  File: mprmaker.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the MPRMaker utility that prepares datasets for multiplanar reconstruction.
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
#include <limits>
#include <QString>

#include <vtkImageResliceToColors.h>
#include <vtkImageReslice.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include "dicomvolume.h"
#include "vtkdicomvolumeloader.h"

namespace isis::core
{
	class Image;
	class Series;
}

namespace isis::gui
{
	class MPRMaker
	{
	public:
		MPRMaker() { initialize(); }
		~MPRMaker() = default;

		void setInitialWindowWidth(const int& t_width) { m_initialWindowWidth = t_width; }
		void setInitialWindowCenter(const int& t_center) { m_initialWindowCenter = t_center; }
		// Backwards-compatible aliases; prefer the explicit width/center setters.
		void setInitialWindow(const int& t_window) { setInitialWindowWidth(t_window); }
		void setInitialLevel(const int& t_level) { setInitialWindowCenter(t_level); }
		void setSeries(core::Series* t_series) { m_series = t_series; }
		void setImage(core::Image* t_image) { m_image = t_image; }
		void SetRenderWindows(
			const vtkSmartPointer<vtkRenderWindow>& t_sagittalWindow,
			const vtkSmartPointer<vtkRenderWindow>& t_coronalWindow,
			const vtkSmartPointer<vtkRenderWindow>& t_axialWindow);

		//getters
		[[nodiscard]] core::Series* getSeries() const { return m_series; }
		[[nodiscard]] core::Image* getImage() const { return m_image; }
		[[nodiscard]] int initialWindowWidth() const { return m_initialWindowWidth; }
		[[nodiscard]] int initialWindowCenter() const { return m_initialWindowCenter; }
		// Backwards-compatible aliases; prefer the explicit accessors.
		[[nodiscard]] int getInitialWindow() const { return initialWindowWidth(); }
		[[nodiscard]] int getInitalLevel() const { return initialWindowCenter(); }
		[[nodiscard]] vtkImageResliceToColors* getImageReslice(const int t_plane) const { return m_reslicer[t_plane]; }
		[[nodiscard]] vtkImageReslice* getOriginalValueImageReslice(int t_plane);
		[[nodiscard]] vtkImageData* getInputData() const { return m_volume && m_volume->ImageData ? m_volume->ImageData : nullptr; }
		[[nodiscard]] std::shared_ptr<VtkDicomVolume> getVolume() const { return m_volume; }
		[[nodiscard]] double getCenterSliceZPosition(int t_plane) const;
		[[nodiscard]] vtkSmartPointer<vtkScalarsToColors> getColorMapScalar() const { return m_colorMap; }
		void updateColorMapWindowLevel(double t_window, double t_level);
                [[nodiscard]] QString lastFailureMessage() const { return m_lastFailure; }
                [[nodiscard]] QString lastWarningMessage() const { return m_lastWarning; }


                void create3DMatrix();
                void createMPR();
                void resetMatrixesToInitialPosition();
                void resetWindowLevel();

                void overrideFailureMessage(const QString& failureMessage);
                void setVolumeForTesting(const std::shared_ptr<VtkDicomVolume>& volume);

	private:
		core::Series* m_series = {};
		core::Image* m_image = {};
		int m_initialWindowWidth = 0;
		int m_initialWindowCenter = 0;
        std::shared_ptr<VtkDicomVolume> m_volume = {};
        VtkDicomVolumeLoader m_volumeLoader = {};
		vtkSmartPointer<vtkImageResliceToColors> m_reslicer[3] = {};
		vtkSmartPointer<vtkImageReslice> m_originalValuesReslicer[3] = {};
		vtkSmartPointer<vtkRenderWindow> m_renderWindow[3] = {};
		vtkSmartPointer<vtkScalarsToColors> m_colorMap = {};
		QString m_lastFailure = {};
		QString m_lastWarning = {};
		double m_lastRenderedWindow = std::numeric_limits<double>::quiet_NaN();
		double m_lastRenderedLevel = std::numeric_limits<double>::quiet_NaN();

		double m_sagittalMatrix[16] = {
			0, 0, 1, 0,
			-1, 0, 0, 0,
			0, -1, 0, 0,
			0, 0, 0, 1
		};
		double m_coronalMatrix[16] = {
			1, 0, 0, 0,
			0, 0, 1, 0,
			0, -1, 0, 0,
			0, 0, 0, 1
		};
		double m_axialMatrix[16] = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};

		void initialize();
		void setInitialMatrix();
		void createMprViews();
        void setMiddleSlice(int t_plane);
		void renderPlaneOffScreen(int t_plane);
        void resetWindowLevelCache();
	};
}


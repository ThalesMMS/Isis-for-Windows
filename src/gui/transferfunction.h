/*
 * ------------------------------------------------------------------------------------
 *  File: transferfunction.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares helpers for constructing color and opacity transfer functions.
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

#include <vector>
#include <memory>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkSmartPointer.h>

#include <utility>

class QJsonArray;
class QString;

namespace isis::gui
{
	class Color
	{
	public:
		Color() = default;
		~Color() = default;

		//getters
		[[nodiscard]] int getValue() const { return m_value; }
		[[nodiscard]] double getRed() const { return m_red; }
		[[nodiscard]] double getGreen() const { return m_green; }
		[[nodiscard]] double getBlue() const { return m_blue; }

		//setters
		void setValue(const int& t_value) { m_value = t_value; }
		void setRed(const double& t_red) { m_red = t_red; }
		void setGreen(const double& t_green) { m_green = t_green; }
		void setBlue(const double& t_blue) { m_blue = t_blue; }

	private:
		int m_value = {};
		double m_red = {};
		double m_green = {};
		double m_blue = {};
	};

	class Opacity
	{
	public:
		Opacity() = default;
		~Opacity() = default;

		//getters
		[[nodiscard]] int getValue() const { return m_value; }
		[[nodiscard]] double getAlpha() const { return m_alpha; }

		//setters
		void setValue(const int& t_value) { m_value = t_value; }
		void setAlpha(const double& t_alpha) { m_alpha = t_alpha; }

	private:
		int m_value = {};
		double m_alpha = {};
	};
	
	class TransferFunction
	{
	public:
		enum class Preset
		{
			Automatic,
			CTSoftTissue,
			MRDefault,
			MaximumIntensity,
			Custom
		};

		TransferFunction() = default;
		~TransferFunction() = default;

		//getters
		[[nodiscard]] int getWindow() const { return m_window; }
		[[nodiscard]] int getLevel() const { return m_level; }
		[[nodiscard]] double getAmbient() const { return m_ambient; }
		[[nodiscard]] double getDiffuse() const { return m_diffuse; }
		[[nodiscard]] double getSpecular() const { return m_specular; }
		[[nodiscard]] double getSpecularPower() const { return m_specularPower; }
		[[nodiscard]] bool getHasShade() const { return m_shade; }
 		[[nodiscard]] vtkColorTransferFunction* getColorFunction() const { return m_colorFunction ? m_colorFunction : nullptr; }
		[[nodiscard]] vtkPiecewiseFunction* getOpacityFunction() const { return m_opacityFunction ? m_opacityFunction : nullptr; }
		[[nodiscard]] vtkPiecewiseFunction* getGradientFunction() const { return m_gradientFunction ? m_gradientFunction : nullptr; }
		[[nodiscard]] Preset currentPreset() const { return m_currentPreset; }
		[[nodiscard]] bool isCustom() const { return m_isCustom; }
		[[nodiscard]] double gradientThreshold() const { return m_gradientThreshold; }
		[[nodiscard]] double gradientMaxOpacity() const { return m_gradientMaxOpacity; }

		//setters
		void setWindow(const int& t_window) { m_window = t_window; }
		void setLevel(const int& t_level) { m_level = t_level; }
		void setAmbient(const double& t_ambient) { m_ambient = t_ambient; }
		void setDiffuse(const double& t_diffuse) { m_diffuse = t_diffuse; }
		void setSpecular(const double& t_specular) { m_specular = t_specular; }
		void setSpecularPower(const double& t_specularPower) { m_specularPower = t_specularPower; }
		void setHasShade(const bool& t_shade) { m_shade = t_shade; }
		void setMaximumIntensityProjectionFunction(const int& t_windowCenter, const int& t_windowWidth);
		void initializeDefaultCurve();
		void applyPreset(Preset preset);
		void setGradientOpacityParameters(double threshold, double maxOpacity);
		void markCustom(Preset preset = Preset::Custom);

                void updateWindowLevel(const double& t_window, const double& t_level);
                void updateWindowLevelDelta(const double& t_windowDelta, const double& t_levelDelta);
		void loadFilterFromFile(const QString& t_fileName);
		

	private:
		int m_window = {};
		int m_level = {};
		double m_ambient = 0.1;
		double m_diffuse = 0.9;
		double m_specular = 1.0;
		double m_specularPower = 64;
		bool m_shade = true;
		std::vector<std::unique_ptr<Color>> m_colors = {};
		std::vector<std::unique_ptr<Opacity>> m_opacities = {};
		vtkSmartPointer<vtkColorTransferFunction> m_colorFunction = {};
		vtkSmartPointer<vtkPiecewiseFunction> m_opacityFunction = {};
		vtkSmartPointer<vtkPiecewiseFunction> m_gradientFunction = {};
		bool m_isCustom = false;
		Preset m_currentPreset = Preset::Automatic;
		double m_gradientThreshold = 30.0;
		double m_gradientMaxOpacity = 0.2;
		

                void extractColorFunctionInfo(const QJsonArray& t_array);
                void extractOpacityFunctionInfo(const QJsonArray& t_array);
                void rebuildTransferFunctions();
		void clearTransferFunctions();
		void addColorPoint(int value, double red, double green, double blue);
		void addOpacityPoint(int value, double alpha);
		
	};
}

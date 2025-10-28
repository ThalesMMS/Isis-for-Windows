/*
 * ------------------------------------------------------------------------------------
 *  File: transferfunction.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Builds and updates VTK color and opacity transfer functions for volume rendering.
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

#include "transferfunction.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::setMaximumIntensityProjectionFunction(const int& t_windowCenter,
                                                                             const int& t_windowWidth)
{
	m_ambient = 0.1;
	m_diffuse = 0.7;
	m_specular = 1;
	m_specularPower = 64;
	m_window = t_windowCenter;
	m_level = t_windowWidth;
	m_shade = true;
	m_isCustom = true;
	m_currentPreset = Preset::MaximumIntensity;
	m_gradientFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_gradientFunction->RemoveAllPoints();
	m_gradientFunction->AddPoint(0.0, 1.0);
	m_gradientFunction->AddPoint(1000.0, 1.0);
	m_gradientThreshold = 0.0;
	m_gradientMaxOpacity = 1.0;
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::clearTransferFunctions()
{
	m_colorFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
	m_opacityFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_colors.clear();
	m_opacities.clear();
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::addColorPoint(const int value,
	double red,
	double green,
	double blue)
{
	auto colorPoint = std::make_unique<Color>();
	colorPoint->setValue(value);
	colorPoint->setRed(red);
	colorPoint->setGreen(green);
	colorPoint->setBlue(blue);
	m_colorFunction->AddRGBPoint(colorPoint->getValue(),
		colorPoint->getRed(),
		colorPoint->getGreen(),
		colorPoint->getBlue());
	m_colors.emplace_back(std::move(colorPoint));
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::addOpacityPoint(const int value, double alpha)
{
	auto opacityPoint = std::make_unique<Opacity>();
	opacityPoint->setValue(value);
	opacityPoint->setAlpha(alpha);
	m_opacityFunction->AddPoint(opacityPoint->getValue(), opacityPoint->getAlpha());
	m_opacities.emplace_back(std::move(opacityPoint));
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::initializeDefaultCurve()
{
	clearTransferFunctions();

	addColorPoint(0, 0.0, 0.0, 0.0);
	addColorPoint(1000, 1.0, 1.0, 1.0);

	addOpacityPoint(0, 0.0);
	addOpacityPoint(1000, 1.0);

	m_window = 0;
	m_level = 0;
	m_ambient = 0.1;
	m_diffuse = 0.9;
	m_specular = 1.0;
	m_specularPower = 64;
	m_shade = true;
	m_isCustom = false;
	m_currentPreset = Preset::Automatic;
	m_gradientFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
	setGradientOpacityParameters(m_gradientThreshold, m_gradientMaxOpacity);
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::applyPreset(const Preset preset)
{
	if (preset == Preset::Automatic)
	{
		initializeDefaultCurve();
		return;
	}

	clearTransferFunctions();
	m_isCustom = false;
	m_currentPreset = preset;

	switch (preset)
	{
	case Preset::CTSoftTissue:
		m_ambient = 0.2;
		m_diffuse = 1.0;
		m_specular = 0.05;
		m_specularPower = 4.0;
		m_shade = true;

		addColorPoint(0, 0.0, 0.0, 0.0);
		addColorPoint(180, 0.25, 0.25, 0.25);
		addColorPoint(500, 0.88, 0.60, 0.29);
		addColorPoint(750, 0.95, 0.82, 0.68);
		addColorPoint(1000, 1.0, 1.0, 1.0);

		addOpacityPoint(0, 0.0);
		addOpacityPoint(200, 0.0);
		addOpacityPoint(520, 0.22);
		addOpacityPoint(780, 0.60);
		addOpacityPoint(950, 1.0);
		break;
	case Preset::MRDefault:
		m_ambient = 0.12;
		m_diffuse = 0.9;
		m_specular = 0.12;
		m_specularPower = 10.0;
		m_shade = true;

		addColorPoint(0, 0.0, 0.0, 0.0);
		addColorPoint(220, 0.08, 0.08, 0.35);
		addColorPoint(500, 0.35, 0.30, 0.62);
		addColorPoint(760, 0.78, 0.78, 0.94);
		addColorPoint(1000, 1.0, 1.0, 1.0);

		addOpacityPoint(0, 0.0);
		addOpacityPoint(250, 0.02);
		addOpacityPoint(540, 0.28);
		addOpacityPoint(780, 0.58);
		addOpacityPoint(970, 0.9);
		break;
	default:
		initializeDefaultCurve();
		return;
	}

	rebuildTransferFunctions();
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::setGradientOpacityParameters(double threshold, double maxOpacity)
{
	m_gradientThreshold = threshold;
	m_gradientMaxOpacity = maxOpacity;
	if (!m_gradientFunction)
	{
		m_gradientFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
	}
	m_gradientFunction->RemoveAllPoints();
	const double epsilon = std::max(1.0, threshold * 0.1);
	const double lower = std::max(threshold - epsilon, 0.0);
	const double upper = threshold + epsilon;
	m_gradientFunction->AddPoint(0.0, 0.0);
	m_gradientFunction->AddPoint(lower, 0.0);
	m_gradientFunction->AddPoint(upper, maxOpacity);
	m_gradientFunction->AddPoint(std::max(upper * 4.0, upper + 100.0), maxOpacity);
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::markCustom(const Preset preset)
{
	m_isCustom = true;
	m_currentPreset = preset;
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::updateWindowLevel(const double& t_window, const double& t_level)
{
        m_window = static_cast<int>(t_window);
        m_level = static_cast<int>(t_level);
        rebuildTransferFunctions();
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::updateWindowLevelDelta(const double& t_windowDelta,
                                                              const double& t_levelDelta)
{
        m_window += static_cast<int>(t_windowDelta);
        m_level += static_cast<int>(t_levelDelta);
        rebuildTransferFunctions();
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::loadFilterFromFile(const QString& t_fileName)
{
	QFile filter(t_fileName);
	if (filter.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		const QByteArray data = filter.readAll();
		filter.close();
		const auto doc = QJsonDocument::fromJson(data);
		const QJsonObject root = doc.object();
		clearTransferFunctions();
		extractColorFunctionInfo(root.value("color").toArray());
		extractOpacityFunctionInfo(root.value("opacity").toArray());
		m_ambient = root.value("ambient").toObject()["value"].toDouble();
		m_diffuse = root.value("diffuse").toObject()["value"].toDouble();
		m_specular = root.value("specular").toObject()["value"].toDouble();
		m_specularPower = root.value("specularpower").toObject()["value"].toDouble();
		m_shade = root.value("shade").toObject()["value"].toInt();
		m_window = 1000;
		m_level = 0;
		m_gradientFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		m_gradientFunction->RemoveAllPoints();
		m_gradientFunction->AddPoint(0.0, 0.0);
		m_gradientFunction->AddPoint(1000.0, 1.0);
		m_gradientThreshold = 0.0;
		m_gradientMaxOpacity = 1.0;
		markCustom(Preset::Custom);
		rebuildTransferFunctions();
	}
	else
	{
		throw std::runtime_error("Filter file not found!");
	}
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::extractColorFunctionInfo(const QJsonArray& t_array)
{
	m_colorFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
	m_colors.clear();
	for (const auto& value : t_array)
	{
		m_colors.emplace_back(std::make_unique<Color>());
		m_colors.back()->setValue(value.toObject()["value"].toDouble());
		m_colors.back()->setRed(value.toObject()["red"].toDouble());
		m_colors.back()->setGreen(value.toObject()["green"].toDouble());
		m_colors.back()->setBlue(value.toObject()["blue"].toDouble());
		m_colorFunction->AddRGBPoint(m_colors.back()->getValue(),
		                             m_colors.back()->getRed(), m_colors.back()->getGreen(),
		                             m_colors.back()->getBlue());
	}
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::extractOpacityFunctionInfo(const QJsonArray& t_array)
{
        m_opacityFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
        m_opacities.clear();
        for (const auto& value : t_array)
	{
		m_opacities.emplace_back(std::make_unique<Opacity>());
		m_opacities.back()->setValue(value.toObject()["value"].toDouble());
		m_opacities.back()->setAlpha(value.toObject()["alpha"].toDouble());
		m_opacityFunction->AddPoint(m_opacities.back()->getValue(),
                                            m_opacities.back()->getAlpha());
        }
}

//-----------------------------------------------------------------------------
void isis::gui::TransferFunction::rebuildTransferFunctions()
{
        if (m_colorFunction)
        {
                m_colorFunction->RemoveAllPoints();
                for (const auto& colorPoint : m_colors)
                {
                        m_colorFunction->AddRGBPoint(
                                m_level + m_window *
                                colorPoint->getValue() / 1000,
                                colorPoint->getRed(),
                                colorPoint->getGreen(),
                                colorPoint->getBlue());
                }
        }
        if (m_opacityFunction)
        {
                m_opacityFunction->RemoveAllPoints();
                for (const auto& opacityPoint : m_opacities)
                {
                        m_opacityFunction->AddPoint(
                                m_level + m_window *
                                opacityPoint->getValue() / 1000,
                                opacityPoint->getAlpha());
                }
        }
}


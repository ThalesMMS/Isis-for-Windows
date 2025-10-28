/*
 * ------------------------------------------------------------------------------------
 *  File: utils.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares shared UI constants, helper enums, and utilities for formatting
 *      DICOM metadata values.
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
#include <vtkCommand.h>

#include "dicomvolume.h"

#define export __declspec(dllexport)
#define unused(x) (void)x;
constexpr auto iconTitleBar = ":/app/icon_small.png";
constexpr auto iconapp = ":/app/icon_large.png";
constexpr auto appBackground = ":/res/background.png";
constexpr auto buttonMaximizeOff = ":/res/maximize-button1.png";
constexpr auto buttonMaximizeOn = ":/res/maximize-button2.png";
constexpr auto overlaysInformation = "res/overlays.json";
constexpr auto scroll2DStyle = "res/scroll2d.css";
constexpr auto filters3dDir = "res/filters3d";
constexpr auto activeTabStyle =
        "QTabWidget::pane[active=\"true\"] { background-color:#000000; border:1px solid #2A6FFF; border-radius:4px; } "
        "QTabWidget::pane { background-color:#000000; } "
        "QTabBar::tab { background-color:#3A3A3A; border:1px solid transparent; border-top-left-radius:4px; border-top-right-radius:4px; padding:6px 16px; margin:0 2px; color:#F5F5F5; min-height:24px; min-width:96px; } "
        "QTabBar::tab:selected { background-color:#4A4A4A; color:#F8D560; border-color:#2A6FFF; } "
        "QTabBar::tab:hover { background-color:#4F4F4F; } "
        "QTabBar::close-button { border:none; background-color:transparent; image:url(:/closeTab); } "
        "QTabBar::close-button:hover { background-color:rgba(248,213,96,0.18); } "
        "QTabBar::close-button:pressed { background-color:rgba(248,213,96,0.28); }";
constexpr auto inActiveTabStyle =
        "QTabWidget::pane[active=\"false\"] { background-color:#000000; border:1px solid #4C4C4C; border-radius:4px; } "
        "QTabWidget::pane { background-color:#000000; } "
        "QTabBar::tab { background-color:#363636; border:1px solid transparent; border-top-left-radius:4px; border-top-right-radius:4px; padding:6px 16px; margin:0 2px; color:#D0D0D0; min-height:24px; min-width:96px; } "
        "QTabBar::tab:selected { background-color:#424242; color:#E6E6E6; border-color:#4C4C4C; } "
        "QTabBar::tab:hover { background-color:#474747; } "
        "QTabBar::close-button { border:none; background-color:transparent; image:url(:/closeTab); } "
        "QTabBar::close-button:hover { background-color:rgba(248,213,96,0.12); } "
        "QTabBar::close-button:pressed { background-color:rgba(248,213,96,0.18); }";
constexpr auto menuBarStyle =
	"QMenuBar{background-color:transparent;border:none;}"
	"QMenuBar::item{color:white;padding:0 12px;margin:0 4px;background-color:transparent;}"
	"QMenuBar::item:selected{border-style:none;background-color:rgba(31,111,235,0.35);}"
	"QMenuBar::item:pressed{border-style:none;background-color:rgba(31,111,235,0.55);}";

enum class transformationType
{
	none,
	flipHorizontal,
	flipVertical,
	rotateLeft,
	rotateRight,
	invert,
	scrollMouse,
	zoom,
	pan,
	windowLevel
};

enum class overlayKey
{
        zoom = 1001,
        series = 1003,
        tool = 1005,
        window = 2625616,
        level = 2625617,
        spacing = 0x00FF0001,
        sliceLocation = 0x00FF0002
};

enum vtkCustomEvents : unsigned long
{
	changeScrollValue = vtkCommand::UserEvent + 1,
	defaultCursor = changeScrollValue + 1,
	cursorMove = defaultCursor + 1,
	cursorFinishMovement = cursorMove + 1,
	cursorRotate = cursorFinishMovement + 1,
	imageChanged = cursorRotate + 1,
	qualityLow = imageChanged + 1,
	qualityHigh = qualityLow + 1,
};

namespace isis::core
{
	class export Utils
	{
	public:
		Utils() = default;
		~Utils() = default;

		static void processTagFormat(const DicomTag& t_tag, std::string& t_value);

	private:
		[[nodiscard]] static std::string getDateFormat(const std::string& t_date);
		[[nodiscard]] static std::string getTimeFormat(const std::string& t_time);
	};
}



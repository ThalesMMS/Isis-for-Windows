/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetoverlay.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements overlay rendering and updates for displaying study information in VTK views.
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

#include "vtkwidgetoverlay.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <vtkRenderWindow.h>
#include <vtkTextProperty.h>
#include "utils.h"
#include "vtkwidgetoverlaycallback.h"

isis::gui::vtkWidgetOverlay::vtkWidgetOverlay()
{
	createOverlayInCorners();
	readOverlayInfo();
}

//-----------------------------------------------------------------------------
isis::gui::vtkWidgetOverlay::~vtkWidgetOverlay()
{
    if (!m_render)
    {
        return;
    }

    vtkRenderWindow* const renderWindow = m_render->GetRenderWindow();
    if (renderWindow && m_observerTag != 0)
    {
        renderWindow->RemoveObserver(m_observerTag);
        m_observerTag = 0;
    }
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::createOverlay(vtkRenderWindow* t_renderWindow, const isis::core::DicomMetadata* t_metadata)
{
    if (!m_render || !t_renderWindow)
    {
        return;
    }
	clearOverlay();
	createOverlayInfo(t_metadata);
	createCallback(t_renderWindow);
	positionOverlay();
	for (const auto& corner : m_cornersOfOverlay)
	{
		m_render->AddActor(corner->getTextActor());
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::clearOverlay()
{
	for (const auto& overlay : m_cornersOfOverlay)
	{
		overlay->clearOverlaysInfo();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::createOverlayInCorners()
{
	for (int i = 0; i < 4; ++i)
	{
		m_cornersOfOverlay[i] = std::make_unique<CornerOverlay>();
		m_cornersOfOverlay[i]->setNumberOfCorner(i);
		if (!m_textProperty[i])
		{
			m_textProperty[i] = vtkSmartPointer<vtkTextProperty>::New();
		}
		m_textProperty[i]->SetColor(1, 1, 1);
		m_textProperty[i]->SetFontFamilyAsString("Arial");
		m_textProperty[i]->SetFontSize(12);
		m_cornersOfOverlay[i]->setTextActorProperty(m_textProperty[i]);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::readOverlayInfo()
{
	const auto parseDocument = [this](const QJsonDocument& doc)
	{
		const QJsonObject root = doc.object();
		const QJsonArray values = root.value(QStringLiteral("Overlay")).toArray();
		for (const auto& value : values)
		{
			const QJsonObject object = value.toObject();
			auto overlay = std::make_unique<OverlayInfo>();
			overlay->setTextBefore(object.value(QStringLiteral("TextBefore")).toString().toStdString());
			overlay->setTextAfter(object.value(QStringLiteral("TextAfter")).toString().toStdString());
			overlay->setTagKey(object.value(QStringLiteral("TagKey")).toInt());
			overlay->setCorner(object.value(QStringLiteral("Corner")).toInt());
			m_overlays.emplace_back(std::move(overlay));
		}
	};

	const auto tryLoad = [&](const QString& candidate, bool logFailure) -> bool
	{
		QFile overlays(candidate);
		if (!overlays.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			if (logFailure)
			{
				qWarning() << "[vtkWidgetOverlay] Failed to open overlay configuration" << candidate << overlays.errorString();
			}
			return false;
		}

		const QByteArray data = overlays.readAll();
		overlays.close();
		const QJsonDocument doc = QJsonDocument::fromJson(data);
		if (doc.isNull())
		{
			qWarning() << "[vtkWidgetOverlay] Invalid JSON contents in overlay configuration" << candidate;
			return false;
		}

		parseDocument(doc);
		return true;
	};

        m_overlays.clear();

        const QString relativePath = QString::fromUtf8(overlaysInformation);
        QStringList searchCandidates;
        searchCandidates.reserve(10);
        searchCandidates.append(QDir::current().filePath(relativePath));

        QDir probeDir(QCoreApplication::applicationDirPath());
        for (int depth = 0; depth < 6; ++depth)
        {
                const QString candidate = probeDir.filePath(relativePath);
                if (!searchCandidates.contains(candidate))
                {
                        searchCandidates.append(candidate);
                }
                if (!probeDir.cdUp())
                {
                        break;
                }
        }

        if (!searchCandidates.contains(relativePath))
        {
                searchCandidates.append(relativePath);
        }
        searchCandidates.append(QStringLiteral(":/res/overlays.json"));

        for (const QString& candidate : searchCandidates)
        {
                const bool isResourcePath = candidate.startsWith(QLatin1Char(':'));
                if (!isResourcePath)
                {
                        QFileInfo info(candidate);
                        if (!info.exists() || !info.isFile())
                        {
                                continue;
                        }
                }
                if (tryLoad(candidate, !isResourcePath))
                {
                        return;
                }
        }

        qWarning() << "[vtkWidgetOverlay] Overlay configuration unavailable; overlays will remain hidden.";
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::positionOverlay()
{
    if (!m_render)
    {
        return;
    }

    vtkRenderWindow* const renderWindow = m_render->GetRenderWindow();
    if (!renderWindow)
    {
        return;
    }

    const int* const size = renderWindow->GetSize();
    if (!size)
    {
        return;
    }

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			const auto nr = i * 2 + j;
			const auto [x, y] =
				computeCurrentOverlayPosition(i, j, size);
			setPositionOfOverlayInCorner(x, y, nr);
		}
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::createOverlayInfo(const isis::core::DicomMetadata* t_metadata)
{
        if (!t_metadata)
        {
                qWarning() << "[vtkWidgetOverlay] Missing metadata. Overlay text will be limited.";
                return;
        }
        for (const auto& info : m_overlays)
        {
                const unsigned int key = info->getTagKey();
                isis::core::DicomTag tag(
                        static_cast<unsigned short>((key >> 16) & 0xFFFF),
                        static_cast<unsigned short>(key & 0xFFFF));
                auto tagValue = t_metadata->getString(tag);
                isis::core::Utils::processTagFormat(tag, tagValue);
                auto tagText = info->getTextBefore();
                tagText.append(replaceInvalidCharactersInString(tagValue));
                tagText.append(info->getTextAfter());
                if (!tagText.empty() && tagText != "\n"
                        && tagText != "Series: \n" && tagText != "Zoom: \n")
                {
                        m_cornersOfOverlay[info->getCorner()]->setOverlayInfo(
                                std::to_string(key), tagText);
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::setPositionOfOverlayInCorner(const int& x, const int& y, const int& nr)
{
	auto* const actor = getOverlayInCorner(nr);
	actor->SetPosition(x, y);
	switch (nr)
	{
	case 1:
		actor->GetTextProperty()->SetJustificationToLeft();
		actor->GetTextProperty()->SetVerticalJustificationToTop();
		break;
	case 2:
		actor->GetTextProperty()->SetJustificationToRight();
		actor->GetTextProperty()->SetVerticalJustificationToBottom();
		break;
	case 3:
		actor->GetTextProperty()->SetJustificationToRight();
		actor->GetTextProperty()->SetVerticalJustificationToTop();
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetOverlay::createCallback(vtkRenderWindow* t_renderWindow)
{
    if (!t_renderWindow)
    {
        return;
    }

    if (m_observerTag != 0)
    {
        t_renderWindow->RemoveObserver(m_observerTag);
        m_observerTag = 0;
    }

	const auto callback =
		vtkSmartPointer<vtkWidgetOverlayCallback>::New();
	callback->setWidget(this);
	m_observerTag = t_renderWindow->AddObserver(vtkCommand::ModifiedEvent, callback);
}

//-----------------------------------------------------------------------------
std::tuple<int, int> isis::gui::vtkWidgetOverlay::computeCurrentOverlayPosition(const int& i, const int& j,
                                                                                     const int* size)
{
	const auto x = !i
		               ? static_cast<double>(i * (size[0] - 5) + 10)
		               : static_cast<double>(i * (size[0] - 5) - 10);
	const auto y = !j
		               ? static_cast<double>(j * (size[1] - 5) + 20)
		               : static_cast<double>(j * (size[1] - 5) - 20);
	return std::make_tuple(x, y);
}

//-----------------------------------------------------------------------------
std::string isis::gui::vtkWidgetOverlay::replaceInvalidCharactersInString(const std::string& t_string)
{
	std::string s;
	const char* cp = t_string.c_str();
	std::size_t m = t_string.size();
	for (std::size_t n = 0; n < t_string.size(); n++)
	{
		m += static_cast<unsigned char>(*cp++) >> 7;
	}
	s.resize(m);
	cp = t_string.c_str();;
	std::size_t i = 0;
	while (i < m)
	{
		while (i < m && (*cp & 0x80) == 0)
		{
			s[i++] = *cp++;
		}
		if (i < m)
		{
			const int code = static_cast<unsigned char>(*cp++);
			s[i++] = (0xC0 | (code >> 6));
			s[i++] = (0x80 | (code & 0x3F));
		}
	}
	return s;
}



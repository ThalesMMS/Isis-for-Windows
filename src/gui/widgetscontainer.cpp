/*
 * ------------------------------------------------------------------------------------
 *  File: widgetscontainer.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Manages the stacked widget layouts used to display different viewer configurations.
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

#include "widgetscontainer.h"

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QSignalBlocker>
#include <QToolButton>
#include <QString>
#include <QVariant>
#include <algorithm>
#include <cmath>
#include <functional>
#include <QtGlobal>
#include <QtCore/Qt>

namespace
{
        QPixmap tintPixmap(const QPixmap& basePixmap, const QColor& tint)
        {
                if (basePixmap.isNull() || !tint.isValid())
                {
                        return basePixmap;
                }

                QImage image = basePixmap.toImage().convertToFormat(QImage::Format_ARGB32);
                for (int y = 0; y < image.height(); ++y)
                {
                        auto* const scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
                        for (int x = 0; x < image.width(); ++x)
                        {
                                const QColor original = QColor::fromRgba(scanLine[x]);
                                const int gray = qGray(original.rgba());
                                scanLine[x] = QColor(gray, gray, gray, original.alpha()).rgba();
                        }
                }

                QPixmap grayPixmap = QPixmap::fromImage(image);
                QPixmap tintedPixmap(grayPixmap.size());
                tintedPixmap.fill(Qt::transparent);

                QPainter painter(&tintedPixmap);
                painter.drawPixmap(0, 0, grayPixmap);
                painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                painter.fillRect(tintedPixmap.rect(), tint);
                painter.end();

                return tintedPixmap;
        }

        QIcon makeMonochromeIcon(const QIcon& source,
                const QColor& normalTint,
                const QColor& disabledTint)
        {
                if (source.isNull() || !normalTint.isValid())
                {
                        return source;
                }

                QIcon tintedIcon;
                QList<QSize> sizes = source.availableSizes(QIcon::Normal, QIcon::Off);
                if (sizes.isEmpty())
                {
                        sizes << QSize(22, 22) << QSize(24, 24) << QSize(32, 32);
                }

                for (const QSize& size : sizes)
                {
                        if (!size.isValid())
                        {
                                continue;
                        }

                        const QPixmap basePixmap = source.pixmap(size);
                        if (basePixmap.isNull())
                        {
                                continue;
                        }

                        const QPixmap normalPixmap = tintPixmap(basePixmap, normalTint);
                        tintedIcon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);

                        const QColor disabledColor = disabledTint.isValid()
                                ? disabledTint
                                : QColor(normalTint).darker(150);
                        const QPixmap disabledPixmap = tintPixmap(basePixmap, disabledColor);
                        tintedIcon.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::Off);
        }

        return tintedIcon.isNull() ? source : tintedIcon;
        }

        QIcon createGlyphIcon(const std::function<void(QPainter&, const QRectF&)>& painterFn)
        {
                if (!painterFn)
                {
                        return {};
                }

                const QSize iconSize(64, 64);
                QPixmap pixmap(iconSize);
                pixmap.fill(Qt::transparent);

                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing, true);

                QPen pen(Qt::white);
                pen.setWidthF(6.0);
                pen.setCapStyle(Qt::RoundCap);
                pen.setJoinStyle(Qt::RoundJoin);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);

                const QRectF bounds(8.0, 8.0,
                        static_cast<double>(iconSize.width()) - 16.0,
                        static_cast<double>(iconSize.height()) - 16.0);

                painterFn(painter, bounds);
                painter.end();
                return QIcon(pixmap);
        }

        void drawArrow(QPainter& painter, const QPointF& from, const QPointF& to)
        {
                painter.drawLine(from, to);
                constexpr double pi = 3.14159265358979323846;
                constexpr double angleSpan = pi / 8.0;
                constexpr double arrowSize = 10.0;
                const double angle = std::atan2(to.y() - from.y(), to.x() - from.x());
                const QPointF offset1(std::cos(angle - angleSpan) * arrowSize,
                        std::sin(angle - angleSpan) * arrowSize);
                const QPointF offset2(std::cos(angle + angleSpan) * arrowSize,
                        std::sin(angle + angleSpan) * arrowSize);
                painter.drawLine(to, to - offset1);
                painter.drawLine(to, to - offset2);
        }

        QIcon createScrollIcon()
        {
                return createGlyphIcon([](QPainter& painter, const QRectF& bounds)
                {
                        const double centerX = bounds.center().x();
                        const double gap = bounds.height() * 0.18;
                        drawArrow(painter,
                                QPointF(centerX, bounds.center().y() + gap),
                                QPointF(centerX, bounds.bottom()));
                        drawArrow(painter,
                                QPointF(centerX, bounds.center().y() - gap),
                                QPointF(centerX, bounds.top()));
                        painter.drawLine(QPointF(centerX, bounds.top() + gap),
                                QPointF(centerX, bounds.bottom() - gap));
                });
        }

        QIcon createWindowIcon()
        {
                return createGlyphIcon([](QPainter& painter, const QRectF& bounds)
                {
                        const QRectF frame(bounds.left(),
                                bounds.top(),
                                bounds.width() * 0.7,
                                bounds.height() * 0.7);
                        painter.drawRoundedRect(frame, 8.0, 8.0);

                        const double sliderY = bounds.bottom() - bounds.height() * 0.18;
                        painter.drawLine(QPointF(bounds.left(), sliderY),
                                QPointF(bounds.right(), sliderY));
                        const double knobX = bounds.left() + bounds.width() * 0.5;
                        painter.drawLine(QPointF(knobX, sliderY - 14.0),
                                QPointF(knobX, sliderY + 14.0));
                        painter.drawEllipse(QPointF(knobX, sliderY), 6.0, 6.0);
                });
        }

        QIcon createZoomIcon()
        {
                return createGlyphIcon([](QPainter& painter, const QRectF& bounds)
                {
                        const double radius = std::min(bounds.width(), bounds.height()) * 0.3;
                        const QPointF center = bounds.center();
                        painter.drawEllipse(center, radius, radius);

                        const QPointF handleStart(center.x() + radius * 0.7,
                                center.y() + radius * 0.7);
                        const QPointF handleEnd(bounds.right() - 4.0, bounds.bottom() - 4.0);
                        painter.drawLine(handleStart, handleEnd);
                });
        }

        QIcon createPanIcon()
        {
                return createGlyphIcon([](QPainter& painter, const QRectF& bounds)
                {
                        const QPointF center = bounds.center();
                        const double verticalTop = bounds.top();
                        const double verticalBottom = bounds.bottom();
                        const double horizontalLeft = bounds.left();
                        const double horizontalRight = bounds.right();

                        drawArrow(painter,
                                QPointF(center.x(), center.y() + bounds.height() * 0.22),
                                QPointF(center.x(), verticalBottom));
                        drawArrow(painter,
                                QPointF(center.x(), center.y() - bounds.height() * 0.22),
                                QPointF(center.x(), verticalTop));
                        drawArrow(painter,
                                QPointF(center.x() + bounds.width() * 0.22, center.y()),
                                QPointF(horizontalRight, center.y()));
                        drawArrow(painter,
                                QPointF(center.x() - bounds.width() * 0.22, center.y()),
                                QPointF(horizontalLeft, center.y()));

                        painter.drawEllipse(center, 6.0, 6.0);
                });
        }

        QIcon createMoonIcon()
        {
                return createGlyphIcon([](QPainter& painter, const QRectF& bounds)
                {
                        const QRectF outer = bounds.adjusted(bounds.width() * 0.20,
                                bounds.height() * 0.20,
                                -bounds.width() * 0.20,
                                -bounds.height() * 0.20);
                        painter.drawArc(outer, 60 * 16, 300 * 16);

                        const QRectF inner = outer.translated(outer.width() * 0.35, 0.0);
                        painter.drawArc(inner, 60 * 16, 300 * 16);
                });
        }

        QIcon createSunIcon()
        {
                return createGlyphIcon([](QPainter& painter, const QRectF& bounds)
                {
                        const QPointF center = bounds.center();
                        const double radius = std::min(bounds.width(), bounds.height()) * 0.22;
                        painter.drawEllipse(center, radius, radius);

                        constexpr int rayCount = 8;
                        constexpr double pi = 3.14159265358979323846;
                        for (int i = 0; i < rayCount; ++i)
                        {
                                const double angle = (static_cast<double>(i) / rayCount) * (2.0 * pi);
                                const double cosA = std::cos(angle);
                                const double sinA = std::sin(angle);
                                const QPointF start(center.x() + cosA * radius * 1.45,
                                        center.y() + sinA * radius * 1.45);
                                const QPointF end(center.x() + cosA * radius * 2.2,
                                        center.y() + sinA * radius * 2.2);
                                painter.drawLine(start, end);
                        }
                });
        }

        QIcon createPatientsToggleIcon(const bool collapsed)
        {
                return createGlyphIcon([collapsed](QPainter& painter, const QRectF& bounds)
                {
                        const double midY = bounds.center().y();
                        const double chevronHeight = bounds.height() * 0.28;
                        const double chevronWidth = bounds.width() * 0.22;
                        const double spacing = bounds.width() * 0.18;
                        const double direction = collapsed ? 1.0 : -1.0;
                        const double centerX = bounds.center().x();

                        const auto drawChevron = [&](double offset)
                        {
                                const double baseX = centerX + offset;
                                const QPointF mid(baseX, midY);
                                const QPointF top(baseX - direction * chevronWidth, midY - chevronHeight);
                                const QPointF bottom(baseX - direction * chevronWidth, midY + chevronHeight);
                                painter.drawLine(top, mid);
                                painter.drawLine(mid, bottom);
                        };

                        drawChevron(direction * spacing * 0.5);
                        drawChevron(-direction * spacing * 0.5);
                });
        }
}

isis::gui::WidgetsContainer::WidgetsContainer(QWidget* t_parent)
	: QWidget(t_parent)
{
	initView();
	setProperties();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setWindowPresets(
        const QVector<Widget2D::WindowPreset>& presets,
        double activeCenter,
        double activeWidth)
{
        m_dicomWindowPresets = presets;
        if (!std::isfinite(activeCenter))
        {
                activeCenter = 0.0;
        }
        if (!std::isfinite(activeWidth) || activeWidth < 0.0)
        {
                activeWidth = 0.0;
        }
        m_lastActiveWindowCenter = activeCenter;
        m_lastActiveWindowWidth = activeWidth;
        rebuildWindowPresetCombo();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onApplyTransformation()
{
	emit applyTransformation(static_cast<transformationType>(sender()
		->property("transformation").toInt()));
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onClosePatients()
{
	emit closePatients();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onCreateWidget3D()
{
	emit createWidget3D();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onCreateWidgetMPR()
{
	emit createWidgetMPR();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onWindowPresetChanged(const int index)
{
        if (!m_ui.comboPreset)
        {
                return;
        }

        const int targetIndex = (index >= 0) ? index : m_ui.comboPreset->currentIndex();
        if (targetIndex < 0 || targetIndex >= m_ui.comboPreset->count())
        {
                return;
        }

        const QVariant presetData = m_ui.comboPreset->itemData(targetIndex);
        if (!presetData.isValid())
        {
                return;
        }

        const QVariantList values = presetData.toList();
        if (values.size() != 2)
        {
                return;
        }

        const double center = values.constFirst().toDouble();
        const double width = values.constLast().toDouble();
        emit windowPresetRequested(center, width);
        refreshPresetHighlight();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setLayout(const layouts& t_layout)
{
	switch (t_layout)
	{
	case layouts::one:
		one();
		break;
	case layouts::twoRowOneBottom:
		twoRowOneBottom();
		break;
	case layouts::twoColumnOneRight:
		twoColumnOneRight();
		break;
	case layouts::threeRowOneBottom:
		threeRowOneBottom();
		break;
	case layouts::threeColumnOneRight:
		threeColumnOneRight();
		break;
	default:
		break;
	}
        setActiveLayout(t_layout);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::initView()
{
	m_ui.setupUi(this);

        if (m_ui.comboPreset)
        {
                QObject::connect(m_ui.comboPreset,
                        QOverload<int>::of(&QComboBox::currentIndexChanged),
                        this,
                        &WidgetsContainer::onWindowPresetChanged);
        }

        m_navigationGroup = new QButtonGroup(this);
        if (m_navigationGroup)
        {
                m_navigationGroup->setExclusive(true);
        }

        const auto registerTool = [this](QToolButton* button, InteractionTool tool)
        {
                if (!button || !m_navigationGroup)
                {
                        return;
                }
                button->setCheckable(true);
                m_navigationGroup->addButton(button, static_cast<int>(tool));
        };

        registerTool(m_ui.buttonScroll, InteractionTool::scroll);
        registerTool(m_ui.buttonWindow, InteractionTool::window);
        registerTool(m_ui.buttonZoom, InteractionTool::zoom);
        registerTool(m_ui.buttonPan, InteractionTool::pan);

        if (m_navigationGroup)
        {
                QObject::connect(m_navigationGroup,
                        &QButtonGroup::idClicked,
                        this,
                        &WidgetsContainer::onNavigationToolChanged);
        }

        if (m_ui.buttonScroll)
        {
                m_ui.buttonScroll->setChecked(true);
        }

        const auto assignIcon = [](QToolButton* button, const QIcon& icon)
        {
                if (button && !icon.isNull())
                {
                        button->setIcon(icon);
                }
        };

        assignIcon(m_ui.buttonScroll, createScrollIcon());
        assignIcon(m_ui.buttonWindow, createWindowIcon());
        assignIcon(m_ui.buttonZoom, createZoomIcon());
        assignIcon(m_ui.buttonPan, createPanIcon());

        if (m_ui.buttonOpenFile)
        {
                QObject::connect(m_ui.buttonOpenFile, &QToolButton::clicked, this,
                        [this]()
                        {
                                emit openFileRequested();
                        });
        }

        if (m_ui.buttonOpenFolder)
        {
                QObject::connect(m_ui.buttonOpenFolder, &QToolButton::clicked, this,
                        [this]()
                        {
                                emit openFolderRequested();
                        });
        }

        if (m_ui.buttonQueryRetrieve)
        {
                QObject::connect(m_ui.buttonQueryRetrieve, &QToolButton::clicked, this,
                        [this]()
                        {
                                emit queryRetrieveRequested();
                        });
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::updateToolbarIconTint(const QColor& normalTint, const QColor& disabledTint)
{
        if (normalTint.isValid())
        {
                m_toolbarTintNormal = normalTint;
        }
        if (disabledTint.isValid())
        {
                m_toolbarTintDisabled = disabledTint;
        }
        for (auto it = m_toolbarBaseIcons.constBegin(); it != m_toolbarBaseIcons.constEnd(); ++it)
        {
                auto* const button = it.key();
                if (!button)
                {
                        continue;
                }
                const QIcon tintedIcon = makeMonochromeIcon(it.value(), normalTint, disabledTint);
                if (!tintedIcon.isNull())
                {
                        button->setIcon(tintedIcon);
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setProperties()
{
        if (m_ui.buttonCloseAll)
        {
                m_ui.buttonCloseAll->setToolTip(tr("Close all patients (Esc)"));
        }
        if (m_ui.buttonRotateLeft)
        {
                m_ui.buttonRotateLeft->setToolTip(tr("Rotate left"));
        }
        if (m_ui.buttonRotateRight)
        {
                m_ui.buttonRotateRight->setToolTip(tr("Rotate right"));
        }
        if (m_ui.buttonVolume)
        {
                m_ui.buttonVolume->setToolTip(tr("3D volume rendering"));
        }
        if (m_ui.buttonMPR)
        {
                m_ui.buttonMPR->setToolTip(tr("Multiplanar reconstruction"));
        }
        if (m_ui.buttonQueryRetrieve)
        {
                m_ui.buttonQueryRetrieve->setToolTip(tr("Query/Retrieve studies from PACS (Ctrl+Q)"));
        }
        if (m_ui.buttonTogglePatients)
        {
                m_ui.buttonTogglePatients->setAutoRaise(true);
                m_ui.buttonTogglePatients->setIcon(createPatientsToggleIcon(false));
                m_ui.buttonTogglePatients->setIconSize(QSize(20, 20));
                m_ui.buttonTogglePatients->setAccessibleName(tr("Toggle patient browser panel"));
                QObject::connect(m_ui.buttonTogglePatients,
                        &QToolButton::clicked,
                        this,
                        &WidgetsContainer::onTogglePatientsPanel);
        }
        initLayoutControls();

	m_ui.buttonInvert->setProperty("transformation",
		static_cast<int>(transformationType::invert));
	m_ui.buttonFlipHorizontal->setProperty("transformation",
		static_cast<int>(transformationType::flipHorizontal));
	m_ui.buttonFlipVerical->setProperty("transformation",
		static_cast<int>(transformationType::flipVertical));
	m_ui.buttonRotateLeft->setProperty("transformation",
		static_cast<int>(transformationType::rotateLeft));
	m_ui.buttonRotateRight->setProperty("transformation",
		static_cast<int>(transformationType::rotateRight));

	const QColor normalTint(245, 245, 245);
	const QColor disabledTint(130, 130, 130);

        const QList<QToolButton*> toolbarButtons = {
                m_ui.buttonOpenFile,
                m_ui.buttonOpenFolder,
                m_ui.buttonQueryRetrieve,
                m_ui.buttonCloseAll,
                m_ui.buttonScroll,
                m_ui.buttonWindow,
                m_ui.buttonZoom,
                m_ui.buttonPan,
		m_ui.buttonInvert,
		m_ui.buttonFlipHorizontal,
		m_ui.buttonFlipVerical,
		m_ui.buttonRotateLeft,
		m_ui.buttonRotateRight,
		m_ui.buttonVolume,
		m_ui.buttonMPR,
                m_ui.buttonLayout
	};

	for (QToolButton* button : toolbarButtons)
	{
		if (!button)
		{
			continue;
		}
                if (!m_toolbarBaseIcons.contains(button))
                {
                        m_toolbarBaseIcons.insert(button, button->icon());
                }
	}
        updateToolbarIconTint(normalTint, disabledTint);
        setActiveLayout(layouts::one);

        m_staticWindowPresets = {
                {tr("Reset to Original"), 0.0, 0.0},
                {tr("Brain"), 40.0, 80.0},
                {tr("Lung"), -600.0, 1500.0},
                {tr("Bone"), 300.0, 2000.0},
                {tr("Soft Tissue"), 50.0, 350.0}};
        rebuildWindowPresetCombo();

        refreshPresetHighlight();
        setPatientsPanelCollapsed(false);
}

void isis::gui::WidgetsContainer::rebuildWindowPresetCombo()
{
        if (!m_ui.comboPreset)
        {
                return;
        }

        QSignalBlocker blocker(m_ui.comboPreset);

        const int previousIndex = m_ui.comboPreset->currentIndex();
        double previousCenter = 0.0;
        double previousWidth = 0.0;
        bool hasPrevious = false;

        if (previousIndex >= 0 && previousIndex < m_ui.comboPreset->count())
        {
                const QVariant data = m_ui.comboPreset->itemData(previousIndex);
                if (data.canConvert<QVariantList>())
                {
                        const QVariantList values = data.toList();
                        if (values.size() == 2)
                        {
                                previousCenter = values.first().toDouble();
                                previousWidth = values.last().toDouble();
                                hasPrevious = true;
                        }
                }
        }

        m_ui.comboPreset->clear();
        m_ui.comboPreset->setMaxVisibleItems(8);

        auto addPreset = [this](const QString& label, double center, double width, bool formatLabel)
        {
                QString display = label;
                if (formatLabel && width > 0.0)
                {
                        display = tr("%1  |  WL %2  WW %3")
                                .arg(label,
                                        QString::number(center, 'f', 0),
                                        QString::number(width, 'f', 0));
                }
                const int index = m_ui.comboPreset->count();
                m_ui.comboPreset->addItem(display);
                QVariantList payload;
                payload << center << width;
                m_ui.comboPreset->setItemData(index, payload);
                m_ui.comboPreset->setItemData(index, display, Qt::ToolTipRole);
        };

        for (const PresetDefinition& preset : m_staticWindowPresets)
        {
                addPreset(preset.label, preset.center, preset.width, true);
        }

        if (!m_dicomWindowPresets.isEmpty())
        {
                m_ui.comboPreset->insertSeparator(m_ui.comboPreset->count());
                for (const Widget2D::WindowPreset& preset : m_dicomWindowPresets)
                {
                        if (!std::isfinite(preset.width) || preset.width <= 0.0)
                        {
                                continue;
                        }
                        QString label = preset.label;
                        if (label.isEmpty())
                        {
                                label = tr("DICOM WL %1  WW %2")
                                        .arg(QString::number(preset.center, 'f', 0),
                                                QString::number(preset.width, 'f', 0));
                        }
                        addPreset(label, preset.center, preset.width, false);
                }
        }

        if (m_ui.comboPreset->count() == 0)
        {
                return;
        }

        int newIndex = findPresetIndex(m_lastActiveWindowCenter, m_lastActiveWindowWidth);
        if (newIndex < 0 && hasPrevious)
        {
                newIndex = findPresetIndex(previousCenter, previousWidth);
        }
        if (newIndex < 0 && previousIndex >= 0 && previousIndex < m_ui.comboPreset->count())
        {
                newIndex = previousIndex;
        }
        if (newIndex < 0)
        {
                newIndex = 0;
        }

        m_ui.comboPreset->setCurrentIndex(std::clamp(newIndex, 0, m_ui.comboPreset->count() - 1));
        refreshPresetHighlight();
}

int isis::gui::WidgetsContainer::findPresetIndex(double center, double width) const
{
        if (!m_ui.comboPreset)
        {
                return -1;
        }
        if (!std::isfinite(center) || !std::isfinite(width))
        {
                return -1;
        }

        for (int index = 0; index < m_ui.comboPreset->count(); ++index)
        {
                const QVariant data = m_ui.comboPreset->itemData(index);
                if (!data.canConvert<QVariantList>())
                {
                        continue;
                }
                const QVariantList values = data.toList();
                if (values.size() != 2)
                {
                        continue;
                }
                const double dataCenter = values.first().toDouble();
                const double dataWidth = values.last().toDouble();
                if (std::abs(dataCenter - center) <= 0.5
                        && std::abs(dataWidth - width) <= 0.5)
                {
                        return index;
                }
        }

        return -1;
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::refreshPresetHighlight()
{
        if (!m_ui.comboPreset)
        {
                return;
        }

        const bool highlight = m_ui.comboPreset->currentIndex() > 0;
        m_ui.comboPreset->setProperty("presetActive", highlight);
        if (auto* const comboStyle = m_ui.comboPreset->style())
        {
                comboStyle->unpolish(m_ui.comboPreset);
                comboStyle->polish(m_ui.comboPreset);
        }
        m_ui.comboPreset->update();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setPatientsPanelCollapsed(const bool collapsed)
{
        m_patientsPanelCollapsed = collapsed;
        if (m_ui.widgetPatients)
        {
                m_ui.widgetPatients->setVisible(!collapsed && m_patientsPanelAvailable);
        }

        if (auto* const toggleButton = m_ui.buttonTogglePatients)
        {
                toggleButton->setIcon(createPatientsToggleIcon(collapsed));
                toggleButton->setToolTip(collapsed
                        ? tr("Show patient browser")
                        : tr("Hide patient browser"));
                toggleButton->setProperty("panelCollapsed", collapsed);
                if (auto* const toggleStyle = toggleButton->style())
                {
                        toggleStyle->unpolish(toggleButton);
                        toggleStyle->polish(toggleButton);
                }
                toggleButton->update();
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onTogglePatientsPanel()
{
        setPatientsPanelCollapsed(!m_patientsPanelCollapsed);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setPatientsPanelAvailability(const bool available)
{
        m_patientsPanelAvailable = available;
        if (auto* const toggleButton = m_ui.buttonTogglePatients)
        {
                toggleButton->setEnabled(available);
        }
        setPatientsPanelCollapsed(m_patientsPanelCollapsed);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::initLayoutControls()
{
        if (!m_ui.buttonLayout)
        {
                return;
        }

        m_ui.buttonLayout->setPopupMode(QToolButton::InstantPopup);
        m_ui.buttonLayout->setIconSize(QSize(28, 28));
        m_ui.buttonLayout->setToolTip(tr("Choose viewport layout"));
        m_ui.buttonLayout->setAccessibleName(tr("Layout selector button"));

        const QIcon layoutIcon(QStringLiteral(":/res/icon_layout.png"));
        if (!layoutIcon.isNull())
        {
                m_ui.buttonLayout->setIcon(layoutIcon);
        }

        if (!m_layoutMenu)
        {
                m_layoutMenu = new QMenu(this);
                m_layoutMenu->setObjectName(QStringLiteral("layoutSelectorMenu"));
        }
        else
        {
                m_layoutMenu->clear();
        }

        if (!m_layoutActionGroup)
        {
                m_layoutActionGroup = new QActionGroup(m_layoutMenu);
                m_layoutActionGroup->setExclusive(true);
        }

        m_layoutActions.clear();

        const struct LayoutDefinition
        {
                layouts layout = layouts::one;
                QString label;
        } definitions[] = {
                {layouts::one, tr("Single viewport")},
                {layouts::twoRowOneBottom, tr("Two horizontal viewports")},
                {layouts::twoColumnOneRight, tr("Two vertical viewports")},
                {layouts::threeRowOneBottom, tr("Three horizontal viewports")},
                {layouts::threeColumnOneRight, tr("Three vertical viewports")}
        };

        for (const auto& definition : definitions)
        {
                auto* action = new QAction(definition.label, m_layoutMenu);
                action->setCheckable(true);
                action->setData(static_cast<int>(definition.layout));
                m_layoutMenu->addAction(action);
                if (m_layoutActionGroup)
                {
                        m_layoutActionGroup->addAction(action);
                }
                m_layoutActions.insert(definition.layout, action);
        }

        QObject::connect(m_layoutMenu,
                &QMenu::triggered,
                this,
                &WidgetsContainer::onLayoutActionTriggered,
                Qt::UniqueConnection);

        m_ui.buttonLayout->setMenu(m_layoutMenu);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onLayoutActionTriggered(QAction* action)
{
        if (!action)
        {
                return;
        }

        const QVariant data = action->data();
        if (!data.isValid())
        {
                return;
        }

        const auto layoutValue = static_cast<layouts>(data.toInt());
        setActiveLayout(layoutValue);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        emit layoutRequested(layoutValue);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setActiveLayout(const layouts layout)
{
        if (!m_layoutActionGroup)
        {
                return;
        }

        if (auto* const action = m_layoutActions.value(layout, nullptr))
        {
                const QSignalBlocker blocker(m_layoutActionGroup);
                action->setChecked(true);
                if (m_ui.buttonLayout)
                {
                        const QString label = action->text();
                        if (!label.isEmpty())
                        {
                                m_ui.buttonLayout->setToolTip(tr("Layout: %1").arg(label));
                        }
                }
        }
        else if (m_ui.buttonLayout)
        {
                m_ui.buttonLayout->setToolTip(tr("Choose viewport layout"));
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::onNavigationToolChanged(const int id)
{
        if (!m_navigationGroup)
        {
                return;
        }

        if (auto* const button = m_navigationGroup->button(id))
        {
                if (!button->isChecked())
                {
                        const QSignalBlocker blocker(m_navigationGroup);
                        button->setChecked(true);
                }
                emit interactionToolRequested(static_cast<InteractionTool>(id));
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::setActiveInteractionTool(InteractionTool tool)
{
        if (!m_navigationGroup)
        {
                return;
        }

        if (auto* const button = m_navigationGroup->button(static_cast<int>(tool)))
        {
                const QSignalBlocker blocker(m_navigationGroup);
                button->setChecked(true);
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::one() const
{
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(0), 0, 0);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::twoRowOneBottom() const
{
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(0), 0, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(1), 0, 1);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(2), 1, 0, 1, 2);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::twoColumnOneRight() const
{
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(0), 0, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(1), 1, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(2), 0, 1, 2, 1);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::threeRowOneBottom() const
{
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(0), 0, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(1), 0, 1);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(2), 0, 2);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(3), 1, 0, 1, 3);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsContainer::threeColumnOneRight() const
{
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(0), 0, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(1), 1, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(2), 2, 0);
	m_ui.gridLayout->addWidget(*&m_widgetsReference->at(3), 0, 1, 3, 1);
}




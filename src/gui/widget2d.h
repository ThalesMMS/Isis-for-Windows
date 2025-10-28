/*
 * ------------------------------------------------------------------------------------
 *  File: widget2d.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
*      Declares the 2D viewing widget and its collaborators for overlay, interaction, and rendering.
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

#include <qscrollbar.h>
#include <QElapsedTimer>
#include <QImage>
#include <QLabel>
#include <QPointer>
#include <QResizeEvent>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QSize>
#include <QTimer>
#include <QVector>
#include "ui_widget2d.h"
#include "widgetbase.h"
#include "widget2dstate.h"
#include "widget2dimagepresenter.h"

#include <memory>
#include <vector>
#include <optional>
#include <array>



class QKeyEvent;

namespace isis::core
{
        class Series;
        class Image;
}

namespace isis::gui
{
        class Widget2DInteractor;
        class Widget2DRenderer;
        class Widget2DOverlayUpdater;
        class Widget2DLoadController;

        enum class InteractionTool
        {
                scroll,
                window,
                zoom,
                pan,
                measureDistance,
                measureAngle,
                measureBiDimensional,
                measureContour
        };

        class Widget2D final : public WidgetBase
        {
        Q_OBJECT
        public:
                explicit Widget2D(QWidget* parent = Q_NULLPTR);
                ~Widget2D() override;
                struct FrameMetrics
                {
                        int frameIndex = 0;
                        int frameCount = 0;
                        double windowCenter = 0.0;
                        double windowWidth = 0.0;
                        double zoomFactor = 1.0;
                        double pixelSpacingX = 0.0;
                        double pixelSpacingY = 0.0;
                        double sliceLocation = 0.0;
                };

                struct CursorInfo
                {
                        bool valid = false;
                        bool hasIntensity = false;
                        QPoint pixel = {};
                        QPointF millimeters = {};
                        double rawValue = 0.0;
                        double huValue = 0.0;
                };

                struct WindowPreset
                {
                        double center = 0.0;
                        double width = 0.0;
                        QString label;
                };

        friend class Widget2DInteractor;
        friend class Widget2DRenderer;
        friend class Widget2DLoadController;

                // Exposes a vtkWidget2D* via QObject::setProperty for optional VTK integrations.
                static constexpr const char* vtkWidgetPropertyName = "vtkWidget2DPointer";

                //getters
                [[nodiscard]] QScrollBar* getScrollBar() const { return m_scroll; }
                [[nodiscard]] InteractionTool activeTool() const { return m_activeTool; }
                void waitForPendingTasks() const;
                void setRenderRequestSource(const QString& t_source) { m_lastRenderRequestSource = t_source; }
                [[nodiscard]] QString getRenderRequestSource() const { return m_lastRenderRequestSource; }
                void setFitToWindowEnabled(bool t_enabled);
                [[nodiscard]] bool isFitToWindowEnabled() const { return m_fitToWindowEnabled; }
                [[nodiscard]] bool wasRenderAbortedDueToMissingContext() const
                {
                        return m_renderAbortedDueToMissingContext;
                }
                void clearRenderAbortedDueToMissingContext()
                {
                        m_renderAbortedDueToMissingContext = false;
                        m_lastRenderRequestSource.clear();
                }

                void setActiveTool(InteractionTool t_tool);

                void render() override;
                void forceFrameMetricsUpdate();
                void applyWindowPreset(double center, double width);

        public slots:
                void onActivateWidget(const bool& t_flag);
                void onApplyTransformation(const transformationType& t_type);
                void onRefreshScrollValues(isis::core::Series* t_series, isis::core::Image* t_image);
                void onSetMaximized() const;
                void onFramesReady(isis::core::Series* t_series, int t_appendedFrames);

        signals:
                void activeToolChanged(InteractionTool t_tool);
                void frameMetricsChanged(const FrameMetrics& metrics);
                void cursorInfoChanged(const CursorInfo& info);
                void windowPresetsChanged(const QVector<WindowPreset>& presets,
                        double activeCenter,
                        double activeWidth);

        private slots :
                void onChangeImage(int t_index);
                void handleFramesReadyOnGuiThread(isis::core::Series* t_series, int t_appendedFrames);
                void onProgressiveRefreshTimeout();

        protected:
                void closeEvent(QCloseEvent* t_event) override;
                bool eventFilter(QObject* t_watched, QEvent* t_event) override;

        private:
                Ui::Widget2D m_ui = {};
                QScrollBar* m_scroll = {};
                QPointer<QLabel> m_errorLabel = {};
                QPointer<QLabel> m_imageLabel = {};
                QPointer<QLabel> m_orientationLabels[4] = {};
                std::shared_ptr<Widget2DImagePresenter> m_imagePresenter = {};
                QString m_lastRenderRequestSource = {};
                Widget2DState m_state = {};
                std::unique_ptr<Widget2DInteractor> m_interactor = {};
                std::unique_ptr<Widget2DRenderer> m_renderer = {};
                std::unique_ptr<Widget2DOverlayUpdater> m_overlayUpdater = {};
                std::unique_ptr<Widget2DLoadController> m_loadController = {};
                bool m_renderAbortedDueToMissingContext = false;
                bool m_renderingActive = false;
                int m_currentFrameIndex = 0;
                QImage m_cachedFrame = {};
                double m_displayZoomFactor = 1.0;
                double m_manualZoomFactor = 1.0;
                bool m_fitToWindowEnabled = false;
                bool m_windowLevelDragging = false;
                bool m_scrollDragging = false;
                bool m_zoomDragging = false;
                bool m_panDragging = false;
                QPoint m_lastMousePosition = {};
                QPointF m_panOffset = {};
                QElapsedTimer m_firstFrameTimer = {};
                QElapsedTimer m_frameLoadTimer = {};
                bool m_reportedFirstFrame = false;
                InteractionTool m_activeTool = InteractionTool::scroll;
                double m_scrollDragAccumulator = 0.0;
                bool m_pendingScrollRefresh = false;
                isis::core::Series* m_pendingSeriesRefresh = nullptr;
                isis::core::Image* m_pendingImageRefresh = nullptr;
                std::unique_ptr<QTimer> m_progressiveRefreshTimer = {};
                bool m_progressiveRefreshPending = false;
                int m_restoreFrameIndex = -1;
                FrameMetrics m_lastFrameMetrics = {};
                bool m_hasLastFrameMetrics = false;
                CursorInfo m_lastCursorInfo = {};
                bool m_hasCursorInfo = false;
                QVector<WindowPreset> m_availableWindowPresets = {};

                void initView() override;
                void initData() override;
                void createConnections() override;
                void resetView() override;
                void connectScroll();
                void startLoadingAnimation() override;
                void disconnectScroll() const;
                void resetScroll();
                void setScrollStyle() const;
                void setSliderValues(const int& t_min, const int& t_max, const int& t_value) override;
                [[nodiscard]] bool canScrollBeRefreshed(const int& t_patientIndex, const int& t_studyIndex,
                                                        const int& t_seriesIndex) const;
                [[nodiscard]] bool startVolumeRendering();
                void ensureImageLabel();
                void ensureOverlayWidget();
                void ensureOrientationLabels();
                void refreshDisplayedFrame(bool t_updateOverlay);
                void applyLoadedFrame(const int t_index);
                void handleRenderingFailure(const QString& t_reason);
                void hideOverlayWidget();
                void resizeEvent(QResizeEvent* t_event) override;
                void adjustFrameByStep(int t_step);
                void resetWindowLevel();
                void positionLoadingAnimation();
                void keyPressEvent(QKeyEvent* t_event) override;

                void updateActiveToolUi();
                void updateToolOverlay();
                void resetPanOffset();
                void clampPanOffset(const QSize& labelSize, const QSize& targetSize);
                void setCursorForActiveTool(bool t_handClosed = false);
                void ensureProgressiveRefreshTimer();
                void scheduleProgressiveRefresh();
                void positionOrientationLabels() const;
                void publishFrameMetrics(int frameIndex, int frameCount, double windowCenter,
                        double windowWidth, double zoomFactor);
                isis::core::Image* imageForFrameIndex(int frameIndex) const;
                void updateOrientationLabels(int frameIndex);
                QString orientationLabelForVector(const std::array<double, 3>& direction) const;
                const core::Image* orientationSourceImage(int frameIndex) const;
                [[nodiscard]] std::optional<QPoint> mapLabelPointToImagePixel(const QPoint& labelPos) const;
                void handleCursorHover(const QPoint& labelPos);
                void clearCursorInfo();
                void updateAvailableWindowPresets();
        };
}

/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dloadcontroller.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the helper coordinating asynchronous GDCM loading for the 2D widget.
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

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

class QString;

namespace isis::core
{
        class Image;
        class Series;
}

namespace isis::gui
{
        class Widget2D;
        class Widget2DImagePresenter;

        class Widget2DLoadController final : public QObject
        {
        Q_OBJECT
        public:
                explicit Widget2DLoadController(Widget2D& t_widget);
                ~Widget2DLoadController() = default;

                bool startRendering();
                void handleFailure(const QString& t_reason);
                void onImagesLoaded();
                void onFramePrefetchFinished();
                void waitForPendingTasks() const;
                void requestAppendFrames(isis::core::Series* t_series);
                [[nodiscard]] bool isAppendRunning() const;

        signals:
                void framesReady(isis::core::Series* t_series, int appendedFrames);

        private:
                using PresenterPtr = std::shared_ptr<Widget2DImagePresenter>;

                void cancelPrefetchIfRunning();
                void ensureImageLoadWatcher();
                void ensureFramePrefetchWatcher();
                void ensureAppendWatcher();
                void resetWidgetStateForLoad();
                void schedulePresenterPrefetch();
                void logPresenterStats() const;
                void logPrefetchStats() const;
                void onAppendFramesFinished();

                Widget2D& m_widget;
                std::unique_ptr<QFutureWatcher<PresenterPtr>> m_imageLoadWatcher = {};
                QFuture<PresenterPtr> m_imageLoadFuture = {};
                std::unique_ptr<QFutureWatcher<void>> m_framePrefetchWatcher = {};
                QFuture<void> m_framePrefetchFuture = {};
                std::unique_ptr<QFutureWatcher<int>> m_appendWatcher = {};
                QFuture<int> m_appendFuture = {};
                isis::core::Series* m_pendingAppendSeries = nullptr;
        };
}

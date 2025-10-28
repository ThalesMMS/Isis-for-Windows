/*
 * ------------------------------------------------------------------------------------
 *  File: tabwidget_mpr_guard_test.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Regression test to ensure TabWidget guards MPR tools correctly.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <QApplication>
#include <QByteArray>

#define private public
#define protected public
#include "src/gui/tabwidget.h"
#undef private
#undef protected

#include "src/core/series.h"
#include "src/core/image.h"

namespace
{
        class StubWidget2D final : public isis::gui::WidgetBase
        {
        public:
                StubWidget2D()
                        : WidgetBase(nullptr)
                {
                        setWidgetType(WidgetType::widget2d);
                }

                void render() override {}

        private:
                void initView() override {}
                void initData() override {}
                void createConnections() override {}
                void startLoadingAnimation() override {}
        };

        std::unique_ptr<isis::core::Image> createSingleFrameImage(
                isis::core::Series* series,
                int instanceNumber,
                double sliceLocation)
        {
                auto image = std::make_unique<isis::core::Image>();
                image->setParentObject(series);
                image->setInstanceNumber(instanceNumber);
                image->setSliceLocation(sliceLocation);
                image->setSOPInstanceUID("1.2.826.0.1.3680043.2.1125.9999." + std::to_string(instanceNumber));
                image->setClassUID("1.2.840.10008.5.1.4.1.1.2");
                image->setFrameOfRefernceID("FOR-TEST");
                image->setModality("CT");
                image->setWindowCenter(40);
                image->setWindowWidth(350);
                image->setRows(16);
                image->setColumns(16);
                image->setNumberOfFrames(1);
                image->setIsMultiFrame(false);
                image->setImagePath("slice_" + std::to_string(instanceNumber) + ".dcm");
                return image;
        }

        std::unique_ptr<isis::core::Image> createMultiFrameImage(isis::core::Series* series,
                                                                  int frameCount)
        {
                auto image = std::make_unique<isis::core::Image>();
                image->setParentObject(series);
                image->setInstanceNumber(1);
                image->setSliceLocation(0.0);
                image->setSOPInstanceUID("1.2.840.10008.MULTI");
                image->setClassUID("1.2.840.10008.5.1.4.1.1.2");
                image->setFrameOfRefernceID("FOR-TEST");
                image->setModality("MR");
                image->setWindowCenter(50);
                image->setWindowWidth(400);
                image->setRows(32);
                image->setColumns(32);
                image->setNumberOfFrames(frameCount);
                image->setIsMultiFrame(true);
                image->setImagePath("multiframe.dcm");
                return image;
        }

        bool evaluateGuardForSingleFrameCount(std::size_t sliceCount)
        {
                isis::gui::TabWidget widget(nullptr);
                auto stubWidget = std::make_unique<StubWidget2D>();
                widget.m_tabbedWidget = stubWidget.get();

                auto series = std::make_unique<isis::core::Series>();

                isis::core::Image* firstImage = nullptr;
                for (std::size_t index = 0; index < sliceCount; ++index)
                {
                        bool newImage = false;
                        auto image = createSingleFrameImage(series.get(), static_cast<int>(index + 1),
                                static_cast<double>(index));
                        auto* storedImage = series->addSingleFrameImage(std::move(image), newImage);
                        if (index == 0)
                        {
                                firstImage = storedImage;
                        }
                }

                if (firstImage)
                {
                        stubWidget->setImage(firstImage);
                }
                stubWidget->setSeries(series.get());

                const bool result = widget.canCreateWidgetMPR3D();

                widget.m_tabbedWidget = nullptr;
                stubWidget->setSeries(nullptr);
                stubWidget->setImage(nullptr);

                return result;
        }

        bool evaluateGuardForMultiFrameSeries(int frameCount)
        {
                isis::gui::TabWidget widget(nullptr);
                auto stubWidget = std::make_unique<StubWidget2D>();
                widget.m_tabbedWidget = stubWidget.get();

                auto series = std::make_unique<isis::core::Series>();

                bool newImage = false;
                auto image = createMultiFrameImage(series.get(), frameCount);
                auto* storedImage = series->addMultiFrameImage(std::move(image), newImage);
                stubWidget->setImage(storedImage);
                stubWidget->setSeries(series.get());

                const bool result = widget.canCreateWidgetMPR3D();

                widget.m_tabbedWidget = nullptr;
                stubWidget->setSeries(nullptr);
                stubWidget->setImage(nullptr);

                return result;
        }
}

int main()
{
        try
        {
                qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
                int argc = 1;
                char appName[] = "tabwidget_mpr_guard_test";
                char* argv[] = {appName, nullptr};
                QApplication app(argc, argv);

                if (!evaluateGuardForSingleFrameCount(2))
                {
                        throw std::runtime_error(
                                "TabWidget::canCreateWidgetMPR3D rejected a two-slice dataset.");
                }

                if (evaluateGuardForSingleFrameCount(1))
                {
                        throw std::runtime_error(
                                "TabWidget::canCreateWidgetMPR3D accepted a single-slice dataset.");
                }

                if (!evaluateGuardForMultiFrameSeries(12))
                {
                        throw std::runtime_error(
                                "TabWidget::canCreateWidgetMPR3D rejected a multi-frame dataset.");
                }

                std::cout << "tabwidget_mpr_guard_test passed" << std::endl;
                return EXIT_SUCCESS;
        }
        catch (const std::exception& ex)
        {
                std::cerr << "tabwidget_mpr_guard_test failed: " << ex.what() << '\n';
                return EXIT_FAILURE;
        }
}

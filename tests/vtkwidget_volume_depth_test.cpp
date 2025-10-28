/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget_volume_depth_test.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Regression test for VtkWidget volume depth and window/level behavior.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "src/gui/vtkwidgetmpr.h"
#include "src/gui/vtkwidget3d.h"
#include "src/gui/vtkdicomvolumeloader.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <QString>

namespace
{
        std::shared_ptr<isis::gui::VtkDicomVolume> createSingleSliceVolume()
        {
                auto volume = std::make_shared<isis::gui::VtkDicomVolume>();
                volume->ImageData = vtkSmartPointer<vtkImageData>::New();
                volume->ImageData->SetDimensions(8, 8, 1);
                volume->ImageData->SetExtent(0, 7, 0, 7, 0, 0);
                volume->ImageData->SetSpacing(1.0, 1.0, 1.0);
                volume->ImageData->AllocateScalars(VTK_SHORT, 1);

                auto* scalars = static_cast<short*>(volume->ImageData->GetScalarPointer());
                if (!scalars)
                {
                        throw std::runtime_error("Failed to allocate voxel buffer for test volume.");
                }

                const int pixelCount = 8 * 8;
                for (int index = 0; index < pixelCount; ++index)
                {
                        scalars[index] = static_cast<short>(index);
                }

                volume->NumberOfFrames = 1;
                volume->WindowCenter = 50.0;
                volume->WindowWidth = 100.0;
                volume->ImageData->Modified();
                return volume;
        }
}

int main()
{
        try
        {
                auto shallowVolume = createSingleSliceVolume();

                isis::gui::vtkWidgetMPR mprWidget;
                mprWidget.setVolumeForTesting(shallowVolume);
                if (mprWidget.hasValidVolume())
                {
                        throw std::runtime_error("vtkWidgetMPR incorrectly accepted a single-slice dataset.");
                }

                const QString mprFailure = mprWidget.lastFailureMessage();
                if (!mprFailure.contains(QStringLiteral("at least two slices")))
                {
                        throw std::runtime_error(
                                std::string("Unexpected MPR failure message: ") + mprFailure.toStdString());
                }

                isis::gui::vtkWidget3D widget3D;
                if (widget3D.composeAndRenderVolume(shallowVolume))
                {
                        throw std::runtime_error("vtkWidget3D attempted to render a single-slice dataset.");
                }

                const QString volumeError = widget3D.lastVolumeError();
                if (!volumeError.contains(QStringLiteral("at least two slices")))
                {
                        throw std::runtime_error(
                                std::string("Unexpected 3D failure message: ") + volumeError.toStdString());
                }

                std::cout << "vtkwidget_volume_depth_test passed" << std::endl;
                return EXIT_SUCCESS;
        }
        catch (const std::exception& ex)
        {
                std::cerr << "vtkwidget_volume_depth_test failed: " << ex.what() << '\n';
                return EXIT_FAILURE;
        }
}

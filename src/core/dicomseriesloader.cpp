#include "dicomseriesloader.h"

#include "dicomvolumemetadata.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <gdcmImageReader.h>
#include <gdcmIPPSorter.h>
#include <gdcmReader.h>
#include <gdcmStringFilter.h>

#include <QLoggingCategory>
#include <QString>

#include <vtkGDCMImageReader2.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkImageFlip.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>

Q_DECLARE_LOGGING_CATEGORY(lcDicomSeriesLoader)
Q_LOGGING_CATEGORY(lcDicomSeriesLoader, "isis.core.seriesloader")

using isis::core::DicomVolume;

namespace
{
        constexpr double kRescaleEpsilon = 1e-8;

        std::vector<std::string> sortSeriesFiles(const std::vector<std::string>& filePaths, double& computedSpacing)
        {
                if (filePaths.empty())
                {
                        return {};
                }

                gdcm::IPPSorter sorter;
                sorter.SetComputeZSpacing(true);
                sorter.SetDropDuplicatePositions(true);

                if (!sorter.Sort(filePaths))
                {
                        // Sorting failed, return original order.
                        computedSpacing = 0.0;
                        return filePaths;
                }

                computedSpacing = sorter.GetZSpacing();
                return sorter.GetFilenames();
        }

        gdcm::File readGdcmFile(const std::string& path)
        {
                gdcm::Reader reader;
                reader.SetFileName(path.c_str());
                if (!reader.Read())
                {
                        throw std::runtime_error("Failed to read DICOM file with GDCM: " + path);
                }
                return reader.GetFile();
        }

        vtkSmartPointer<vtkImageData> cloneImageData(vtkImageData* source)
        {
                auto clone = vtkSmartPointer<vtkImageData>::New();
                clone->DeepCopy(source);
                return clone;
        }

        void populateDirectionMatrix(isis::core::DicomVolume& volume)
        {
                if (!volume.Direction)
                {
                        volume.Direction = vtkSmartPointer<vtkMatrix4x4>::New();
                }

                auto* matrix = volume.Direction.GetPointer();
                for (int row = 0; row < 3; ++row)
                {
                        matrix->SetElement(row, 0, volume.Geometry.RowDirection[row]);
                        matrix->SetElement(row, 1, volume.Geometry.ColumnDirection[row]);
                        matrix->SetElement(row, 2, volume.Geometry.NormalDirection[row]);
                        matrix->SetElement(row, 3, volume.Geometry.Origin[row]);
                }

                matrix->SetElement(3, 0, 0.0);
                matrix->SetElement(3, 1, 0.0);
                matrix->SetElement(3, 2, 0.0);
                matrix->SetElement(3, 3, 1.0);

                if (volume.ImageData)
                {
                        double origin[3] = {
                                matrix->GetElement(0, 3),
                                matrix->GetElement(1, 3),
                                matrix->GetElement(2, 3)};
                        volume.ImageData->SetOrigin(origin);
                        volume.ImageData->Modified();
                }
        }

        void populatePixelInfoFromReader(vtkGDCMImageReader2* reader, isis::core::DicomPixelInfo& info)
        {
                if (auto* output = reader->GetOutput())
                {
                        auto* scalars = output->GetPointData() ? output->GetPointData()->GetScalars() : nullptr;
                        if (scalars)
                        {
                                info.BitsAllocated = scalars->GetDataTypeSize() * 8;

                                const int scalarType = scalars->GetDataType();
                                switch (scalarType)
                                {
                                case VTK_CHAR:
                                case VTK_SIGNED_CHAR:
                                case VTK_SHORT:
                                case VTK_INT:
                                case VTK_FLOAT:
                                case VTK_DOUBLE:
                                        info.IsSigned = true;
                                        break;
                                default:
                                        info.IsSigned = false;
                                        break;
                                }

                                info.SamplesPerPixel = scalars->GetNumberOfComponents();
                        }
                }

                info.IsPlanar = (reader->GetPlanarConfiguration() != 0);
                info.InvertMonochrome = (reader->GetImageFormat() == VTK_INVERSE_LUMINANCE);
                const double metadataSlope = info.RescaleSlope;
                const double metadataIntercept = info.RescaleIntercept;
                const double readerSlope = reader->GetScale();
                const double readerIntercept = reader->GetShift();
                info.RescaleSlope = readerSlope;
                info.RescaleIntercept = readerIntercept;

                constexpr double slopeEpsilon = 1e-9;
                constexpr double interceptEpsilon = 1e-3;
                const bool readerProvidesRescale =
                        (std::abs(readerSlope - 1.0) > slopeEpsilon) ||
                        (std::abs(readerIntercept) > interceptEpsilon);
                const bool metadataProvidesRescale =
                        (std::abs(metadataSlope - 1.0) > slopeEpsilon) ||
                        (std::abs(metadataIntercept) > interceptEpsilon);

                if (!readerProvidesRescale && metadataProvidesRescale)
                {
                        info.RescaleSlope = metadataSlope;
                        info.RescaleIntercept = metadataIntercept;
                        qCInfo(lcDicomSeriesLoader)
                                << "Preserving metadata rescale parameters"
                                << "slope" << metadataSlope
                                << "intercept" << metadataIntercept;
                }
        }

        void rescaleVolumeScalarsIfNeeded(isis::core::DicomVolume& volume)
        {
                auto* const imageData = volume.ImageData.GetPointer();
                if (!imageData)
                {
                        return;
                }

                vtkPointData* const pointData = imageData->GetPointData();
                vtkDataArray* const scalars = pointData ? pointData->GetScalars() : nullptr;
                if (!scalars)
                {
                        return;
                }

                const double slope = volume.PixelInfo.RescaleSlope;
                const double intercept = volume.PixelInfo.RescaleIntercept;
                const bool requiresRescale =
                        (std::abs(slope - 1.0) > kRescaleEpsilon) ||
                        (std::abs(intercept) > kRescaleEpsilon);

                const vtkIdType tupleCount = scalars->GetNumberOfTuples();
                const int componentCount = scalars->GetNumberOfComponents();
                double originalRange[2] = {0.0, 0.0};
                scalars->GetRange(originalRange);
                const bool metadataUnsigned = !volume.PixelInfo.OriginalIsSigned;
                qCInfo(lcDicomSeriesLoader)
                        << "Evaluating rescale application"
                        << "slope" << slope
                        << "intercept" << intercept
                        << "tupleCount" << tupleCount
                        << "components" << componentCount
                        << "scalarType" << scalars->GetDataType()
                        << "originalRange" << originalRange[0]
                        << originalRange[1]
                        << "metadataUnsigned" << metadataUnsigned
                        << "originalIsSignedFlag" << volume.PixelInfo.OriginalIsSigned;
                const bool scalarsContainNegative = originalRange[0] < -kRescaleEpsilon;
                if (metadataUnsigned && scalarsContainNegative)
                {
                        qCInfo(lcDicomSeriesLoader)
                                << "Detected unsigned pixel data already containing negative values; "
                                << "skipping additional rescale."
                                << "range" << originalRange[0]
                                << originalRange[1];
                        volume.PixelInfo.IsSigned = true;
                        volume.PixelInfo.RescaleSlope = 1.0;
                        volume.PixelInfo.RescaleIntercept = 0.0;
                        return;
                }

                if (!requiresRescale)
                {
                        return;
                }

                if (tupleCount <= 0 || componentCount <= 0)
                {
                        return;
                }

                vtkNew<vtkDoubleArray> rescaled;
                rescaled->SetNumberOfComponents(componentCount);
                rescaled->SetNumberOfTuples(tupleCount);

                const char* const scalarName = scalars->GetName();
                if (scalarName && scalarName[0] != '\0')
                {
                        rescaled->SetName(scalarName);
                }
                else
                {
                        rescaled->SetName("RescaledScalars");
                }

                std::vector<double> tuple(static_cast<std::size_t>(componentCount), 0.0);
                for (vtkIdType tupleIndex = 0; tupleIndex < tupleCount; ++tupleIndex)
                {
                        scalars->GetTuple(tupleIndex, tuple.data());
                        for (int component = 0; component < componentCount; ++component)
                        {
                                const double value = tuple[static_cast<std::size_t>(component)];
                                const double rescaledValue = slope * value + intercept;
                                rescaled->SetComponent(tupleIndex, component, rescaledValue);
                        }
                }

                if (pointData)
                {
                        pointData->SetScalars(rescaled);
                }
                imageData->Modified();

                volume.PixelInfo.RescaleSlope = 1.0;
                volume.PixelInfo.RescaleIntercept = 0.0;
                volume.PixelInfo.IsSigned = true;

                double range[2] = {0.0, 0.0};
                rescaled->GetRange(range);
                qCInfo(lcDicomSeriesLoader)
                        << "Applied rescale slope/intercept to volume scalars."
                        << "newRange" << range[0]
                        << range[1];
        }

        std::shared_ptr<DicomVolume> assembleVolume(vtkGDCMImageReader2* reader,
                const gdcm::File& referenceFile,
                double ippSpacing)
        {
                reader->Update();

                vtkImageData* rawOutput = reader->GetOutput();

                auto volume = std::make_shared<DicomVolume>();

                // Populate metadata / geometry from the reference dataset.
                const auto metadataBundle = isis::core::metadata::loadVolumeMetadata(referenceFile);
                volume->Metadata = metadataBundle.Metadata;
                volume->Geometry = metadataBundle.Geometry;
                volume->PixelInfo = metadataBundle.PixelInfo;

                // Flip the voxel data along Y to match the expected display orientation.
                vtkSmartPointer<vtkImageData> orientedOutput = rawOutput;
                int extent[6] = {0};
                rawOutput->GetExtent(extent);
                const int yExtent = extent[3] - extent[2];
                if (yExtent > 0)
                {
                        vtkNew<vtkImageFlip> flipY;
                        flipY->SetFilteredAxis(1); // flip vertical axis
                        flipY->FlipAboutOriginOff();
                        flipY->SetInputData(rawOutput);
                        flipY->Update();
                        orientedOutput = flipY->GetOutput();

                        const double originalColumn[3] = {
                                volume->Geometry.ColumnDirection[0],
                                volume->Geometry.ColumnDirection[1],
                                volume->Geometry.ColumnDirection[2]};
                        const double offset = volume->Geometry.Spacing[1] * static_cast<double>(yExtent);
                        for (int axis = 0; axis < 3; ++axis)
                        {
                                volume->Geometry.Origin[axis] += originalColumn[axis] * offset;
                                volume->Geometry.ColumnDirection[axis] = -originalColumn[axis];
                        }
                }

                volume->ImageData = cloneImageData(orientedOutput);
                volume->NumberOfFrames = std::max(1, volume->ImageData->GetDimensions()[2]);

                // Prefer computed spacing along Z when available.
                if (ippSpacing > 0.0)
                {
                        volume->Geometry.Spacing[2] = ippSpacing;
                        if (volume->ImageData)
                        {
                                volume->ImageData->SetSpacing(volume->Geometry.Spacing);
                        }
                }

                populatePixelInfoFromReader(reader, volume->PixelInfo);
                rescaleVolumeScalarsIfNeeded(*volume);
                populateDirectionMatrix(*volume);

                return volume;
        }
}

namespace isis::core::seriesloader
{
        std::shared_ptr<DicomVolume> loadVolumeFromFile(const std::string& path)
        {
                if (path.empty())
                {
                        throw std::invalid_argument("Empty DICOM path provided to loadVolumeFromFile.");
                }

                return loadVolumeFromSeries({path});
        }

        std::shared_ptr<DicomVolume> loadVolumeFromSeries(const std::vector<std::string>& slicePaths)
        {
                if (slicePaths.empty())
                {
                        throw std::invalid_argument("No DICOM files provided to loadVolumeFromSeries.");
                }

                double computedSpacing = 0.0;
                const auto sortedFiles = sortSeriesFiles(slicePaths, computedSpacing);
                const auto referenceFile = readGdcmFile(sortedFiles.front());

                vtkNew<vtkStringArray> fileArray;
                for (const auto& filePath : sortedFiles)
                {
                        fileArray->InsertNextValue(filePath.c_str());
                }

                vtkNew<vtkGDCMImageReader2> reader;
                reader->SetFileNames(fileArray);

                try
                {
                        auto volume = assembleVolume(reader, referenceFile, computedSpacing);
                        if (volume)
                        {
                                volume->SourceFiles = sortedFiles;
                        }
                        return volume;
                }
                catch (const std::exception& ex)
                {
                        qCCritical(lcDicomSeriesLoader)
                                << "GDCM volume assembly failed:"
                                << QString::fromStdString(ex.what());
                        throw;
                }
                catch (...)
                {
                        qCCritical(lcDicomSeriesLoader)
                                << "GDCM volume assembly failed with an unknown exception.";
                        throw;
                }
        }
}

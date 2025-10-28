#include "dicomvolumemetadata.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>

#include <gdcmAttribute.h>
#include <gdcmByteValue.h>
#include <gdcmDataElement.h>
#include <gdcmDataSet.h>
#include <gdcmTag.h>

namespace
{
        constexpr double kEpsilon = 1e-8;

        std::string trimString(std::string value)
        {
                const auto isTrimChar = [](unsigned char c) {
                        return c == '\0' || std::isspace(c) != 0;
                };

                while (!value.empty() && isTrimChar(static_cast<unsigned char>(value.back())))
                {
                        value.pop_back();
                }
                std::size_t start = 0;
                while (start < value.size() && isTrimChar(static_cast<unsigned char>(value[start])))
                {
                        ++start;
                }
                return value.substr(start);
        }

        std::string extractString(const gdcm::File& file, const gdcm::Tag& tag)
        {
                const gdcm::DataSet& dataSet = file.GetDataSet();
                if (!dataSet.FindDataElement(tag))
                {
                        return {};
                }
        const gdcm::DataElement& element = dataSet.GetDataElement(tag);
        if (element.IsEmpty())
        {
                return {};
        }

        const gdcm::ByteValue* byteValue = element.GetByteValue();
        if (!byteValue)
        {
                return {};
        }

        return trimString(std::string(byteValue->GetPointer(), byteValue->GetLength()));
        }

        double getNumericValue(const gdcm::File& file,
                const gdcm::Tag& tag,
                double defaultValue,
                unsigned int index = 0)
        {
                const std::string value = extractString(file, tag);
                if (value.empty())
                {
                        return defaultValue;
                }

                std::size_t start = 0;
                unsigned int currentIndex = 0;
                while (currentIndex < index)
                {
                        const auto position = value.find('\\', start);
                        if (position == std::string::npos)
                        {
                                return defaultValue;
                        }
                        start = position + 1;
                        ++currentIndex;
                }

                const auto end = value.find('\\', start);
                const auto token = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
                try
                {
                        return std::stod(token);
                }
                catch (...)
                {
                        return defaultValue;
                }
        }

        std::vector<std::string> splitMultiValue(const std::string& value)
        {
                std::vector<std::string> tokens;
                std::string current;
                for (char ch : value)
                {
                        if (ch == '\\')
                        {
                                tokens.emplace_back(trimString(current));
                                current.clear();
                        }
                        else
                        {
                                current.push_back(ch);
                        }
                }
                tokens.emplace_back(trimString(current));
                tokens.erase(std::remove_if(tokens.begin(),
                        tokens.end(),
                        [](const std::string& item) { return item.empty(); }),
                        tokens.end());
                return tokens;
        }

        std::vector<double> parseNumericList(const std::string& value)
        {
                std::vector<double> result;
                for (const std::string& token : splitMultiValue(value))
                {
                        try
                        {
                                result.push_back(std::stod(token));
                        }
                        catch (...)
                        {
                                // ignore malformed tokens
                        }
                }
                return result;
        }

        std::vector<std::string> parseStringList(const std::string& value)
        {
                return splitMultiValue(value);
        }

        void normalizeVector(double (&vector)[3])
        {
                const double magnitude =
                        std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
                if (magnitude <= kEpsilon)
                {
                        return;
                }

                for (double& component : vector)
                {
                        component /= magnitude;
                }
        }
}

namespace isis::core::metadata
{
        DicomVolumeMetadata loadVolumeMetadata(const gdcm::File& file)
        {
                DicomVolumeMetadata metadata{};
                populateMetadata(file, metadata.Metadata);
                populatePixelInfo(file, metadata.PixelInfo);
                populateGeometry(file, metadata.Geometry);
                normalizeDirections(metadata.Geometry);
                return metadata;
        }

        void populateMetadata(const gdcm::File& file, DicomMetadata& metadata)
        {
                const gdcm::Tag keys[] = {
                        {0x0010, 0x0010}, // PatientName
                        {0x0010, 0x0020}, // PatientID
                        {0x0010, 0x0030}, // PatientBirthDate
                        {0x0010, 0x0040}, // PatientSex
                        {0x0020, 0x000D}, // StudyInstanceUID
                        {0x0020, 0x000E}, // SeriesInstanceUID
                        {0x0020, 0x0011}, // SeriesNumber
                        {0x0008, 0x0060}, // Modality
                        {0x0008, 0x0020}, // StudyDate
                        {0x0008, 0x0030}, // StudyTime
                        {0x0008, 0x0050}, // AccessionNumber
                        {0x0008, 0x0070}, // Manufacturer
                        {0x0008, 0x0080}, // InstitutionName
                        {0x0008, 0x0090}, // ReferringPhysicianName
                        {0x0018, 0x1030}, // ProtocolName
                        {0x0008, 0x103E}, // SeriesDescription
                        {0x0008, 0x1030}  // StudyDescription
                };

                for (const auto& tag : keys)
                {
                        const std::string value = extractString(file, tag);
                        if (!value.empty())
                        {
                                const DicomTag dicomTag(tag.GetGroup(), tag.GetElement());
                                metadata.Values.try_emplace(dicomTag.packed(), value);
                        }
                }
        }

        void populatePixelInfo(const gdcm::File& file, DicomPixelInfo& info)
        {
                info.SamplesPerPixel = static_cast<int>(
                        getNumericValue(file, gdcm::Tag(0x0028, 0x0002), info.SamplesPerPixel));
                info.BitsAllocated = static_cast<int>(
                        getNumericValue(file, gdcm::Tag(0x0028, 0x0100), info.BitsAllocated));
                info.IsSigned = (getNumericValue(file, gdcm::Tag(0x0028, 0x0103), info.IsSigned ? 1.0 : 0.0) == 1.0);
                info.OriginalIsSigned = info.IsSigned;
                info.IsPlanar = (getNumericValue(file, gdcm::Tag(0x0028, 0x0006), info.IsPlanar ? 1.0 : 0.0) != 0.0);
                info.RescaleSlope = getNumericValue(file, gdcm::Tag(0x0028, 0x1053), info.RescaleSlope);
                info.RescaleIntercept = getNumericValue(file, gdcm::Tag(0x0028, 0x1052), info.RescaleIntercept);
                info.WindowCenter = getNumericValue(file, gdcm::Tag(0x0028, 0x1050), info.WindowCenter);
                info.WindowWidth = getNumericValue(file, gdcm::Tag(0x0028, 0x1051), info.WindowWidth);
                const std::string centersRaw = extractString(file, gdcm::Tag(0x0028, 0x1050));
                const std::string widthsRaw = extractString(file, gdcm::Tag(0x0028, 0x1051));
                const std::string explanationsRaw = extractString(file, gdcm::Tag(0x0028, 0x1055));
                const std::vector<double> centers = parseNumericList(centersRaw);
                const std::vector<double> widths = parseNumericList(widthsRaw);
                const std::vector<std::string> explanations = parseStringList(explanationsRaw);
                const std::size_t presetCount = std::min<std::size_t>(centers.size(), widths.size());
                if (presetCount > 0)
                {
                        info.WindowPresets.clear();
                        info.WindowPresets.reserve(presetCount);
                        for (std::size_t index = 0; index < presetCount; ++index)
                        {
                                DicomPixelInfo::WindowPreset preset;
                                preset.Center = centers[index];
                                preset.Width = widths[index];
                                if (index < explanations.size())
                                {
                                        preset.Explanation = explanations[index];
                                }
                                info.WindowPresets.push_back(std::move(preset));
                        }
                        info.WindowCenter = info.WindowPresets.front().Center;
                        info.WindowWidth = info.WindowPresets.front().Width;
                }

                const std::string photometric = extractString(file, gdcm::Tag(0x0028, 0x0004));
                info.InvertMonochrome = (!photometric.empty() && photometric == "MONOCHROME1");
        }

        void populateGeometry(const gdcm::File& file, DicomGeometry& geometry)
        {
                for (unsigned int i = 0; i < 3; ++i)
                {
                        geometry.Origin[i] = getNumericValue(file, gdcm::Tag(0x0020, 0x0032), geometry.Origin[i], i);
                        geometry.RowDirection[i] = getNumericValue(
                                file, gdcm::Tag(0x0020, 0x0037), geometry.RowDirection[i], i);
                        geometry.ColumnDirection[i] = getNumericValue(
                                file, gdcm::Tag(0x0020, 0x0037), geometry.ColumnDirection[i], i + 3);
                }

                geometry.Spacing[0] = getNumericValue(file, gdcm::Tag(0x0028, 0x0030), geometry.Spacing[0], 0);
                geometry.Spacing[1] = getNumericValue(file, gdcm::Tag(0x0028, 0x0030), geometry.Spacing[1], 1);

                const double normal[3] = {
                        geometry.RowDirection[1] * geometry.ColumnDirection[2] -
                                geometry.RowDirection[2] * geometry.ColumnDirection[1],
                        geometry.RowDirection[2] * geometry.ColumnDirection[0] -
                                geometry.RowDirection[0] * geometry.ColumnDirection[2],
                        geometry.RowDirection[0] * geometry.ColumnDirection[1] -
                                geometry.RowDirection[1] * geometry.ColumnDirection[0]};
                std::copy(std::begin(normal), std::end(normal), std::begin(geometry.NormalDirection));

                double spacingZ = getNumericValue(file, gdcm::Tag(0x0018, 0x0088), 0.0);
                if (spacingZ <= kEpsilon)
                {
                        spacingZ = getNumericValue(file, gdcm::Tag(0x0018, 0x0050), 1.0);
                }
                if (spacingZ <= kEpsilon)
                {
                        spacingZ = 1.0;
                }
                geometry.Spacing[2] = spacingZ;
        }

        void normalizeDirections(DicomGeometry& geometry)
        {
                normalizeVector(geometry.RowDirection);
                normalizeVector(geometry.ColumnDirection);
                normalizeVector(geometry.NormalDirection);
        }
}

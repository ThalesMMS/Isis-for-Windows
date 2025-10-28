#include "dicomreader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gdcmAttribute.h>
#include <gdcmByteValue.h>
#include <gdcmDataElement.h>
#include <gdcmDataSet.h>
#include <gdcmImage.h>
#include <gdcmImageReader.h>
#include <gdcmReader.h>
#include <gdcmTag.h>

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>

namespace isis::core
{
        Q_LOGGING_CATEGORY(lcDicomReader, "isis.core.dicom")
}

namespace
{
        QString sanitizePath(const std::string& path)
        {
                const QFileInfo fileInfo(QString::fromStdString(path));
                return fileInfo.fileName().isEmpty() ? fileInfo.filePath() : fileInfo.fileName();
        }

        QString formatTagKey(const gdcm::Tag& tag)
        {
                return QStringLiteral("(%1,%2)")
                        .arg(tag.GetGroup(), 4, 16, QLatin1Char('0'))
                        .arg(tag.GetElement(), 4, 16, QLatin1Char('0'))
                        .toUpper();
        }

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

        template <typename T>
        std::pair<double, double> computeMinMax(const T* data, std::size_t count)
        {
                if (count == 0)
                {
                        return {0.0, 0.0};
                }

                auto minValue = static_cast<double>(data[0]);
                auto maxValue = static_cast<double>(data[0]);
                for (std::size_t index = 1; index < count; ++index)
                {
                        const auto value = static_cast<double>(data[index]);
                        minValue = std::min(minValue, value);
                        maxValue = std::max(maxValue, value);
                }
                return {minValue, maxValue};
        }

        int parseInt(const std::string& value, int defaultValue = 0)
        {
                if (value.empty())
                {
                        return defaultValue;
                }

                std::string digits;
                digits.reserve(value.size());
                for (char ch : value)
                {
                        if (digits.empty() && (ch == '-' || ch == '+'))
                        {
                                digits.push_back(ch);
                                continue;
                        }

                        if (std::isdigit(static_cast<unsigned char>(ch)))
                        {
                                digits.push_back(ch);
                        }
                        else if (!digits.empty())
                        {
                                break;
                        }
                }

                if (digits.empty() || (digits.size() == 1 && (digits[0] == '-' || digits[0] == '+')))
                {
                        return defaultValue;
                }

                try
                {
                        return std::stoi(digits);
                }
                catch (...)
                {
                        return defaultValue;
                }
        }

        bool parsePatientPosition(const std::string& value, std::array<double, 3>& position)
        {
                if (value.empty())
                {
                        return false;
                }

                std::array<double, 3> components{};
                std::size_t start = 0;
                for (int index = 0; index < 3; ++index)
                {
                        const auto separator = value.find('\\', start);
                        const bool isLastComponent = (separator == std::string::npos);
                        const auto token = value.substr(start, isLastComponent ? std::string::npos : separator - start);
                        if (token.empty())
                        {
                                return false;
                        }
                        try
                        {
                                components[index] = std::stod(token);
                        }
                        catch (...)
                        {
                                return false;
                        }

                        if (isLastComponent)
                        {
                                if (index != 2)
                                {
                                        return false;
                                }
                                break;
                        }
                        start = separator + 1;
                }

                position = components;
                return true;
        }

        bool parsePatientOrientation(const std::string& value,
                std::array<double, 3>& row,
                std::array<double, 3>& column)
        {
                if (value.empty())
                {
                        return false;
                }

                std::array<double, 6> components{};
                std::size_t start = 0;
                for (int index = 0; index < 6; ++index)
                {
                        const auto separator = value.find('\\', start);
                        const bool isLastComponent = (separator == std::string::npos);
                        const auto token = value.substr(start, isLastComponent ? std::string::npos : separator - start);
                        if (token.empty())
                        {
                                return false;
                        }
                        try
                        {
                                components[index] = std::stod(token);
                        }
                        catch (...)
                        {
                                return false;
                        }

                        if (isLastComponent)
                        {
                                if (index != 5)
                                {
                                        return false;
                                }
                                break;
                        }
                        start = separator + 1;
                }

                row = {components[0], components[1], components[2]};
                column = {components[3], components[4], components[5]};
                return true;
        }

}

void isis::core::DicomReader::readFile(const std::string& filePath)
{
        m_filePath = filePath;
        const auto sanitizedPath = sanitizePath(filePath);
        qInfo(lcDicomReader) << "[Logging] Starting DICOM read for" << sanitizedPath;

        gdcm::Reader reader;
        reader.SetFileName(filePath.c_str());
        if (!reader.Read())
        {
                qCritical(lcDicomReader) << "[Logging] Failed to load DICOM file" << sanitizedPath;
                m_hasFile = false;
                throw std::runtime_error("Cannot open file!");
        }

        m_file = reader.GetFile();
        m_hasFile = true;
        qInfo(lcDicomReader) << "[Logging] Successfully loaded DICOM file" << sanitizedPath;
}

std::unique_ptr<isis::core::Patient> isis::core::DicomReader::getReadPatient() const
{
        auto patient = std::make_unique<Patient>();
        const auto age = getTagValue({0x0010, 0x1010});
        patient->setAge(parseInt(age, 0));
        patient->setBirthDate(getTagValue({0x0010, 0x0030}));
        patient->setID(getTagValue({0x0010, 0x0020}));
        patient->setName(getTagValue({0x0010, 0x0010}));
        return patient;
}

std::unique_ptr<isis::core::Study> isis::core::DicomReader::getReadStudy() const
{
        auto study = std::make_unique<Study>();
        study->setID(getTagValue({0x0020, 0x0010}));
        study->setDate(getTagValue({0x0008, 0x0020}));
        study->setDescription(getTagValue({0x0008, 0x1030}));
        study->setUID(getTagValue({0x0020, 0x000D}));
        return study;
}

std::unique_ptr<isis::core::Series> isis::core::DicomReader::getReadSeries() const
{
        const auto modality = getTagValue({0x0008, 0x0060});
        if (!isModalitySupported(modality))
        {
                const auto formattedModality = QString::fromStdString(modality);
                const auto path = QString::fromStdString(m_filePath);
                qWarning(lcDicomReader)
                        << "[Logging] Modality" << formattedModality
                        << "is not supported for" << path;
                return nullptr;
        }

        auto series = std::make_unique<Series>();
        series->setNumber(getTagValue({0x0020, 0x0011}));
        series->setDescription(getTagValue({0x0008, 0x103E}));
        series->setUID(getTagValue({0x0020, 0x000E}));
        series->setDate(getTagValue({0x0008, 0x0021}));
        return series;
}

std::unique_ptr<isis::core::Image> isis::core::DicomReader::getReadImage() const
{
        auto image = std::make_unique<Image>();
        const auto modality = getTagValue({0x0008, 0x0060});
        image->setImagePath(m_filePath);
        image->setSOPInstanceUID(getTagValue({0x0008, 0x0018}));
        image->setClassUID(getTagValue({0x0008, 0x0016}));

        const auto [window, level] = getWindowLevel();
        image->setWindowCenter(window);
        image->setWindowWidth(level);

        const auto rows = getTagValue({0x0028, 0x0010});
        image->setRows(parseInt(rows, 0));
        const auto columns = getTagValue({0x0028, 0x0011});
        image->setColumns(parseInt(columns, 0));

        const auto sliceLocation = getTagValue({0x0020, 0x1041});
        image->setSliceLocation(sliceLocation.empty() ? 0.0 : std::stod(sliceLocation));

        const auto acquisitionNumber = getTagValue({0x0020, 0x0012});
        image->setAcquisitionNumber(parseInt(acquisitionNumber, 0));

        const auto numberOfFrames = getTagValue({0x0028, 0x0008});
        const int parsedNumberOfFrames = parseInt(numberOfFrames, 0);
        image->setNumberOfFrames(parsedNumberOfFrames);
        // Treat "NumberOfFrames == 1" as single-frame; many vendors populate tag with 1 by default.
        image->setIsMultiFrame(parsedNumberOfFrames > 1);

        if (!image->getIsMultiFrame())
        {
                image->setFrameOfRefernceID(getTagValue({0x0020, 0x0052}));
        }

        const auto [spacingX, spacingY] = getPixelSpacing();
        image->setPixelSpacingX(spacingX);
        image->setPixelSpacingY(spacingY);

        const auto instanceNumber = getTagValue({0x0020, 0x0013});
        image->setInstanceNumber(parseInt(instanceNumber, 0));

        const auto imagePositionPatient = getTagValue({0x0020, 0x0032});
        std::array<double, 3> position = {};
        if (parsePatientPosition(imagePositionPatient, position))
        {
                image->setImagePositionPatient(position);
        }
        else
        {
                image->clearImagePositionPatient();
        }

        const auto imageOrientationPatient = getTagValue({0x0020, 0x0037});
        std::array<double, 3> orientationRow = {};
        std::array<double, 3> orientationColumn = {};
        if (parsePatientOrientation(imageOrientationPatient, orientationRow, orientationColumn))
        {
                image->setImageOrientationPatient(orientationRow, orientationColumn);
        }
        else
        {
                image->clearImageOrientationPatient();
        }
        return image;
}

std::string isis::core::DicomReader::getTagValue(const gdcm::Tag& tag) const
{
        if (!m_hasFile)
        {
                return {};
        }

        const gdcm::DataSet& dataSet = m_file.GetDataSet();
        if (!dataSet.FindDataElement(tag))
        {
                return {};
        }

        const gdcm::DataElement& element = dataSet.GetDataElement(tag);
        const auto* const byteValue = element.GetByteValue();
        if (!byteValue || byteValue->GetLength() == 0)
        {
                return {};
        }

        try
        {
                const std::string raw(byteValue->GetPointer(), byteValue->GetLength());
                return trimString(raw);
        }
        catch (const std::exception& ex)
        {
                qWarning(lcDicomReader)
                        << "[Logging] Exception while decoding tag"
                        << formatTagKey(tag)
                        << ':' << ex.what();
                return {};
        }
}

bool isis::core::DicomReader::isModalitySupported(const std::string& modality)
{
        return modality != "PR" && modality != "KO" && modality != "SR";
}

namespace
{
        std::optional<double> parseFirstNumericValue(const std::string& rawValue)
        {
                if (rawValue.empty())
                {
                        return std::nullopt;
                }

                const auto trim = [](const std::string& value) -> std::string
                {
                        std::size_t start = 0;
                        std::size_t end = value.size();
                        while (start < end && std::isspace(static_cast<unsigned char>(value[start])) != 0)
                        {
                                ++start;
                        }
                        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
                        {
                                --end;
                        }
                        return value.substr(start, end - start);
                };

                const std::size_t delimiterPos = rawValue.find('\\');
                const std::string token = trim(rawValue.substr(0, delimiterPos));
                if (token.empty())
                {
                        return std::nullopt;
                }

                try
                {
                        return std::stod(token);
                }
                catch (...)
                {
                        return std::nullopt;
                }
        }
}

std::tuple<int, int> isis::core::DicomReader::getWindowLevel() const
{
        const auto centerStr = getTagValue({0x0028, 0x1050});
        const auto widthStr = getTagValue({0x0028, 0x1051});

        const auto centerValue = parseFirstNumericValue(centerStr);
        const auto widthValue = parseFirstNumericValue(widthStr);

        if (centerValue && widthValue && *widthValue > 0.0)
        {
                return {static_cast<int>(std::lround(*centerValue)),
                        static_cast<int>(std::lround(std::max(*widthValue, 1.0)))};
        }

        const auto fallback = computeWindowLevelFromPixels();
        const double minValue = fallback.first;
        const double maxValue = fallback.second;
        double computedWidth = maxValue - minValue;
        if (!(computedWidth > 0.0) || !std::isfinite(computedWidth))
        {
                computedWidth = std::max(std::abs(maxValue), std::abs(minValue));
        }
        if (!(computedWidth > 0.0) || !std::isfinite(computedWidth))
        {
                computedWidth = 1.0;
        }
        const double computedCenter = minValue + (computedWidth * 0.5);

        return {static_cast<int>(std::lround(computedCenter)),
                static_cast<int>(std::lround(std::max(computedWidth, 1.0)))};
}

std::pair<double, double> isis::core::DicomReader::computeWindowLevelFromPixels() const
{
        gdcm::ImageReader imageReader;
        imageReader.SetFileName(m_filePath.c_str());
        if (!imageReader.Read())
        {
                return {0.0, 0.0};
        }

        const gdcm::Image& image = imageReader.GetImage();
        const gdcm::PixelFormat& pixelFormat = image.GetPixelFormat();
        const unsigned long bufferLength = image.GetBufferLength();
        if (bufferLength == 0)
        {
                return {0.0, 0.0};
        }

        std::vector<char> buffer(bufferLength);
        if (!image.GetBuffer(buffer.data()))
        {
                return {0.0, 0.0};
        }

        const auto componentSize = pixelFormat.GetBitsAllocated() / 8;
        if (componentSize == 0)
        {
                return {0.0, 0.0};
        }
        const auto totalSamples = bufferLength / componentSize;
        std::pair<double, double> range{0.0, 0.0};

        switch (pixelFormat.GetScalarType())
        {
        case gdcm::PixelFormat::UINT8:
                range = computeMinMax(reinterpret_cast<const uint8_t*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::INT8:
                range = computeMinMax(reinterpret_cast<const int8_t*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::UINT16:
                range = computeMinMax(reinterpret_cast<const uint16_t*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::INT16:
                range = computeMinMax(reinterpret_cast<const int16_t*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::UINT32:
                range = computeMinMax(reinterpret_cast<const uint32_t*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::INT32:
                range = computeMinMax(reinterpret_cast<const int32_t*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::FLOAT32:
                range = computeMinMax(reinterpret_cast<const float*>(buffer.data()), totalSamples);
                break;
        case gdcm::PixelFormat::FLOAT64:
                range = computeMinMax(reinterpret_cast<const double*>(buffer.data()), totalSamples);
                break;
        default:
                return {0.0, 0.0};
        }

        const double slope = getNumericValue({0x0028, 0x1053}, 1.0);
        const double intercept = getNumericValue({0x0028, 0x1052}, 0.0);
        const double minValue = range.first * slope + intercept;
        const double maxValue = range.second * slope + intercept;
        const double width = maxValue - minValue;
        const double center = minValue + width / 2.0;
        return {center, width};
}

std::tuple<double, double> isis::core::DicomReader::getPixelSpacing() const
{
        const gdcm::Tag pixelSpacingTag(0x0028, 0x0030);
        const gdcm::Tag imagerPixelSpacingTag(0x0018, 0x1164);

        auto pixelSpacing = getTagValue(pixelSpacingTag);
        auto usedTag = pixelSpacingTag;
        if (pixelSpacing.empty())
        {
                pixelSpacing = getTagValue(imagerPixelSpacingTag);
                usedTag = imagerPixelSpacingTag;
        }

        if (pixelSpacing.empty())
        {
                qWarning(lcDicomReader)
                        << "[Logging] Missing pixel spacing metadata for tags"
                        << formatTagKey(pixelSpacingTag) << "and"
                        << formatTagKey(imagerPixelSpacingTag);
                return {1.0, 1.0};
        }

        const auto separator = pixelSpacing.find('\\\\');
        try
        {
                const auto first = pixelSpacing.substr(0, separator);
                const auto second = separator == std::string::npos ? std::string() : pixelSpacing.substr(separator + 1);
                return {std::stod(first), second.empty() ? std::stod(first) : std::stod(second)};
        }
        catch (const std::exception& ex)
        {
                qWarning(lcDicomReader)
                        << "[Logging] Exception while parsing pixel spacing for tag"
                        << formatTagKey(usedTag) << ':' << ex.what();
                return {1.0, 1.0};
        }
}

double isis::core::DicomReader::getNumericValue(const gdcm::Tag& tag, double defaultValue, unsigned int index) const
{
        const auto value = getTagValue(tag);
        if (value.empty())
        {
                return defaultValue;
        }

        std::size_t start = 0;
        unsigned int currentIndex = 0;
        while (currentIndex < index)
        {
                const auto position = value.find('\\\\', start);
                if (position == std::string::npos)
                {
                        return defaultValue;
                }
                start = position + 1;
                ++currentIndex;
        }

        const auto end = value.find('\\\\', start);
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

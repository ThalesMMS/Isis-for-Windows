/*
 * ------------------------------------------------------------------------------------
 *  File: image.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements Image behaviour including lazy DICOM volume loading,
 *      comparisons, and ordering utilities.
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

#include "image.h"
#include "smartdjdecoderregistration.h"
#include "dicomvolume.h"

#include <QString>
#include <QLoggingCategory>

#include <array>
#include <cmath>
#include <limits>

Q_LOGGING_CATEGORY(lcImage, "isis.core.image")

namespace
{
        class CodecRegistrationGuard
        {
        public:
                CodecRegistrationGuard()
                {
                        isis::core::SmartDJDecoderRegistration::registerCodecs();
                }

                ~CodecRegistrationGuard()
                {
                        isis::core::SmartDJDecoderRegistration::cleanup();
                }

                CodecRegistrationGuard(const CodecRegistrationGuard&) = delete;
                CodecRegistrationGuard& operator=(const CodecRegistrationGuard&) = delete;
        };
}

std::shared_ptr<isis::core::DicomVolume> isis::core::Image::getDicomVolume(
        QString* t_failureReason) const
{
        if (t_failureReason)
        {
                t_failureReason->clear();
        }

        if (m_volume)
        {
                return m_volume;
        }

        if (m_path.empty())
        {
                const auto reason = QStringLiteral("Image has no filesystem path associated with it.");
                qWarning(lcImage)
                        << "getDicomVolume() called without a valid path."
                        << "reason:" << reason;
                if (t_failureReason)
                {
                        *t_failureReason = reason;
                }
                return nullptr;
        }

        QString failureReason;
        CodecRegistrationGuard guard;
        try
        {
                m_volume = DicomVolumeLoader::loadImage(m_path);
        }
        catch (const std::exception& ex)
        {
                const auto path = QString::fromStdString(m_path);
                const auto exceptionText = QString::fromUtf8(ex.what());
                failureReason = QStringLiteral("GDCM failed to load %1: %2").arg(path, exceptionText);
                qCCritical(lcImage)
                        << "GDCM failed to load"
                        << QString::fromStdString(m_path)
                        << ":" << ex.what();
                m_volume.reset();
        }

        if (m_volume && m_volume->ImageData)
        {
                int dimensions[3] = {0, 0, 0};
                m_volume->ImageData->GetDimensions(dimensions);
                qCInfo(lcImage)
                        << "Loaded DICOM image"
                        << QString::fromStdString(m_path)
                        << "dimensions:" << dimensions[0] << "x"
                        << dimensions[1] << "x" << dimensions[2];
        }
        else
        {
                if (failureReason.isEmpty())
                {
                        failureReason = QStringLiteral("Unable to obtain image data for %1.")
                                                .arg(QString::fromStdString(m_path));
                }
                qCWarning(lcImage)
                        << "Unable to obtain image data for"
                        << QString::fromStdString(m_path)
                        << "reason:" << failureReason;
        }

        if (t_failureReason && !failureReason.isEmpty())
        {
                *t_failureReason = failureReason;
        }

        return m_volume;
}

//-----------------------------------------------------------------------------
bool isis::core::Image::equal(Image* t_image) const
{
	return getParentObject() == t_image->getParentObject() &&
		getImagePath() == t_image->getImagePath() &&
		getSOPInstanceUID() == t_image->getSOPInstanceUID() &&
		getClassUID() == t_image->getClassUID() &&
		getFrameOfRefernceID() == t_image->getFrameOfRefernceID() &&
		getModality() == t_image->getModality() &&
		getWindowCenter() == t_image->getWindowCenter() &&
		getWindowWidth() == t_image->getWindowWidth() &&
		getRows() == t_image->getRows() &&
		getColumns() == t_image->getColumns() &&
		getNumberOfFrames() == t_image->getNumberOfFrames() &&
		getSliceLocation() == t_image->getSliceLocation() &&
		getAcquisitionNumber() == t_image->getAcquisitionNumber() &&
		getIsMultiFrame() == t_image->getIsMultiFrame();
}

//-----------------------------------------------------------------------------
bool isis::core::Image::isLess(Image* t_lhs, Image* t_rhs)
{
	if (!t_lhs || !t_rhs)
	{
		return false;
	}

	constexpr double positionEpsilon = 1e-4;
	constexpr double normalEpsilon = 1e-6;

	const auto isFiniteVector = [](const std::array<double, 3>& values) -> bool
	{
		for (double value : values)
		{
			if (!std::isfinite(value))
			{
				return false;
			}
		}
		return true;
	};

	const auto computeNormal = [&](const Image* image, std::array<double, 3>& normal) -> bool
	{
		if (!image || !image->m_hasImageOrientationPatient)
		{
			return false;
		}
		if (!isFiniteVector(image->m_imageOrientationRow) || !isFiniteVector(image->m_imageOrientationColumn))
		{
			return false;
		}
		const auto& row = image->m_imageOrientationRow;
		const auto& column = image->m_imageOrientationColumn;
		normal = {
			row[1] * column[2] - row[2] * column[1],
			row[2] * column[0] - row[0] * column[2],
			row[0] * column[1] - row[1] * column[0]};
		const double magnitude = std::sqrt(normal[0] * normal[0]
			+ normal[1] * normal[1]
			+ normal[2] * normal[2]);
		if (!(magnitude > normalEpsilon))
		{
			return false;
		}
		normal[0] /= magnitude;
		normal[1] /= magnitude;
		normal[2] /= magnitude;
		return true;
	};

	const auto comparePositions = [](const Image* lhs, const Image* rhs) -> int
	{
		for (int axis = 0; axis < 3; ++axis)
		{
			const double lhsValue = lhs->m_imagePositionPatient[axis];
			const double rhsValue = rhs->m_imagePositionPatient[axis];
			if (!std::isfinite(lhsValue) || !std::isfinite(rhsValue))
			{
				return 0;
			}
			const double delta = lhsValue - rhsValue;
			if (std::abs(delta) > positionEpsilon)
			{
				return (delta < 0.0) ? -1 : 1;
			}
		}
		return 0;
	};

	if (t_lhs->m_hasImagePositionPatient && t_rhs->m_hasImagePositionPatient)
	{
		std::array<double, 3> normal = {};
		bool hasNormal = computeNormal(t_lhs, normal);
		if (!hasNormal)
		{
			hasNormal = computeNormal(t_rhs, normal);
		}

		if (hasNormal)
		{
			std::array<double, 3> delta = {
				t_lhs->m_imagePositionPatient[0] - t_rhs->m_imagePositionPatient[0],
				t_lhs->m_imagePositionPatient[1] - t_rhs->m_imagePositionPatient[1],
				t_lhs->m_imagePositionPatient[2] - t_rhs->m_imagePositionPatient[2]};
			const double projected = delta[0] * normal[0]
				+ delta[1] * normal[1]
				+ delta[2] * normal[2];
			if (std::abs(projected) > positionEpsilon)
			{
				return projected < 0.0;
			}
		}

		const int positionComparison = comparePositions(t_lhs, t_rhs);
		if (positionComparison != 0)
		{
			return positionComparison < 0;
		}
	}

	const int lhsInstance = t_lhs->getInstanceNumber();
	const int rhsInstance = t_rhs->getInstanceNumber();
	if (lhsInstance != rhsInstance)
	{
		return lhsInstance < rhsInstance;
	}

	const double lhsSlice = t_lhs->getSliceLocation();
	const double rhsSlice = t_rhs->getSliceLocation();
	if (std::abs(lhsSlice - rhsSlice) > positionEpsilon)
	{
		return lhsSlice < rhsSlice;
	}

	const double lhsPathOrder = t_lhs->m_index >= 0 ? static_cast<double>(t_lhs->m_index) : 0.0;
	const double rhsPathOrder = t_rhs->m_index >= 0 ? static_cast<double>(t_rhs->m_index) : 0.0;
	return lhsPathOrder < rhsPathOrder;
}

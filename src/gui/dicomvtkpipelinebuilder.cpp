/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvtkpipelinebuilder.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements helpers that encapsulate the VTK image pipeline construction for the
 *      DICOM viewer widget.
 * ------------------------------------------------------------------------------------
 */

#include "dicomvtkpipelinebuilder.h"

#include "windowlevelfilter.h"

#include "dicomvolume.h"

#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkDataObject.h>
#include <vtkDataSetAttributes.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageMapper3D.h>
#include <vtkImageSliceMapper.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTrivialProducer.h>

#include <QByteArray>
#include <QLoggingCategory>

#include <algorithm>
#include <cmath>
#include <vector>

Q_DECLARE_LOGGING_CATEGORY(lcWidgetDicom)

namespace
{
	constexpr double epsilon = 1e-8;
	constexpr double uniformRangeThreshold = 1e-5;

	void copyDirectionToImage(vtkImageData* imageData, vtkMatrix4x4* direction)
	{
#if VTK_MAJOR_VERSION >= 9
		if (imageData && direction)
		{
			const double elements[9] = {
				direction->GetElement(0, 0), direction->GetElement(0, 1), direction->GetElement(0, 2),
				direction->GetElement(1, 0), direction->GetElement(1, 1), direction->GetElement(1, 2),
				direction->GetElement(2, 0), direction->GetElement(2, 1), direction->GetElement(2, 2) };
			imageData->SetDirectionMatrix(elements);
		}
#else
		(void)imageData;
		(void)direction;
#endif
	}

	void applySlopeIntercept(vtkImageData* imageData, isis::core::DicomPixelInfo& pixelInfo)
	{
		if (!imageData)
		{
			return;
		}
		if (std::abs(pixelInfo.RescaleSlope - 1.0) <= epsilon &&
			std::abs(pixelInfo.RescaleIntercept) <= epsilon)
		{
			return;
		}

		auto* const scalars = imageData->GetPointData() ? imageData->GetPointData()->GetScalars() : nullptr;
		if (!scalars)
		{
			return;
		}

		const vtkIdType tupleCount = scalars->GetNumberOfTuples();
		const int componentCount = scalars->GetNumberOfComponents();
		if (tupleCount <= 0 || componentCount <= 0)
		{
			return;
		}

		vtkNew<vtkDoubleArray> rescaled;
		rescaled->SetNumberOfComponents(componentCount);
		rescaled->SetNumberOfTuples(tupleCount);

		const char* const name = scalars->GetName();
		if (name && name[0] != '\0')
		{
			rescaled->SetName(name);
		}
		else
		{
			rescaled->SetName("RescaledScalars");
		}

		std::vector<double> tupleValues(static_cast<std::size_t>(componentCount), 0.0);
		const double slope = pixelInfo.RescaleSlope;
		const double intercept = pixelInfo.RescaleIntercept;
		for (vtkIdType tupleIndex = 0; tupleIndex < tupleCount; ++tupleIndex)
		{
			scalars->GetTuple(tupleIndex, tupleValues.data());
			for (int component = 0; component < componentCount; ++component)
			{
				const double value = tupleValues[static_cast<std::size_t>(component)];
				const double rescaledValue = slope * value + intercept;
				rescaled->SetComponent(tupleIndex, component, rescaledValue);
			}
		}

		if (auto* const pointData = imageData->GetPointData())
		{
			pointData->SetScalars(rescaled);
		}

		pixelInfo.RescaleSlope = 1.0;
		pixelInfo.RescaleIntercept = 0.0;
		imageData->Modified();
	}

	bool isUniformRange(const double* range)
	{
		if (!range)
		{
			return true;
		}
		const double span = range[1] - range[0];
		return !std::isfinite(range[0]) || !std::isfinite(range[1]) || std::abs(span) <= uniformRangeThreshold;
	}
}

using namespace isis;

gui::DicomVTKPipelineBuilder::DicomVTKPipelineBuilder()
	: m_windowLevelFilter(std::make_unique<WindowLevelFilter>())
{
	m_windowLevelColors = vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
	m_windowLevelColors->PassAlphaToOutputOff();
	m_windowLevelColors->SetOutputFormatToLuminance();

	m_inputProducer = vtkSmartPointer<vtkTrivialProducer>::New();
	m_windowLevelColors->SetInputConnection(m_inputProducer->GetOutputPort());

	m_imageActor = vtkSmartPointer<vtkImageActor>::New();
	m_imageActor->InterpolateOn();

	const QByteArray useSliceMapperEnv = qgetenv("ISIS_2D_USE_SLICE_MAPPER").trimmed().toLower();
	m_useSliceMapper =
		useSliceMapperEnv == "1" || useSliceMapperEnv == "true" || useSliceMapperEnv == "yes" ||
		useSliceMapperEnv == "on";
	if (m_useSliceMapper)
	{
		m_sliceMapper = vtkSmartPointer<vtkImageSliceMapper>::New();
		if (m_sliceMapper)
		{
			m_sliceMapper->SliceAtFocalPointOff();
			m_sliceMapper->SliceFacesCameraOn();
			m_sliceMapper->BorderOff();
		}
		m_imageActor->SetMapper(m_sliceMapper);
		qCInfo(lcWidgetDicom) << "Using vtkImageSliceMapper due to ISIS_2D_USE_SLICE_MAPPER" << useSliceMapperEnv;
	}
	else
	{
		qCInfo(lcWidgetDicom) << "Using default vtkImageActor mapper";
	}

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->SetBackground(0.0, 0.0, 0.0);
	m_renderer->AddActor(m_imageActor);

	m_windowLevelFilter->setWindowLevelColors(m_windowLevelColors);
}

gui::DicomVTKPipelineBuilder::VolumeBuildResult gui::DicomVTKPipelineBuilder::buildVolume(
	const std::shared_ptr<core::DicomVolume>& volume)
{
	VolumeBuildResult result;
	if (!volume || !volume->ImageData)
	{
		qCWarning(lcWidgetDicom) << "Clearing DICOM widget input.";
		clearVolume();
		return result;
	}

	int dims[3] = {0, 0, 0};
	volume->ImageData->GetDimensions(dims);
	qCInfo(lcWidgetDicom)
		<< "Configuring pipeline with dimensions"
		<< dims[0]
		<< dims[1]
		<< dims[2];

	auto preparation = prepareVolume(volume);
	updatePipelineWithVolume(preparation);

	initializeWindowParameters(*volume);

	if (auto* const actor = imageActor())
	{
		actor->SetVisibility(1);
		actor->Modified();
	}

	result.HasVolume = true;
	result.WindowWidth = m_windowWidth;
	result.WindowCenter = m_windowCenter;
	result.ColorsInverted = m_colorsInverted;

	return result;
}

void gui::DicomVTKPipelineBuilder::clearVolume()
{
	if (m_inputProducer)
	{
		m_inputProducer->SetOutput(nullptr);
		m_inputProducer->Modified();
	}
	if (auto* mapper = imageMapper())
	{
		mapper->SetInputData(nullptr);
		mapper->Modified();
	}
	if (m_imageActor)
	{
		m_imageActor->SetVisibility(0);
		m_imageActor->Modified();
	}
	if (m_windowLevelColors)
	{
		m_windowLevelColors->RemoveAllInputs();
		m_windowLevelColors->Update();
	}

	m_windowWidth = 0;
	m_windowCenter = 0;
	m_colorsInverted = false;
}

void gui::DicomVTKPipelineBuilder::setInvertColors(const bool flag)
{
	m_colorsInverted = flag;
	if (m_windowLevelFilter)
	{
		m_windowLevelFilter->setAreColorsInverted(m_colorsInverted);
	}
}

void gui::DicomVTKPipelineBuilder::setWindowWidthCenter(const int width, const int center)
{
	const int safeWidth = (width == 0) ? 1 : width;
	if (m_windowLevelFilter)
	{
		m_windowLevelFilter->setWindowWidthCenter(safeWidth, center);
	}
	if (m_windowLevelColors)
	{
		m_windowLevelColors->SetWindow(static_cast<double>(safeWidth));
		m_windowLevelColors->SetLevel(static_cast<double>(center));
		m_windowLevelColors->Update();
	}
	if (m_imageActor)
	{
		m_imageActor->Modified();
	}
	m_windowWidth = safeWidth;
	m_windowCenter = center;
	qCInfo(lcWidgetDicom)
		<< "setWindowWidthCenter() applied"
		<< "width"
		<< m_windowWidth
		<< "center"
		<< m_windowCenter;
}

void gui::DicomVTKPipelineBuilder::changeWindowWidthCenter(const int deltaWidth, const int deltaCenter)
{
	m_windowWidth += deltaWidth;
	m_windowCenter += deltaCenter;
	setWindowWidthCenter(m_windowWidth, m_windowCenter);
	qCInfo(lcWidgetDicom)
		<< "changeWindowWidthCenter()"
		<< "deltaWidth"
		<< deltaWidth
		<< "deltaCenter"
		<< deltaCenter
		<< "newWidth"
		<< m_windowWidth
		<< "newCenter"
		<< m_windowCenter;
}

void gui::DicomVTKPipelineBuilder::resetWindowFromVolume(const core::DicomVolume& volume)
{
	initializeWindowParameters(volume);
}

vtkImageMapper3D* gui::DicomVTKPipelineBuilder::imageMapper() const
{
	return m_imageActor ? m_imageActor->GetMapper() : nullptr;
}

gui::DicomVTKPipelineBuilder::VolumePreparation gui::DicomVTKPipelineBuilder::prepareVolume(
	const std::shared_ptr<core::DicomVolume>& volume) const
{
	VolumePreparation preparation;
	preparation.Volume = volume;
	if (!volume || !volume->ImageData)
	{
		return preparation;
	}

	copyDirectionToImage(volume->ImageData, volume->Direction.GetPointer());

	const bool requiresRescale =
		(std::abs(volume->PixelInfo.RescaleSlope - 1.0) > epsilon) ||
		(std::abs(volume->PixelInfo.RescaleIntercept) > epsilon);
	auto* scalarsBefore = volume->ImageData->GetPointData()
		                       ? volume->ImageData->GetPointData()->GetScalars()
		                       : nullptr;
	if (!scalarsBefore)
	{
		qCCritical(lcWidgetDicom) << "Scalar array missing prior to rescale.";
	}
	else if (!scalarsBefore->GetName())
	{
		scalarsBefore->SetName("Scalars");
		if (volume->ImageData->GetPointData())
		{
			volume->ImageData->GetPointData()->SetActiveScalars(scalarsBefore->GetName());
		}
	}
	if (requiresRescale)
	{
		applySlopeIntercept(volume->ImageData, volume->PixelInfo);
	}
	auto* scalarsAfter = volume->ImageData->GetPointData()
		                      ? volume->ImageData->GetPointData()->GetScalars()
		                      : nullptr;
	if (scalarsAfter)
	{
		double range[2] = {0.0, 0.0};
		scalarsAfter->GetRange(range);
		if (!scalarsAfter->GetName())
		{
			scalarsAfter->SetName("Scalars");
		}
		if (volume->ImageData->GetPointData())
		{
			volume->ImageData->GetPointData()->SetActiveScalars(scalarsAfter->GetName());
		}
		qCInfo(lcWidgetDicom)
			<< "Scalar array confirmed after rescale. Range:"
			<< range[0]
			<< range[1];
		if (isUniformRange(range))
		{
			qCWarning(lcWidgetDicom)
				<< "Volume scalars are uniform; the rendered slice will appear black.";
		}
	}
	else
	{
		qCCritical(lcWidgetDicom) << "Scalar array missing after rescale.";
	}

	const char* scalarName = "Scalars";
	if (scalarsAfter && scalarsAfter->GetName())
	{
		scalarName = scalarsAfter->GetName();
	}
	else if (scalarsBefore && scalarsBefore->GetName())
	{
		scalarName = scalarsBefore->GetName();
	}
	if (volume->ImageData->GetPointData())
	{
		volume->ImageData->GetPointData()->SetActiveScalars(scalarName);
	}

	vtkDataArray* activeScalars =
		volume->ImageData->GetPointData() ? volume->ImageData->GetPointData()->GetScalars() : nullptr;
	preparation.ActiveScalarName = scalarName;
	preparation.ScalarType = activeScalars ? activeScalars->GetDataType() : VTK_DOUBLE;
	preparation.ScalarComponents = activeScalars ? activeScalars->GetNumberOfComponents() : 1;

	return preparation;
}

void gui::DicomVTKPipelineBuilder::updatePipelineWithVolume(const VolumePreparation& preparation)
{
	if (!preparation.Volume || !preparation.Volume->ImageData)
	{
		return;
	}

	if (m_inputProducer)
	{
		m_inputProducer->SetOutput(preparation.Volume->ImageData);
		if (vtkInformation* producerInfo = m_inputProducer->GetOutputInformation(0))
		{
			vtkDataObject::SetPointDataActiveScalarInfo(
				producerInfo,
				preparation.ScalarType,
				preparation.ScalarComponents);
			vtkDataObject::SetActiveAttribute(
				producerInfo,
				vtkDataObject::FIELD_ASSOCIATION_POINTS,
				preparation.ActiveScalarName,
				vtkDataSetAttributes::SCALARS);
		}
		m_inputProducer->Modified();
		m_inputProducer->UpdateInformation();
	}

	if (m_windowLevelColors)
	{
		m_windowLevelColors->SetInputArrayToProcess(
			0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, preparation.ActiveScalarName);
		if (vtkInformation* inputInfo = m_windowLevelColors->GetInputInformation())
		{
			vtkDataObject::SetPointDataActiveScalarInfo(
				inputInfo,
				preparation.ScalarType,
				preparation.ScalarComponents);
			vtkDataObject::SetActiveAttribute(
				inputInfo,
				vtkDataObject::FIELD_ASSOCIATION_POINTS,
				preparation.ActiveScalarName,
				vtkDataSetAttributes::SCALARS);
		}
		m_windowLevelColors->UpdateInformation();
		m_windowLevelColors->Update();
		if (auto* output = m_windowLevelColors->GetOutput())
		{
			int dims[3] = {0, 0, 0};
			output->GetDimensions(dims);
			double range[2] = {0.0, 0.0};
			output->GetScalarRange(range);
			qCInfo(lcWidgetDicom)
				<< "Window-level output dimensions"
				<< dims[0]
				<< dims[1]
				<< dims[2]
				<< "range"
				<< range[0]
				<< range[1];
		}
		else
		{
			qCWarning(lcWidgetDicom) << "Window-level filter produced null output.";
		}
	}

	if (auto* mapper = imageMapper())
	{
		if (m_windowLevelColors)
		{
			mapper->SetInputConnection(m_windowLevelColors->GetOutputPort());
		}
		else
		{
			mapper->SetInputData(nullptr);
		}
		mapper->Modified();
	}
}

void gui::DicomVTKPipelineBuilder::initializeWindowParameters(const core::DicomVolume& volume)
{
	if (!volume.ImageData)
	{
		qCWarning(lcWidgetDicom) << "Missing volume while initializing window/level.";
		m_windowWidth = 0;
		m_windowCenter = 0;
		return;
	}
	if (volume.PixelInfo.WindowWidth > 0.0)
	{
		m_windowWidth = static_cast<int>(std::lround(volume.PixelInfo.WindowWidth));
		m_windowCenter = static_cast<int>(std::lround(volume.PixelInfo.WindowCenter));
		qCInfo(lcWidgetDicom)
			<< "Using DICOM window from metadata (width > 0). Width:"
			<< m_windowWidth
			<< "Center:"
			<< m_windowCenter;
	}
	else
	{
		applyDefaultWindowLevelFromRange(volume);
	}

	setInvertColors(volume.PixelInfo.InvertMonochrome);
	setWindowWidthCenter(m_windowWidth, m_windowCenter);
	qCInfo(lcWidgetDicom)
		<< "Initial window configured. Width:"
		<< m_windowWidth
		<< "Center:"
		<< m_windowCenter
		<< "Invert:"
		<< m_colorsInverted;
}

void gui::DicomVTKPipelineBuilder::applyDefaultWindowLevelFromRange(const core::DicomVolume& volume)
{
	auto* const scalars = volume.ImageData
		                      ? volume.ImageData->GetPointData()->GetScalars()
		                      : nullptr;
	if (!scalars)
	{
		m_windowWidth = 4096;
		m_windowCenter = 2048;
		qCWarning(lcWidgetDicom)
			<< "Fallback window level because scalars were unavailable.";
		return;
	}
	double range[2] = {0.0, 0.0};
	scalars->GetRange(range);
	m_windowWidth = static_cast<int>(std::round(range[1] - range[0]));
	m_windowCenter = static_cast<int>(std::round((range[1] + range[0]) * 0.5));
	if (m_windowWidth <= 0)
	{
		m_windowWidth = 4096;
	}
	qCInfo(lcWidgetDicom)
		<< "Computed window from scalar range:"
		<< range[0]
		<< range[1]
		<< "Width:"
		<< m_windowWidth
		<< "Center:"
		<< m_windowCenter;
}

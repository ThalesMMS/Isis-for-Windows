/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvtkpipelinebuilder.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares helpers that encapsulate the VTK image pipeline construction for the
 *      DICOM viewer widget.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "windowlevelfilter.h"
#include <vtkImageActor.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageSliceMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTrivialProducer.h>
#include <vtkType.h>

class vtkImageMapper3D;
class vtkMatrix4x4;

namespace isis::core
{
	struct DicomVolume;
}

namespace isis::gui
{

	class DicomVTKPipelineBuilder
	{
	public:
		struct VolumeBuildResult
		{
			bool HasVolume = false;
			int WindowWidth = 0;
			int WindowCenter = 0;
			bool ColorsInverted = false;
		};

		DicomVTKPipelineBuilder();

		VolumeBuildResult buildVolume(const std::shared_ptr<core::DicomVolume>& volume);
		void clearVolume();

		void setInvertColors(bool flag);
		void setWindowWidthCenter(int width, int center);
		void changeWindowWidthCenter(int deltaWidth, int deltaCenter);
		void resetWindowFromVolume(const core::DicomVolume& volume);

		[[nodiscard]] int windowWidth() const { return m_windowWidth; }
		[[nodiscard]] int windowCenter() const { return m_windowCenter; }
		[[nodiscard]] bool colorsInverted() const { return m_colorsInverted; }

		[[nodiscard]] vtkRenderer* renderer() const { return m_renderer; }
		[[nodiscard]] vtkImageActor* imageActor() const { return m_imageActor; }
		[[nodiscard]] vtkImageMapper3D* imageMapper() const;
		[[nodiscard]] vtkImageMapToWindowLevelColors* windowLevelColors() const { return m_windowLevelColors; }
		[[nodiscard]] vtkTrivialProducer* inputProducer() const { return m_inputProducer; }
		[[nodiscard]] WindowLevelFilter* windowLevelFilter() const { return m_windowLevelFilter.get(); }
		[[nodiscard]] bool isSliceMapperEnabled() const { return m_useSliceMapper; }

	private:
		struct VolumePreparation
		{
			std::shared_ptr<core::DicomVolume> Volume;
			const char* ActiveScalarName = "Scalars";
			int ScalarType = VTK_DOUBLE;
			int ScalarComponents = 1;
		};

		[[nodiscard]] VolumePreparation prepareVolume(const std::shared_ptr<core::DicomVolume>& volume) const;
		void updatePipelineWithVolume(const VolumePreparation& preparation);
		void initializeWindowParameters(const core::DicomVolume& volume);
		void applyDefaultWindowLevelFromRange(const core::DicomVolume& volume);

		std::unique_ptr<WindowLevelFilter> m_windowLevelFilter;
		vtkSmartPointer<vtkImageMapToWindowLevelColors> m_windowLevelColors;
		vtkSmartPointer<vtkTrivialProducer> m_inputProducer;
		vtkSmartPointer<vtkImageActor> m_imageActor;
		vtkSmartPointer<vtkImageSliceMapper> m_sliceMapper;
		vtkSmartPointer<vtkRenderer> m_renderer;
		bool m_useSliceMapper = false;

		int m_windowWidth = 0;
		int m_windowCenter = 0;
		bool m_colorsInverted = false;
	};
}

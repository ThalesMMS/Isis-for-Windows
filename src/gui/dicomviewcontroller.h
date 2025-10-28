/*
 * ------------------------------------------------------------------------------------
 *  File: dicomviewcontroller.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares the controller orchestrating render window interactions, camera logic,
 *      and slice management for the DICOM VTK widget.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <limits>
#include <tuple>

class vtkImageActor;
class vtkInformation;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;

namespace isis::gui
{
	class DicomCoreSyncAdapter;
	class DicomVTKPipelineBuilder;

	class DicomViewController
	{
	public:
		static constexpr int SLICE_ORIENTATION_YZ = 0;
		static constexpr int SLICE_ORIENTATION_XZ = 1;
		static constexpr int SLICE_ORIENTATION_XY = 2;

		DicomViewController(DicomVTKPipelineBuilder& pipelineBuilder, DicomCoreSyncAdapter& coreAdapter);

		void setRenderWindow(vtkRenderWindow* renderWindow);
		void setupInteractor(vtkRenderWindowInteractor* interactor);
		void render();

		void onVolumeCleared();
		void onVolumeLoaded();

		void setSliceOrientation(int orientation);
		void setSlice(int slice);

		void updateDisplayExtent();

		[[nodiscard]] vtkRenderWindow* renderWindow() const { return m_renderWindow; }
		[[nodiscard]] vtkRenderer* renderer() const;
		[[nodiscard]] vtkRenderWindowInteractor* interactor() const { return m_interactor; }

		[[nodiscard]] int sliceOrientation() const { return m_sliceOrientation; }
		[[nodiscard]] int sliceMin() const { return m_sliceMin; }
		[[nodiscard]] int sliceMax() const { return m_sliceMax; }
		[[nodiscard]] int currentSlice() const { return m_currentSlice; }

		[[nodiscard]] double getZoomFactor() const;
		[[nodiscard]] std::tuple<int, int> getImageActorDisplayValue() const;

		[[nodiscard]] std::array<double, 3> lastSanitizedSpacing() const { return m_cachedSpacing; }
		[[nodiscard]] std::array<double, 3> originalSpacing() const { return m_originalSpacing; }
		[[nodiscard]] bool hasOriginalSpacing() const { return m_hasOriginalSpacing; }

	private:
		void ensureRendererAttached();
		void updateSliceRange();
		void updateActorExtentWithInformation(vtkInformation* info);

		DicomVTKPipelineBuilder& m_pipelineBuilder;
		DicomCoreSyncAdapter& m_coreAdapter;

		vtkRenderWindow* m_renderWindow = nullptr;
		vtkRenderWindowInteractor* m_interactor = nullptr;

		int m_sliceOrientation = SLICE_ORIENTATION_XY;
		int m_currentSlice = 0;
		int m_sliceMin = 0;
		int m_sliceMax = 0;

		double m_lastClippingRange = -1.0;
		double m_lastAvgSpacing = -1.0;
		int m_lastSliceOrientation = -1;

		std::array<double, 3> m_cachedSpacing = {1.0, 1.0, 1.0};
		std::array<double, 3> m_originalSpacing = {1.0, 1.0, 1.0};
		std::array<double, 3> m_cachedCameraPosition = {std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN()};
		std::array<double, 3> m_cachedCameraFocalPoint = {std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN()};
		bool m_hasCachedSpacing = false;
		bool m_hasOriginalSpacing = false;
		bool m_hasCachedCamera = false;
	};
}

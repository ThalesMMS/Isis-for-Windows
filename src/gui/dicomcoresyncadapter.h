/*
 * ------------------------------------------------------------------------------------
 *  File: dicomcoresyncadapter.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares the adapter responsible for synchronising DICOM core data with the
 *      viewer's VTK pipeline.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>

namespace isis::core
{
	struct DicomVolume;
}

namespace isis::gui
{
	class DicomVTKPipelineBuilder;

	class DicomCoreSyncAdapter
	{
	public:
		struct VolumeState
		{
			bool HasVolume = false;
			int WindowWidth = 0;
			int WindowCenter = 0;
			bool ColorsInverted = false;
		};

		explicit DicomCoreSyncAdapter(DicomVTKPipelineBuilder& pipelineBuilder);

		VolumeState setVolume(const std::shared_ptr<core::DicomVolume>& volume);
		void clearVolume();
		void applyDirectionMatrix();

		[[nodiscard]] const std::shared_ptr<core::DicomVolume>& volume() const { return m_volume; }

	private:
		DicomVTKPipelineBuilder& m_pipelineBuilder;
		std::shared_ptr<core::DicomVolume> m_volume;
	};
}

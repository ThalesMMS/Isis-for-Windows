/*
 * ------------------------------------------------------------------------------------
 *  File: dicomcoresyncadapter.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements the adapter responsible for synchronising DICOM core data with the
 *      viewer's VTK pipeline.
 * ------------------------------------------------------------------------------------
 */

#include "dicomcoresyncadapter.h"

#include "dicomvtkpipelinebuilder.h"

#include "dicomvolume.h"

#include <vtkImageActor.h>
#include <vtkMatrix4x4.h>
#include <vtkVersionMacros.h>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcWidgetDicom)

using namespace isis;

gui::DicomCoreSyncAdapter::DicomCoreSyncAdapter(DicomVTKPipelineBuilder& pipelineBuilder)
	: m_pipelineBuilder(pipelineBuilder)
{
}

gui::DicomCoreSyncAdapter::VolumeState gui::DicomCoreSyncAdapter::setVolume(
	const std::shared_ptr<core::DicomVolume>& volume)
{
	m_volume = volume;
	VolumeState state;

	if (!m_volume || !m_volume->ImageData)
	{
		m_pipelineBuilder.clearVolume();
		return state;
	}

	const auto buildResult = m_pipelineBuilder.buildVolume(m_volume);
	state.HasVolume = buildResult.HasVolume;
	state.WindowWidth = buildResult.WindowWidth;
	state.WindowCenter = buildResult.WindowCenter;
	state.ColorsInverted = buildResult.ColorsInverted;

	applyDirectionMatrix();

	return state;
}

void gui::DicomCoreSyncAdapter::clearVolume()
{
	m_pipelineBuilder.clearVolume();
	m_volume.reset();
}

void gui::DicomCoreSyncAdapter::applyDirectionMatrix()
{
#if VTK_MAJOR_VERSION >= 9
	if (!m_volume || !m_volume->Direction)
	{
		return;
	}
	auto* const actor = m_pipelineBuilder.imageActor();
	if (!actor)
	{
		return;
	}
	actor->SetUserMatrix(m_volume->Direction);
#endif
}

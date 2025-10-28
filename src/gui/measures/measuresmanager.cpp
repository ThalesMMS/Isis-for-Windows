/*
 * ------------------------------------------------------------------------------------
 *  File: measuresmanager.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of the central measurement tools manager
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "measuresmanager.h"

namespace isis::gui::measures
{
    MeasuresManager::MeasuresManager()
        : m_distanceTool(std::make_unique<DistanceMeasureTool>())
        , m_angleTool(std::make_unique<AngleMeasureTool>())
        , m_biDimensionalTool(std::make_unique<BiDimensionalMeasureTool>())
        , m_contourTool(std::make_unique<ContourTool>())
    {
    }

    void MeasuresManager::initialize(vtkRenderWindowInteractor* interactor)
    {
        if (!interactor || m_initialized)
        {
            return;
        }

        // Initialize all tools with the interactor
        m_distanceTool->initialize(interactor);
        m_angleTool->initialize(interactor);
        m_biDimensionalTool->initialize(interactor);
        m_contourTool->initialize(interactor);

        m_initialized = true;
    }

    void MeasuresManager::activateTool(MeasureToolType toolType)
    {
        if (!m_initialized)
        {
            return;
        }

        // Deactivate current tool if any
        deactivateCurrentTool();

        // Activate the requested tool
        switch (toolType)
        {
        case MeasureToolType::Distance:
            m_distanceTool->enable();
            m_currentTool = MeasureToolType::Distance;
            break;

        case MeasureToolType::Angle:
            m_angleTool->enable();
            m_currentTool = MeasureToolType::Angle;
            break;

        case MeasureToolType::BiDimensional:
            m_biDimensionalTool->enable();
            m_currentTool = MeasureToolType::BiDimensional;
            break;

        case MeasureToolType::Contour:
            m_contourTool->enable();
            m_currentTool = MeasureToolType::Contour;
            break;

        case MeasureToolType::None:
        default:
            m_currentTool = MeasureToolType::None;
            break;
        }
    }

    void MeasuresManager::deactivateCurrentTool()
    {
        if (!m_initialized)
        {
            return;
        }

        // Deactivate based on current tool
        switch (m_currentTool)
        {
        case MeasureToolType::Distance:
            m_distanceTool->disable();
            break;

        case MeasureToolType::Angle:
            m_angleTool->disable();
            break;

        case MeasureToolType::BiDimensional:
            m_biDimensionalTool->disable();
            break;

        case MeasureToolType::Contour:
            m_contourTool->disable();
            break;

        case MeasureToolType::None:
        default:
            break;
        }

        m_currentTool = MeasureToolType::None;
    }

    void MeasuresManager::deactivateAllTools()
    {
        if (!m_initialized)
        {
            return;
        }

        m_distanceTool->disable();
        m_angleTool->disable();
        m_biDimensionalTool->disable();
        m_contourTool->disable();

        m_currentTool = MeasureToolType::None;
    }

} // namespace isis::gui::measures

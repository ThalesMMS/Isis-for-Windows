/*
 * ------------------------------------------------------------------------------------
 *  File: measuresmanager.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Central manager for all measurement tools (distance, angle, bidimensional, contour)
 *      Provides unified interface for tool activation and measurement persistence.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "distancemeasuretool.h"
#include "anglemeasuretool.h"
#include "bidimensionalmeasuretool.h"
#include "contourtool.h"
#include <memory>
#include <vector>
#include <vtkRenderWindowInteractor.h>

namespace isis::gui::measures
{
    /**
     * @brief Type of measurement tool
     */
    enum class MeasureToolType
    {
        None,
        Distance,
        Angle,
        BiDimensional,
        Contour
    };

    /**
     * @brief Manager for all measurement tools
     *
     * This class provides a centralized interface for managing multiple
     * measurement tools, ensuring only one tool is active at a time.
     */
    class MeasuresManager
    {
    public:
        MeasuresManager();
        ~MeasuresManager() = default;

        /**
         * @brief Initialize all measurement tools with a render window interactor
         * @param interactor The VTK render window interactor
         */
        void initialize(vtkRenderWindowInteractor* interactor);

        /**
         * @brief Activate a specific measurement tool
         * @param toolType Type of tool to activate
         */
        void activateTool(MeasureToolType toolType);

        /**
         * @brief Deactivate the currently active tool
         */
        void deactivateCurrentTool();

        /**
         * @brief Deactivate all tools
         */
        void deactivateAllTools();

        /**
         * @brief Get the currently active tool type
         * @return Current tool type
         */
        [[nodiscard]] MeasureToolType getCurrentToolType() const { return m_currentTool; }

        /**
         * @brief Check if any tool is currently active
         * @return true if a tool is active, false otherwise
         */
        [[nodiscard]] bool isAnyToolActive() const { return m_currentTool != MeasureToolType::None; }

        // Tool accessors
        [[nodiscard]] DistanceMeasureTool* getDistanceTool() { return m_distanceTool.get(); }
        [[nodiscard]] AngleMeasureTool* getAngleTool() { return m_angleTool.get(); }
        [[nodiscard]] BiDimensionalMeasureTool* getBiDimensionalTool() { return m_biDimensionalTool.get(); }
        [[nodiscard]] ContourTool* getContourTool() { return m_contourTool.get(); }

        // Const accessors
        [[nodiscard]] const DistanceMeasureTool* getDistanceTool() const { return m_distanceTool.get(); }
        [[nodiscard]] const AngleMeasureTool* getAngleTool() const { return m_angleTool.get(); }
        [[nodiscard]] const BiDimensionalMeasureTool* getBiDimensionalTool() const { return m_biDimensionalTool.get(); }
        [[nodiscard]] const ContourTool* getContourTool() const { return m_contourTool.get(); }

    private:
        std::unique_ptr<DistanceMeasureTool> m_distanceTool;
        std::unique_ptr<AngleMeasureTool> m_angleTool;
        std::unique_ptr<BiDimensionalMeasureTool> m_biDimensionalTool;
        std::unique_ptr<ContourTool> m_contourTool;

        MeasureToolType m_currentTool = MeasureToolType::None;
        bool m_initialized = false;
    };

} // namespace isis::gui::measures

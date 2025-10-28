/*
 * ------------------------------------------------------------------------------------
 *  File: anglemeasuretool.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Angle measurement tool for measuring angles between anatomical structures
 *      using VTK widgets. Supports acute, obtuse, and reflex angles.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkAngleWidget.h>
#include <vtkAngleRepresentation.h>
#include <vtkRenderWindowInteractor.h>

namespace isis::gui::measures
{
    /**
     * @brief Angle measurement tool for measuring angles
     *
     * This class encapsulates a vtkAngleWidget to provide angle
     * measurements in DICOM images with automatic calculation in degrees.
     */
    class AngleMeasureTool
    {
    public:
        AngleMeasureTool();
        ~AngleMeasureTool() = default;

        /**
         * @brief Initialize the angle widget with a render window interactor
         * @param interactor The VTK render window interactor
         */
        void initialize(vtkRenderWindowInteractor* interactor);

        /**
         * @brief Enable the angle measurement tool
         */
        void enable();

        /**
         * @brief Disable the angle measurement tool
         */
        void disable();

        /**
         * @brief Check if the tool is currently enabled
         * @return true if enabled, false otherwise
         */
        [[nodiscard]] bool isEnabled() const;

        /**
         * @brief Reset the tool to start a new measurement
         */
        void reset();

        /**
         * @brief Get the current angle measurement value
         * @return Angle in degrees
         */
        [[nodiscard]] double getAngle() const;

        /**
         * @brief Get the underlying VTK widget
         * @return Pointer to the vtkAngleWidget
         */
        [[nodiscard]] vtkAngleWidget* getWidget() const { return m_angleWidget.Get(); }

    private:
        vtkSmartPointer<vtkAngleWidget> m_angleWidget;
        bool m_enabled = false;
    };

} // namespace isis::gui::measures

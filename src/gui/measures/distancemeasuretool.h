/*
 * ------------------------------------------------------------------------------------
 *  File: distancemeasuretool.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Distance measurement tool using VTK widgets for precise linear measurements
 *      with sub-pixel accuracy. Supports DICOM physical units (mm).
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkDistanceWidget.h>
#include <vtkDistanceRepresentation.h>
#include <vtkRenderWindowInteractor.h>
#include <memory>
#include <vector>

namespace isis::gui::measures
{
    /**
     * @brief Distance measurement tool for linear measurements
     *
     * This class encapsulates a vtkDistanceWidget to provide distance
     * measurements in DICOM images with physical unit support (mm).
     */
    class DistanceMeasureTool
    {
    public:
        DistanceMeasureTool();
        ~DistanceMeasureTool() = default;

        /**
         * @brief Initialize the distance widget with a render window interactor
         * @param interactor The VTK render window interactor
         */
        void initialize(vtkRenderWindowInteractor* interactor);

        /**
         * @brief Enable the distance measurement tool
         */
        void enable();

        /**
         * @brief Disable the distance measurement tool
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
         * @brief Get the current distance measurement value
         * @return Distance in millimeters
         */
        [[nodiscard]] double getDistance() const;

        /**
         * @brief Set the label format for distance display
         * @param format Printf-style format string (default: "%-#6.3g mm")
         */
        void setLabelFormat(const char* format);

        /**
         * @brief Get the underlying VTK widget
         * @return Pointer to the vtkDistanceWidget
         */
        [[nodiscard]] vtkDistanceWidget* getWidget() const { return m_distanceWidget.Get(); }

    private:
        vtkSmartPointer<vtkDistanceWidget> m_distanceWidget;
        bool m_enabled = false;
    };

} // namespace isis::gui::measures

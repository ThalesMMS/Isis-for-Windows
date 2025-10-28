/*
 * ------------------------------------------------------------------------------------
 *  File: bidimensionalmeasuretool.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Bi-dimensional measurement tool for measuring area and perimeter
 *      of 2D structures using VTK widgets with DICOM calibration support.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkBiDimensionalWidget.h>
#include <vtkBiDimensionalRepresentation2D.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCommand.h>

namespace isis::gui::measures
{
    /**
     * @brief Callback for bi-dimensional widget interaction events
     */
    class BiDimensionalCallback : public vtkCommand
    {
    public:
        static BiDimensionalCallback* New();

        void Execute(vtkObject* caller, unsigned long eventId, void* callData) override;

        BiDimensionalCallback() = default;
        ~BiDimensionalCallback() override = default;
    };

    /**
     * @brief Bi-dimensional measurement tool for area and perimeter measurements
     *
     * This class encapsulates a vtkBiDimensionalWidget to provide 2D
     * measurements (area and perimeter) in DICOM images with physical units.
     */
    class BiDimensionalMeasureTool
    {
    public:
        BiDimensionalMeasureTool();
        ~BiDimensionalMeasureTool() = default;

        /**
         * @brief Initialize the bi-dimensional widget with a render window interactor
         * @param interactor The VTK render window interactor
         */
        void initialize(vtkRenderWindowInteractor* interactor);

        /**
         * @brief Enable the bi-dimensional measurement tool
         */
        void enable();

        /**
         * @brief Disable the bi-dimensional measurement tool
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
         * @brief Get the first axis length
         * @return Length in millimeters
         */
        [[nodiscard]] double getLength1() const;

        /**
         * @brief Get the second axis length
         * @return Length in millimeters
         */
        [[nodiscard]] double getLength2() const;

        /**
         * @brief Calculate area (assuming elliptical shape)
         * @return Area in mm²
         */
        [[nodiscard]] double getArea() const;

        /**
         * @brief Get the underlying VTK widget
         * @return Pointer to the vtkBiDimensionalWidget
         */
        [[nodiscard]] vtkBiDimensionalWidget* getWidget() const { return m_biDimensionalWidget.Get(); }

    private:
        vtkSmartPointer<vtkBiDimensionalWidget> m_biDimensionalWidget;
        vtkSmartPointer<BiDimensionalCallback> m_callback;
        bool m_enabled = false;
    };

} // namespace isis::gui::measures

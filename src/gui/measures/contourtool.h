/*
 * ------------------------------------------------------------------------------------
 *  File: contourtool.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Contour tool for manual delineation of regions of interest (ROIs)
 *      with support for open and closed contours, area/perimeter calculation.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkContourWidget.h>
#include <vtkOrientedGlyphContourRepresentation.h>
#include <vtkRenderWindowInteractor.h>

namespace isis::gui::measures
{
    /**
     * @brief Contour tool for ROI delineation
     *
     * This class encapsulates a vtkContourWidget to provide manual
     * contour drawing with automatic area, perimeter, and statistics.
     */
    class ContourTool
    {
    public:
        ContourTool();
        ~ContourTool() = default;

        /**
         * @brief Initialize the contour widget with a render window interactor
         * @param interactor The VTK render window interactor
         */
        void initialize(vtkRenderWindowInteractor* interactor);

        /**
         * @brief Enable the contour tool
         */
        void enable();

        /**
         * @brief Disable the contour tool
         */
        void disable();

        /**
         * @brief Check if the tool is currently enabled
         * @return true if enabled, false otherwise
         */
        [[nodiscard]] bool isEnabled() const;

        /**
         * @brief Reset the tool to start a new contour
         */
        void reset();

        /**
         * @brief Close the current contour
         */
        void closeContour();

        /**
         * @brief Check if the contour is closed
         * @return true if closed, false otherwise
         */
        [[nodiscard]] bool isClosed() const;

        /**
         * @brief Get the number of nodes in the contour
         * @return Number of contour nodes
         */
        [[nodiscard]] int getNumberOfNodes() const;

        /**
         * @brief Calculate the perimeter of the contour
         * @return Perimeter in millimeters
         */
        [[nodiscard]] double getPerimeter() const;

        /**
         * @brief Calculate the area enclosed by the contour (closed contours only)
         * @return Area in mm²
         */
        [[nodiscard]] double getArea() const;

        /**
         * @brief Set the contour line color
         * @param r Red component (0.0-1.0)
         * @param g Green component (0.0-1.0)
         * @param b Blue component (0.0-1.0)
         */
        void setLineColor(double r, double g, double b);

        /**
         * @brief Get the underlying VTK widget
         * @return Pointer to the vtkContourWidget
         */
        [[nodiscard]] vtkContourWidget* getWidget() const { return m_contourWidget.Get(); }

    private:
        vtkSmartPointer<vtkContourWidget> m_contourWidget;
        vtkSmartPointer<vtkOrientedGlyphContourRepresentation> m_contourRepresentation;
        bool m_enabled = false;
        bool m_closed = false;
    };

} // namespace isis::gui::measures

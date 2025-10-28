/*
 * ------------------------------------------------------------------------------------
 *  File: contourtool.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of contour tool for ROI delineation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "contourtool.h"
#include <vtkContourRepresentation.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkProperty.h>
#include <cmath>

namespace isis::gui::measures
{
    ContourTool::ContourTool()
    {
        m_contourWidget = vtkSmartPointer<vtkContourWidget>::New();
        m_contourRepresentation = vtkSmartPointer<vtkOrientedGlyphContourRepresentation>::New();
    }

    void ContourTool::initialize(vtkRenderWindowInteractor* interactor)
    {
        if (!interactor)
        {
            return;
        }

        m_contourWidget->SetInteractor(interactor);
        m_contourWidget->SetRepresentation(m_contourRepresentation);

        // Set default line color to red
        m_contourRepresentation->GetLinesProperty()->SetColor(1.0, 0.0, 0.0);
        m_contourRepresentation->GetLinesProperty()->SetLineWidth(2.0);
    }

    void ContourTool::enable()
    {
        if (m_contourWidget)
        {
            m_contourWidget->EnabledOn();
            m_enabled = true;
        }
    }

    void ContourTool::disable()
    {
        if (m_contourWidget)
        {
            m_contourWidget->EnabledOff();
            m_enabled = false;
        }
    }

    bool ContourTool::isEnabled() const
    {
        return m_enabled;
    }

    void ContourTool::reset()
    {
        if (m_contourWidget && m_enabled)
        {
            m_contourWidget->Initialize();
            m_closed = false;
        }
    }

    void ContourTool::closeContour()
    {
        if (m_contourWidget && m_enabled)
        {
            auto* representation = m_contourWidget->GetContourRepresentation();
            if (representation)
            {
                representation->SetClosedLoop(1);
                m_closed = true;
            }
        }
    }

    bool ContourTool::isClosed() const
    {
        return m_closed;
    }

    int ContourTool::getNumberOfNodes() const
    {
        if (!m_contourWidget)
        {
            return 0;
        }

        auto* representation = m_contourWidget->GetContourRepresentation();
        if (representation)
        {
            return representation->GetNumberOfNodes();
        }

        return 0;
    }

    double ContourTool::getPerimeter() const
    {
        if (!m_contourWidget)
        {
            return 0.0;
        }

        auto* representation = m_contourWidget->GetContourRepresentation();
        if (!representation)
        {
            return 0.0;
        }

        const int numNodes = representation->GetNumberOfNodes();
        if (numNodes < 2)
        {
            return 0.0;
        }

        double perimeter = 0.0;
        double prevPos[3], currPos[3];
        representation->GetNthNodeWorldPosition(0, prevPos);

        for (int i = 1; i < numNodes; ++i)
        {
            representation->GetNthNodeWorldPosition(i, currPos);

            const double dx = currPos[0] - prevPos[0];
            const double dy = currPos[1] - prevPos[1];
            const double dz = currPos[2] - prevPos[2];

            perimeter += std::sqrt(dx*dx + dy*dy + dz*dz);

            prevPos[0] = currPos[0];
            prevPos[1] = currPos[1];
            prevPos[2] = currPos[2];
        }

        // If closed, add distance from last to first point
        if (m_closed && numNodes > 2)
        {
            double firstPos[3];
            representation->GetNthNodeWorldPosition(0, firstPos);

            const double dx = firstPos[0] - prevPos[0];
            const double dy = firstPos[1] - prevPos[1];
            const double dz = firstPos[2] - prevPos[2];

            perimeter += std::sqrt(dx*dx + dy*dy + dz*dz);
        }

        return perimeter;
    }

    double ContourTool::getArea() const
    {
        if (!m_contourWidget || !m_closed)
        {
            return 0.0;
        }

        auto* representation = m_contourWidget->GetContourRepresentation();
        if (!representation)
        {
            return 0.0;
        }

        const int numNodes = representation->GetNumberOfNodes();
        if (numNodes < 3)
        {
            return 0.0;
        }

        // Calculate area using shoelace formula (2D projection on XY plane)
        double area = 0.0;
        double pos1[3], pos2[3];

        for (int i = 0; i < numNodes; ++i)
        {
            representation->GetNthNodeWorldPosition(i, pos1);
            representation->GetNthNodeWorldPosition((i + 1) % numNodes, pos2);

            area += (pos1[0] * pos2[1]) - (pos2[0] * pos1[1]);
        }

        return std::abs(area) / 2.0;
    }

    void ContourTool::setLineColor(double r, double g, double b)
    {
        if (m_contourRepresentation)
        {
            m_contourRepresentation->GetLinesProperty()->SetColor(r, g, b);
        }
    }

} // namespace isis::gui::measures

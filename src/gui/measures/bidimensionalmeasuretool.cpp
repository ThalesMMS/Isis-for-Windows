/*
 * ------------------------------------------------------------------------------------
 *  File: bidimensionalmeasuretool.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of bi-dimensional measurement tool
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "bidimensionalmeasuretool.h"
#include <vtkBiDimensionalRepresentation2D.h>
#include <cmath>
#include <iostream>

namespace isis::gui::measures
{
    // BiDimensionalCallback implementation
    BiDimensionalCallback* BiDimensionalCallback::New()
    {
        return new BiDimensionalCallback;
    }

    void BiDimensionalCallback::Execute(vtkObject* caller, unsigned long eventId, void* callData)
    {
        auto* widget = reinterpret_cast<vtkBiDimensionalWidget*>(caller);
        if (!widget)
        {
            return;
        }

        auto* representation = static_cast<vtkBiDimensionalRepresentation2D*>(
            widget->GetRepresentation());

        if (!representation)
        {
            return;
        }

        // Get measurement points
        double p1[3], p2[3], p3[3], p4[3];
        representation->GetPoint1DisplayPosition(p1);
        representation->GetPoint2DisplayPosition(p2);
        representation->GetPoint3DisplayPosition(p3);
        representation->GetPoint4DisplayPosition(p4);

        // Calculate lengths
        double length1 = representation->GetLength1();
        double length2 = representation->GetLength2();

        // Display measurements (for debugging/logging)
        std::cout << "BiDimensional Measurement: "
                  << "L1=" << length1 << " mm, "
                  << "L2=" << length2 << " mm" << std::endl;
    }

    // BiDimensionalMeasureTool implementation
    BiDimensionalMeasureTool::BiDimensionalMeasureTool()
    {
        m_biDimensionalWidget = vtkSmartPointer<vtkBiDimensionalWidget>::New();
        m_callback = vtkSmartPointer<BiDimensionalCallback>::New();
    }

    void BiDimensionalMeasureTool::initialize(vtkRenderWindowInteractor* interactor)
    {
        if (!interactor)
        {
            return;
        }

        m_biDimensionalWidget->SetInteractor(interactor);
        m_biDimensionalWidget->CreateDefaultRepresentation();

        // Add observer for interaction events
        m_biDimensionalWidget->AddObserver(vtkCommand::InteractionEvent, m_callback);
    }

    void BiDimensionalMeasureTool::enable()
    {
        if (m_biDimensionalWidget)
        {
            m_biDimensionalWidget->EnabledOn();
            m_biDimensionalWidget->SetWidgetStateToStart();
            m_enabled = true;
        }
    }

    void BiDimensionalMeasureTool::disable()
    {
        if (m_biDimensionalWidget)
        {
            m_biDimensionalWidget->EnabledOff();
            m_enabled = false;
        }
    }

    bool BiDimensionalMeasureTool::isEnabled() const
    {
        return m_enabled;
    }

    void BiDimensionalMeasureTool::reset()
    {
        if (m_biDimensionalWidget && m_enabled)
        {
            m_biDimensionalWidget->SetWidgetStateToStart();
        }
    }

    double BiDimensionalMeasureTool::getLength1() const
    {
        if (!m_biDimensionalWidget)
        {
            return 0.0;
        }

        auto* representation = static_cast<vtkBiDimensionalRepresentation2D*>(
            m_biDimensionalWidget->GetRepresentation());

        if (representation)
        {
            return representation->GetLength1();
        }

        return 0.0;
    }

    double BiDimensionalMeasureTool::getLength2() const
    {
        if (!m_biDimensionalWidget)
        {
            return 0.0;
        }

        auto* representation = static_cast<vtkBiDimensionalRepresentation2D*>(
            m_biDimensionalWidget->GetRepresentation());

        if (representation)
        {
            return representation->GetLength2();
        }

        return 0.0;
    }

    double BiDimensionalMeasureTool::getArea() const
    {
        // Calculate area assuming elliptical shape: A = π * (L1/2) * (L2/2)
        const double length1 = getLength1();
        const double length2 = getLength2();

        if (length1 > 0.0 && length2 > 0.0)
        {
            constexpr double pi = 3.14159265358979323846;
            return pi * (length1 / 2.0) * (length2 / 2.0);
        }

        return 0.0;
    }

} // namespace isis::gui::measures

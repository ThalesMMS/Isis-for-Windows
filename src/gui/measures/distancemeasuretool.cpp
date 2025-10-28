/*
 * ------------------------------------------------------------------------------------
 *  File: distancemeasuretool.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of distance measurement tool
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "distancemeasuretool.h"
#include <vtkDistanceRepresentation.h>

namespace isis::gui::measures
{
    DistanceMeasureTool::DistanceMeasureTool()
    {
        m_distanceWidget = vtkSmartPointer<vtkDistanceWidget>::New();
    }

    void DistanceMeasureTool::initialize(vtkRenderWindowInteractor* interactor)
    {
        if (!interactor)
        {
            return;
        }

        m_distanceWidget->SetInteractor(interactor);
        m_distanceWidget->CreateDefaultRepresentation();

        // Set default label format to show distance in millimeters
        auto* representation = static_cast<vtkDistanceRepresentation*>(
            m_distanceWidget->GetRepresentation());

        if (representation)
        {
            representation->SetLabelFormat("%-#6.3g mm");
        }
    }

    void DistanceMeasureTool::enable()
    {
        if (m_distanceWidget)
        {
            m_distanceWidget->EnabledOn();
            m_distanceWidget->SetWidgetStateToStart();
            m_enabled = true;
        }
    }

    void DistanceMeasureTool::disable()
    {
        if (m_distanceWidget)
        {
            m_distanceWidget->EnabledOff();
            m_enabled = false;
        }
    }

    bool DistanceMeasureTool::isEnabled() const
    {
        return m_enabled;
    }

    void DistanceMeasureTool::reset()
    {
        if (m_distanceWidget && m_enabled)
        {
            m_distanceWidget->SetWidgetStateToStart();
        }
    }

    double DistanceMeasureTool::getDistance() const
    {
        if (!m_distanceWidget)
        {
            return 0.0;
        }

        auto* representation = static_cast<vtkDistanceRepresentation*>(
            m_distanceWidget->GetRepresentation());

        if (representation)
        {
            return representation->GetDistance();
        }

        return 0.0;
    }

    void DistanceMeasureTool::setLabelFormat(const char* format)
    {
        if (!m_distanceWidget)
        {
            return;
        }

        auto* representation = static_cast<vtkDistanceRepresentation*>(
            m_distanceWidget->GetRepresentation());

        if (representation && format)
        {
            representation->SetLabelFormat(format);
        }
    }

} // namespace isis::gui::measures

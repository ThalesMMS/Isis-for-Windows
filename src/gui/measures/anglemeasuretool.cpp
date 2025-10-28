/*
 * ------------------------------------------------------------------------------------
 *  File: anglemeasuretool.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of angle measurement tool
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "anglemeasuretool.h"
#include <vtkAngleRepresentation.h>

namespace isis::gui::measures
{
    AngleMeasureTool::AngleMeasureTool()
    {
        m_angleWidget = vtkSmartPointer<vtkAngleWidget>::New();
    }

    void AngleMeasureTool::initialize(vtkRenderWindowInteractor* interactor)
    {
        if (!interactor)
        {
            return;
        }

        m_angleWidget->SetInteractor(interactor);
        m_angleWidget->CreateDefaultRepresentation();
    }

    void AngleMeasureTool::enable()
    {
        if (m_angleWidget)
        {
            m_angleWidget->EnabledOn();
            m_angleWidget->SetWidgetStateToStart();
            m_enabled = true;
        }
    }

    void AngleMeasureTool::disable()
    {
        if (m_angleWidget)
        {
            m_angleWidget->EnabledOff();
            m_enabled = false;
        }
    }

    bool AngleMeasureTool::isEnabled() const
    {
        return m_enabled;
    }

    void AngleMeasureTool::reset()
    {
        if (m_angleWidget && m_enabled)
        {
            m_angleWidget->SetWidgetStateToStart();
        }
    }

    double AngleMeasureTool::getAngle() const
    {
        if (!m_angleWidget)
        {
            return 0.0;
        }

        auto* representation = static_cast<vtkAngleRepresentation*>(
            m_angleWidget->GetRepresentation());

        if (representation)
        {
            return representation->GetAngle();
        }

        return 0.0;
    }

} // namespace isis::gui::measures

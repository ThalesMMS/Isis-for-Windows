/*
 * ------------------------------------------------------------------------------------
 *  File: vtkresliceplanecursorwidget.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements a VTK widget that displays intersecting plane cursors across slice views.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in
 *      all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 * ------------------------------------------------------------------------------------
 */

#include "vtkresliceplanecursorwidget.h"
#include <vtkCallbackCommand.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkWidgetCallbackMapper.h>
#include <vtkWidgetEvent.h>
#include <vtkWidgetEventTranslator.h>
#include <vtkWidgetRepresentation.h>
#include "utils.h"
#include "vtkreslicewidgetrepresentation.h"

vtkStandardNewMacro(isis::gui::vtkReslicePlaneCursorWidget);


class vtkReslicePlaneCursorCallback final : public vtkCommand
{
public:
	static vtkReslicePlaneCursorCallback* New()
	{
		return new vtkReslicePlaneCursorCallback;
	}

	void Execute(vtkObject*, const unsigned long eventId, void*) override
	{
		switch (eventId)
		{
		case StartInteractionEvent:
			m_resliceWidget->startWidgetInteraction(m_handleNumber);
			break;
		case InteractionEvent:
			m_resliceWidget->widgetInteraction(m_handleNumber);
			break;
		case EndInteractionEvent:
			m_resliceWidget->endWidgetInteraction(m_handleNumber);
			break;
		default:
			break;
		}
	}

	int m_handleNumber = {};
	isis::gui::vtkReslicePlaneCursorWidget* m_resliceWidget = {};
};


isis::gui::vtkReslicePlaneCursorWidget::vtkReslicePlaneCursorWidget()
{
	initializeWidget();
}


isis::gui::vtkReslicePlaneCursorWidget::~vtkReslicePlaneCursorWidget()
{
	RemoveAllObservers();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::initializeWidget()
{
	m_centerMovementWidget = vtkSmartPointer<vtkHandleWidget>::New();
	m_centerMovementWidget->SetParent(this);
	m_centerMovementCallback =
		vtkReslicePlaneCursorCallback::New();
	m_centerMovementCallback->m_handleNumber = 1;
	m_centerMovementCallback->m_resliceWidget = this;
	m_centerMovementWidget->AddObserver(
		vtkCommand::StartInteractionEvent,
		m_centerMovementCallback,
		Priority);
	m_centerMovementWidget->AddObserver(
		vtkCommand::InteractionEvent,
		m_centerMovementCallback,
		Priority);
	m_centerMovementWidget->AddObserver(
		vtkCommand::EndInteractionEvent,
		m_centerMovementCallback,
		Priority);
	m_centerMovementWidget->AddObserver(
		vtkCommand::Move3DEvent,
		m_centerMovementCallback,
		Priority);
	CallbackMapper->SetCallbackMethod(
		vtkCommand::LeftButtonPressEvent,
		vtkWidgetEvent::AddPoint, this,
		leftMouseDownAction);
	CallbackMapper->SetCallbackMethod(
		vtkCommand::MouseMoveEvent,
		vtkWidgetEvent::Move, this,
		moveMouse);
	CallbackMapper->SetCallbackMethod(
		vtkCommand::LeftButtonReleaseEvent,
		vtkWidgetEvent::EndSelect,
		this, leftMouseUpAction);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::SetEnabled(const int enabling)
{
	if (enabling) //----------------
	{
		vtkDebugMacro(<< "Enabling widget");
		if (Enabled)
		{
			return;
		}
		if (!Interactor)
		{
			vtkErrorMacro(<< "The interactor must be set prior to enabling the widget");
			return;
		}
		if (m_plane == -1)
		{
			return;
		}
		const int X = Interactor->GetEventPosition()[0];
		const int Y = Interactor->GetEventPosition()[1];
		if (!CurrentRenderer)
		{
			SetCurrentRenderer(Interactor->FindPokedRenderer(X, Y));

			if (!CurrentRenderer)
			{
				return;
			}
		}

		Enabled = 1;
		CreateDefaultRepresentation();
		WidgetRep->SetRenderer(CurrentRenderer);

		m_centerMovementWidget->SetRepresentation(
			reinterpret_cast<vtkResliceWidgetRepresentation*>(WidgetRep)->
			getCenterMovementRepresentation());
		reinterpret_cast<vtkResliceWidgetRepresentation*>(WidgetRep)->setPlane(m_plane);
		m_centerMovementWidget->SetInteractor(Interactor);
		m_centerMovementWidget->GetRepresentation()->SetRenderer(CurrentRenderer);
		m_centerMovementWidget->SetEnabled(1);
		m_centerMovementWidget->ManagesCursorOff();
		if (!Parent)
		{
			EventTranslator->AddEventsToInteractor(Interactor,
			                                       EventCallbackCommand, Priority);
		}
		else
		{
			EventTranslator->AddEventsToParent(Parent,
			                                   EventCallbackCommand, Priority);
		}
		if (ManagesCursor)
		{
			WidgetRep->ComputeInteractionState(X, Y);
			SetCursor(WidgetRep->GetInteractionState());
		}
		WidgetRep->BuildRepresentation();
		CurrentRenderer->AddViewProp(WidgetRep);
		InvokeEvent(vtkCommand::EnableEvent, nullptr);
	}
	else //disabling------------------
	{
		vtkDebugMacro(<< "Disabling widget");

		if (!Enabled)
		{
			return;
		}
		Enabled = 0;
		if (!Parent)
		{
			Interactor->RemoveObserver(EventCallbackCommand);
		}
		else
		{
			Parent->RemoveObserver(EventCallbackCommand);
		}
		CurrentRenderer->RemoveViewProp(WidgetRep);
		InvokeEvent(vtkCommand::DisableEvent, nullptr);
		SetCurrentRenderer(nullptr);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::CreateDefaultRepresentation()
{
	if (!WidgetRep)
	{
		WidgetRep = vtkResliceWidgetRepresentation::New();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::leftMouseDownAction(vtkAbstractWidget* w)
{
	vtkReslicePlaneCursorWidget* self =
		reinterpret_cast<vtkReslicePlaneCursorWidget*>(w);
	const int X = self->Interactor->GetEventPosition()[0];
	const int Y = self->Interactor->GetEventPosition()[1];
	auto* const rep =
		dynamic_cast<vtkResliceWidgetRepresentation*>(self->WidgetRep);
	if (!rep)
	{
		self->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
	}
	vtkNew<vtkCellPicker> picker;
	picker->SetTolerance(0.01);
	picker->InitializePickList();
	picker->PickFromListOn();
	const auto resliceActor =
		rep->getResliceActor();
	if (resliceActor && rep->GetVisibility())
	{
		picker->AddPickList(resliceActor->getActor());
	}
	if (picker->Pick(X, Y, 0, self->GetCurrentRenderer()))
	{
		if (self->m_state != translate)
		{
			if (picker->GetActor() == resliceActor->getActor())
			{
				self->m_state = rotate;
			}
		}
		self->InvokeEvent(qualityLow, &self->m_plane);
		self->GrabFocus(self->EventCallbackCommand);
		self->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
		self->EventCallbackCommand->SetAbortFlag(1);
	}
	else
	{
		self->GrabFocus(self->EventCallbackCommand);
		self->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
	}
	self->Render();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::moveMouse(vtkAbstractWidget* w)
{
	auto* const self = reinterpret_cast<vtkReslicePlaneCursorWidget*>(w);
	auto* const rep = dynamic_cast<vtkResliceWidgetRepresentation*>(self->WidgetRep);
	const auto x = self->Interactor->GetEventPosition()[0];
	const auto y = self->Interactor->GetEventPosition()[1];
	const auto lastX = self->Interactor->GetLastEventPosition()[0];
	const auto lastY = self->Interactor->GetLastEventPosition()[1];
	vtkNew<vtkCoordinate> actorCoordinates;
	switch (self->m_state)
	{
	case start:
		self->SetCursor(self->WidgetRep->ComputeInteractionState(x, y));
		if (!self->m_centerMovementWidget->GetEnabled())
		{
			rep->BuildRepresentation();
			self->m_centerMovementWidget->SetEnabled(1);
		}
		break;
	case rotate:
		{
			double* centerActor = rep->getResliceActor()->getActor()->GetPosition();
			actorCoordinates->SetCoordinateSystemToWorld();
			actorCoordinates->SetValue(centerActor[0], centerActor[1]);
			const auto* center = actorCoordinates->GetComputedDisplayValue(self->CurrentRenderer);
			const auto newAngle = atan2(y - center[1], x - center[0]);
			const auto oldAngle = atan2(lastY - center[1], lastX - center[0]);
			self->rotateCursor(newAngle - oldAngle);
			double angle[1] = {vtkMath::DegreesFromRadians(newAngle - oldAngle)};
			self->InvokeEvent(cursorRotate, angle);
		}
		break;
	default:
		break;
	}
	self->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
	self->Render();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::leftMouseUpAction(vtkAbstractWidget* w)
{
	vtkReslicePlaneCursorWidget* self = reinterpret_cast<vtkReslicePlaneCursorWidget*>(w);
	self->SetCursor(0);
	self->m_state = start;
	self->ReleaseFocus();
	self->InvokeEvent(qualityHigh, &self->m_plane);
	self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
	self->EventCallbackCommand->SetAbortFlag(1);
	self->Render();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::rotateCursor(double t_angle) const
{
	dynamic_cast<vtkResliceWidgetRepresentation*>(WidgetRep)->rotate(t_angle);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::setPlane(int t_plane)
{
	m_plane = t_plane;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::SetCursor(int t_state)
{
	if (!WidgetRep->GetVisibility())
	{
		return;
	}
	switch (t_state)
	{
	case vtkResliceWidgetRepresentation::outside:
		RequestCursorShape(VTK_CURSOR_DEFAULT);
		break;
	case vtkResliceWidgetRepresentation::mprCursor:
		RequestCursorShape(VTK_CURSOR_HAND);
		break;
	case vtkResliceWidgetRepresentation::handleCursor:
		RequestCursorShape(VTK_CURSOR_SIZEALL);
		break;
	default:
		RequestCursorShape(VTK_CURSOR_ARROW);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::setCursorPosition(double* t_position) const
{
	dynamic_cast<vtkResliceWidgetRepresentation*>
		(WidgetRep)->setCursorPosition(t_position);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::setCursorCenterPosition(const double* t_position)
{
	m_cursorCenterPosition[0] = t_position[0];
	m_cursorCenterPosition[1] = t_position[1];
	m_cursorCenterPosition[2] = t_position[2];
}

double* isis::gui::vtkReslicePlaneCursorWidget::getCursorCenterPosition()
{
	return m_cursorCenterPosition;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::startWidgetInteraction(int handleNum)
{
	Superclass::StartInteraction();
	m_state = translate;
	InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
	auto* const representation =
		dynamic_cast<vtkResliceWidgetRepresentation*>(WidgetRep);
	lastCursorPos[0] = representation->getResliceActor()->getActor()->GetPosition()[0];
	lastCursorPos[1] = representation->getResliceActor()->getActor()->GetPosition()[1];
	lastCursorPos[2] = representation->getResliceActor()->getActor()->GetPosition()[2];
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::widgetInteraction([[maybe_unused]] int handleNum)
{
	InvokeEvent(vtkCommand::InteractionEvent, nullptr);
	auto* const representation =
		dynamic_cast<vtkResliceWidgetRepresentation*>(WidgetRep);
	double pos[3] = {0.00, 0.00, 0.00};
	pos[0] = representation->getResliceActor()->getActor()->GetPosition()[0] - lastCursorPos[0];
	pos[1] = representation->getResliceActor()->getActor()->GetPosition()[1] - lastCursorPos[1];
	pos[2] = representation->getResliceActor()->getActor()->GetPosition()[2] - lastCursorPos[2];
	InvokeEvent(cursorMove, pos);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkReslicePlaneCursorWidget::endWidgetInteraction([[maybe_unused]] int handleNum)
{
	Superclass::EndInteraction();
	InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
	InvokeEvent(cursorFinishMovement, &m_plane);
}


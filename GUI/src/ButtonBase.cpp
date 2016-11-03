#include "stdafx.h"
#include "ButtonBase.hpp"
#include "GUI.hpp"

ButtonBase::~ButtonBase()
{
	if(m_slot)
	{
		delete m_slot;
	}
}
void ButtonBase::OnMouseMove(MouseMovedEvent& event)
{
	m_UpdateHoverState(event.newPosition);
}
void ButtonBase::OnMouseButton(MouseButtonEvent& event)
{
	if(event.button == Graphics::MouseButton::Left)
	{
		if(event.state)
		{
			if(m_focusHandle && !m_focusHandle->keyboard)
			{
				m_held = m_gui->AcquireMouseDown(this);
				if(m_held)
				{
					Logf("Pressed", Logger::Info);
					m_OnPressed(false);
					OnPressed.Call();
				}
			}
		}
		else if(m_held)
		{
			Logf("Released", Logger::Info);
			m_OnReleased();
			m_held.Release();
		}
	}
}
void ButtonBase::OnFocusLost()
{
	Logf("Mouse Leave", Logger::Info);
	m_hovered = false;
}
void ButtonBase::OnFocus()
{
	Logf("Mouse Enter", Logger::Info);
	m_hovered = true;
}
void ButtonBase::OnConfirm()
{
	Logf("Pressed", Logger::Info);
	m_OnPressed(true);
	OnPressed.Call();
}
void ButtonBase::Update(GUIUpdateData data)
{
	// Update transformations
	GUIElementBase::m_UpdateTransform(data);

	if(m_slot)
	{
		m_slot->Update(data);
	}
}
void ButtonBase::Render(GUIRenderData data)
{
	if(m_slot)
	{
		m_slot->element->Render(data);
	}
}
void ButtonBase::InvalidateArea()
{
	ContainerBase::InvalidateArea();
	m_inverseObjectTransform.Invalidate();
}
bool ButtonBase::IsHovered() const
{
	return m_hovered;
}
GUIElementBase* ButtonBase::SelectNext(GUIElementBase* from, GUIElementBase* item, int dir, int layoutDirection)
{
	// Use parent navigation
	return GetParent()->SelectNext(from, this, dir, layoutDirection);
}
void ButtonBase::Focus()
{
	// Acquire keyboard focus
	m_focusHandle = m_gui->AcquireFocus(this, true);
}
GUISlotBase* ButtonBase::SetContent(GUIElement content)
{
	if(m_slot)
		delete m_slot;
	return m_slot = CreateSlot<GUISlotBase>(content);
}
GUISlotBase* ButtonBase::GetContent()
{
	return m_slot;
}
Vector2 ButtonBase::m_GetDesiredBaseSize(GUIUpdateData data)
{
	if(!m_slot)
		return Vector2();
	return m_slot->GetDesiredSize(data);
}
void ButtonBase::m_PropagateEventToChildren(GUIEvent& event)
{
	if(m_slot)
	{
		event.Propagate(*m_slot->element);
	}
}
void ButtonBase::m_InvalidateSlotAreas()
{
	if(m_slot)
	{
		m_slot->InvalidateArea();
	}
}
void ButtonBase::m_UpdateHoverState(Vector2 mousePosition)
{
	if(!m_focusHandle)
	{
		if(m_HitTest(mousePosition))
		{
			assert(m_gui);
			m_focusHandle = m_gui->AcquireFocus(this, false);
		}
	}
	else
	{
		if(!m_HitTest(mousePosition))
		{
			// Release handle if not a keyboard handle
			if(!m_focusHandle->keyboard)
				m_focusHandle.Release();
		}
	}
}
void ButtonBase::m_PostAnimationUpdate()
{
	ContainerBase::m_PostAnimationUpdate();
	//m_UpdateHoverState(m_gui->GetMousePos());
}
void ButtonBase::m_UpdateTransform(Rect area, Transform2D parentTransform)
{
	GUIElementBase::m_UpdateTransform(area, parentTransform);
	if(!m_inverseObjectTransform.IsValid())
	{
		m_inverseObjectTransform = m_objectTransform.Inverted();
	}
}
bool ButtonBase::m_HitTest(const Vector2& point)
{
	// From world to local
	Vector2 localPosition = Transform2D(m_inverseObjectTransform).TransformPoint(point);
	static Rect unitRectangle = Rect(Vector2(), Vector2(1.0f));
	return unitRectangle.ContainsPoint(localPosition);
}

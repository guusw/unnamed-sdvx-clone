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
			if(m_hoveredHandle)
			{
				m_held = m_gui->AcquireMouseDown(this);
				if(m_held)
				{
					Logf("Pressed", Logger::Info);
					m_OnPressed();
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
void ButtonBase::Update(GUIUpdateData data)
{
	// Update transformations
	m_UpdateTransform(data);

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
bool ButtonBase::IsHovered() const
{
	return m_hovered;
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
	if(!m_hoveredHandle)
	{
		if(m_HitTest(mousePosition))
		{
			assert(m_gui);
			m_hoveredHandle = m_gui->AcquireMouseFocus(this);
		}
	}
	else
	{
		if(!m_HitTest(mousePosition))
		{
			m_hoveredHandle.Release();
		}
	}
}
void ButtonBase::m_PostAnimationUpdate()
{
	ContainerBase::m_PostAnimationUpdate();
	m_UpdateHoverState(m_gui->GetMousePos());
}
bool ButtonBase::m_HitTest(const Vector2& point)
{
	// From world to local
	Vector2 localPosition = Transform2D(m_cachedInverseObjectTransform).TransformPoint(point);
	static Rect unitRectangle = Rect(Vector2(), Vector2(1.0f));
	return unitRectangle.ContainsPoint(localPosition);
}

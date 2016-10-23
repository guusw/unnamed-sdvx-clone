#include "stdafx.h"
#include "ContainerBase.hpp"

void ContainerBase::OnMouseMove(MouseMovedEvent& event)
{
	m_PropagateEventToChildren(event);
}
void ContainerBase::OnMouseButton(MouseButtonEvent& event)
{
	m_PropagateEventToChildren(event);
}
void ContainerBase::OnInvalidate(InvalidationEvent& event)
{
	GUIElementBase::OnInvalidate(event);
	if(event.invalidateArea)
	{
		m_InvalidateSlotAreas();
	}
	m_PropagateEventToChildren(event);
}

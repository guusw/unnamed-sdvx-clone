#include "stdafx.h"
#include "GUIEvents.hpp"
#include "GUIElement.hpp"

void GUIEvent::Propagate(class GUIElementBase& child)
{
	(child.*m_handlerFunction)(*this);
}

MouseMovedEvent::MouseMovedEvent()
{
	m_SetEventHandlerFunction(&GUIElementBase::OnMouseMove);
}
MouseButtonEvent::MouseButtonEvent()
{
	m_SetEventHandlerFunction(&GUIElementBase::OnMouseButton);
}
InvalidationEvent::InvalidationEvent()
{
	m_SetEventHandlerFunction(&GUIElementBase::OnInvalidate);
}
AssignGUIEvent::AssignGUIEvent()
{
	m_SetEventHandlerFunction(&GUIElementBase::OnAssignGUI);
}

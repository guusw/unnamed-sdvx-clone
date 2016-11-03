#pragma once
#include <Graphics/Keys.hpp>

/*
	Base class for GUI events
*/
class GUIElementBase;
struct GUIEvent
{
	typedef void (GUIElementBase::*HandlerFunction)(GUIEvent& event);
	void Propagate(GUIElementBase& child);

protected:
	template<typename TEventStruct>
	void m_SetEventHandlerFunction(void (GUIElementBase::*handlerFunction)(TEventStruct& event))
	{
		struct This
		{
			void (GUIElementBase::*handlerFunction)(TEventStruct& event);
		};
		This* thisPtr = (This*)this;
		thisPtr->handlerFunction = handlerFunction;
	}
private:
	void (GUIElementBase::*m_handlerFunction)(GUIEvent& event);
};

struct MouseMovedEvent : public GUIEvent
{
	MouseMovedEvent();
	Vector2 delta;
	Vector2 newPosition;
};

struct MouseButtonEvent : public GUIEvent
{
	MouseButtonEvent();
	Graphics::MouseButton button;
	// State
	// false = up
	// true = pressed
	bool state;
	Vector2 mousePosition;
};

struct InvalidationEvent : public GUIEvent
{
	InvalidationEvent();
	bool invalidateArea = true;
};

struct AssignGUIEvent : public GUIEvent
{
	AssignGUIEvent();
	class GUI* gui;
};

// Single element key event
struct KeyEvent
{
	Graphics::Key key;
	bool keyState;
	bool repeated;
};
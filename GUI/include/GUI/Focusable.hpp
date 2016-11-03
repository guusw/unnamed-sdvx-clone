#pragma once
#include <Shared/Ref.hpp>
#include <GUI/GUIEvents.hpp>

class Focusable;
class GUIElementBase;

struct FocusHandleData
{
	// Is keyboard focus
	bool keyboard;
	GUIElementBase* element;
	Focusable* focusable;
};

typedef Ref<FocusHandleData> FocusHandle;

class Focusable
{
public:
	virtual ~Focusable() = default;

	// Called when a key is pressed/released/repeated while this element has input focus
	virtual void OnKeyEvent(KeyEvent& event) {};

	// Called when focus acquired with AcquireFocus was lost
	virtual void OnFocusLost() {};

	// Called when focused using AcquireFocus
	virtual void OnFocus() {};

	// Called when input focus is lost
	virtual void OnInputFocusLost() {};

	// Called when focused using AcquireInputFocus
	virtual void OnInputFocus() {};

	// Called when confirm button was pressed on this element while in focus
	virtual void OnConfirm() {};

	// Called when back button was pressed on this element while in focus
	virtual void OnCancel() {};

	// Can this element be engaged by pressing confirm on it
	virtual bool CanEngage() const { return false; }

	// Called when this element should acquire input focus using AcquireInputFocus
	virtual void Engage() {};

	// Called when this element should be focused using AcquireFocus
	virtual void Focus() = 0;
};

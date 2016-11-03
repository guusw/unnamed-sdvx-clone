#pragma once
#include <GUI/GUISlotBase.hpp>

class ContainerBase : public GUIElementBase
{
public:
	// Event handlers
	// these route the events to children by default
	virtual void OnMouseMove(MouseMovedEvent& event) override;
	virtual void OnMouseButton(MouseButtonEvent& event) override;
	virtual void OnInvalidate(InvalidationEvent& event) override;
	virtual void OnAssignGUI(AssignGUIEvent& event) override;

	// Select the next element vertically relative from the given element
	// from = the elment that was selected
	// item = the item in the current container that owns the previous selection
	// dir = direction to navigate (-1,1) (Left,Right) (Up,Down)
	// layoutDirection = direction of the navigation 0 = Horizontal, 1 = Vertical, 2 = Any
	virtual GUIElementBase* SelectNext(GUIElementBase* from, GUIElementBase* item, int dir, int layoutDirection) = 0;

protected:
	virtual ~ContainerBase() = default;

	// Template slot creation helper
	template<typename T> T* CreateSlot(Ref<GUIElementBase> element);

	// Override this with a function that calls event.Propagate() on all children
	virtual void m_PropagateEventToChildren(GUIEvent& event) = 0;
	// Invalidate the placement of all the slots in this container
	virtual void m_InvalidateSlotAreas() = 0;

	// Child notification events
	// Called when the child area should be recomputed
	virtual void m_OnChildSlotChanged(GUISlotBase* slot) {};
	// Called when the child ordering changed
	virtual void m_OnChildZOrderChanged(GUISlotBase* slot) {};

	friend class GUISlotBase;
};


// Slot creation helper
template<typename T> T* ContainerBase::CreateSlot(Ref<GUIElementBase> element)
{
	static_assert(std::is_base_of<GUISlotBase, T>::value, "Class does not inherit from GUISlotBase");

	assert(element->m_slot == nullptr);
	T* newSlot = new T();
	newSlot->parent = this;
	newSlot->element = element;
	m_AttachChild(*element, newSlot);
	return newSlot;
}
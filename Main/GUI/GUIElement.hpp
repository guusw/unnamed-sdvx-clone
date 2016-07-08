#pragma once

#include "GUIRenderData.hpp"
#include "Anchor.hpp"

/*
Base class for GUI elements
*/
class GUIElementBase : public Unique, public RefCounted<GUIElementBase>
{
public:
	GUIElementBase() = default;
	virtual ~GUIElementBase() = default;
	// Called to draw the GUI element and it's children
	virtual void Render(GUIRenderData rd);
	// Calculates the desired size of this element, or false if it does not
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut);

	// The slot that contains this element
	class GUISlotBase* slot = nullptr;

protected:
	// Template slot creation helper
	template<typename T> T* CreateSlot(Ref<GUIElementBase> element);
	// Called when added to slot
	virtual void m_AddedToSlot(GUISlotBase* slot);
	// Called when the ZOrder of a child slot changed
	virtual void m_OnZOrderChanged(GUISlotBase* slot);

	friend class GUISlotBase;
};

// Typedef pointer to GUI element objects
typedef Ref<GUIElementBase> GUIElement;

// Fill mode for gui elements
enum class FillMode
{
	// Stretches the element, may result in incorrect image ratio
	Stretch,
	// Fills the entire space with the content, may crop the element
	Fill,
	// Take the smallest size to fit the element, leaves black bars if it doesn't fit completely
	Fit,
};

/*
Base class used for slots that can contain child elements inside an existing element.
The slot class contains specific properties of how to layout the child element in its parent
*/
class GUISlotBase : public Unique
{
public:
	virtual ~GUISlotBase() = default;
	virtual void Render(GUIRenderData rd);
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut);
	// Applies filling logic based on the selected fill mode
	static Rect ApplyFill(FillMode fillMode, const Vector2& inSize, const Rect& rect);
	// Applies alignment to an input rectangle
	static Rect ApplyAlignment(const Vector2& alignment, const Rect& rect, const Rect& parent);

	// Element that contains this slot
	GUIElementBase* parent = nullptr;
	// Padding that is applied to the element after it is fully placed
	Margin padding;
	// The element contained in this slot
	GUIElement element;

	void SetZOrder(int32 zorder);
	int32 GetZOrder() const;

private:
	// Depth sorting for this slot, if applicable
	int32 m_zorder = 0;
};

// Slot creation helper
template<typename T> T* GUIElementBase::CreateSlot(Ref<GUIElementBase> element)
{
	static_assert(std::is_base_of<GUISlotBase, T>::value, "Class does not inherit from GUISlotBase");

	assert(element->slot == nullptr);
	T* newSlot = new T();
	newSlot->parent = this;
	newSlot->element = element;
	element->slot = newSlot;
	return newSlot;
}
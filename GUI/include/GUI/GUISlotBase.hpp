#pragma once
#include "GUI/GUIElement.hpp"

// Fill mode for gui elements
enum class FillMode
{
	// Stretches the element, may result in incorrect image ratio
	Stretch,
	// No filling, just keep original image size
	None,
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
	virtual ~GUISlotBase();

	// Evaluate positioning, etc.
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
	// Fill RenderQueue with render commands and process input
	virtual void Render(GUIRenderData rd);

	// Aquire desired element size
	virtual Vector2 GetDesiredSize(GUIRenderData rd);

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
	// Allow overflow of content outside of this slot
	bool allowOverflow = false;

	void SetZOrder(int32 zorder);
	int32 GetZOrder() const;

protected:
	// Cached placement of item
	Rect m_cachedArea;

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
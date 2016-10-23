#pragma once
#include <GUI/GUIElement.hpp>
#include <Shared/Invalidation.hpp>

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
class ContainerBase;
class GUISlotBase : public Unique
{
public:
	// Constant for a vector of {-1,-1} to indicate the value should not be used
	static const Vector2 MinusOne;

public:
	GUISlotBase();
	virtual ~GUISlotBase();

	void InvalidateArea();

	virtual void Update(GUIUpdateData data);

	// Aquire desired element size
	virtual Vector2 GetDesiredSize(GUIUpdateData data);

	// Applies filling logic based on the selected fill mode
	static Rect ApplyFill(FillMode fillMode, const Vector2& inSize, const Rect& rect);
	// Applies alignment to an input rectangle
	static Rect ApplyAlignment(const Vector2& alignment, const Rect& rect, const Rect& parent);

	// Padding that is applied to the element after it is fully placed
	NotifyDirty<Margin> padding;

	// Allow overflow of content outside of this slot
	NotifyDirty<bool> allowOverflow = false;

	// Alignment of the element in the parent.
	//	a value of (0,0) places the widget starting from the top-left corner extending to bottom-right
	//	a value of(1,0) places the widget starting from the top-right corner extending to bottom-left
	NotifyDirty<Vector2> alignment;

	// Fill horizontally to parent
	NotifyDirty<bool> fillX = false;

	// Fill vertically to parent
	NotifyDirty<bool> fillY = false;

	// Z-Order of the element
	NotifyDirty<int> zOrder = 0;

	// Element that contains this slot
	// only GUIContainer elements may contain child elements
	ContainerBase* parent = nullptr;

	// The element contained in this slot
	GUIElement element;
protected:
	void m_UpdateArea(Rect area);

	Cached<Rect> m_cachedArea;
};
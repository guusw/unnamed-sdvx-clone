#pragma once
#include "GUIRenderData.hpp"
#include "Anchor.hpp"
#include "GUIAnimation.hpp"

// GUI Element visiblity
enum class Visibility
{
	Visible = 0,
	Hidden, // No visible
	Collapsed, // No space used
};

/*
	Base class for GUI elements
*/
class GUIElementBase : public Unique, public RefCounted<GUIElementBase>
{
public:
	GUIElementBase() = default;
	virtual ~GUIElementBase();
	// Called to place element and handle input focus
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
	// Called to draw the GUI element and it's children
	virtual void Render(GUIRenderData rd) = 0;
	// Calculates the desired size of this element, or false if it does not
	virtual Vector2 GetDesiredSize(GUIRenderData rd);
	// Add an animation related to this element
	virtual bool AddAnimation(Ref<IGUIAnimation> anim, bool removeOld = false);
	Ref<IGUIAnimation> GetAnimation(void* target);
	Ref<IGUIAnimation> GetAnimation(uint32 uid);

	// If this element should receive keyboard input events or not
	virtual bool HasInputFocus() const;

	static bool OverlapTest(Rect rect, Vector2 point);

	// The slot that contains this element
	class GUISlotBase* slot = nullptr;

	// The visiblity of this GUI element
	Visibility visibility = Visibility::Visible;

protected:
	// Template slot creation helper
	template<typename T> T* CreateSlot(Ref<GUIElementBase> element);
	// Handle removal logic
	void m_OnRemovedFromParent();
	// Called when added to slot
	virtual void m_AddedToSlot(GUISlotBase* slot);
	// Called when the ZOrder of a child slot changed
	virtual void m_OnZOrderChanged(GUISlotBase* slot);
	void m_TickAnimations(float deltaTime);

	// Animation mapped to target
	Map<void*, Ref<IGUIAnimation>> m_animationMap;

	// Set if we got input focus from a renderer
	// this should then be cleared when this element is destroyed
	GUIRenderer* m_rendererFocus = nullptr;

	friend class GUISlotBase;
	friend class GUIRenderer;
};

// Typedef pointer to GUI element objects
typedef Ref<GUIElementBase> GUIElement;

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
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
	virtual void Render(GUIRenderData rd);
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
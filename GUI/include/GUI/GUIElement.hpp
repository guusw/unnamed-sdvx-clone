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

	const Transform2D& GetRenderTransform() const;
	void SetRenderTransform(const Transform2D& transform);

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

	// Local render transform
	Transform2D m_renderTransform;

	friend class GUISlotBase;
	friend class GUIRenderer;
};

// Typedef pointer to GUI element objects
typedef Ref<GUIElementBase> GUIElement;
#pragma once
#include <GUI/GUIRenderData.hpp>
#include <GUI/GUIUpdateData.hpp>
#include <GUI/GUIAnimation.hpp>
#include <GUI/GUIEvents.hpp>
#include <Shared/Invalidation.hpp>
#include <Shared/UnorderedMap.hpp>

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
class ContainerBase;
class GUIElementBase : public Unique, public RefCounted<GUIElementBase>
{
public:
	GUIElementBase();
	virtual ~GUIElementBase();

	// Invalidates the cached render position and transforms
	virtual void InvalidateArea();

	// Event handlers
	virtual void OnMouseMove(MouseMovedEvent& event) {};
	virtual void OnMouseButton(MouseButtonEvent& event) {};
	virtual void OnInvalidate(InvalidationEvent& event);
	virtual void OnAssignGUI(AssignGUIEvent& event);

	// Updates animations
	virtual void UpdateAnimations(float deltaTime);
	// Update function, processes mouse focus
	virtual void Update(GUIUpdateData data);
	// Called to draw the GUI element and it's children
	virtual void Render(GUIRenderData data) = 0;
	// Desired size, or zero if collapsed
	Vector2 GetDesiredSize(GUIUpdateData data);
	// Returns the cached area of this element
	Rect GetCachedArea() const;

	// Add an animation related to this element
	Ref<GUIAnimation> AddAnimation(Action<void, float> anim, float duration, Interpolation::TimeFunction timeFunction = Interpolation::Linear);
	void RemoveAnimation(Action<void, float> anim);
	void RemoveAnimation(Ref<GUIAnimation> anim);
	Ref<GUIAnimation> FindAnimation(Action<void, float> updater);

	// Retrieve the slot this element is placed in
	class GUISlotBase* GetSlot();
	template<typename T>
	T* GetSlot()
	{
		return dynamic_cast<T*>(GetSlot());
	}

	// Retrieve the parent element
	ContainerBase* GetParent();

	// The visiblity of this GUI element
	Visibility visibility = Visibility::Visible;

	// Local render transform
	NotifyDirty<Transform2D> renderTransform;
protected:
	// Calculates the desired size of this element
	virtual Vector2 m_GetDesiredBaseSize(GUIUpdateData data);

	void m_AttachChild(GUIElementBase& element, GUISlotBase* slot);

	// Updates the cached areas and transforms
	virtual void m_UpdateTransform(Rect area, Transform2D parentTransform);
	// Updates the cached areas and transforms and stores the new renderTransform in the renderdata
	void m_UpdateTransform(GUIUpdateData& data);
	virtual void m_PostAnimationUpdate() {};

	// Handle removal logic
	void m_OnRemovedFromParent();
	// Called when added to slot
	virtual void m_AddedToSlot(GUISlotBase* slot);

	// Cached Render Transform including all parent transforms
	Transform2D m_renderTransform;
	// Combined render transform and area rectangle to transform a (0,0,1,1) rectangle to the target position
	Transform2D m_objectTransform;
	// The cached area of this gui element
	Rect m_area;
	// Handle for above states
	CachedState m_transformValid;

	// Animation mapped to target
	Set<Ref<GUIAnimation> > m_animations;
	UnorderedMap<GUIAnimationUpdater, Ref<GUIAnimation>> m_animationsByUpdater;

	// Handle to the GUI System
	class GUI* m_gui;

	friend class GUISlotBase;
	friend class ContainerBase;
	friend class GUI;

private:
	// The slot that contains this element
	class GUISlotBase* m_slot = nullptr;
};

// Typedef pointer to GUI element objects
typedef Ref<GUIElementBase> GUIElement;
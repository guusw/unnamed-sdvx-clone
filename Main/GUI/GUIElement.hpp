#pragma once

#include "GUIRenderData.hpp"
#include "Anchor.hpp"

// GUI Animation object
class IGUIAnimation : public RefCounted<IGUIAnimation>
{
public:
	virtual ~IGUIAnimation() = default;
	// Return false when done
	virtual bool Update(float deltaTime) = 0;
	// Target value of the animation
	virtual void* GetTarget() = 0;
};

// A templated animation of a single vector/float/color variable
template<typename T>
class GUIAnimation : public IGUIAnimation
{
public:
	// A->B animation with A set to the current value
	GUIAnimation(T* target, T newValue, float duration)
	{
		assert(target);
		m_target = target;
		m_duration = duration;
		m_last = m_target[0];
		m_next = newValue;
	}
	// A->B animation with A and B provided
	GUIAnimation(T* target, T newValue, T lastValue, float duration)
	{
		assert(target);
		m_target = target; 
		m_duration = duration;
		m_last = lastValue;
		m_next = newValue;
	}
	virtual bool Update(float deltaTime) override
	{
		if(m_time >= m_duration)
			return false;

		m_time += deltaTime;
		float r = m_time / m_duration;
		if(m_time >= m_duration)
		{
			r = 1.0f;
			m_time = m_duration;
		}

		T current = (m_next - m_last)*r + m_last;
		m_target[0] = current;

		return r < 1.0f;
	}
	// Target of the animation
	virtual void* GetTarget() override
	{
		return m_target;
	}
private:
	T m_last;
	T m_next;
	T* m_target;
	float m_time = 0.0f;
	float m_duration;
};

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
	virtual ~GUIElementBase() = default;
	// Called to draw the GUI element and it's children
	virtual void Render(GUIRenderData rd);
	// Calculates the desired size of this element, or false if it does not
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut);
	// Add an animation related to this element
	virtual bool AddAnimation(Ref<IGUIAnimation> anim, bool removeOld = false);

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

	friend class GUISlotBase;
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
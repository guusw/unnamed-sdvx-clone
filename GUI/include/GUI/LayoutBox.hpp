#pragma once
#include <GUI/ContainerBase.hpp>

/*
Container that arranges elements in a vertical/horizontal list
*/
class LayoutBox : public ContainerBase
{
public:
	~LayoutBox();

	virtual void UpdateAnimations(float deltaTime) override;
	virtual void Update(GUIUpdateData data) override;
	virtual void Render(GUIRenderData data) override;

	// Calculates child element sizes based on the current settings
	Vector<float> CalculateSizes(const GUIUpdateData& data) const;

	enum LayoutDirection
	{
		Horizontal = 0,
		Vertical
	};

	// The layout direction of the box
	LayoutDirection layoutDirection = LayoutDirection::Horizontal;

	class Slot : public GUISlotBase
	{
	public:
		Slot();
		// The amount of space to fill
		//	0.5 would take up half of what it would normally
		//	this only works if this slot is set to fill in the container's direction
		NotifyDirty<float> fillAmount = 1.0f;

		virtual void Update(GUIUpdateData data) override;
		void Render(GUIRenderData rd);
	};

	Slot* Add(GUIElement element);
	void Remove(GUIElement element);
	void Clear();

	const Vector<LayoutBox::Slot*>& GetChildren();

	GUIElementBase* SelectNext(GUIElementBase* from, GUIElementBase* item, int dir, int layoutDirection) override;

protected:
	void m_PropagateEventToChildren(GUIEvent& event) override;
	void m_InvalidateSlotAreas() override;
	void m_OnChildSlotChanged(GUISlotBase* slot) override;
	virtual Vector2 m_GetDesiredBaseSize(GUIUpdateData data) override;

private:

	// Used to prevent m_OnChildSlotChanged to keep being called while calling invalidateArea on children
	bool m_alreadyInvalidatingChildren = false;
	Vector<Slot*> m_children;
	CachedState m_composition;
};
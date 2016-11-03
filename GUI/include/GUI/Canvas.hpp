#pragma once
#include <GUI/ContainerBase.hpp>
#include <GUI/Anchor.hpp>
#include <Shared/Color.hpp>

/*
Canvas that sorts elements by depth and anchors them to the screen
*/
class Canvas : public ContainerBase
{
public:
	~Canvas();

	virtual void UpdateAnimations(float deltaTime) override;
	virtual void Update(GUIUpdateData data) override;
	virtual void Render(GUIRenderData data) override;

	class Slot : public GUISlotBase
	{
	public:
		Slot();

		virtual void Update(GUIUpdateData data) override;

		// Anchor for the element
		NotifyDirty<Anchor> anchor = Anchors::TopLeft;

		// Offset from anchored position
		NotifyDirty<Rect> offset;
	};

	class Canvas::Slot* Add(GUIElement element);
	void Remove(GUIElement element);
	void Clear();
	const Vector<Canvas::Slot*>& GetChildren();

	GUIElementBase* SelectNext(GUIElementBase* from, GUIElementBase* item, int dir, int layoutDirection) override;

protected:
	void m_OnChildSlotChanged(GUISlotBase* slot) override;
	void m_OnChildZOrderChanged(GUISlotBase* slot) override;
	void m_PropagateEventToChildren(GUIEvent& event) override;
	void m_InvalidateSlotAreas() override;
	virtual Vector2 m_GetDesiredBaseSize(GUIUpdateData data) override;

private:
	void m_SortChildren();

private:
	CachedState m_sortingOrder;
	Vector<Slot*> m_children;
};
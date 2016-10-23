#pragma once
#include <GUI/ContainerBase.hpp>
#include <Shared/Handle.hpp>

/*
	Non-Visual button, just the event handling
*/
class ButtonBase : public ContainerBase
{
public:
	~ButtonBase();
	virtual void OnMouseMove(MouseMovedEvent& event) override;
	virtual void OnMouseButton(MouseButtonEvent& event) override;
	virtual void OnFocusLost() override;
	virtual void OnFocus() override;
	virtual void Update(GUIUpdateData data) override;
	virtual void Render(GUIRenderData data) override;

	void InvalidateArea() override;
	bool IsHovered() const;
	bool IsHeld() const;

	GUISlotBase* SetContent(GUIElement content);
	GUISlotBase* GetContent();

	Delegate<> OnPressed;

protected:
	Vector2 m_GetDesiredBaseSize(GUIUpdateData data) override;
	void m_PropagateEventToChildren(GUIEvent& event) override;
	void m_InvalidateSlotAreas() override;
	void m_UpdateHoverState(Vector2 mousePosition);
	void m_PostAnimationUpdate() override;

	// Overriden to calculate inverse transforms for buttons
	void m_UpdateTransform(Rect area, Transform2D parentTransform) override;

	// Button hit test
	virtual bool m_HitTest(const Vector2& point);
	virtual void m_OnPressed() {};
	virtual void m_OnReleased() {};
	bool m_hovered = false;

private:
	GUISlotBase* m_slot = nullptr;
	Handle m_hoveredHandle;
	Handle m_held;

	Cached<Transform2D> m_inverseObjectTransform;
};
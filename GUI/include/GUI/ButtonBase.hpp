#pragma once
#include <GUI/ContainerBase.hpp>
#include "Focusable.hpp"

/*
	Non-Visual button, just the event handling
*/
class ButtonBase : public ContainerBase, public Focusable
{
public:
	~ButtonBase();
	virtual void OnMouseMove(MouseMovedEvent& event) override;
	virtual void OnMouseButton(MouseButtonEvent& event) override;
	virtual void OnFocusLost() override;
	virtual void OnFocus() override;
	virtual void OnConfirm() override;
	virtual void Update(GUIUpdateData data) override;
	virtual void Render(GUIRenderData data) override;

	void InvalidateArea() override;
	bool IsHovered() const;
	bool IsHeld() const;

	GUIElementBase* SelectNext(GUIElementBase* from, GUIElementBase* item, int dir, int layoutDirection) override;
	void Focus() override;

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
	// Occurs when pressed, when isKeyboard is true, a released event is never sent
	virtual void m_OnPressed(bool isKeyboard) {};
	virtual void m_OnReleased() {};
	bool m_hovered = false;

private:
	GUISlotBase* m_slot = nullptr;
	FocusHandle m_focusHandle;
	FocusHandle m_held;

	Cached<Transform2D> m_inverseObjectTransform;
};
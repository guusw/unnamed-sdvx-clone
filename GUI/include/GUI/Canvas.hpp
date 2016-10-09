#pragma once
#include "GUIElement.hpp"

/*
Canvas that sorts elements by depth and anchors them to the screen
*/
class Canvas : public GUIElementBase
{
public:
	~Canvas();
	void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	class Slot : public GUISlotBase
	{
	public:
		virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
		virtual void Render(GUIRenderData rd) override;

		// Anchor for the element
		Anchor anchor = Anchors::TopLeft;

		// Offset from anchored position
		Rect offset;

		// Fixed Size override, set to (0,0) to ignore
		Vector2 size;

		// if set to true, the size of the child element will be used instead
		bool autoSizeX = false;
		bool autoSizeY = false;

		// Alignment of the element in the parent.
		//	a value of (0,0) places the widget starting from the top-left corner extending to bottom-right
		//	a value of(1,0) places the widget starting from the top-right corner extending to bottom-left
		Vector2 alignment;
	};

	class Canvas::Slot* Add(GUIElement element);
	void Remove(GUIElement element);
	void Clear();
	const Vector<Canvas::Slot*>& GetChildren();

protected:
	virtual void m_OnZOrderChanged(GUISlotBase* slot);

private:
	void m_SortChildren();

	Vector<Slot*> m_children;
};
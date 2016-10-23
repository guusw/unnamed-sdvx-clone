#pragma once
#include <GUI/GUISlotBase.hpp>
#include <GUI/CommonGUIStyle.hpp>
#include "Canvas.hpp"

class ScrollBox : public GUIElementBase
{
public:
	ScrollBox(Ref<CommonGUIStyle> style);
	~ScrollBox();
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData data) override;

	// Calculates the image size if set
	Vector2 m_GetDesiredBaseSize(GUIRenderData rd) override;

	class Slot : public GUISlotBase
	{
	public:
		virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
		virtual void Render(GUIRenderData rd) override;

		void SetScrollPercent(float percent);
		void SetScroll(int32 pixels);
		int32 ConvertFromPercent(float percent);
		int32 GetScroll() const { return m_scroll; }
		float GetScrollPercent() const;

	private:
		int32 m_scroll = 0;
	};

	// Sets panel content
	Slot* SetContent(GUIElement content);
	Slot* GetContentSlot();

private:
	void m_AnimateScrollDelta(int32 offset);

	bool m_hovered = false;
	// Current scroll animation target
	int32 m_scrollTarget = 0;

	float m_scrollSpeed = 0.0f;
	float m_scrollOffset = 0.0f;

	void m_OnSetScroll(float val);
	class Slider* m_vscroll;

	Rect m_cachedContentClipRect;
	Rect m_cachedContentRect;
	Rect m_cachedSliderRect;

	Ref<CommonGUIStyle> m_style;
	Slot* m_content = nullptr;

	friend class Slot;
};

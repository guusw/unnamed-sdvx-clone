#include "stdafx.h"
#include <Shared/Thread.hpp>
#include <GUI/ButtonBase.hpp>
#include <GUI/LayoutBox.hpp>
#include "GraphicsBase.hpp"
#include <GUI/Label.hpp>

class TestButton : public ButtonBase
{
public:
	virtual void Render(GUIRenderData data) override
	{
		ButtonBase::Render(data);

		Color c = m_hovered ? Color::Green : Color::White;
		data.guiRenderer->RenderWireBox(m_objectTransform, c);
	}

	void Anim_Hover(float time)
	{
		Transform2D zoom;
		zoom *= Transform2D::Translation(Vector2(0.5f) * m_area.size);
		zoom *= Transform2D::Scale(Vector2(1.0f + time * 0.05f));
		zoom *= Transform2D::Translation(Vector2(-0.5f) * m_area.size);
		m_hoverTransform = zoom; 
		m_animationUpdated.Invalidate();
	}
	void Anim_Click(float time)
	{
		static Interpolation::CubicBezier bounceLerp(.38, 1.55, .84, 1.43);
		float bounce = bounceLerp.Sample(time);

		Transform2D zoom;
		zoom *= Transform2D::Translation(Vector2(0.5f) * m_area.size);
		zoom *= Transform2D::Scale(Vector2(1.0f + bounce * 0.05f));
		zoom *= Transform2D::Translation(Vector2(-0.5f) * m_area.size);
		m_clickTransform = zoom;
		m_animationUpdated.Invalidate();
	}
	void Anim_ClickSingle(float time)
	{
		time = time * 2;
		if(time > 1)
			time = 1 - (time - 1);

		static Interpolation::CubicBezier bounceLerp(.38, 1.55, .84, 1.43);
		float bounce = bounceLerp.Sample(time);

		Transform2D zoom;
		zoom *= Transform2D::Translation(Vector2(0.5f) * m_area.size);
		zoom *= Transform2D::Scale(Vector2(1.0f + bounce * 0.05f));
		zoom *= Transform2D::Translation(Vector2(-0.5f) * m_area.size);
		m_clickTransform = zoom;
		m_animationUpdated.Invalidate();
	}

	virtual void OnFocusLost() override
	{
		ButtonBase::OnFocusLost();
		float start = 0.0f;
		if(m_focusAnimation)
		{
			start = 1.0f - m_focusAnimation->GetPhase();
			RemoveAnimation(m_focusAnimation);
		}
		m_focusAnimation = AddAnimation(GUIAnimationUpdater::FromObject(this, &TestButton::Anim_Hover), 0.15f);
		m_focusAnimation->SetPhase(start);
		m_focusAnimation->reversed = true;
	}
	virtual void OnFocus() override
	{
		ButtonBase::OnFocus();
		float start = 0.0f;
		if(m_focusAnimation)
		{
			start = 1.0f - m_focusAnimation->GetPhase();
			RemoveAnimation(m_focusAnimation);
		}
		m_focusAnimation = AddAnimation(GUIAnimationUpdater::FromObject(this, &TestButton::Anim_Hover), 0.3f, Interpolation::EaseOutCubic);
		m_focusAnimation->SetPhase(start);
	}
protected:
	void m_OnPressed(bool isKeyboard) override
	{
		if(isKeyboard)
		{
			AddAnimation(GUIAnimationUpdater::FromObject(this, &TestButton::Anim_ClickSingle), 0.2f);
			return;
		}
		auto anim = AddAnimation(GUIAnimationUpdater::FromObject(this, &TestButton::Anim_Click), 0.2f);
	}
	void m_OnReleased() override
	{
		auto anim = AddAnimation(GUIAnimationUpdater::FromObject(this, &TestButton::Anim_Click), 0.10f);
		anim->reversed = true;
	}
	void m_PostAnimationUpdate() override
	{
		if(!m_animationUpdated.IsValid())
		{
			renderTransform = m_clickTransform * m_hoverTransform;
			m_animationUpdated.Update();
		}
		ButtonBase::m_PostAnimationUpdate();
	}

private:
	Transform2D m_hoverTransform;
	Transform2D m_clickTransform;
	CachedState m_animationUpdated;
	Ref<GUIAnimation> m_focusAnimation;
};

class AnimationBarButton : public ButtonBase
{
public:
	virtual void Render(GUIRenderData data) override
	{
		ButtonBase::Render(data);

		Color c = m_hovered ? Color::Green : Color::White;
		data.guiRenderer->RenderWireBox(m_objectTransform, c);
	}
	void Anim_Hover(float time)
	{
		GetSlot<LayoutBox::Slot>()->fillAmount = 1.0f + time;
		Transform2D zoom;
		zoom *= Transform2D::Translation(Vector2(0.5f, 0.5f) * m_area.size);
		zoom *= Transform2D::Scale(Vector2(1.0f + time * 0.2f));
		zoom *= Transform2D::Rotation(-time * 10.0f);
		zoom *= Transform2D::Translation(Vector2(-0.5f, -0.5f) * m_area.size);
		renderTransform = zoom;
	}
	virtual void OnFocusLost() override
	{
		ButtonBase::OnFocusLost();
		float start = 0.0f;
		if(m_focusAnimation)
		{
			start = 1.0f - m_focusAnimation->GetPhase();
			RemoveAnimation(m_focusAnimation);
		}
		m_focusAnimation = AddAnimation(GUIAnimationUpdater::FromObject(this, &AnimationBarButton::Anim_Hover), 0.2f);
		m_focusAnimation->SetPhase(start);
		m_focusAnimation->reversed = true;
	}
	virtual void OnFocus() override
	{
		ButtonBase::OnFocus();
		float start = 0.0f;
		if(m_focusAnimation)
		{
			start = 1.0f - m_focusAnimation->GetPhase();
			RemoveAnimation(m_focusAnimation);
		}
		m_focusAnimation = AddAnimation(GUIAnimationUpdater::FromObject(this, &AnimationBarButton::Anim_Hover), 0.2f);
		m_focusAnimation->SetPhase(start);
	}

private:
	Ref<GUIAnimation> m_focusAnimation;
	Vector2 m_extendSize;
};

class GUITest : public GUITestBase
{
public:
	virtual void Init() override
	{
		GUITestBase::Init();

		// A button in a canvas
		auto test = new TestButton();
		auto slot = canvas->Add(*test);
		slot->anchor = Anchor(0.5f);
		slot->alignment = Vector2(0.5f);
		slot->offset = Rect(-200, -40, 200, 40);
	}

	Timer timer;
};
Test("GUI.ButtonBase")
{
	GUITest test;
	test.Run();
}

class GUILayoutBoxTest : public GUITestBase
{
public:
	virtual void Init() override
	{
		GUITestBase::Init();

		{
			//auto box = new LayoutBox();
			//auto boxSlot = canvas->Add(*box);
			//boxSlot->anchor = Anchor(0.5f);
			//boxSlot->alignment = Vector2(0.5f);
			//box->layoutDirection = LayoutBox::Vertical;
			//
			//// Some buttons in a layout box
			//for(uint32 i = 0; i < 8; i++)
			//{
			//	auto btn = new TestButton();
			//	auto slot = box->Add(*btn);
			//	slot->fillX = true;
			//	slot->fixedSize = Vector2(100, 30);
			//	slot->padding = Margin(0,2,0,2);
			//
			//	auto text = new Label();
			//	text->text = L"Hoi";
			//	text->fontSize = 20;
			//	auto textSlot = btn->SetContent(*text);
			//	textSlot->alignment = Vector2(0.5f);
			//}
		}
		
		{
			// Second button bar
			auto box = new LayoutBox();
			auto boxSlot = canvas->Add(*box);
			boxSlot->anchor = Anchor(0.0f, 0.3f, 1.0f, 0.3f);
			boxSlot->offset = Rect(Vector2(), Vector2(0.0f, 70.0f)); // height fixed
			boxSlot->alignment = Vector2(0.5f);
			boxSlot->padding = Margin(2);
			boxSlot->fillX = true;
			boxSlot->fillY = false;
			box->layoutDirection = LayoutBox::Horizontal;

			// Some buttons in a layout box
			for(uint32 i = 0; i < 6; i++)
			{
				auto btn = new AnimationBarButton();
				auto slot = box->Add(*btn);
				slot->fillX = true;
				slot->padding = Margin(0, 2, 0, 2);

				auto text = new Label();
				text->font = guiRenderer->font;
				text->text = L"Hoi";
				text->fontSize = 64;
				auto textSlot = btn->SetContent(*text);
				textSlot->alignment = Vector2(0.5f);
			}
		}
	}

	Timer timer;
};
Test("GUI.GUILayoutBox")
{
	GUILayoutBoxTest test;
	test.Run();
}

class GUILayoutBoxTest1 : public GUITestBase
{
public:
	virtual void Init() override
	{
		GUITestBase::Init();

		{
			auto box = new LayoutBox();
			auto boxSlot = canvas->Add(*box);
			boxSlot->anchor = Anchor(0.5f);
			boxSlot->alignment = Vector2(0.5f);
			boxSlot->fillX = false;
			boxSlot->fillY = false;
			box->layoutDirection = LayoutBox::Vertical;
			
			// Some buttons in a layout box
			for(uint32 i = 0; i < 8; i++)
			{
				auto btn = new TestButton();
				auto slot = box->Add(*btn);
				slot->fillX = true;
				slot->padding = Margin(0,2,0,2);
			
				auto text = new Label();
				text->text = L"Hoi";
				text->font = guiRenderer->font;
				text->fontSize = 20;
				auto textSlot = btn->SetContent(*text);
				textSlot->alignment = Vector2(0.5f);
			}
		}
	}

	Timer timer;
};
Test("GUI.GUILayoutBox1")
{
	GUILayoutBoxTest1 test;
	test.Run();
}

class Transform2DTest : public GUITestBase
{
public:
	virtual void Render(float deltaTime) override
	{
		// Transformation tests
		gui->Update(deltaTime);
		RenderQueue& rq = guiRenderer->Begin(Rect(Vector2(), m_window->GetWindowSize()));
		{
			Vector2 mousePos = gui->GetMousePos();
			Vector2 lastMousePos = mousePos - gui->GetMouseDelta();

			Rect boxRect = Rect(Vector2(200.0f, 200.0f), Vector2(100.0f, 20.0f));

			Transform2D boxRenderTransform;
			boxRenderTransform *= Transform2D::Translation(Vector2(0.5f)  * boxRect.size);
			boxRenderTransform *= Transform2D::Rotation(timer.SecondsAsFloat() * 100.0f);
			boxRenderTransform *= Transform2D::Scale(Vector2(1.0f + 0.5f * abs(cos(timer.SecondsAsFloat()))));
			boxRenderTransform *= Transform2D::Translation(Vector2(-0.5f) * boxRect.size);

			Transform2D boxTransform;
			boxTransform *= Transform2D::Translation(boxRect.pos);
			boxTransform *= boxRenderTransform;
			boxTransform *= Transform2D::Scale(boxRect.size);

			Transform2D inverseBoxTransform = boxTransform.Inverted();

			Rect bb = Rect::Empty;
			bb.Expand(Rect(Vector2(0.0f), Vector2(1.0f)), boxTransform);

			Color color0 = Color::White.WithAlpha(0.7f);
			bool contains0 = bb.ContainsPoint(mousePos);
			if(contains0)
				color0 = Color::Green;
			Color color1 = Color::White.WithAlpha(0.7f);

			if(contains0)
			{
				Rect unitRect = Rect(Vector2(), Vector2(1.0f));

				Vector2 ivpoint = inverseBoxTransform.TransformPoint(mousePos);
				bool contains1 = unitRect.ContainsPoint(ivpoint);
				if(contains1)
					color1 = Color::Green;
			}

			guiRenderer->RenderWireBox(boxTransform, color1);
			guiRenderer->RenderWireBox(bb.ToTransform(), color0);

			guiRenderer->RenderPoint(mousePos, Color::Cyan, 2.0f);
			guiRenderer->RenderPoint(boxRect.Center(), Color::Cyan, 2.0f);
		}
		guiRenderer->End();
	}
	Timer timer;
};
Test("GUI.Transform2D")
{
	Transform2DTest test;
	test.Run();
}

class GUIFocusTest : public GUITestBase
{
public:
	void Init() override
	{
		GUITestBase::Init();

		LayoutBox* box0 = new LayoutBox();
		box0->layoutDirection = LayoutBox::Horizontal;
		for(int j = 0; j < 4; j++)
		{
			LayoutBox* box1 = new LayoutBox();
			box1->layoutDirection = LayoutBox::Vertical;

			// Some buttons in a layout box
			for(int i = 0; i < (8-j); i++)
			{
				auto btn = new TestButton();
				auto slot = box1->Add(*btn);
				slot->fillX = true;
				slot->fillY = true;
				slot->padding = Margin(0, 2, 0, 2);

				auto text = new Label();
				text->text = Utility::WSprintf(L"Buttons %d", i);
				text->font = guiRenderer->font;
				text->fontSize = 32;
				auto textSlot = btn->SetContent(*text);
				textSlot->alignment = Vector2(0.5f);

				if(i == 0)
					gui->SetLastSelectedElement(btn); // Initial selection
			}

			auto slot = box0->Add(*box1);
			slot->fillY = true;
		}

		auto slot = canvas->Add(*box0);
		slot->offset = Rect(0, 0, 0, 0);
		slot->padding = Margin(10, 10, 0, 0);
		slot->fillX = false;
		slot->fillY = false;
	}
};
Test("GUI.Focus")
{
	GUIFocusTest test;
	test.Run();
}
#include "stdafx.h"
#include "GraphicsBase.hpp"

class GUITest : public GUITestBase
{
public:
	LayoutBox* m_box;

	virtual void Init() override
	{
		GUITestBase::Init();

		// A panel in a canvas
		Ref<Panel> test = Ref<Panel>(new Panel());
		test->color = Color::Cyan;
		auto slot = canvas->Add(test.As<GUIElementBase>());
		slot->anchor = Anchor(0.5f);
		slot->alignment = Vector2(0.5f);
		slot->size = Vector2(100);
	}
	virtual void Render(float deltaTime) override
	{
		Rect viewport;
		viewport.size = m_window->GetWindowSize();
		guiRenderer->Render(deltaTime, viewport, canvas.As<GUIElementBase>());

		// Transformation tests
		//auto font = guiRenderer->font;
		//auto fontMaterial = guiRenderer->fontMaterial;
		//RenderQueue& rq = guiRenderer->Begin();
		//{
		//	Transform2D box;
		//	box *= Transform2D::Translation(Vector2(100.0f));
		//	box *= Transform2D::Scale(Vector2(100.0f));
		//	box *= Transform2D::RotateAround(timer.SecondsAsFloat() * 200.0f, Vector2(0.5f));
		//
		//	Rect bb = Rect::Empty;
		//	bb.Expand(Rect(Vector2(0.0f), Vector2(1.0f)), box);
		//
		//	guiRenderer->RenderWireBox(box);
		//	guiRenderer->RenderWireBox(bb.ToTransform(), Color::Green);
		//	guiRenderer->RenderPoint(Transform2D(Vector2(150.0f)), Color::Green, 8.0f);
		//}
		//guiRenderer->End();

		m_gl->SwapBuffers();
	}

	Timer timer;
};

Test("GUI")
{
	GUITest test;
	test.Run();
}
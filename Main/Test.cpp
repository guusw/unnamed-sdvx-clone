#include "stdafx.h"
#include "Test.hpp"
#include "Application.hpp"
#include "Profiling.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include "MapDatabase.hpp"

class MapButton : public LayoutBox
{
public:
	MapButton()
	{

		layoutDirection = LayoutDirection::Horizontal;

		{
			panel = Ref<Panel>(new Panel());
			panel->texture = g_application->LoadTexture("laser.png");
			panel->imageFillMode = FillMode::Fit;
			panel->color = Color::Green;
			Add(panel.As<GUIElementBase>());
		}
		{
			panel = Ref<Panel>(new Panel());
			panel->texture = g_application->LoadTexture("cat.jpeg");
			panel->color = Color::White;
			panel->imageFillMode = FillMode::Fit;
			Add(panel.As<GUIElementBase>());
			((Slot*)panel->slot)->fill = true;


			Label* label = new Label();
			label->SetText(L"”L");
			label->color = Color::White;
			label->SetFontSize(64);
			panel->SetContent(label->MakeShared());
			((Panel::Slot*)label->slot)->alignment = Vector2(0.5f, 0.5f);
		}
		{
			panel = Ref<Panel>(new Panel());
			panel->texture = g_application->LoadTexture("laser.png");
			panel->imageFillMode = FillMode::Fit;
			panel->color = Color::Green;
			Add(panel.As<GUIElementBase>());
		}
		//((Slot*)panel->slot)->

		//texture = g_application->LoadTexture("laser.png");
	}
	~MapButton()
	{
	}

	Ref<Panel> panel;
};

class Test_Impl : public Test
{
private:
	Timer m_dbUpdateTimer;
	Ref<Canvas> m_canvas;
	GUIRenderer m_guiRenderer;
	MapDatabase m_mapDatabase;

	Texture m_cats[3];
	Texture m_rabbits[2];

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl))
			return false;

		m_mapDatabase.AddSearchPath("D:\\KShoot\\songs");
		m_mapDatabase.StartSearching();

		CheckedLoad(m_cats[0] = g_application->LoadTexture("cat.jpeg"));
		CheckedLoad(m_cats[1] = g_application->LoadTexture("cat1.jpeg"));
		CheckedLoad(m_cats[2] = g_application->LoadTexture("cat2.jpeg"));
		CheckedLoad(m_rabbits[0] = g_application->LoadTexture("rabbit.jpeg"));
		CheckedLoad(m_rabbits[1] = g_application->LoadTexture("rabbit_and_cat.jpg"));

		m_canvas = Utility::MakeRef(new Canvas());

		//{
		//	MapButton* testButton = new MapButton();
		//	m_canvas->Add(testButton->MakeShared());
		//	Canvas::Slot* testButtonSlot = (Canvas::Slot*)testButton->slot;
		//	testButtonSlot->anchor = Anchor(0, 0.5f, 1, 0.5f);
		//	testButtonSlot->offset = Rect(0, -100, 0, 100);
		//}
		{
			MapButton* testButton = new MapButton();
			m_canvas->Add(testButton->MakeShared());
			Canvas::Slot* testButtonSlot = (Canvas::Slot*)testButton->slot;
			testButtonSlot->anchor = Anchors::Full;
			testButtonSlot->SetZOrder(-2);
		}
		{
			MapButton* testButton = new MapButton();
			m_canvas->Add(testButton->MakeShared());
			Canvas::Slot* testButtonSlot = (Canvas::Slot*)testButton->slot;
			testButtonSlot->anchor = Anchors::TopMiddle;
			testButtonSlot->offset = Rect(-300, 50, 300, 200);
		}
		{
			MapButton* testButton = new MapButton();
			m_canvas->Add(testButton->MakeShared());
			Canvas::Slot* testButtonSlot = (Canvas::Slot*)testButton->slot;
			testButtonSlot->anchor = Anchors::BottomMiddle;
			testButtonSlot->offset = Rect(-300, -200, 300, -50);
		}
		//testButtonSlot->offset = Margin(0, 200.0f);
		//testButtonSlot->anchor = Margin(0.0f, 0.5f, 0.9f, 0.5f);
		//testButtonSlot->alignment = Vector2(0, 0.5f);

		return true;
	}

	virtual void Render(float deltaTime) override
	{
		if(m_dbUpdateTimer.Milliseconds() > 500)
		{
			m_mapDatabase.Update();
			m_dbUpdateTimer.Restart();
		}

		Rect viewport(Vector2(), g_gameWindow->GetWindowSize());
		m_guiRenderer.Render(deltaTime, viewport, m_canvas.As<GUIElementBase>());
	}
};

Test* Test::Create()
{
	Test_Impl* impl = new Test_Impl();
	if(!impl->Init())
	{
		delete impl;
		return nullptr;
	}
	return impl;
}

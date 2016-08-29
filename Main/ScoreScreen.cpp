#include "stdafx.h"
#include "ScoreScreen.hpp"
#include "Application.hpp"
#include "GUI.hpp"
#include "CommonGUIStyle.hpp"
#include "Audio.hpp"
#include "Scoring.hpp"
#include "Game.hpp"
#include "AsyncAssetLoader.hpp"

class ScoreScreen_Impl : public ScoreScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;

	Ref<LayoutBox> m_itemBox;

	// Things for score screen
	Graphics::Font m_specialFont;
	Sample m_applause;

	float m_gauge;
	bool m_autoplay;
	uint32 m_score;
	uint32 m_categorizedHits[3];

	BeatmapSettings m_beatmapSettings;
	Texture m_jacketImage;

public:
	ScoreScreen_Impl(class Game* game)
	{
		Scoring& scoring = game->GetScoring();
		m_autoplay = scoring.autoplay;
		m_score = scoring.CalculateCurrentScore();
		m_gauge = scoring.currentGauge;
		memcpy(m_categorizedHits, scoring.categorizedHits, sizeof(scoring.categorizedHits));

		m_beatmapSettings = game->GetBeatmap()->GetMapSettings();
		m_jacketImage = game->GetJacketImage();
	}
	~ScoreScreen_Impl()
	{
		g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}

	AsyncAssetLoader loader;
	virtual bool AsyncLoad() override
	{
		m_guiStyle = CommonGUIStyle::Get();

		m_canvas = Utility::MakeRef(new Canvas());

		// Font
		CheckedLoad(m_specialFont = FontRes::Create(g_gl, "fonts/divlit_custom.ttf"));
		CheckedLoad(m_applause = g_audio->CreateSample("audio/applause.wav"));
		m_applause->Play();

		LayoutBox* scoreContainer = new LayoutBox();
		scoreContainer->layoutDirection = LayoutBox::Vertical;
		Canvas::Slot* slot = m_canvas->Add(scoreContainer->MakeShared());
		slot->anchor = Anchors::Full;
		slot->padding = Margin(150, 120);

		// Background
		Panel* fullBg = new Panel();
		loader.AddTexture(fullBg->texture, "bg.png");
		fullBg->color = Color(0.3f);
		slot = m_canvas->Add(fullBg->MakeShared());
		slot->anchor = Anchors::Full;
		slot->SetZOrder(-2);

		// Border color
		Panel* border = new Panel();
		border->color = Color(0.5f).WithAlpha(0.2f);
		slot = m_canvas->Add(border->MakeShared());
		slot->anchor = Anchors::Full;
		slot->padding = Margin(100);
		slot->SetZOrder(-1);

		Label* title = new Label();
		title->SetFont(m_specialFont);
		title->SetFontSize(128);
		/// TODO: Research actual pass conditions
		if(m_gauge > 0.75f)
			title->SetText(L"PASS");
		else
			title->SetText(L"FAILURE");
		LayoutBox::Slot* slot1 = scoreContainer->Add(title->MakeShared());
		slot1->alignment = Vector2(0.0f, 0.0f);

		// Song metadata
		{
			// Song title
			Label* songName = new Label();
			songName->SetText(Utility::ConvertToWString(m_beatmapSettings.title) + Utility::WSprintf(L" [LVL_%d]", m_beatmapSettings.level));
			songName->SetFontSize(48);
			slot1 = scoreContainer->Add(songName->MakeShared());
			slot1->padding.left = 20;

			// Song artist name
			Label* artistName = new Label();
			artistName->SetFontSize(48);
			artistName->SetText(Utility::ConvertToWString(m_beatmapSettings.artist));
			slot1 = scoreContainer->Add(artistName->MakeShared());
			slot1->padding.left = 20;
		}

		// Lower box that holds the hit counts
		Canvas* inner = new Canvas();
		LayoutBox::Slot* slot2 = scoreContainer->Add(inner->MakeShared());
		slot2->fillX = true;
		slot2->fillY = true;
		slot2->padding = Margin(50, 25, 50, 50);

		m_itemBox = Ref<LayoutBox>(new LayoutBox());
		m_itemBox->layoutDirection = LayoutBox::Vertical;
		Canvas::Slot* slot3 = inner->Add(m_itemBox.As<GUIElementBase>());
		slot3->anchor = Anchor(0.0f, 0.0f, 0.5f, 1.0f);

		// Big score number
		Label* scoreNumber = new Label();
		scoreNumber->SetFont(m_specialFont);
		scoreNumber->SetFontSize(80);
		scoreNumber->SetText(Utility::WSprintf(L"%07d", m_score));
		scoreNumber->SetTextOptions(FontRes::Monospace);
		LayoutBox::Slot* scoreSlot = m_itemBox->Add(scoreNumber->MakeShared());
		scoreSlot->padding = Margin(0, -20, 0, 20);
		scoreSlot->alignment = Vector2(0.0f, 0.0f);

		return loader.Load();
	}
	virtual bool AsyncFinalize() override
	{
		if(!loader.Finalize())
			return false;

		// Add indicators for ammounts of miss/good/perfect hits
		auto AddScoreRow = [&](Texture texture, int32 count)
		{
			Canvas* canvas = new Canvas();
			LayoutBox::Slot* slot4 = m_itemBox->Add(canvas->MakeShared());
			slot4->fillX = true;
			slot4->fillY = false;
			slot4->alignment = Vector2(0.0f, 0.5f);

			Panel* icon = new Panel();
			icon->texture = texture;
			Canvas::Slot* canvasSlot = canvas->Add(icon->MakeShared());
			canvasSlot->anchor = Anchor(0.0f, 0.5f);
			canvasSlot->autoSizeX = true;
			canvasSlot->autoSizeY = true;
			canvasSlot->alignment = Vector2(0.0f, 0.5f);

			Label* countLabel = new Label();
			countLabel->SetFont(m_specialFont);
			countLabel->SetFontSize(64);
			countLabel->color = Color(0.6f);
			countLabel->SetText(Utility::WSprintf(L"%d", count));
			canvasSlot = canvas->Add(countLabel->MakeShared());
			canvasSlot->anchor = Anchor(0.5f, 0.5f);
			canvasSlot->autoSizeX = true;
			canvasSlot->autoSizeY = true;
			canvasSlot->alignment = Vector2(0.0f, 0.5f);
		};
		AddScoreRow(g_application->LoadTexture("score2.png"), m_categorizedHits[2]);
		AddScoreRow(g_application->LoadTexture("score1.png"), m_categorizedHits[1]);
		AddScoreRow(g_application->LoadTexture("score0.png"), m_categorizedHits[0]);

		return true;
	}
	bool Init() override
	{
		// Add to screen
		Canvas::Slot* rootSlot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		rootSlot->anchor = Anchors::Full;
		return true;
	}

	virtual void OnKeyPressed(Key key) override
	{
		if(key == Key::Escape)
		{
			g_application->RemoveTickable(this);
		}
	}
	virtual void OnKeyReleased(Key key) override
	{
	}
	virtual void Render(float deltaTime) override
	{
	}
	virtual void Tick(float deltaTime) override
	{
	}
};

ScoreScreen* ScoreScreen::Create(class Game* game)
{
	ScoreScreen_Impl* impl = new ScoreScreen_Impl(game);
	return impl;
}

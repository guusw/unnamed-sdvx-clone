#include "stdafx.h"
#include "ScoreScreen.hpp"
#include "Application.hpp"
#include "GUI.hpp"
#include "CommonGUIStyle.hpp"
#include "Audio.hpp"
#include "Scoring.hpp"
#include "Game.hpp"
#include "AsyncAssetLoader.hpp"
#include "HealthGauge.hpp"
#include "SongSelectStyle.hpp"
#include "PerformanceGraph.hpp"

class ScoreScreen_Impl : public ScoreScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;
	Ref<HealthGauge> m_gauge;
	Ref<Panel> m_jacket;
	Ref<LayoutBox> m_itemBox;

	// Things for score screen
	Graphics::Font m_specialFont;
	Sample m_applause;
	Texture m_categorizedHitTextures[4];

	bool m_autoplay;
	uint32 m_score;
	uint32 m_grade;
	uint32 m_maxCombo;
	uint32 m_categorizedHits[3];
	float m_finalGaugeValue;
	String m_jacketPath;

	Ref<SongSelectStyle> m_songSelectStyle;

	BeatmapSettings m_beatmapSettings;
	Texture m_jacketImage;

public:
	ScoreScreen_Impl(class Game* game)
	{
		Scoring& scoring = game->GetScoring();
		m_autoplay = scoring.autoplay;
		m_score = scoring.CalculateCurrentScore();
		m_grade = scoring.CalculateCurrentGrade();
		m_maxCombo = scoring.maxComboCounter;
		m_finalGaugeValue = scoring.currentGauge;
		memcpy(m_categorizedHits, scoring.categorizedHits, sizeof(scoring.categorizedHits));

		// Used for jacket images
		m_songSelectStyle = SongSelectStyle::Get(g_application);

		m_beatmapSettings = game->GetBeatmap()->GetMapSettings();
		m_jacketPath = Path::Normalize(game->GetMapRootPath() + Path::sep + m_beatmapSettings.jacketPath);
		m_jacketImage = m_songSelectStyle->GetJacketThumnail(m_jacketPath);
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

		// Background
		Panel* fullBg = new Panel();
		loader.AddTexture(fullBg->texture, "score/bg.png");
		fullBg->color = Color(1.0f);
		{
			Canvas::Slot* slot = m_canvas->Add(fullBg->MakeShared());
			slot->anchor = Anchors::Full;
			slot->SetZOrder(-2);
		}

		// Border
		Panel* border = new Panel();
		border->color = Color::Black.WithAlpha(0.25f);
		{
			Canvas::Slot* slot = m_canvas->Add(border->MakeShared());
			slot->anchor = Anchors::Full;
			slot->padding = Margin(0, 30);
			slot->SetZOrder(-1);
		}

		float screenSplit = 0.35f;
		float sidePadding = 40.0f;

		// Song info container
		{
			// Contains all song info
			LayoutBox* songInfoContainer = new LayoutBox();
			songInfoContainer->layoutDirection = LayoutBox::Horizontal;
			{
				Canvas::Slot* slot = m_canvas->Add(songInfoContainer->MakeShared());
				slot->anchor = Anchor(0.0f, 0.0f, 1.0f, screenSplit);
				slot->padding = Margin(sidePadding);
				slot->padding.top = 30 + 20;
				slot->padding.bottom = 20;
				slot->alignment = Vector2(0.0f, 0.5f);
			}

			// Jacket image
			Panel* jacketImage = new Panel();
			m_jacket = Ref<Panel>(jacketImage);
			jacketImage->texture = m_jacketImage;
			jacketImage->imageFillMode = FillMode::Fit;
			{
				LayoutBox::Slot* slot = songInfoContainer->Add(jacketImage->MakeShared());
				slot->fillY = true;
				slot->alignment = Vector2(0.0f, 0.0f);
			}

			// Metadata container
			LayoutBox* metadataContainer = new LayoutBox();
			metadataContainer->layoutDirection = LayoutBox::Vertical;
			{
				LayoutBox::Slot* slot = songInfoContainer->Add(metadataContainer->MakeShared());
				slot->alignment = Vector2(0.0f, 0.5f);
				slot->padding.left = 30;
			}
			auto AddMetadataLine = [&](const String& text)
			{
				Label* label = new Label();
				label->SetText(Utility::ConvertToWString(text));
				label->SetFontSize(48);
				LayoutBox::Slot* slot = metadataContainer->Add(label->MakeShared());
			};
			// Title/Artist/Effector/Etc.
			AddMetadataLine(m_beatmapSettings.title + Utility::Sprintf(" [%d]", m_beatmapSettings.level));
			AddMetadataLine(m_beatmapSettings.artist);
			AddMetadataLine("Effected By: " + m_beatmapSettings.effector);
		}

		// Main score container
		{
			Panel* scoreContainerBg = new Panel();
			scoreContainerBg->color = Color::Black.WithAlpha(0.5f);
			{
				Canvas::Slot* slot = m_canvas->Add(scoreContainerBg->MakeShared());
				slot->anchor = Anchor(0.0f, screenSplit, 1.0f, 1.0f);
				slot->padding = Margin(0, 0, 0, 50);
			}

			LayoutBox* scoreContainer = new LayoutBox();
			scoreContainer->layoutDirection = LayoutBox::Horizontal;
			{
				Canvas::Slot* slot = m_canvas->Add(scoreContainer->MakeShared());
				slot->anchor = Anchor(0.0f, screenSplit, 1.0f, 1.0f); 
				slot->padding = Margin(0, 0, 0, 50);
			}

			m_gauge = Ref<HealthGauge>(new HealthGauge());
			loader.AddTexture(m_gauge->fillTexture, "gauge_fill.png");
			loader.AddTexture(m_gauge->frameTexture, "gauge_frame.png");
			loader.AddTexture(m_gauge->bgTexture, "gauge_bg.png");
			loader.AddMaterial(m_gauge->fillMaterial, "gauge");
			m_gauge->barMargin = Margin(36, 34, 33, 34);
			m_gauge->rate = m_finalGaugeValue;
			{
				LayoutBox::Slot* slot = scoreContainer->Add(m_gauge.As<GUIElementBase>());
				slot->fillY = true;
			}

			// Score and graph
			LayoutBox* scoreAndGraph = new LayoutBox();
			scoreAndGraph->layoutDirection = LayoutBox::Vertical;
			{
				LayoutBox::Slot* slot = scoreContainer->Add(scoreAndGraph->MakeShared());
				slot->alignment = Vector2(0.0f, 0.5f);
				slot->padding = Margin(20, 10);
				slot->fillX = true;
				slot->fillY = true;
				slot->fillAmount = 1.0f;
			}

			Label* score = new Label();
			score->SetText(Utility::WSprintf(L"%08d", m_score));
			score->SetFont(m_specialFont);
			score->SetFontSize(140);
			score->color = Color(0.75f);
			score->SetTextOptions(FontRes::Monospace);
			{
				LayoutBox::Slot* slot = scoreAndGraph->Add(score->MakeShared());
				slot->padding = Margin(0, 0, 0, 20);
				slot->fillX = false;
				slot->alignment = Vector2(0.5f, 0.0f);
			}

			Label* perfomanceTitle = new Label();
			perfomanceTitle->SetText(L"Performance");
			perfomanceTitle->SetFont(m_specialFont);
			perfomanceTitle->SetFontSize(40);
			perfomanceTitle->color = Color(0.6f);
			{
				LayoutBox::Slot* slot = scoreAndGraph->Add(perfomanceTitle->MakeShared());
				slot->padding = Margin(5, 0, 0, 5);
				slot->alignment = Vector2(0.0f, 0.0f);
			}

			PerformanceGraph* graphPanel = new PerformanceGraph();
			loader.AddTexture(graphPanel->borderTexture, "ui/button.png");
			graphPanel->border = Margini(5);
			{
				LayoutBox::Slot* slot = scoreAndGraph->Add(graphPanel->MakeShared());
				slot->fillY = true;
				slot->fillX = true;
				slot->padding = Margin(0, 0);
			}

			// Grade and hits
			LayoutBox* gradePanel = new LayoutBox();
			gradePanel->layoutDirection = LayoutBox::Vertical;
			{
				LayoutBox::Slot* slot = scoreContainer->Add(gradePanel->MakeShared());
				slot->alignment = Vector2(0.0f, 0.5f);
				slot->padding = Margin(30, 10);
				slot->fillX = true;
				slot->fillY = true;
			}

			String gradeImages[] =
			{
				"AAA",
				"AA",
				"A",
				"B",
				"C",
				"D",
			};
			String gradeImagePath = String("score") + Path::sep + gradeImages[m_grade] + ".png";
			Panel* gradeImage = new Panel();
			loader.AddTexture(gradeImage->texture, gradeImagePath);
			gradeImage->imageFillMode = FillMode::Fit;
			{
				LayoutBox::Slot* slot = gradePanel->Add(gradeImage->MakeShared());
				slot->fillX = true;
				slot->fillY = true;
				slot->fillAmount = 0.7f;
			}

			// Hit items
			m_itemBox = Ref<LayoutBox>(new LayoutBox());
			m_itemBox->layoutDirection = LayoutBox::Vertical;
			{
				LayoutBox::Slot* slot = gradePanel->Add(m_itemBox->MakeShared());
				slot->fillX = true;
				slot->fillY = true;
				slot->padding = Margin(0, 20);
			}
		}

		// Load hit textures (Good, Perfect, Miss)
		loader.AddTexture(m_categorizedHitTextures[3], "score/scorec.png"); // Max combo
		loader.AddTexture(m_categorizedHitTextures[2], "score/score2.png");
		loader.AddTexture(m_categorizedHitTextures[1], "score/score1.png");
		loader.AddTexture(m_categorizedHitTextures[0], "score/score0.png");

		return loader.Load();
	}
	virtual bool AsyncFinalize() override
	{
		if(!loader.Finalize())
			return false;

		// Make gauge material transparent
		m_gauge->fillMaterial->opaque = false;

		// Add indicators for ammounts of miss/good/perfect hits
		auto AddScoreRow = [&](Texture texture, int32 count)
		{
			Canvas* canvas = new Canvas();
			LayoutBox::Slot* slot4 = m_itemBox->Add(canvas->MakeShared());
			slot4->fillX = true;
			slot4->fillY = false;
			slot4->alignment = Vector2(0.0f, 0.5f);
			slot4->padding = Margin(0, 5);

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
			countLabel->SetTextOptions(FontRes::Monospace);
			countLabel->color = Color(0.5f);
			countLabel->SetText(Utility::WSprintf(L"%05d", count));
			canvasSlot = canvas->Add(countLabel->MakeShared());
			canvasSlot->anchor = Anchor(0.9f, 0.4f); // Slight y offset because of the font
			canvasSlot->autoSizeX = true;
			canvasSlot->autoSizeY = true;
			canvasSlot->alignment = Vector2(1.0f, 0.5f);
		};

		// Add hit statistics
		AddScoreRow(m_categorizedHitTextures[2], m_categorizedHits[2]);
		AddScoreRow(m_categorizedHitTextures[1], m_categorizedHits[1]);
		AddScoreRow(m_categorizedHitTextures[0], m_categorizedHits[0]);
		AddScoreRow(m_categorizedHitTextures[3], m_maxCombo);

		return true;
	}
	bool Init() override
	{
		// Add to screen
		Canvas::Slot* rootSlot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		rootSlot->anchor = Anchors::Full;

		// Play score screen sound
		m_applause->Play();

		return true;
	}

	virtual void OnKeyPressed(Key key) override
	{
        if(key == Key::Escape || key = Key::Return)
		{
			g_application->RemoveTickable(this);
		}
	}
	virtual void OnKeyReleased(Key key) override
	{
	}
	virtual void Render(float deltaTime) override
	{
		// Poll for loaded jacket image
		if(m_jacketImage == m_songSelectStyle->loadingJacketImage)
		{
			m_jacketImage = m_songSelectStyle->GetJacketThumnail(m_jacketPath);
			m_jacket->texture = m_jacketImage;
		}
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

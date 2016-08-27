#include "stdafx.h"
#include "Game.hpp"
#include "Application.hpp"
#include "BeatmapPlayback.hpp"
#include "Profiling.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "AudioPlayback.hpp"
#include "Input.hpp"
#include "SongSelect.hpp"

#include "GUI/GUI.hpp"
#include "GUI/HealthGauge.hpp"
#include "GUI/SettingsBar.hpp"

class Game_Impl : public Game
{
	String m_mapPath;
	Beatmap* m_beatmap;
	bool m_playing = true;
	bool m_started = false;
	bool m_paused = false;

	bool m_renderDebugHUD = false;

	// GUI
	GUIRenderer m_guiRenderer;
	// Main Canvas
	Ref<Canvas> m_canvas;
	Ref<HealthGauge> m_scoringGauge;
	Ref<SettingsBar> m_settingsBar;
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Label> m_scoreText;

	Graphics::Font m_fontDivlit;

	// Texture of the map jacket image, if available
	Texture m_jacketTexture;

	// Scoring system object
	Scoring m_scoring;
	// Beatmap playback manager (object and timing point selector)
	BeatmapPlayback m_playback;
	// Audio playback manager (music and FX))
	AudioPlayback m_audioPlayback;
	// Applied audio offset
	int32 m_audioOffset = 0;

	// The play field
	Track* m_track;

	// Input controller
	Input m_input;

	// The camera watching the playfield
	Camera m_camera;

	// Current background visualization
	Background* m_background;

	// Currently active timing point
	const TimingPoint* m_currentTiming;
	// Currently visible gameplay objects
	Vector<ObjectState*> m_currentObjectSet;
	MapTime m_lastMapTime;

	// Combo gain animation
	Timer m_comboAnimation;

	Sample m_slamSample;
	Sample m_clickSamples[2];

	// Roll intensity, default = 1
	const float m_rollIntensityBase = 0.03f;
	float m_rollIntensity = m_rollIntensityBase;

	// Particle effects
	Material particleMaterial;
	Texture basicParticleTexture;
	Texture squareParticleTexture;
	ParticleSystem m_particleSystem;
	Ref<ParticleEmitter> m_laserFollowEmitters[2];
	Ref<ParticleEmitter> m_holdEmitters[6];
public:
	~Game_Impl()
	{
		if(m_track)
			delete m_track;
		if(m_background)
			delete m_background;
		m_input.Cleanup();

		// Allow frameliming again
		g_application->SetFrameLimiter(60);
	}

	// Normal/FX button x placement
	float GetButtonPlacement(uint32 buttonIdx)
	{
		if(buttonIdx < 4)
			return buttonIdx * m_track->buttonWidth - (m_track->buttonWidth * 1.5f);
		else
			return (buttonIdx - 4) * m_track->fxbuttonWidth - (m_track->fxbuttonWidth * 0.5f);
	}

	// Main update routine for the game logic
	virtual bool Init(Beatmap* map, String mapPath) override
	{
		ProfilerScope $("Init Game");

		if(g_application->GetAppCommandLine().Contains("-debug"))
		{
			m_renderDebugHUD = true;
		}

		assert(map);
		m_mapPath = mapPath;
		m_beatmap = map;
		const BeatmapSettings& mapSettings = m_beatmap->GetMapSettings();

		// Try to load beatmap jacket image
		String jacketPath = mapPath + "/" + mapSettings.jacketPath;
		Image jacketImage = ImageRes::Create(jacketPath);
		if(jacketImage)
		{
			m_jacketTexture = TextureRes::Create(g_gl, jacketImage);
		}

		// Load beatmap audio
		if(!m_audioPlayback.Init(*m_beatmap, mapPath))
			return false;

		if(!InitGameplay())
			return false;

		if(!InitSFX())
			return false;


		// Intialize track graphics
		m_track = new Track();
		if(!m_track->Init())
		{
			return false;
		}

		if(!InitParticles())
			return false;

		// Background graphics
		CheckedLoad(m_background = CreateBackground(this));

		if(!InitHUD())
			return false;

		return true;
	}
	virtual void Tick(float deltaTime) override
	{
		TickGameplay(deltaTime);
	}
	virtual void Render(float deltaTime) override
	{
		// The amount of bars visible on the track at one time
		m_track->trackViewRange = Vector2(m_playback.GetBarTime(), 0.0f);
		m_track->trackViewRange.y = m_track->trackViewRange.x + m_track->viewRange;

		m_track->Tick(m_playback, deltaTime);

		// Get render state from the camera
		float rollA = m_scoring.GetLaserRollOutput(0);
		float rollB = m_scoring.GetLaserRollOutput(1);
		m_camera.SetTargetRoll((rollA + rollB) * m_rollIntensity);

		// Set track zoom
		if(!m_settingsBar->IsShown()) // Overridden settings?
		{
			m_camera.zoomBottom = m_playback.GetZoom(0);
			m_camera.zoomTop = m_playback.GetZoom(1);
		}
		m_camera.track = m_track;
		m_camera.Tick(deltaTime);
		RenderState rs = m_camera.CreateRenderState(true);

		// Draw BG first
		g_gl->SetViewport(rs.viewportSize);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		m_background->Render(deltaTime);

		// Main render queue
		RenderQueue renderQueue(g_gl, rs);

		// Draw the base track + time division ticks
		m_track->DrawBase(renderQueue);

		// Sort objects to draw
		m_currentObjectSet.Sort([](const TObjectState<void>* a, const TObjectState<void>* b)
		{
			auto ObjectRenderPriorty = [](const TObjectState<void>* a)
			{
				if(a->type == ObjectType::Single || a->type == ObjectType::Hold)
					return (((ButtonObjectState*)a)->index < 4) ? 1 : 0;
				else
					return 2;
			};
			uint32 renderPriorityA = ObjectRenderPriorty(a);
			uint32 renderPriorityB = ObjectRenderPriorty(b);
			return renderPriorityA < renderPriorityB;
		});

		for(auto& object : m_currentObjectSet)
		{
			m_track->DrawObjectState(renderQueue, m_playback, object, m_scoring.IsObjectHeld(object));
		}

		// Use new camera for scoring overlay
		//	this is because otherwise some of the scoring elements would get clipped to
		//	the track's near and far planes
		rs = m_camera.CreateRenderState(false);
		RenderQueue scoringRq(g_gl, rs);

		// Copy over laser position
		for(uint32 i = 0; i < 2; i++)
		{
			if(m_scoring.IsLaserHeld(i))
			{
				m_track->laserPositions[i] = m_scoring.laserTargetPositions[i];
			}
			else
			{
				m_track->laserPositions[i] = m_scoring.laserPositions[i];
			}
		}

		m_track->DrawOverlays(scoringRq);
		float comboZoom = Math::Max(0.0f, (1.0f - (m_comboAnimation.SecondsAsFloat() / 0.2f)) * 0.5f);
		m_track->DrawCombo(scoringRq, m_scoring.currentComboCounter, Color::White, 1.0f + comboZoom);

		// Render queues
		renderQueue.Process();
		scoringRq.Process();

		// Set laser follow particle visiblity
		for(uint32 i = 0; i < 2; i++)
		{
			if(m_scoring.IsLaserHeld(i))
			{
				if(!m_laserFollowEmitters[i])
					m_laserFollowEmitters[i] = CreateTrailEmitter(m_track->laserColors[i]);

				// Set particle position to follow laser
				m_laserFollowEmitters[i]->position.x = m_track->trackWidth * m_scoring.laserTargetPositions[i] - m_track->trackWidth * 0.5f;
			}
			else
			{
				if(m_laserFollowEmitters[i])
				{
					m_laserFollowEmitters[i].Release();
				}
			}
		}

		// Set hold button particle visibility
		for(uint32 i = 0; i < 6; i++)
		{
			if(m_scoring.IsObjectHeld(i))
			{
				if(!m_holdEmitters[i])
				{
					Color hitColor = (i < 4) ? Color::White : Color::FromHSV(20, 0.7f, 1.0f);
					float hitWidth = (i < 4) ? m_track->buttonWidth : m_track->fxbuttonWidth;
					m_holdEmitters[i] = CreateHoldEmitter(hitColor, hitWidth);
					m_holdEmitters[i]->position.x = GetButtonPlacement(i);
				}
			}
			else
			{
				if(m_holdEmitters[i])
				{
					m_holdEmitters[i].Release();
				}
			}

		}

		// Render particle effects last
		RenderParticles(rs, deltaTime);

		RenderHUD(deltaTime);
	}

	bool InitParticles()
	{
		// Load particle effects
		m_particleSystem = ParticleSystemRes::Create(g_gl);
		CheckedLoad(particleMaterial = g_application->LoadMaterial("particle"));
		particleMaterial->blendMode = MaterialBlendMode::Additive;
		particleMaterial->opaque = false;
		CheckedLoad(basicParticleTexture = g_application->LoadTexture("particle_flare.png"));
		CheckedLoad(squareParticleTexture = g_application->LoadTexture("particle_square.png"));

		m_laserFollowEmitters[0] = CreateTrailEmitter(m_track->laserColors[0]);

		return true;
	}
	void RenderParticles(const RenderState& rs, float deltaTime)
	{
		// Render particle effects
		m_particleSystem->Render(rs, deltaTime);
	}
	Ref<ParticleEmitter> CreateTrailEmitter(const Color& color)
	{
		Ref<ParticleEmitter> emitter = m_particleSystem->AddEmitter();
		emitter->material = particleMaterial;
		emitter->texture = basicParticleTexture;
		emitter->loops = 0;
		emitter->duration = 5.0f;
		emitter->SetSpawnRate(PPRandomRange<float>(250, 300));
		emitter->SetStartPosition(PPBox({ 0.5f, 0.0f, 0.0f }));
		emitter->SetStartSize(PPRandomRange<float>(0.25f, 0.4f));
		emitter->SetScaleOverTime(PPRange<float>(2.0f, 1.0f));
		emitter->SetFadeOverTime(PPRangeFadeIn<float>(1.0f, 0.0f, 0.4f));
		emitter->SetLifetime(PPRandomRange<float>(0.17f, 0.2f));
		emitter->SetStartDrag(PPConstant<float>(0.0f));
		emitter->SetStartVelocity(PPConstant<Vector3>({ 0, 0.0f, 2.0f }));
		emitter->SetSpawnVelocityScale(PPRandomRange<float>(0.9f, 2));
		emitter->SetStartColor(PPConstant<Color>(color * 0.7f));
		emitter->SetGravity(PPConstant<Vector3>(Vector3(0.0f, 0.0f, -9.81f)));
		emitter->position.y = 0.0f;
		emitter->scale = 0.3f;
		return emitter;
	}
	Ref<ParticleEmitter> CreateHoldEmitter(const Color& color, float width)
	{
		Ref<ParticleEmitter> emitter = m_particleSystem->AddEmitter();
		emitter->material = particleMaterial;
		emitter->texture = basicParticleTexture;
		emitter->loops = 0;
		emitter->duration = 5.0f;
		emitter->SetSpawnRate(PPRandomRange<float>(50, 100));
		emitter->SetStartPosition(PPBox({ width, 0.0f, 0.0f }));
		emitter->SetStartSize(PPRandomRange<float>(0.3f, 0.35f));
		emitter->SetScaleOverTime(PPRange<float>(1.2f, 1.0f));
		emitter->SetFadeOverTime(PPRange<float>(1.0f, 0.0f));
		emitter->SetLifetime(PPRandomRange<float>(0.10f, 0.15f));
		emitter->SetStartDrag(PPConstant<float>(0.0f));
		emitter->SetStartVelocity(PPConstant<Vector3>({ 0.0f, 0.0f, 0.0f }));
		emitter->SetSpawnVelocityScale(PPRandomRange<float>(0.2f, 0.2f));
		emitter->SetStartColor(PPConstant<Color>(color*0.6f));
		emitter->SetGravity(PPConstant<Vector3>(Vector3(0.0f, 0.0f, -4.81f)));
		emitter->position.y = 0.01f;
		emitter->scale = 1.0f;
		return emitter;
	}
	Ref<ParticleEmitter> CreateExplosionEmitter(const Color& color, const Vector3 dir)
	{
		Ref<ParticleEmitter> emitter = m_particleSystem->AddEmitter();
		emitter->material = particleMaterial;
		emitter->texture = basicParticleTexture;
		emitter->loops = 1;
		emitter->duration = 0.2f;
		emitter->SetSpawnRate(PPRange<float>(200, 0));
		emitter->SetStartPosition(PPSphere(0.1f));
		emitter->SetStartSize(PPRandomRange<float>(0.7f, 1.1f));
		emitter->SetFadeOverTime(PPRangeFadeIn<float>(0.9f, 0.0f, 0.0f));
		emitter->SetLifetime(PPRandomRange<float>(0.22f, 0.3f));
		emitter->SetStartDrag(PPConstant<float>(0.2f));
		emitter->SetSpawnVelocityScale(PPRandomRange<float>(1.0f, 4.0f));
		emitter->SetScaleOverTime(PPRange<float>(1.0f, 0.4f));
		emitter->SetStartVelocity(PPConstant<Vector3>(dir * 5.0f));
		emitter->SetStartColor(PPConstant<Color>(color));
		emitter->SetGravity(PPConstant<Vector3>(Vector3(0.0f, 0.0f, -9.81f)));
		emitter->scale = 0.4f;
		return emitter;
	}
	Ref<ParticleEmitter> CreateHitEmitter(const Color& color, float width)
	{
		Ref<ParticleEmitter> emitter = m_particleSystem->AddEmitter();
		emitter->material = particleMaterial;
		emitter->texture = basicParticleTexture;
		emitter->loops = 1;
		emitter->duration = 0.15f;
		emitter->SetSpawnRate(PPRange<float>(50, 0));
		emitter->SetStartPosition(PPBox(Vector3(width * 0.5f, 0.0f, 0)));
		emitter->SetStartSize(PPRandomRange<float>(0.3f, 0.1f));
		emitter->SetFadeOverTime(PPRangeFadeIn<float>(0.7f, 0.0f, 0.0f));
		emitter->SetLifetime(PPRandomRange<float>(0.35f, 0.4f));
		emitter->SetStartDrag(PPConstant<float>(6.0f));
		emitter->SetSpawnVelocityScale(PPConstant<float>(0.0f));
		emitter->SetScaleOverTime(PPRange<float>(1.0f, 0.4f));
		emitter->SetStartVelocity(PPCone(Vector3(0,0,-1), 90.0f, 1.0f, 4.0f));
		emitter->SetStartColor(PPConstant<Color>(color));
		return emitter;
	}

	// Initialize HUD elements/layout
	bool InitHUD()
	{
		if(!m_guiRenderer.Init(g_gl, g_gameWindow))
			return false;

		CheckedLoad(m_fontDivlit = FontRes::Create(g_gl, "fonts/divlit_custom.ttf"));

		m_guiStyle = Ref<CommonGUIStyle>(new CommonGUIStyle(g_application));

		// Gauge
		{
			m_canvas = Utility::MakeRef(new Canvas());
			m_scoringGauge = Utility::MakeRef(new HealthGauge());
			Canvas::Slot* slot = m_canvas->Add(m_scoringGauge.As<GUIElementBase>());
			slot->anchor = Anchor(0.95f, 0.5f);
			slot->alignment = Vector2(1.0f, 0.5f);
			slot->autoSizeX = true;
			slot->autoSizeY = true;
		}

		// Setting bar
		{
			SettingsBar* sb = new SettingsBar(m_guiStyle);
			m_settingsBar = Ref<SettingsBar>(sb);
			sb->AddSetting(&m_camera.zoomBottom, -1.0f, 1.0f, "Bottom Zoom");
			sb->AddSetting(&m_camera.zoomTop, -1.0f, 1.0f, "Top Zoom");
			sb->AddSetting(&m_camera.cameraNearBase, 0.01f, 1.0f, "Camera Near Base");
			sb->AddSetting(&m_camera.cameraNearMult, 0.0f, 2.0f, "Camera Near Mult");
			sb->AddSetting(&m_camera.cameraHeightBase, 0.01f, 1.0f, "Camera Height Base");
			sb->AddSetting(&m_camera.cameraHeightMult, 0.0f, 2.0f, "Camera Height Mult");
			m_settingsBar->SetShow(false);

			Canvas::Slot* settingsSlot = m_canvas->Add(sb->MakeShared());
			settingsSlot->anchor = Anchor(0.75f, 0.0f, 1.0f, 1.0f);
			settingsSlot->autoSizeX = false;
			settingsSlot->autoSizeY = false;
			settingsSlot->SetZOrder(2);
		}

		// Score
		{
			Panel* scorePanel = new Panel();
			scorePanel->texture = g_application->LoadTexture("scoring_base.png");
			scorePanel->color = Color::White;

			Canvas::Slot* scoreSlot = m_canvas->Add(scorePanel->MakeShared());
			scoreSlot->anchor = Anchors::TopsRight;
			scoreSlot->alignment = Vector2(1.0f, 0.0f);
			scoreSlot->autoSizeX = true;
			scoreSlot->autoSizeY = true;

			m_scoreText = Ref<Label>(new Label());
			m_scoreText->SetFontSize(75);
			m_scoreText->SetText(L"0");
			m_scoreText->SetFont(m_fontDivlit);
			m_scoreText->SetTextOptions(FontRes::Monospace);
			// Padding for this specific font
			Margin textPadding = Margin(0, -20, 0, 0);

			Panel::Slot* slot = scorePanel->SetContent(m_scoreText.As<GUIElementBase>());
			slot->padding = Margin(30, 0, 30, 30) + textPadding;
			slot->alignment = Vector2(1.0f, 0.5f);
		}

		return true;
	}

	// Main GUI/HUD Rendering loop
	virtual void RenderDebugHUD(float deltaTime)
	{
		// Render debug overlay elements
		RenderQueue& debugRq = m_guiRenderer.Begin();
		auto RenderText = [&](const String& text, const Vector2& pos, const Color& color = Color::White)
		{
			return m_guiRenderer.RenderText(text, pos, color);
		};

		const BeatmapSettings& bms = m_beatmap->GetMapSettings();
		const TimingPoint& tp = m_playback.GetCurrentTimingPoint();
		Vector2 textPos = Vector2(10, 10);
		textPos.y += RenderText(bms.title, textPos).y;
		textPos.y += RenderText(bms.artist, textPos).y;
		textPos.y += RenderText(Utility::Sprintf("UpdateTime: %.2f FPS", g_application->GetUpdateFPS()), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("RenderTime: %.2f FPS", g_application->GetRenderFPS()), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("Audio Offset: %d ms", g_audio->audioLatency), textPos).y;

		float currentBPM = (float)(60000.0 / tp.beatDuration);
		textPos.y += RenderText(Utility::Sprintf("BPM: %.1f", currentBPM), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("Time Signature: %d/4", tp.numerator), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("Laser Effect Mix: %f", m_audioPlayback.GetLaserEffectMix()), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("Laser Filter Input: %f", m_scoring.GetLaserOutput()), textPos).y;

		textPos.y += RenderText(Utility::Sprintf("Score: %d (Max: %d)", m_scoring.currentHitScore, m_scoring.totalMaxScore), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("Actual Score: %d", m_scoring.CalculateCurrentScore()), textPos).y;

		textPos.y += RenderText(Utility::Sprintf("Health Gauge: %f", m_scoring.currentGauge), textPos).y;

		textPos.y += RenderText(Utility::Sprintf("Roll: %f(x%f) %s",
			m_camera.GetRoll(), m_rollIntensity, m_camera.rollKeep ? "[Keep]" : ""), textPos).y;

		textPos.y += RenderText(Utility::Sprintf("Track Zoom Top: %f", m_camera.zoomTop), textPos).y;
		textPos.y += RenderText(Utility::Sprintf("Track Zoom Bottom: %f", m_camera.zoomBottom), textPos).y;

		if(m_scoring.autoplay)
			textPos.y += RenderText("Autoplay enabled", textPos, Color::Blue).y;

		// List recent hits and their delay
		Vector2 tableStart = textPos;
		uint32 hitsShown = 0;
		// Show all hit debug info on screen (up to a maximum)
		for(auto it = m_scoring.hitStats.rbegin(); it != m_scoring.hitStats.rend(); it++)
		{
			if(hitsShown++ > 16) // Max of 16 entries to display
				break;


			static Color hitColors[] = {
				Color::Red,
				Color::Yellow,
				Color::Green,
			};
			Color c = hitColors[(size_t)(*it)->rating];
			if((*it)->hasMissed && (*it)->hold > 0)
				c = Color(1, 0.65f, 0);
			String text;

			MultiObjectState* obj = *(*it)->object;
			if(obj->type == ObjectType::Single)
			{
				text = Utility::Sprintf("[%d] %d", obj->button.index, (*it)->delta);
			}
			else if(obj->type == ObjectType::Hold)
			{
				text = Utility::Sprintf("Hold [%d] [%d/%d]", obj->button.index, (*it)->hold, (*it)->holdMax);
			}
			else if(obj->type == ObjectType::Laser)
			{
				text = Utility::Sprintf("Laser [%d] [%d/%d]", obj->laser.index, (*it)->hold, (*it)->holdMax);
			}
			textPos.y += RenderText(text, textPos, c).y;
		}

		m_guiRenderer.End();
	}
	virtual void RenderHUD(float deltaTime)
	{
		// Render main canvas
		Rect viewport(Vector2(), g_gameWindow->GetWindowSize());
		m_guiRenderer.Render(deltaTime, viewport, m_canvas.As<GUIElementBase>());

		// Render debug hud if enabled
		if(m_renderDebugHUD)
		{
			RenderDebugHUD(deltaTime);
		}
	}

	void OnLaserSlamHit(LaserObjectState* object)
	{
		CameraShake shake(0.2f, 0.5f, 170.0f);
		shake.amplitude = Vector3(0.02f, 0.01f, 0.0f); // Mainly x-axis
		m_camera.AddCameraShake(shake);
		m_slamSample->Play();

		float dir = Math::Sign(object->points[1] - object->points[0]);
		float laserPos = m_track->trackWidth * object->points[1] - m_track->trackWidth * 0.5f;
		Ref<ParticleEmitter> ex = CreateExplosionEmitter(m_track->laserColors[object->index], Vector3(dir, 0, 0));
		ex->position = Vector3(laserPos, 0.0f, -0.05f);
	}
	void OnButtonHit(Input::Button button, ScoreHitRating rating, ObjectState* hitObject)
	{
		uint32 buttonIdx = (uint32)button;
		Color c = m_track->hitColors[(size_t)rating];

		// The color effect in the button lane
		m_track->AddEffect(new ButtonHitEffect(buttonIdx, c));

		if(rating != ScoreHitRating::Idle)
		{
			// Floating text effect
			m_track->AddEffect(new ButtonHitRatingEffect(buttonIdx, rating));

			// Create hit effect particle
			Color hitColor = (buttonIdx < 4) ? Color::White : Color::FromHSV(20, 0.7f, 1.0f);
			float hitWidth = (buttonIdx < 4) ? m_track->buttonWidth : m_track->fxbuttonWidth;
			Ref<ParticleEmitter> emitter = CreateHitEmitter(hitColor, hitWidth);
			emitter->position.x = GetButtonPlacement(buttonIdx);
			emitter->position.z = -0.05f;
			emitter->position.y = 0.0f;
		}

	}
	void OnButtonMiss(Input::Button button)
	{
		uint32 buttonIdx = (uint32)button;
		m_track->AddEffect(new ButtonHitRatingEffect(buttonIdx, ScoreHitRating::Miss));
	}
	void OnComboChanged(uint32 newCombo)
	{
		m_comboAnimation.Restart();
	}
	void OnScoreChanged(uint32 newScore)
	{
		// Update score text
		if(m_scoreText)
		{
			m_scoreText->SetText(Utility::WSprintf(L"%d", newScore));
		}
	}

	// These functions control if FX button DSP's are muted or not
	void OnObjectHold(Input::Button, ObjectState* object)
	{
		if(object->type == ObjectType::Hold)
		{
			HoldObjectState* hold = (HoldObjectState*)object;
			if(hold->effectType != EffectType::None)
			{
				m_audioPlayback.SetEffectEnabled(hold->index - 4, true);
			}
		}
	}
	void OnObjectReleased(Input::Button, ObjectState* object)
	{
		if(object->type == ObjectType::Hold)
		{
			HoldObjectState* hold = (HoldObjectState*)object;
			if(hold->effectType != EffectType::None)
			{
				m_audioPlayback.SetEffectEnabled(hold->index - 4, false);
			}
		}
	}

	void OnEventChanged(EventKey key, EventData data)
	{
		if(key == EventKey::LaserEffectType)
		{
			m_audioPlayback.SetLaserEffect(data.effectVal);
		}
		else if(key == EventKey::LaserEffectMix)
		{
			m_audioPlayback.SetLaserEffectMix(data.floatVal);
		}
		else if(key == EventKey::TrackRollBehaviour)
		{
			m_camera.rollKeep = (data.rollVal & TrackRollBehaviour::Keep) == TrackRollBehaviour::Keep;
			int32 i = (uint8)data.rollVal & 0x3;
			if(i == 0)
				m_rollIntensity = 0;
			else
			{
				m_rollIntensity = m_rollIntensityBase + (float)(i - 1) * 0.03f;
			}
		}
		else if(key == EventKey::SlamVolume)
		{
			m_slamSample->SetVolume(data.floatVal);
		}
	}

	// These functions register / remove DSP's for the effect buttons
	// the actual hearability of these is toggled in the tick by wheneter the buttons are held down
	void OnFXBegin(HoldObjectState* object)
	{
		assert(object->index >= 4 && object->index <= 5);
		m_audioPlayback.SetEffect(object->index - 4, object, m_playback);
	}
	void OnFXEnd(HoldObjectState* object)
	{
		assert(object->index >= 4 && object->index <= 5);
		m_audioPlayback.ClearEffect(object->index - 4);
	}

	// Loads sound effects
	bool InitSFX()
	{
		CheckedLoad(m_slamSample = g_application->LoadSample("laser_slam"));
		CheckedLoad(m_clickSamples[0] = g_application->LoadSample("click-01"));
		CheckedLoad(m_clickSamples[1] = g_application->LoadSample("click-02"));
		return true;
	}
	bool InitGameplay()
	{
		// Select the correct first object to set the intial playback position
		// if it starts before a certain time frame, the song starts at a negative time (lead-in)
		ObjectState *const* firstObj = &m_beatmap->GetLinearObjects().front();
		while((*firstObj)->type == ObjectType::Event && firstObj != &m_beatmap->GetLinearObjects().back())
		{
			firstObj++;
		}
		MapTime firstObjectTime = (*firstObj)->time;
		if(firstObjectTime < 1000)
		{
			// Set start time
			m_lastMapTime = firstObjectTime - 1000;
			m_audioPlayback.SetPosition(m_lastMapTime);
		}

		// Load audio offset
		Variant* offsetSetting = g_mainConfig.Get("offset");
		if(offsetSetting)
            m_audioOffset = offsetSetting->ToInt();
        g_mainConfig.Add("offset", Variant::Create(m_audioOffset));

		// Playback and timing
		m_playback = BeatmapPlayback(*m_beatmap);
		m_playback.OnEventChanged.Add(this, &Game_Impl::OnEventChanged);
		m_playback.OnFXBegin.Add(this, &Game_Impl::OnFXBegin);
		m_playback.OnFXEnd.Add(this, &Game_Impl::OnFXEnd);
		if(!m_playback.Reset(m_lastMapTime)) // Initialize
			return false;

		m_playback.hittableObjectTreshold = Scoring::goodHitTime;

		// Initialize game input
		m_input.Init(*g_gameWindow);

		m_scoring.SetPlayback(m_playback);
		m_scoring.SetInput(&m_input);
		m_scoring.OnButtonMiss.Add(this, &Game_Impl::OnButtonMiss);
		m_scoring.OnLaserSlamHit.Add(this, &Game_Impl::OnLaserSlamHit);
		m_scoring.OnButtonHit.Add(this, &Game_Impl::OnButtonHit);
		m_scoring.OnComboChanged.Add(this, &Game_Impl::OnComboChanged);
		m_scoring.OnObjectHold.Add(this, &Game_Impl::OnObjectHold);
		m_scoring.OnObjectReleased.Add(this, &Game_Impl::OnObjectReleased);
		m_scoring.OnScoreChanged.Add(this, &Game_Impl::OnScoreChanged);
		m_scoring.Reset(); // Initialize

		return true;
	}
	// Processes input and Updates scoring, also handles audio timing management
	void TickGameplay(float deltaTime)
	{
		if(!m_started)
		{
			// Start playback of audio in first gameplay tick
			m_audioPlayback.Play();
			m_started = true;

			if(g_application->GetAppCommandLine().Contains("-autoskip"))
			{
				SkipIntro();
			}

			// Disable framerate limiting while playing
			g_application->SetFrameLimiter(-1);
		}

		const BeatmapSettings& beatmapSettings = m_beatmap->GetMapSettings();

		// Update beatmap playback
		MapTime playbackPositionMs = m_audioPlayback.GetPosition() - m_audioOffset;
		m_playback.Update(playbackPositionMs);

		MapTime delta = playbackPositionMs - m_lastMapTime;
		int32 beatStart = 0;
		uint32 numBeats = m_playback.CountBeats(m_lastMapTime, delta, beatStart, 1);
		if(numBeats > 0)
		{
			// Click Track
			//uint32 beat = beatStart % m_playback.GetCurrentTimingPoint().measure;
			//if(beat == 0)
			//{
			//	m_clickSamples[0]->Play();
			//}
			//else
			//{
			//	m_clickSamples[1]->Play();
			//}
		}

		/// #Scoring
		// Update music filter states
		m_audioPlayback.SetLaserFilterInput(m_scoring.GetLaserOutput(), m_scoring.IsLaserHeld(0, false) || m_scoring.IsLaserHeld(1, false));
		m_audioPlayback.Tick(m_playback, deltaTime);

		// Link FX track to combo counter for now
		m_audioPlayback.SetFXTrackEnabled(m_scoring.currentComboCounter > 0);

		// Update scoring
		m_scoring.Tick(deltaTime);

		// Update scoring gauge
		m_scoringGauge->rate = m_scoring.currentGauge;

		// Get the current timing point
		m_currentTiming = &m_playback.GetCurrentTimingPoint();

		// Get objects in range
		MapTime msViewRange = m_playback.BarDistanceToDuration(m_track->viewRange);
		m_currentObjectSet = m_playback.GetObjectsInRange(msViewRange);

		m_lastMapTime = playbackPositionMs;
	}
	// SKips ahead to the right before the first object in the map
	void SkipIntro()
	{
		ObjectState *const* firstObj = &m_beatmap->GetLinearObjects().front();
		while((*firstObj)->type == ObjectType::Event && firstObj != &m_beatmap->GetLinearObjects().back())
		{
			firstObj++;
		}
		MapTime skipTime = (*firstObj)->time - 1000;
		if(skipTime > m_lastMapTime)
		{
			m_audioPlayback.SetPosition(skipTime);
		}
	}
	virtual bool IsPlaying() const override
	{
		return m_playing;
	}

	virtual void OnKeyPressed(Key key) override
	{
		if(key == Key::Pause)
		{
			m_audioPlayback.TogglePause();
			if(m_audioPlayback.IsPaused())
			{
				g_application->SetFrameLimiter(60);
			}
			else
			{
				g_application->SetFrameLimiter(-1);
			}
		}
		else if(key == Key::Return) // Skip intro
		{
			SkipIntro();
		}
		else if(key == Key::PageUp)
		{
			m_audioPlayback.Advance(5000);
		}
		else if(key == Key::Escape)
		{
			g_application->CleanupGame();
		}
		else if(key == Key::F5) // Restart map
		{
			g_application->CleanupGame();
			if(!g_application->LaunchMap(g_application->GetCurrentMapPath()))
			{
				g_application->Shutdown();
			}
		}
		else if(key == Key::Tab)
		{
			m_settingsBar->SetShow(!m_settingsBar->IsShown());
		}
	}

	virtual class Track& GetTrack() override
	{
		return *m_track;
	}
	virtual class Camera& GetCamera() override
	{
		return m_camera;
	}
	virtual class BeatmapPlayback& GetPlayback() override
	{
		return m_playback;
	}
	virtual class Scoring& GetScoring() override
	{
		return m_scoring;
	}

};

Game* Game::Create(Beatmap* map, String mapPath)
{
	Game_Impl* impl = new Game_Impl();
	if(!impl->Init(map, mapPath))
	{
		Log("Game initialization failed", Logger::Error);
		delete impl;
		return nullptr;
	}
	return impl;
}

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

class Game_Impl : public Game
{
	String m_mapPath;
	Beatmap* m_beatmap;
	bool m_playing = true;
	bool m_started = false;
	bool m_paused = false;

	// Texture of the map jacket image, if available
	Texture m_jacketTexture;

	// Scoring system object
	Scoring m_scoring;
	// Beatmap playback manager (object and timing point selector)
	BeatmapPlayback m_playback;
	// Audio playback manager (music and FX))
	AudioPlayback m_audioPlayback;

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
		m_camera.zoomBottom = m_playback.GetZoom(0);
		m_camera.zoomTop = m_playback.GetZoom(1);
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
		for(auto& object : m_currentObjectSet)
		{
			/// #Scoring
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
					m_holdEmitters[i]->position.y = 0.1f;
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
		emitter->SetStartPosition(PPBox({ 0.5f, 0.1f, 0.0f }));
		emitter->SetStartSize(PPRandomRange<float>(0.25f, 0.4f));
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
		emitter->SetStartPosition(PPBox({ width * 1.1f, 0.1f, 0.0f }));
		emitter->SetStartSize(PPRandomRange<float>(0.5f, 0.6f));
		emitter->SetFadeOverTime(PPRangeFadeIn<float>(1.0f, 0.0f, 0.4f));
		emitter->SetLifetime(PPRandomRange<float>(0.17f, 0.2f));
		emitter->SetStartDrag(PPConstant<float>(0.0f));
		emitter->SetStartVelocity(PPConstant<Vector3>({ 0,-10.0f,0.5f }));
		emitter->SetSpawnVelocityScale(PPRandomRange<float>(0.9f, 2));
		emitter->SetStartColor(PPConstant<Color>(color*0.6f));
		emitter->SetGravity(PPConstant<Vector3>(Vector3(0.0f, 0.0f, -1.81f)));
		emitter->position.y = 0.2f;
		emitter->scale = 0.4f;
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
		emitter->scale = 0.3f;
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
		emitter->SetStartPosition(PPBox(Vector3(width * 0.5f, 0.1f, 0)));
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

	// Draws HUD and debug overlay text
	Font font;
	Material fontMaterial;
	Mesh guiQuad;
	Material guiTextureMaterial;
	Material guiColorMaterial;
	bool InitHUD()
	{
		CheckedLoad(font = FontRes::Create(g_gl, "fonts/segoeui.ttf"));
		CheckedLoad(fontMaterial = g_application->LoadMaterial("font"));
		fontMaterial->opaque = false;
		CheckedLoad(guiTextureMaterial = g_application->LoadMaterial("guiTex"));
		guiTextureMaterial->opaque = false;
		CheckedLoad(guiColorMaterial = g_application->LoadMaterial("guiColor"));
		guiColorMaterial->opaque = false;
		guiQuad = MeshGenerators::Quad(g_gl, Vector2(0, 0), Vector2(1, 1));
		return true;
	}
	
	// Draws text, returns the size of the drawn text
	Vector2i RenderText(RenderQueue& rq, const String& str, const Vector2& position, const Color& color = Color(1.0f), uint32 fontSize = 16)
	{
		return RenderText(rq, Utility::ConvertToUnicode(str), position, color, fontSize);
	}
	Vector2i RenderText(RenderQueue& rq, const WString& str, const Vector2& position, const Color& color = Color(1.0f), uint32 fontSize = 16)
	{
		Text text = font->CreateText(str, fontSize);
		Transform textTransform;
		textTransform *= Transform::Translation(position);
		MaterialParameterSet params;
		params.SetParameter("color", color);
		rq.Draw(textTransform, text, fontMaterial, params);
		return text->size;
	}
	// Draws a rectangle, either with a texture or just a color
	void RenderRect(RenderQueue& rq, const Rect& rect, const Color& color = Color(1.0f), Texture texture = Texture())
	{
		Transform textTransform;
		textTransform *= Transform::Translation(rect.pos);
		textTransform *= Transform::Scale(Vector3(rect.size.x, rect.size.y, 1.0f));
		MaterialParameterSet params;
		params.SetParameter("color", color);
		if(texture)
		{
			params.SetParameter("mainTex", texture);
			rq.Draw(textTransform, guiQuad, guiTextureMaterial, params);
		}
		else
		{
			rq.Draw(textTransform, guiQuad, guiColorMaterial, params);
		}
	}
	// Main GUI/HUD Rendering loop
	virtual void RenderHUD(float DeltaTime)
	{
		RenderState guiRs;
		guiRs.projectionTransform = g_application->GetGUIProjection();
		guiRs.aspectRatio = g_aspectRatio;
		guiRs.viewportSize = g_resolution;

		RenderQueue guiRq(g_gl, guiRs);

		// Draw the jacket image
		Rect jrect = Rect(Vector2(10.0f), Vector2(100.0f));
		RenderRect(guiRq, jrect, Color::White);
		jrect = jrect.Offset(-2.0f);
		RenderRect(guiRq, jrect, Color::White, m_jacketTexture);

		const BeatmapSettings& bms = m_beatmap->GetMapSettings();
		const TimingPoint& tp = m_playback.GetCurrentTimingPoint();
		Vector2 textPos = Vector2(jrect.pos.x, jrect.Bottom() + 10.0f);
		textPos.y += RenderText(guiRq, bms.title, textPos).y;
		textPos.y += RenderText(guiRq, bms.artist, textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("RenderTime: %.2f ms", DeltaTime * 1000.0f), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Audio Offset: %d ms", g_audio->audioLatency), textPos).y;

		float currentBPM = (float)(60000.0 / tp.beatDuration);
		textPos.y += RenderText(guiRq, Utility::Sprintf("BPM: %.1f", currentBPM), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Time Signature: %d/4", tp.numerator), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Laser Effect Mix: %f", m_audioPlayback.GetLaserEffectMix()), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Laser Filter Input: %f", m_scoring.GetLaserOutput()), textPos).y;
		
		textPos.y += RenderText(guiRq, Utility::Sprintf("Score: %d (Max: %d)", m_scoring.currentHitScore, m_scoring.totalMaxScore), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Actual Score: %d", m_scoring.CalculateCurrentScore()), textPos).y;

		textPos.y += RenderText(guiRq, Utility::Sprintf("Health Gauge: %f", m_scoring.currentGauge), textPos).y;

		textPos.y += RenderText(guiRq, Utility::Sprintf("Roll: %f(x%f) %s", 
			m_camera.GetRoll(), m_rollIntensity, m_camera.rollKeep ? "[Keep]" : ""), textPos).y;

		textPos.y += RenderText(guiRq, Utility::Sprintf("Track Zoom Top: %f", m_camera.zoomTop), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Track Zoom Bottom: %f", m_camera.zoomBottom), textPos).y;

		if(m_scoring.autoplay)
			textPos.y += RenderText(guiRq, "Autoplay enabled", textPos, Color::Blue).y;

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
			textPos.y += RenderText(guiRq, text, textPos, c).y;
		}

		glCullFace(GL_FRONT); // Flipped culling mode for GUI
		guiRq.Process();
		glCullFace(GL_BACK);
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
		ex->position = Vector3(laserPos, 0.5f, -0.1f);
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
			emitter->position.z = -0.1f;
			emitter->position.y = 0.2f;
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
		// Playback and timing
		m_playback = BeatmapPlayback(*m_beatmap);
		m_playback.OnEventChanged.Add(this, &Game_Impl::OnEventChanged);
		m_playback.OnFXBegin.Add(this, &Game_Impl::OnFXBegin);
		m_playback.OnFXEnd.Add(this, &Game_Impl::OnFXEnd);
		if(!m_playback.Reset()) // Initialize
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
		m_scoring.Reset(); // Initialize

		// Autoplay enabled?
		if(g_application->GetAppCommandLine().Contains("-autoplay"))
		{
			m_scoring.autoplay = true;
		}

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
		}

		const BeatmapSettings& beatmapSettings = m_beatmap->GetMapSettings();

		// Update beatmap playback
		MapTime playbackPositionMs = m_audioPlayback.GetPosition();

		// Apply offset correction and clamp to 0->
		if(playbackPositionMs < g_audio->audioLatency)
			playbackPositionMs = 0;
		else
			playbackPositionMs -= (MapTime)g_audio->audioLatency;
		if(playbackPositionMs > 0)
			m_playback.Update(playbackPositionMs);

		MapTime delta = playbackPositionMs - m_lastMapTime;
		uint32 beatStart = 0;
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
		m_audioPlayback.SetLaserFilterInput(m_scoring.GetLaserOutput(), m_scoring.IsLaserHeld(0) || m_scoring.IsLaserHeld(1));
		m_audioPlayback.Tick(m_playback, deltaTime);

		// Link FX track to combo counter for now
		m_audioPlayback.SetFXTrackEnabled(m_scoring.currentComboCounter > 0);

		// Update scoring
		m_scoring.Tick(deltaTime);

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
		}
		else if(key == Key::Return) // Skip intro
		{
			SkipIntro();
		}
		else if(key == Key::PageUp)
		{
			m_audioPlayback.Advance(5000);
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
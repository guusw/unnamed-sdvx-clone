#include "stdafx.h"
#include "Game.hpp"
#include "Application.hpp"
#include "BeatmapPlayback.hpp"
#include "Image.hpp"
#include "MeshGenerators.hpp"
#include "Profiling.hpp"
#include "Framebuffer.hpp"
#include "OpenGL.hpp"
#include "Font.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "bass.h"

class Game_Impl : public Game
{
	String m_mapPath;
	Beatmap* m_beatmap;
	HCHANNEL m_audio;
	bool m_playing = true;
	bool m_started = false;
	bool m_paused = false;

	// Texture of the map jacket image, if available
	Texture m_jacketTexture;

	// Scoring system object
	Scoring m_scoring;
	// Beatmap playback manager (object and timing point selector)
	BeatmapPlayback m_playback;

	// The play field
	Track* m_track;

	// The camera watching the playfield
	Camera m_camera;

	// Current background visualization
	Background* m_background;

	// Currently active timing point
	const TimingPoint* m_currentTiming;
	// Currently visible gameplay objects
	Vector<ObjectState*> m_currentObjectSet;
	MapTime m_lastMapTime;

public:

	~Game_Impl()
	{
		if(m_track)
			delete m_track;
		if(m_background)
			delete m_background;
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
		String audioPath = mapPath + "\\" + mapSettings.audioFX;
		if(!Path::FileExists(audioPath) || Path::IsDirectory(audioPath))
			audioPath = mapPath + "\\" + mapSettings.audioNoFX;
		WString audioPathUnicode = Utility::ConvertToUnicode(audioPath);
		m_audio = BASS_StreamCreateFile(false, *audioPathUnicode, 0, 0, BASS_UNICODE);
		if(m_audio == 0)
		{
			int32 bassError = BASS_ErrorGetCode();
			Logf("Failed to load audio for beatmap \"%s\" [%d]", Logger::Error, audioPath, bassError);
			return false;
		}

		if(!InitGameplay())
			return false;

		// Intialize track graphics
		m_track = new Track();
		if(!m_track->Init())
		{
			return false;
		}

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
		float rollA = m_scoring.GetActiveLaserRoll(0);
		float rollB = m_scoring.GetActiveLaserRoll(1);
		m_camera.SetTargetRoll((rollA + rollB) * 0.05f);
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
			m_track->DrawObjectState(renderQueue, m_playback, object, m_scoring.IsActive(object));
		}

		// Use new camera for scoring overlay
		//	this is because otherwise some of the scoring elements would get clipped to 
		//	the track's near and far planes
		rs = m_camera.CreateRenderState(false);
		RenderQueue scoringRq(g_gl, rs);

		// Copy laser positions
		memcpy(m_track->laserPositions, m_scoring.laserPositions, sizeof(float) * 2);
		m_track->DrawOverlays(scoringRq);

		// Render queues
		
		renderQueue.Process();
		scoringRq.Process();

		RenderHUD(deltaTime);
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
		textPos.y += RenderText(guiRq, Utility::Sprintf("Combo: %d", m_scoring.currentComboCounter), textPos).y;

		float currentBPM = (float)(60000.0 / tp.beatDuration);
		textPos.y += RenderText(guiRq, Utility::Sprintf("BPM: %.1f", currentBPM), textPos).y;
		textPos.y += RenderText(guiRq, Utility::Sprintf("Time Signature: %d/4", tp.measure), textPos).y;

		if(m_scoring.autoplay)
			textPos.y += RenderText(guiRq, "Autoplay enabled", textPos, Color::Blue).y;

		// List recent hits and their delay
		Vector2 tableStart = textPos;
		uint32 hitsShown = 0;
		for(auto it = m_scoring.hitStats.rbegin(); it != m_scoring.hitStats.rend(); it++)
		{
			if(hitsShown++ > 16)
				break;

			float time = Math::Clamp<float>((m_lastMapTime - it->time) / 3000.0f, 0.0f, 0.5f);

			Color baseColor;
			WString what = (it->delta < 0) ? L"Early" : L"Late ";
			switch(m_scoring.GetHitRatingFromDelta(it->delta))
			{
			case ScoreHitRating::Perfect:
				baseColor = Color::Green;
				break;
			case ScoreHitRating::Good:
				baseColor = Color(1.0f, 0.5f, 0.0f);
				break;
			case ScoreHitRating::Miss:
				baseColor = Color::Red;
				what = L"Miss ";
				break;
			}
			Color c = VectorMath::Lerp(baseColor, Color::Black, time);
			WString text = Utility::WSprintf(L"%s %i", what, it->delta);
			textPos.y += RenderText(guiRq, text, textPos, c).y;
		}

		glCullFace(GL_FRONT); // Flipped culling mode for GUI
		guiRq.Process();
		glCullFace(GL_BACK);
	}

	enum class Button
	{
		BT_0,
		BT_1,
		BT_2,
		BT_3,
		BT_0Alt, // Button alternatives
		BT_1Alt, // Button alternatives
		BT_2Alt, // Button alternatives
		BT_3Alt, // Button alternatives
		FX_0,
		FX_1,
		FX_0Alt, // Button alternatives
		FX_1Alt, // Button alternatives
		LS_0Neg, // Left laser- 
		LS_0Pos, // Left laser+		(|---->)
		LS_1Neg, // Right laser-	(<----|)
		LS_1Pos, // Right laser+
		Length,
	};
	bool buttonStates[(size_t)Button::Length];
	Map<uint32, Button> buttonMap;
	float GetInputLaserDir(uint32 laserIdx)
	{
		size_t base = (size_t)Button::LS_0Neg;
		if(laserIdx == 1)
		{
			base = (size_t)Button::LS_1Neg;
		}
		float r = 0.0f;
		r -= 1.0f * buttonStates[base];
		r += 1.0f * buttonStates[base + 1];
		return r;
	}
	void InitButtonMapping()
	{
		memset(buttonStates, 0, sizeof(buttonStates));

		// Default KShoot mapping
		buttonMap['S'] = Button::BT_0;
		buttonMap['D'] = Button::BT_1;
		buttonMap['K'] = Button::BT_2;
		buttonMap['L'] = Button::BT_3;
		buttonMap['H'] = Button::BT_0Alt;
		buttonMap['J'] = Button::BT_1Alt;
		buttonMap['F'] = Button::BT_2Alt;
		buttonMap['G'] = Button::BT_3Alt;

		buttonMap['C'] = Button::FX_0;
		buttonMap['M'] = Button::FX_1;
		buttonMap['N'] = Button::FX_0Alt;
		buttonMap['V'] = Button::FX_1Alt;

		buttonMap['W'] = Button::LS_0Neg;
		buttonMap['E'] = Button::LS_0Pos;
		buttonMap['O'] = Button::LS_1Neg;
		buttonMap['P'] = Button::LS_1Pos;

	}
	virtual void OnKeyPressed(uint8 key) override
	{
		if(key == VK_PAUSE)
		{
			if(m_paused)
			{
				BASS_ChannelPlay(m_audio, false);
			}
			else
			{
				BASS_ChannelPause(m_audio);
			}
			m_paused = !m_paused;
		}
		else if(key == VK_RETURN) // Skip intro
		{
			SkipIntro();
		}
		else if(key == VK_PRIOR)
		{
			QWORD bytePos = BASS_ChannelGetPosition(m_audio, BASS_POS_BYTE);
			double playbackPosition = BASS_ChannelBytes2Seconds(m_audio, bytePos);
			playbackPosition += 5.0f;
			BASS_ChannelSetPosition(m_audio, BASS_ChannelSeconds2Bytes(m_audio, playbackPosition), BASS_POS_BYTE);
		}
		else
		{
			// Handle button mappings
			auto it = buttonMap.find(key);
			if(it != buttonMap.end())
				OnButtonInput(it->second, true);
		}
	}
	virtual void OnKeyReleased(uint8 key) override
	{
		// Handle button mappings
		auto it = buttonMap.find(key);
		if(it != buttonMap.end())
			OnButtonInput(it->second, false);
	}

	// Handle button input for game
	void OnButtonInput(Button b, bool pressed) // Raw input handler
	{
		bool& state = buttonStates[(size_t)b];
		if(state == pressed)
			return; // Nothing changed
		state = pressed; // Store state

		// Ignore game input when autoplay is on
		if(m_scoring.autoplay)
			return;

		if(b >= Button::BT_0 && b <= Button::BT_3Alt)
		{
			OnButtonInput((size_t)b % 4, pressed);
		}
		else if(b >= Button::FX_0 && b <= Button::FX_1Alt)
		{
			OnButtonInput(4 + (size_t)b % 2, pressed);
		}
		else // Update lasers
		{
			m_scoring.laserInput[0] = GetInputLaserDir(0);
			m_scoring.laserInput[1] = GetInputLaserDir(1);
		}
	}
	void OnButtonInput(uint32 buttonIdx, bool pressed)
	{
		if(pressed)
		{
			ObjectState* hitObject = m_scoring.OnButtonPressed(buttonIdx);
			if(!hitObject)
			{
				m_track->AddEffect(new ButtonHitEffect(buttonIdx, m_track->hitColors[0]));
			}
		}
		else
		{
			m_scoring.OnButtonReleased(buttonIdx);
		}
	}
	void OnButtonMiss(uint32 buttonIdx)
	{
		m_track->AddEffect(new ButtonHitRatingEffect(buttonIdx, ScoreHitRating::Miss));
	}
	void OnLaserSlamHit(uint32 laserIndex)
	{
		CameraShake shake(0.2f, 0.5f, 170.0f);
		shake.amplitude = Vector3(0.02f, 0.01f, 0.0f); // Mainly x-axis
		m_camera.AddCameraShake(shake);
	}
	void OnButtonHit(uint32 buttonIdx, ObjectState* hitObject)
	{
		if(hitObject->type == ObjectType::Single)
		{
			MapTime hitDelta = m_scoring.GetObjectHitDelta(hitObject);
			ScoreHitRating rating = m_scoring.GetHitRatingFromDelta(hitDelta);
			Color c = m_track->hitColors[(size_t)rating + 1];
			m_track->AddEffect(new ButtonHitEffect(buttonIdx, c));
			m_track->AddEffect(new ButtonHitRatingEffect(buttonIdx, rating));
		}
	}

	bool InitGameplay()
	{
		// Input
		InitButtonMapping();

		// Playback and timing
		m_playback = BeatmapPlayback(*m_beatmap);
		if(!m_playback.Reset())
			return false;
		m_scoring.SetPlayback(m_playback);
		m_scoring.OnButtonMiss.Add(this, &Game_Impl::OnButtonMiss);
		m_scoring.OnLaserSlamHit.Add(this, &Game_Impl::OnLaserSlamHit);
		m_scoring.OnButtonHit.Add(this, &Game_Impl::OnButtonHit);

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
			BASS_ChannelPlay(m_audio, true);
			m_started = true;

			if(g_application->GetAppCommandLine().Contains("-autoskip"))
			{
				SkipIntro();
			}
		}

		const BeatmapSettings& beatmapSettings = m_beatmap->GetMapSettings();

		// Update beatmap playback
		QWORD bytePos = BASS_ChannelGetPosition(m_audio, BASS_POS_BYTE);
		double playbackPosition = BASS_ChannelBytes2Seconds(m_audio, bytePos);
		MapTime playbackPositionMs = (MapTime)(playbackPosition * 1000.0);

		// Apply offset correction and clamp to 0->
		if(playbackPositionMs < g_audio->audioLatency)
			playbackPositionMs = 0;
		else
			playbackPositionMs += (MapTime)g_audio->audioLatency;
		if(playbackPositionMs > 0)
			m_playback.Update(playbackPositionMs);

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
		ObjectState* firstObj = m_beatmap->GetLinearObjects().front();
		MapTime skipTime = firstObj->time - 1000;
		if(skipTime > m_lastMapTime)
		{
			QWORD dstBytes = BASS_ChannelSeconds2Bytes(m_audio, (double)skipTime / 1000.0);
			BASS_ChannelSetPosition(m_audio, dstBytes, BASS_POS_BYTE);
		}
	}
	virtual bool IsPlaying() const override
	{
		return m_playing;
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
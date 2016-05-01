#include "stdafx.h"
#include "Game.hpp"
#include "Application.hpp"
#include "BeatmapPlayback.hpp"
#include "Image.hpp"
#include "MeshGenerators.hpp"
#include "Profiling.hpp"
#include "LaserTrackBuilder.hpp"
#include "Framebuffer.hpp"
#include "OpenGL.hpp"
#include "Font.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "bass.h"

#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Error, #__stmt); return false; }

// Button hit effect
struct ButtonHitEffect
{
	ButtonHitEffect(uint32 buttonCode, ScoreHitRating rating) : buttonCode(buttonCode), rating(rating){};
	float GetRate() const { return time / duration; }
	static const float duration;
	float time = duration;

	uint32 buttonCode;
	ScoreHitRating rating;
};
const float ButtonHitEffect::duration = 0.5f;

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

	LaserTrackBuilder* m_laserTrackBuilder[2] = { 0 };

	// Visible time elements on the playfield track
	// a single unit is 1 beat in distance
	Vector2 m_trackViewRange;

	// Scoring system object
	//Scoring m_scoring;
	// Beatmap playback manager (object and timing point selector)
	BeatmapPlayback m_playback;

	// Effects
	float m_objectGlow = 0.0f;
	Vector<ButtonHitEffect> m_buttonHitEffects;

	// Currently active timing point
	const TimingPoint* m_currentTiming;
	// Currently visible gameplay objects
	Vector<ObjectState*> m_currentObjectSet;
	MapTime m_lastMapTime;

public:
	// Track positioning
	float trackTickLength;
	float buttonLength;
	float fxbuttonLength;
	const float trackWidth = 1.0f;
	const float trackLength = 25.0f;
	float cameraTilt = 7.0f;
	float cameraHeight = 0.8f;
	// This is due to the viewing angle of the camera, objects should be longer to appear their normal size
	float buttonHeightScale = 5.0f;
	const float buttonWidth = trackWidth / 6;
	const float laserWidth = buttonWidth * 0.8f;
	const float fxbuttonWidth = buttonWidth * 2;
	const float buttonTrackWidth = buttonWidth * 4;

	// Laser color setting
	Color laserColors[2] = {};

	// Calculates the correct height for a given with and a texture as aspect ratio source
	float CalculateTextureHeight(Texture tex, float givenWidth)
	{
		Vector2 size = tex->GetSize();
		float aspect = size.y / size.x;
		return aspect * givenWidth;
	}

	// Logic for drawing the game field
	Mesh trackMesh;
	Mesh trackTickMesh;
	Material trackMaterial;
	Texture trackTexture;
	Texture trackTickTexture;
	void DrawTrack(RenderQueue& rq)
	{
		// Base
		MaterialParameterSet params;
		params.SetParameter("mainTex", trackTexture);
		rq.Draw(Transform(), trackMesh, trackMaterial, params);

		// Draw the main beat ticks on the track
		params.SetParameter("mainTex", trackTickTexture);
		float range = m_trackViewRange.y - m_trackViewRange.x;
		float step = 0.25f;
		float start = m_trackViewRange.x + (step - fmodf(m_trackViewRange.x, step));
		for(float f = start; f < m_trackViewRange.y; f += step)
		{
			float fLocal = (f - m_trackViewRange.x) / range;
			Vector3 tickPosition = Vector3(0.0f, trackLength * fLocal - trackTickLength * 0.5f, 0.01f);
			Transform tickTransform;
			tickTransform *= Transform::Translation(tickPosition);
			tickTransform *= Transform::Scale({ 1.0f, buttonHeightScale, 1.0f });
			rq.Draw(tickTransform, trackTickMesh, trackMaterial, params);
		}
	}

	// Draws a plane over the track
	Material trackOverlay;
	void DrawTrackOverlay(RenderQueue& rq, Texture texture, float heightOffset = 0.05f, float widthScale = 1.0f)
	{
		MaterialParameterSet params;
		params.SetParameter("mainTex", texture);
		Transform transform;
		transform *= Transform::Scale({ widthScale, 1.0f, 1.0f });
		transform *= Transform::Translation({ 0.0f, heightOffset, 0.0f });
		rq.Draw(transform, trackMesh, trackOverlay, params);
	}

	// Button graphics resources
	Mesh buttonMesh;
	Texture buttonTexture;
	Mesh fxbuttonMesh;
	Texture fxbuttonTexture;
	Material fxbuttonMaterial;
	// Laser graphics resources
	Texture laserTexture;
	Material laserMaterial;
	// Main object draw function
	void DrawObjectState(RenderQueue& rq, ObjectState* obj)
	{
		// Calculate height based on time on current track
		float viewRange = m_trackViewRange.y - m_trackViewRange.x;
		float position = m_playback.TimeToBarDistance(obj->time) / viewRange;
		float glow = 0.0f;

		if(obj->type == ObjectType::Single || obj->type == ObjectType::Hold)
		{
			MultiObjectState* mobj = (MultiObjectState*)obj;
			MaterialParameterSet params;
			Material mat;
			Mesh mesh;
			float width;
			float xposition;
			float length;
			if(mobj->button.index < 4) // Normal button
			{
				width = buttonWidth;
				xposition = buttonTrackWidth * -0.5f + width * mobj->button.index;
				length = buttonLength;
				mat = trackMaterial;
				params.SetParameter("mainTex", buttonTexture);
				mesh = buttonMesh;
			}
			else // FX Button
			{
				width = fxbuttonWidth;
				xposition = buttonTrackWidth * -0.5f + fxbuttonWidth *(mobj->button.index -4);
				length = fxbuttonLength;
				mat = fxbuttonMaterial;
				params.SetParameter("mainTex", fxbuttonTexture);
				params.SetParameter("objectGlow", glow);
				mesh = fxbuttonMesh;
			}

			Vector3 buttonPos = Vector3(xposition, trackLength * position, 0.02f);

			Transform buttonTransform;
			buttonTransform *= Transform::Translation(buttonPos);
			float scale = buttonHeightScale;
			if(obj->type == ObjectType::Hold) // Hold Note?
			{
				scale = (m_playback.DurationToBarDistance(mobj->hold.duration) / viewRange) / length  * trackLength;
			}
			buttonTransform *= Transform::Scale({ 1.0f, scale, 1.0f });
			rq.Draw(buttonTransform, mesh, mat, params);
		}
		else // Draw laser
		{
			MaterialParameterSet laserParams;

			/// TODO: Add glow for lasers that are active
			laserParams.SetParameter("objectGlow", glow);
			laserParams.SetParameter("mainTex", laserTexture);
			LaserObjectState* laser = (LaserObjectState*)obj;

			// Get the length of this laser segment
			Transform laserTransform;
			laserTransform *= Transform::Translation(Vector3{ 0.0f, trackLength * position, 0.02f + 0.02f * laser->index });
			laserTransform *= Transform::Scale({ 1.0f, trackLength / viewRange, 1.0f });

			// Set laser color
			laserParams.SetParameter("color", laserColors[laser->index]);

			Mesh laserMesh = m_laserTrackBuilder[laser->index]->GenerateTrackMesh(m_playback, laser);
			if(laserMesh)
			{
				rq.Draw(laserTransform, laserMesh, laserMaterial, laserParams);
			}
		}
	}

	// Draw all the scoring related elements, like the scoring line, hit effects, laser pointers
	Texture scoreBarTexture;
	Texture scoreHitTexture;
	Texture laserPointerTexture;
	Texture scoreHitTextures[3]; // Ok, Miss, Perfect
	Mesh centeredTrackMesh;
	Material spriteMaterial;
	// Draw a centered sprite at pos, relative from the track
	void DrawSprite(RenderQueue& rq, Vector3 pos, Vector2 size, Texture tex, Color color = Color::White, float tilt = 0.0f)
	{
		Transform spriteTransform;
		spriteTransform *= Transform::Translation(pos);
		spriteTransform *= Transform::Scale({ size.x, size.y, 1.0f });
		if(tilt != 0.0f)
			spriteTransform *= Transform::Rotation({ tilt, 0.0f, 0.0f });

		MaterialParameterSet params;
		params.SetParameter("mainTex", tex);
		params.SetParameter("color", color);
		rq.Draw(spriteTransform, centeredTrackMesh, spriteMaterial, params);
	}
	void DrawScoringOverlay(RenderQueue& rq)
	{
		Vector2 barSize = Vector2(trackWidth * 1.4f, 1.0f);
		barSize.y = CalculateTextureHeight(scoreBarTexture, barSize.x);

		DrawSprite(rq, Vector3(0.0f, 0.1f, -0.03f), barSize, scoreBarTexture, Color::White, 4.0f);

		// Draw button hit effect sprites
		for(auto& hfx : m_buttonHitEffects)
		{
			float x = 0.0f;
			float w = buttonWidth;
			Color c = Color::White;
			if(hfx.buttonCode < 4)
			{
				w = buttonWidth;
				x = (-buttonWidth * 1.5f) + w * hfx.buttonCode;
			}
			else
			{
				w = buttonWidth * 2.0f;
				x = -buttonWidth + w * (hfx.buttonCode - 4);
				c = Color::Yellow;
			}

			if(hfx.rating != ScoreHitRating::Miss)
			{
				Vector2 hitEffectSize = Vector2(w * 1.2f, 0.0f);
				hitEffectSize.y = CalculateTextureHeight(scoreHitTexture, hitEffectSize.x) * 4.0f;
				hitEffectSize.y *= hfx.GetRate();
				Color c = Color::White.WithAlpha(hfx.GetRate()*hfx.GetRate());
				DrawSprite(rq, Vector3(x, 0.05f + hitEffectSize.y * 0.5f, -0.03f), hitEffectSize, scoreHitTexture, c);
			}

			{
				Texture hitTexture = scoreHitTextures[(size_t)hfx.rating];
				Vector2 hitEffectSize = Vector2(buttonWidth * 0.5f, 0.0f);
				hitEffectSize.y = CalculateTextureHeight(hitTexture, hitEffectSize.x) * 4.0f;
				hitEffectSize *= (hfx.GetRate() * hfx.GetRate()) + 0.5f;
				Color c = Color::White.WithAlpha(hfx.GetRate());
				DrawSprite(rq, Vector3(x, 0.45f + hitEffectSize.y * 0.5f, -0.03f), hitEffectSize, hitTexture, c, 0.0f);
			}
		}

		// Draw laser pointers
		for(uint32 i = 0; i < 2; i++)
		{
			float pos = i * 1.0f;
			Vector2 objectSize = Vector2(buttonWidth * 0.7f, 0.0f);
			objectSize.y = CalculateTextureHeight(laserPointerTexture, objectSize.x) * 4.0f;
			DrawSprite(rq, Vector3(pos - trackWidth * 0.5f, 0.0f, 0.0f), objectSize, laserPointerTexture, laserColors[i]);
		}
	}

	// Ticks decay of button hit sprites, etc.
	void TickEffects(float deltaTime) 
	{
		// Glow animation decay
		m_objectGlow = m_objectGlow * (1.0f - 2.0f * deltaTime);

		// Button Hit FX
		for(auto it = m_buttonHitEffects.begin(); it != m_buttonHitEffects.end();)
		{
			if((it->time -= deltaTime) <= 0.0f)
			{
				it = m_buttonHitEffects.erase(it);
				continue;
			}
			it++;
		}
	}

	// Set's up the perspective camera for track rendering
	void SetupCamera(RenderState& rs, float viewRangeExtension = 0.0f)
	{
		static const float nearDistBase = 4.0f;
		static const float maxNearPlane = 0.2f;

		Transform cameraTransform;
		float nearDistance = Math::Max(maxNearPlane, nearDistBase - viewRangeExtension);
		float farDistance = nearDistance + trackLength + viewRangeExtension;
		cameraTransform *= Transform::Translation({ 0.0f, -cameraHeight, -nearDistBase });
		cameraTransform *= Transform::Rotation({-90.0f + cameraTilt, 0.0f, 0.0f});

		rs.cameraTransform = cameraTransform;
		rs.projectionTransform = ProjectionMatrix::CreatePerspective(30.0f, g_aspectRatio, nearDistance, farDistance);
	}

	// Processes input and Updates scoring, also handles audio timing management
	/// TODO: Use BPM scale for view range
	const float viewRange = 0.5f;
	void OnButtonHit(ObjectState* obj, uint32 buttonCode)
	{
		ScoreHitRating rating = ScoreHitRating::Miss;
		ButtonHitEffect& bhe = m_buttonHitEffects.Add(ButtonHitEffect(buttonCode, rating));
	}
	void TickGameplay(float deltaTime)
	{
		if(!m_started)
		{
			// Start playback of audio in first gameplay tick
			BASS_ChannelPlay(m_audio, true);
			m_started = true;
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

		if(playbackPositionMs > beatmapSettings.offset)
		{
			playbackPositionMs -= m_beatmap->GetMapSettings().offset;
			m_playback.Update(playbackPositionMs);
			uint32 startBeat = 0;

			// Timing based object glow
			uint32 b = m_playback.CountBeats(m_lastMapTime, playbackPositionMs - m_lastMapTime, startBeat, 2);
			const TimingPoint& timingPoint = m_playback.GetCurrentTimingPoint();
			if(b > 0)
			{
				m_objectGlow = 1.0f;
			}
		}
		else
		{
			playbackPositionMs = 0;
		}

		// Get the current timing point
		m_currentTiming = &m_playback.GetCurrentTimingPoint();

		// Get objects in range
		MapTime msViewRange = m_playback.BarDistanceToDuration(viewRange);
		m_currentObjectSet = m_playback.GetObjectsInRange(msViewRange);

		m_lastMapTime = playbackPositionMs;
	}

	virtual bool IsPlaying() const override
	{
		return m_playing;
	}
	~Game_Impl()
	{
		for(uint32 i = 0; i < 2; i++)
		{
			if(m_laserTrackBuilder[i])
				delete m_laserTrackBuilder[i];
		}
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
		if(!Path::FileExists(audioPath))
			audioPath = mapPath + "\\" + mapSettings.audioNoFX;
		WString audioPathUnicode = Utility::ConvertToUnicode(audioPath);
		m_audio = BASS_StreamCreateFile(false, *audioPathUnicode, 0, 0, BASS_UNICODE);
		if(m_audio == 0)
		{
			int32 bassError = BASS_ErrorGetCode();
			Logf("Failed to load audio for beatmap \"%s\" [%d]", Logger::Error, audioPath, bassError);
			return false;
		}

		// Input
		InitButtonMapping();

		// Playback and timing
		m_playback = BeatmapPlayback(*m_beatmap);
		if(!m_playback.Reset())
			return false;

		// Load laser colors
		Image laserColorPalette;
		CheckedLoad(laserColorPalette = ImageRes::Create("textures/lasercolors.png"));
		assert(laserColorPalette->GetSize().x >= 2);
		laserColors[0] = laserColorPalette->GetBits()[0];
		laserColors[1] = laserColorPalette->GetBits()[1];

		// mip-mapped and anisotropicaly filtered track textures
		CheckedLoad(trackTexture = g_application->LoadTexture("track.png"));
		trackTexture->SetMipmaps(false);
		trackTexture->SetFilter(true, true, 16.0f);
		CheckedLoad(trackTickTexture = g_application->LoadTexture("tick.png"));
		trackTickTexture->SetMipmaps(true);
		trackTickTexture->SetFilter(true, true, 16.0f);
		trackTickTexture->SetWrap(TextureWrap::Repeat, TextureWrap::Clamp);
		trackTickLength = CalculateTextureHeight(trackTickTexture, buttonTrackWidth);

		// Material used for buttons, lines and other simple track elements
		CheckedLoad(trackMaterial = g_application->LoadMaterial("track"));
		trackMaterial->opaque = false;

		// Generate simple planes for the playfield track and elements
		trackMesh = MeshGenerators::Quad(g_gl, Vector2(-trackWidth * 0.5f, 0.0f), Vector2(trackWidth, trackLength));
		trackTickMesh = MeshGenerators::Quad(g_gl, Vector2(-buttonTrackWidth * 0.5f, 0.0f), Vector2(buttonTrackWidth, trackTickLength));
		centeredTrackMesh = MeshGenerators::Quad(g_gl, Vector2(-0.5f, -0.5f), Vector2(1.0f, 1.0f));

		// Scoring texture
		CheckedLoad(scoreBarTexture = g_application->LoadTexture("scorebar.png"));
		CheckedLoad(scoreHitTexture = g_application->LoadTexture("scorehit.png"));
		scoreHitTexture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		CheckedLoad(laserPointerTexture = g_application->LoadTexture("pointer.png"));

		for(uint32 i = 0; i < 3; i++)
		{
			CheckedLoad(scoreHitTextures[i] = g_application->LoadTexture(Utility::Sprintf("score%d.png", i)));
		}

		// Sprite material
		CheckedLoad(spriteMaterial = g_application->LoadMaterial("sprite"));
		spriteMaterial->opaque = false;

		// Load Button object
		CheckedLoad(buttonTexture = g_application->LoadTexture("button.png"));
		buttonTexture->SetMipmaps(true);
		buttonTexture->SetFilter(true, true, 16.0f);
		buttonLength = CalculateTextureHeight(buttonTexture, buttonWidth);
		buttonMesh = MeshGenerators::Quad(g_gl, Vector2(0.0f, 0.0f), Vector2(buttonWidth, buttonLength));

		// Load FX object
		CheckedLoad(fxbuttonTexture = g_application->LoadTexture("fxbutton.png"));
		fxbuttonTexture->SetMipmaps(true);
		fxbuttonTexture->SetFilter(true, true, 16.0f);
		fxbuttonLength = CalculateTextureHeight(buttonTexture, fxbuttonWidth);
		fxbuttonMesh = MeshGenerators::Quad(g_gl, Vector2(0.0f, 0.0f), Vector2(fxbuttonWidth, fxbuttonLength));

		CheckedLoad(fxbuttonMaterial = g_application->LoadMaterial("fxbutton"));
		fxbuttonMaterial->opaque = false;
		fxbuttonMaterial->blendMode = MaterialBlendMode::Additive;

		// Load Laser object
		CheckedLoad(laserTexture = g_application->LoadTexture("laser.png"));
		laserTexture->SetMipmaps(true);
		laserTexture->SetFilter(true, true, 16.0f);

		// Laser object material, allows coloring and sampling laser edge texture
		CheckedLoad(laserMaterial = g_application->LoadMaterial("laser"));
		laserMaterial->blendMode = MaterialBlendMode::Additive;
		laserMaterial->opaque = false;

		// Overlay shader
		CheckedLoad(trackOverlay = g_application->LoadMaterial("overlay"));
		trackOverlay->opaque = false;

		// Create a laser track builder for each laser object
		// these will output and cache meshes for rendering lasers
		for(uint32 i = 0; i < 2; i++)
		{
			m_laserTrackBuilder[i] = new LaserTrackBuilder(g_gl, i, trackWidth, laserWidth);
			m_laserTrackBuilder[i]->laserTextureSize = laserTexture->GetSize();
			m_laserTrackBuilder[i]->laserBorderPixels = 10;
		}

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
		m_trackViewRange = Vector2(m_playback.GetBarTime(), 0.0f);
		m_trackViewRange.y = m_trackViewRange.x + viewRange;

		// Perform laser track cache cleanup, etc.
		for(uint32 i = 0; i < 2; i++)
		{
			/// TODO: Add this back when lasers are fixedto support this
			// Set the length of laser slams based on the approach rate
			//m_laserTrackBuilder[i]->laserSlamHeight = Math::Clamp(buttonWidth / (viewRange / 1.2f), buttonWidth * 0.5f, buttonWidth * 4.0f);
			//m_laserTrackBuilder[i]->laserSlamHeight = buttonWidth;
			m_laserTrackBuilder[i]->Update(m_lastMapTime);
		}

		RenderState rs;
		rs.cameraTransform = Transform::Rotation({ 0.0f, 0.0f, 0.0f });
		rs.viewportSize = g_resolution;
		rs.aspectRatio = g_aspectRatio;

		// Use perspective projected track
		SetupCamera(rs);

		// Main render queue
		RenderQueue renderQueue(g_gl, rs);

		// Draw the base track + time division ticks
		DrawTrack(renderQueue);

		// Draw items on the track
		for(uint32 i = 0; i < 2; i++)
			m_laserTrackBuilder[i]->currentTimingPoint = *m_currentTiming;
		for(auto& object : m_currentObjectSet)
		{
			DrawObjectState(renderQueue, object);
		}

		// Use new camera for scoring overlay
		//	this is because otherwise some of the scoring elements would get clipped to 
		//	the track's near and far planes
		SetupCamera(rs, 5.0f);
		RenderQueue scoringRq(g_gl, rs);
		DrawScoringOverlay(scoringRq);

		// Render queues
		g_gl->SetViewport(rs.viewportSize);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		renderQueue.Process();
		scoringRq.Process();

		RenderHUD(deltaTime);

		TickEffects(deltaTime);
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
		Vector2 textPos = Vector2(jrect.pos.x, jrect.Bottom() + 10.0f);
		textPos.y += RenderText(guiRq, bms.title, textPos).y;
		textPos.y += RenderText(guiRq, bms.artist, textPos).y;

		int32 renderTimeMs = (int32)(DeltaTime * 1000.0f);
		textPos.y += RenderText(guiRq, Utility::WSprintf(L"MapTime: %i", m_lastMapTime), textPos).y;
		textPos.y += RenderText(guiRq, Utility::WSprintf(L"RenderTime: %i", renderTimeMs), textPos).y;

		// List recent hits and their delay
		/*
		Vector2 tableStart = textPos;
		uint32 hitsShown = 0;
		for(auto it = m_scoring.hitStats.rbegin(); it != m_scoring.hitStats.rend(); it++)
		{
			if(hitsShown++ > 16)
				break;

			float time = Math::Clamp<float>((m_lastMapTime - it->time) / 3000.0f, 0.0f, 0.5f);

			Color baseColor;
			WString what = (it->delta < 0) ? L"Early" : L"Late ";
			switch(m_scoring.GetScoreHitRatingFromMs(it->delta))
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
		*/

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
	void OnButtonInput(Button b, bool pressed) // Raw input handler
	{
		bool& state = buttonStates[(size_t)b];
		if(state == pressed)
			return; // Nothing changed

		if(b >= Button::BT_0 && b <= Button::BT_3Alt)
		{
			OnButtonInput((size_t)b % 4, pressed);
		}
		else if(b >= Button::FX_0 && b <= Button::FX_1Alt)
		{
			OnButtonInput(4 + (size_t)b % 2, pressed);
		}
		// else -> ignore laser state changes, those are checked continuously 

		state = pressed; // Store state
	}
	void OnButtonInput(uint32 buttonIdx, bool pressed)
	{
		/// TODO: Send input to score
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
		if(key == VK_RETURN)
		{
			QWORD bytePos = BASS_ChannelGetPosition(m_audio, BASS_POS_BYTE);
			double playbackPosition = BASS_ChannelBytes2Seconds(m_audio, bytePos);
			playbackPosition += 5.0f;
			BASS_ChannelSetPosition(m_audio, BASS_ChannelSeconds2Bytes(m_audio, playbackPosition), BASS_POS_BYTE);
		}

		// Handle button mappings
		auto it = buttonMap.find(key);
		if(it != buttonMap.end())
			OnButtonInput(it->second, true);
	}
	virtual void OnKeyReleased(uint8 key) override
	{
		// Handle button mappings
		auto it = buttonMap.find(key);
		if(it != buttonMap.end())
			OnButtonInput(it->second, false);
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

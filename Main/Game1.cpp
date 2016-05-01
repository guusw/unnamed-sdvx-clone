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
#include "bass.h"

#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Error, #__stmt); return false; }

class Game_Impl : public Game
{
	String m_mapPath;
	Beatmap* m_beatmap;
	HCHANNEL m_audio;
	bool m_playing = true;
	bool m_started = false;

	BeatmapPlayback m_playback;
	LaserTrackBuilder* m_laserTrackBuilder[2] = { 0 };

	// Visible time elements on the playfield track
	// a single unit is 1 beat in distance
	Vector2 m_trackViewRange;

public:
	// Track positioning
	float trackTickLength;
	float buttonLength;
	float fxbuttonLength;
	const float trackWidth = 1.0f;
	const float trackLength = 6.0f;
	Vector3 trackPosition = Vector3(0.0f, -1.0f, -2.0f);
	float trackTilt = -90.0f;
	const float buttonWidth = trackWidth / 6;
	const float fxbuttonWidth = buttonWidth * 2;
	const float buttonTrackWidth = buttonWidth * 4;

	float CalculateTextureHeight(Texture tex, float givenWidth)
	{
		Vector2 size = tex->GetSize();
		float aspect = size.y / size.x;
		return aspect * givenWidth;
	}

	// Track transformation
	Transform trackTransform;

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
		rq.Draw(trackTransform, trackMesh, trackMaterial, params);

		// Draw the main beat ticks on the track
		params.SetParameter("mainTex", trackTickTexture);
		float range = m_trackViewRange.y - m_trackViewRange.x;
		for(float f = ceilf(m_trackViewRange.x); f < m_trackViewRange.y; f += 1.0f)
		{
			float fLocal = (f - m_trackViewRange.x) / range;
			Vector3 tickPosition = Vector3(0.0f, trackLength * fLocal - trackTickLength * 0.5f, 0.01f);
			Transform tickTransform = trackTransform;
			tickTransform *= Transform::Translation(tickPosition);
			rq.Draw(tickTransform, trackTickMesh, trackMaterial, params);
		}
	}

	// Draws a plane over the track
	Material trackOverlay;
	void DrawTrackOverlay(RenderQueue& rq, Texture texture, float heightOffset = 0.05f, float widthScale = 1.0f)
	{
		MaterialParameterSet params;
		params.SetParameter("mainTex", texture);
		Transform transform = trackTransform;
		transform *= Transform::Scale({ widthScale, 1.0f, 1.0f });
		transform *= Transform::Translation({ 0.0f, heightOffset, 0.0f });
		rq.Draw(transform, trackMesh, trackOverlay, params);
	}

	// Button graphics resources
	Mesh buttonMesh;
	Texture buttonTexture;
	Mesh fxbuttonMesh;
	Texture fxbuttonTexture;
	void DrawObjectState(RenderQueue& rq, ObjectState obj)
	{
		// Calculate height based on time on current track
		float viewRange = m_trackViewRange.y - m_trackViewRange.x;
		float position = m_playback.TimeToBarDistance(obj.time) / viewRange;

		// Draw the buttons
		MaterialParameterSet buttonParams;
		buttonParams.SetParameter("mainTex", buttonTexture);
		Vector3 buttonPos = Vector3(0.0f, trackLength * position, 0.02f);
		for(uint32 i = 0; i < 4; i++)
		{
			const ButtonState& button = obj->buttons[i];
			buttonPos.x = buttonTrackWidth * -0.5f + buttonWidth * i;
			if(button.on)
			{
				Transform buttonTransform = trackTransform;
				buttonTransform *= Transform::Translation(buttonPos);
				if(button.duration != -1) // Hold Note?
				{
					float scale = (m_playback.DurationToBarDistance(button.duration) / viewRange) / buttonLength  * trackLength;
					buttonTransform *= Transform::Scale({ 1.0f, scale, 1.0f });
				}
				rq.Draw(buttonTransform, buttonMesh, trackMaterial, buttonParams);
			}
		}

		// Draw fx buttons
		buttonParams.SetParameter("mainTex", fxbuttonTexture);
		for(uint32 i = 0; i < 2; i++)
		{
			const ButtonState& button = obj->buttons[4 + i];

			if(button.on)
			{
				buttonPos.x = buttonTrackWidth * -0.5f + fxbuttonWidth * i;
				Transform buttonTransform = trackTransform;
				buttonTransform *= Transform::Translation(buttonPos);
				if(button.duration != -1) // Hold Note?
				{
					float scale = (m_playback.DurationToBarDistance(button.duration) / viewRange) / fxbuttonLength  * trackLength;
					buttonTransform *= Transform::Scale({ 1.0f, scale, 1.0f });
				}
				rq.Draw(buttonTransform, fxbuttonMesh, trackMaterial, buttonParams);
			}
		}
	}

	// Laser graphics resources
	Texture laserTexture;
	Material laserMaterial;
	void DrawLaserObject(RenderQueue& rq, ObjectState obj)
	{
		// Calculate height based on time on current track
		float viewRange = m_trackViewRange.y - m_trackViewRange.x;
		float position = m_playback.TimeToBarDistance(obj.time) / viewRange;

		// Draw lasers
		Color laserColors[] =
		{
			Color(0.0f, 0.0f, 1.0f, 1.0f),
			Color(1.0f, 0.0f, 0.0f, 1.0f),
		};
		MaterialParameterSet laserParams;
		laserParams.SetParameter("mainTex", laserTexture);
		for(uint32 i = 0; i < 2; i++)
		{
			const LaserState& laser = obj->lasers[i];
			if(laser.duration != 0)
			{
				// Get the length of this laser segment
				float segmentLength = (m_playback.DurationToBarDistance(laser.duration) / viewRange) * trackLength;

				Transform laserTransform = trackTransform;
				laserTransform *= Transform::Translation(Vector3{ 0.0f, trackLength * position, 0.02f + 0.02f * i });

				// Set laser color
				laserParams.SetParameter("color", laserColors[i]);

				Mesh laserMesh = m_laserTrackBuilder[i]->GenerateTrackMesh(obj, segmentLength);
				if(laserMesh)
				{
					rq.Draw(laserTransform, laserMesh, laserMaterial, laserParams);
				}
			}
		}
	}

	virtual bool IsPlaying() const override
	{
		return m_playing;
	}
	virtual bool Init(Beatmap* map, String mapPath) override
	{
		ProfilerScope $("Init Game");

		assert(map);
		m_mapPath = mapPath;
		m_beatmap = map;
		const BeatmapSettings& mapSettings = m_beatmap->GetMapSettings();

		// Load beatmap audio
		String audioPath = mapPath + "\\" + mapSettings.audioNoFX;
		WString audioPathUnicode = Utility::ConvertToUnicode(audioPath);
		m_audio = BASS_StreamCreateFile(false, *audioPathUnicode, 0, 0, BASS_UNICODE);
		if(m_audio == 0)
		{
			int32 bassError = BASS_ErrorGetCode();
			Logf("Failed to load audio for beatmap \"%s\" [%d]", Logger::Error, audioPath, bassError);
			return false;
		}

		m_playback = BeatmapPlayback(*m_beatmap);

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

		// Generate simple planes for the playfield track and elements
		trackMesh = MeshGenerators::Quad(g_gl, Vector2(-trackWidth * 0.5f, 0.0f), Vector2(trackWidth, trackLength));
		trackTickMesh = MeshGenerators::Quad(g_gl, Vector2(-buttonTrackWidth * 0.5f, 0.0f), Vector2(buttonTrackWidth, trackTickLength));

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

		// Load Laser object
		CheckedLoad(laserTexture = g_application->LoadTexture("laser.png"));
		laserTexture->SetMipmaps(false);
		laserTexture->SetFilter(true, true);

		// Laser object material, allows coloring and sampling laser edge texture
		CheckedLoad(laserMaterial = g_application->LoadMaterial("laser"));

		// Overlay shader
		CheckedLoad(trackOverlay = g_application->LoadMaterial("overlay"));

		for(uint32 i = 0; i < 2; i++)
		{
			m_laserTrackBuilder[i] = new LaserTrackBuilder(g_gl, i, trackWidth, buttonWidth);
		}

		CreateRenderBuffers();

		return true;
	}
	~Game_Impl()
	{
		for(uint32 i = 0; i < 2; i++)
		{
			if(m_laserTrackBuilder[i])
				delete m_laserTrackBuilder[i];
		}
	}

	Framebuffer fbLaser;
	Texture fbLaserColor;
	Vector2 laserRenderArea;
	Vector2i laserTextureRes;
	void CreateRenderBuffers()
	{
		fbLaser = FramebufferRes::Create(g_gl);
		fbLaserColor = TextureRes::Create(g_gl);
		laserRenderArea = Vector2(trackWidth, trackLength);
		laserTextureRes.y = g_application->GetResolution().y;
		laserTextureRes.x = (int32)(laserTextureRes.y * (laserRenderArea.x / laserRenderArea.y));
		fbLaserColor->Init(laserTextureRes);
		bool ok = fbLaser->AttachTexture(fbLaserColor);
		assert(ok);
	}

	virtual void Tick(float DeltaTime) override
	{
		if(!m_started)
		{
			BASS_ChannelPlay(m_audio, true);
			m_started = true;
		}

		// Update beatmap playback
		QWORD bytePos = BASS_ChannelGetPosition(m_audio, BASS_POS_BYTE);
		double playbackPosition = BASS_ChannelBytes2Seconds(m_audio, bytePos);
		MapTime playbackPositionMs = (int64)(playbackPosition * 1000.0);
		m_playback.Update(playbackPositionMs);

		// The amount of bars visible on the track at one time
		/// TODO: Use BPM scale
		const float viewRange = 0.5f; 
		m_trackViewRange = Vector2(m_playback.GetBarTime(), 0.0f);
		m_trackViewRange.y = m_trackViewRange.x + viewRange;

		// Perform laser track cache cleanup, etc.
		for(uint32 i = 0; i < 2; i++)
		{
			// Set the length of laser slams based on the approach rate
			//m_laserTrackBuilder[i]->laserSlamHeight = Math::Clamp(buttonWidth / (viewRange / 1.2f), buttonWidth * 0.5f, buttonWidth * 4.0f);
			//m_laserTrackBuilder[i]->laserSlamHeight = buttonWidth;
			m_laserTrackBuilder[i]->Update(playbackPositionMs);
		}

		RenderState rs;
		rs.cameraTransform = Transform::Rotation({ 0.0f, 0.0f, 0.0f });
		rs.viewportSize = g_application->GetResolution();
		rs.aspectRatio = g_aspectRatio;

		// Draw the track
		const TimingPoint& currentTimingPoint = m_playback.GetCurrentTimingPoint();

		// Set the track transform used for a base for all the objects
		const bool usePerspective = false;
		if(usePerspective)
		{
			// Use perspective track
			trackTransform = Transform::Translation(trackPosition);
			trackTransform *= Transform::Rotation({ trackTilt, 0.0f, 0.0f });

			rs.projectionTransform = ProjectionMatrix::CreatePerspective(60.0f, g_aspectRatio, 0.1f, trackLength + 100.0f);
		}
		else
		{
			// Use orthographically projected track
			trackTransform = Transform::Translation(Vector3(0.0f, 0.0f, -1.0f));

			float trackAspect = trackLength / trackWidth;
			Vector2 viewSize;
			{
				viewSize.y = trackLength;
				viewSize.x = trackWidth / g_aspectRatio;
			}
			rs.projectionTransform = ProjectionMatrix::CreateOrthographic(
				-viewSize.x * 0.5f, viewSize.x * 0.5f,
				0.0f, viewSize.y,
				0.0f, 10.0f);
		}

		// RenderState and queue for drawing laser trakcs
		RenderState laserRs;
		{
			laserRs.viewportSize = laserTextureRes;
			laserRs.aspectRatio = g_aspectRatio;
			float wh = laserRenderArea.x * 0.5f;
			laserRs.projectionTransform = ProjectionMatrix::CreateOrthographic(-wh, wh, 0.0f, laserRenderArea.y, 0.0f, 10.0f);
		}
		RenderQueue laserQueue(g_gl, laserRs);

		// Main render queue
		RenderQueue renderQueue(g_gl, rs);

		// Draw the base track + time division ticks
		DrawTrack(renderQueue);

		// Draw items on the track
		Vector<ObjectState> objects = m_playback.GetObjectsInRange(viewRange);
		for(uint32 i = 0; i < 2; i++)
			m_laserTrackBuilder[i]->currentTimingPoint = m_playback.GetCurrentTimingPoint();
		for(auto& object : objects)
		{
			DrawObjectState(renderQueue, object);
			DrawLaserObject(renderQueue, object);
		}

		// Render lasers to texture
		//fbLaser->Bind();
		//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		//glClear(GL_COLOR_BUFFER_BIT);
		////glDisable(GL_BLEND);
		////glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//g_gl->SetViewport(laserTextureRes);
		//laserQueue.Process();
		//fbLaser->Unbind();

		// Render this texture to the track
		//DrawTrackOverlay(renderQueue, fbLaserColor);
		g_gl->SetViewport(rs.viewportSize);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		renderQueue.Process();
	}

	bool m_paused = false;
	virtual void OnKeyPressed(uint8 key) override
	{
		if(key == VK_SPACE)
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
	}
	virtual void OnKeyReleased(uint8 key) override
	{
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

#include "stdafx.h"
#include "Application.hpp"
#include "Game.hpp"
#include "Track.hpp"
#include "OpenGL.hpp"
#include "Image.hpp"
#include "MeshGenerators.hpp"
#include "LaserTrackBuilder.hpp"
#include "RenderQueue.hpp"
#include "BeatmapPlayback.hpp"
#include "BeatmapObjects.hpp"

Track::~Track()
{
	for(uint32 i = 0; i < 2; i++)
	{
		if(m_laserTrackBuilder[i])
			delete m_laserTrackBuilder[i];
	}
}
bool Track::Init()
{
	// Load laser colors
	Image laserColorPalette;
	CheckedLoad(laserColorPalette = ImageRes::Create("textures/lasercolors.png"));
	assert(laserColorPalette->GetSize().x >= 2);
	for(uint32 i = 0; i < 2; i++)
		laserColors[i] = laserColorPalette->GetBits()[i];

	// Load hit effect colors
	Image hitColorPalette;
	CheckedLoad(hitColorPalette = ImageRes::Create("textures/hitcolors.png"));
	assert(hitColorPalette->GetSize().x >= 4);
	for(uint32 i = 0; i < 4; i++)
		hitColors[i] = hitColorPalette->GetBits()[i];

	// mip-mapped and anisotropicaly filtered track textures
	CheckedLoad(trackTexture = g_application->LoadTexture("track.png"));
	trackTexture->SetMipmaps(false);
	trackTexture->SetFilter(true, true, 16.0f);
	CheckedLoad(trackTickTexture = g_application->LoadTexture("tick.png"));
	trackTickTexture->SetMipmaps(true);
	trackTickTexture->SetFilter(true, true, 16.0f);
	trackTickTexture->SetWrap(TextureWrap::Repeat, TextureWrap::Clamp);
	trackTickLength = trackTickTexture->CalculateHeight(buttonTrackWidth);

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
	buttonLength = buttonTexture->CalculateHeight(buttonWidth);
	buttonMesh = MeshGenerators::Quad(g_gl, Vector2(0.0f, 0.0f), Vector2(buttonWidth, buttonLength));

	// Load FX object
	CheckedLoad(fxbuttonTexture = g_application->LoadTexture("fxbutton.png"));
	fxbuttonTexture->SetMipmaps(true);
	fxbuttonTexture->SetFilter(true, true, 16.0f);
	CheckedLoad(fxbuttonHoldTexture = g_application->LoadTexture("fxbuttonhold.png"));
	fxbuttonHoldTexture->SetMipmaps(true);
	fxbuttonHoldTexture->SetFilter(true, true, 16.0f);
	fxbuttonLength = fxbuttonTexture->CalculateHeight(fxbuttonWidth);
	fxbuttonMesh = MeshGenerators::Quad(g_gl, Vector2(0.0f, 0.0f), Vector2(fxbuttonWidth, fxbuttonLength));

	CheckedLoad(holdButtonMaterial = g_application->LoadMaterial("holdbutton"));
	holdButtonMaterial->opaque = false;
	holdButtonMaterial->blendMode = MaterialBlendMode::Additive;

	// Load Laser object
	CheckedLoad(laserTexture = g_application->LoadTexture("laser.png"));
	laserTexture->SetMipmaps(true);
	laserTexture->SetFilter(true, true, 16.0f);
	laserTexture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);

	CheckedLoad(laserTailTextures[0] = g_application->LoadTexture("laser_entry.png"));
	CheckedLoad(laserTailTextures[1] = g_application->LoadTexture("laser_exit.png"));
	for(uint32 i = 0; i < 2; i++)
	{
		laserTailTextures[i]->SetMipmaps(true);
		laserTailTextures[i]->SetFilter(true, true, 16.0f);
		laserTailTextures[i]->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
	}

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
		m_laserTrackBuilder[i]->laserEntryTextureSize = laserTailTextures[0]->GetSize();
		m_laserTrackBuilder[i]->laserExitTextureSize = laserTailTextures[1]->GetSize();
		m_laserTrackBuilder[i]->laserBorderPixels = 12;
		m_laserTrackBuilder[i]->perspectiveHeightScale = perspectiveHeightScale;
		m_laserTrackBuilder[i]->laserLengthScale = trackLength / viewRange;
		m_laserTrackBuilder[i]->Reset(); // Also initializes the track builder
	}

	return true;
}
void Track::Tick(class BeatmapPlayback& playback, float deltaTime)
{
	// Button Hit FX
	for(auto it = m_hitEffects.begin(); it != m_hitEffects.end();)
	{
		(*it)->Tick(deltaTime);
		if((*it)->time <= 0.0f)
		{
			delete *it;
			it = m_hitEffects.erase(it);
			continue;
		}
		it++;
	}

	// Set Object glow
	MapTime currentTime = playback.GetLastTime();
	uint32 startBeat = 0;
	uint32 numBeats = playback.CountBeats(m_lastMapTime, currentTime - m_lastMapTime, startBeat, 4);
	m_lastMapTime = currentTime;
	if(numBeats > 0)
	{
		objectGlow = 1.0f;
	}
	else
	{
		objectGlow -= 4.0f * deltaTime;
		if(objectGlow < 0.0f)
			objectGlow = 0.0f;
	}

	// Perform laser track cache cleanup, etc.
	for(uint32 i = 0; i < 2; i++)
	{
		/// TODO: Add this back when lasers are fixedto support this
		// Set the length of laser slams based on the approach rate
		//m_laserTrackBuilder[i]->laserSlamHeight = Math::Clamp(buttonWidth / (viewRange / 1.2f), buttonWidth * 0.5f, buttonWidth * 4.0f);
		//m_laserTrackBuilder[i]->laserSlamHeight = buttonWidth;
		m_laserTrackBuilder[i]->Update(m_lastMapTime);
	}
}
void Track::DrawBase(class RenderQueue& rq)
{
	// Base
	MaterialParameterSet params;
	params.SetParameter("mainTex", trackTexture);
	rq.Draw(Transform(), trackMesh, trackMaterial, params);

	// Draw the main beat ticks on the track
	params.SetParameter("mainTex", trackTickTexture);
	float range = trackViewRange.y - trackViewRange.x;
	float step = 0.25f;
	float start = trackViewRange.x + (step - fmodf(trackViewRange.x, step));
	for(float f = start; f < trackViewRange.y; f += step)
	{
		float fLocal = (f - trackViewRange.x) / range;
		Vector3 tickPosition = Vector3(0.0f, trackLength * fLocal - trackTickLength * 0.5f, 0.01f);
		Transform tickTransform;
		tickTransform *= Transform::Translation(tickPosition);
		tickTransform *= Transform::Scale({ 1.0f, perspectiveHeightScale, 1.0f });
		rq.Draw(tickTransform, trackTickMesh, trackMaterial, params);
	}
}
void Track::DrawObjectState(RenderQueue& rq, class BeatmapPlayback& playback, ObjectState* obj, bool active)
{
	// Calculate height based on time on current track
	float viewRange = trackViewRange.y - trackViewRange.x;
	float position = playback.TimeToBarDistance(obj->time) / viewRange;
	float glow = 0.0f;

	if(obj->type == ObjectType::Single || obj->type == ObjectType::Hold)
	{
		bool isHold = obj->type == ObjectType::Hold;
		MultiObjectState* mobj = (MultiObjectState*)obj;
		MaterialParameterSet params;
		Material mat = trackMaterial;
		Mesh mesh;
		float width;
		float xposition;
		float length;
		if(mobj->button.index < 4) // Normal button
		{
			width = buttonWidth;
			xposition = buttonTrackWidth * -0.5f + width * mobj->button.index;
			length = buttonLength;
			params.SetParameter("mainTex", buttonTexture);
			mesh = buttonMesh;
		}
		else // FX Button
		{
			width = fxbuttonWidth;
			xposition = buttonTrackWidth * -0.5f + fxbuttonWidth *(mobj->button.index - 4);
			length = fxbuttonLength;
			params.SetParameter("mainTex", isHold ? fxbuttonHoldTexture : fxbuttonTexture);
			mesh = fxbuttonMesh;
		}

		if(isHold)
		{
			mat = holdButtonMaterial;
			params.SetParameter("objectGlow", active ? objectGlow : 0.0f);
		}

		Vector3 buttonPos = Vector3(xposition, trackLength * position, 0.02f);

		Transform buttonTransform;
		buttonTransform *= Transform::Translation(buttonPos);
		float scale = perspectiveHeightScale;
		if(isHold) // Hold Note?
		{
			scale = (playback.DurationToBarDistance(mobj->hold.duration) / viewRange) / length  * trackLength;
		}
		buttonTransform *= Transform::Scale({ 1.0f, scale, 1.0f });
		rq.Draw(buttonTransform, mesh, mat, params);
	}
	else // Draw laser
	{
		LaserObjectState* laser = (LaserObjectState*)obj;

		// Draw segment function
		auto DrawSegment = [&](Mesh mesh, Texture texture)
		{
			MaterialParameterSet laserParams;

			/// TODO: Add glow for lasers that are active
			laserParams.SetParameter("objectGlow", active ? objectGlow : 0.0f);
			laserParams.SetParameter("mainTex", texture);

			// Get the length of this laser segment
			Transform laserTransform;
			laserTransform *= Transform::Translation(Vector3{ 0.0f, trackLength * position, 0.02f + 0.02f * laser->index });

			// Set laser color
			laserParams.SetParameter("color", laserColors[laser->index]);

			if(mesh)
			{
				rq.Draw(laserTransform, mesh, laserMaterial, laserParams);
			}
		};

		// Draw entry?
		if(!laser->prev)
		{
			Mesh laserTail = m_laserTrackBuilder[laser->index]->GenerateTrackEntry(playback, laser);
			DrawSegment(laserTail, laserTailTextures[0]);
		}

		// Body
		Mesh laserMesh = m_laserTrackBuilder[laser->index]->GenerateTrackMesh(playback, laser);
		DrawSegment(laserMesh, laserTexture);

		// Draw exit?
		if(!laser->next)
		{
			Mesh laserTail = m_laserTrackBuilder[laser->index]->GenerateTrackExit(playback, laser);
			DrawSegment(laserTail, laserTailTextures[1]);
		}
	}
}
void Track::DrawOverlays(class RenderQueue& rq)
{
	Vector2 barSize = Vector2(trackWidth * 1.4f, 1.0f);
	barSize.y = scoreBarTexture->CalculateHeight(barSize.x);

	DrawSprite(rq, Vector3(0.0f, 0.1f, -0.03f), barSize, scoreBarTexture, Color::White, 4.0f);

	// Draw button hit effect sprites
	for(auto& hfx : m_hitEffects)
	{
		hfx->Draw(rq);
	}

	// Draw laser pointers
	for(uint32 i = 0; i < 2; i++)
	{
		float pos = laserPositions[i];
		Vector2 objectSize = Vector2(buttonWidth * 0.7f, 0.0f);
		objectSize.y = laserPointerTexture->CalculateHeight(objectSize.x) * perspectiveHeightScale;
		DrawSprite(rq, Vector3(pos - trackWidth * 0.5f, 0.0f, 0.0f), objectSize, laserPointerTexture, laserColors[i]);
	}
}
void Track::DrawTrackOverlay(RenderQueue& rq, Texture texture, float heightOffset /*= 0.05f*/, float widthScale /*= 1.0f*/)
{
	MaterialParameterSet params;
	params.SetParameter("mainTex", texture);
	Transform transform;
	transform *= Transform::Scale({ widthScale, 1.0f, 1.0f });
	transform *= Transform::Translation({ 0.0f, heightOffset, 0.0f });
	rq.Draw(transform, trackMesh, trackOverlay, params);
}
void Track::DrawSprite(RenderQueue& rq, Vector3 pos, Vector2 size, Texture tex, Color color /*= Color::White*/, float tilt /*= 0.0f*/)
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
TimedEffect* Track::AddEffect(TimedEffect* effect)
{
	m_hitEffects.Add(effect);
	effect->track = this;
	return effect;
}
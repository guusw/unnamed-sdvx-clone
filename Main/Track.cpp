#include "stdafx.h"
#include "Application.hpp"
#include "Game.hpp"
#include "Track.hpp"
#include "LaserTrackBuilder.hpp"
#include "BeatmapPlayback.hpp"
#include "BeatmapObjects.hpp"
#include "AsyncAssetLoader.hpp"

const float Track::trackWidth = 1.0f;
const float Track::trackLength = 6.0f;
const float Track::buttonWidth = 1.0f / 6;
const float Track::laserWidth = buttonWidth * 0.7f;
const float Track::fxbuttonWidth = buttonWidth * 2;
const float Track::buttonTrackWidth = buttonWidth * 4;
const float Track::viewRange = .7f;

Track::~Track()
{
	if(loader)
		delete loader;

	for(uint32 i = 0; i < 2; i++)
	{
		if(m_laserTrackBuilder[i])
			delete m_laserTrackBuilder[i];
	}
	for(auto it = m_hitEffects.begin(); it != m_hitEffects.end(); it++)
	{
		delete *it;
	}
}
bool Track::AsyncLoad()
{
	loader = new AsyncAssetLoader();

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
	loader->AddTexture(trackTexture, "track.png");
	loader->AddTexture(trackTickTexture, "tick.png");

	// Scoring texture
	loader->AddTexture(scoreBarTexture, "scorebar.png");
	loader->AddTexture(scoreHitTexture, "scorehit.png");

	loader->AddTexture(laserPointerTexture, "pointer.png"); 

	for(uint32 i = 0; i < 3; i++)
	{
		loader->AddTexture(scoreHitTextures[i], Utility::Sprintf("score%d.png", i));
	}

	// Load Button object
	loader->AddTexture(buttonTexture, "button.png");
	loader->AddTexture(buttonHoldTexture, "buttonhold.png");

	// Load FX object
	loader->AddTexture(fxbuttonTexture, "fxbutton.png");
	loader->AddTexture(fxbuttonHoldTexture, "fxbuttonhold.png");

	// Load Laser object
	loader->AddTexture(laserTexture, "laser.png");

	// Entry and exit textures for laser
	loader->AddTexture(laserTailTextures[0], "laser_entry.png");
	loader->AddTexture(laserTailTextures[1], "laser_exit.png");

	loader->AddTexture(comboSpriteSheet, "combo.png");

	// Track materials
	loader->AddMaterial(trackMaterial, "track");
	loader->AddMaterial(spriteMaterial, "sprite"); // General purpose material
	loader->AddMaterial(holdButtonMaterial, "holdbutton");
	loader->AddMaterial(laserMaterial, "laser");
	loader->AddMaterial(trackOverlay, "overlay");

	return loader->Load();
}
bool Track::AsyncFinalize()
{
	// Finalizer loading textures/material/etc.
	bool success = loader->Finalize();
	delete loader;
	loader = nullptr;

	// Set Texture states
	trackTexture->SetMipmaps(false);
	trackTexture->SetFilter(true, true, 16.0f);
	trackTickTexture->SetMipmaps(true);
	trackTickTexture->SetFilter(true, true, 16.0f);
	trackTickTexture->SetWrap(TextureWrap::Repeat, TextureWrap::Clamp);
	trackTickLength = trackTickTexture->CalculateHeight(buttonTrackWidth);
	scoreHitTexture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);

	buttonTexture->SetMipmaps(true);
	buttonTexture->SetFilter(true, true, 16.0f);
	buttonHoldTexture->SetMipmaps(true);
	buttonHoldTexture->SetFilter(true, true, 16.0f);
	buttonLength = buttonTexture->CalculateHeight(buttonWidth);
	buttonMesh = MeshGenerators::Quad(g_gl, Vector2(0.0f, 0.0f), Vector2(buttonWidth, buttonLength));

	fxbuttonTexture->SetMipmaps(true);
	fxbuttonTexture->SetFilter(true, true, 16.0f);
	fxbuttonHoldTexture->SetMipmaps(true);
	fxbuttonHoldTexture->SetFilter(true, true, 16.0f);
	fxbuttonLength = fxbuttonTexture->CalculateHeight(fxbuttonWidth);
	fxbuttonMesh = MeshGenerators::Quad(g_gl, Vector2(0.0f, 0.0f), Vector2(fxbuttonWidth, fxbuttonLength));

	holdButtonMaterial->opaque = false;
	holdButtonMaterial->blendMode = MaterialBlendMode::Additive;

	laserTexture->SetMipmaps(true);
	laserTexture->SetFilter(true, true, 16.0f);
	laserTexture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);

	for(uint32 i = 0; i < 2; i++)
	{
		laserTailTextures[i]->SetMipmaps(true);
		laserTailTextures[i]->SetFilter(true, true, 16.0f);
		laserTailTextures[i]->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
	}

	// Track and sprite material (all transparent)
	trackMaterial->opaque = false;
	spriteMaterial->opaque = false;

	// Laser object material, allows coloring and sampling laser edge texture
	laserMaterial->blendMode = MaterialBlendMode::Additive;
	laserMaterial->opaque = false;

	// Overlay shader
	trackOverlay->opaque = false;

	// Combo number meshes for the combo sprite sheet
	Vector2i comboFontSize = comboSpriteSheet->GetSize();
	Vector2i comboFontSizePerCharacter = comboFontSize / Vector2i(10, 1);
	Vector2 comboFontTexCoordSize = Vector2(1.0f / 10.0f, 1.0f);
	for(uint32 i = 0; i < 10; i++)
	{
		Vector2 texStart = comboFontTexCoordSize * Vector2((float)i, 0);
		Vector<MeshGenerators::SimpleVertex> verts;
		MeshGenerators::GenerateSimpleXYQuad(Rect3D(Vector2(-0.5f), Vector2(1.0f)), Rect(texStart, comboFontTexCoordSize), verts);
		Mesh m = comboSpriteMeshes[i] = MeshRes::Create(g_gl);
		m->SetData(verts);
		m->SetPrimitiveType(PrimitiveType::TriangleList);
	}

	// Create a laser track builder for each laser object
	// these will output and cache meshes for rendering lasers
	for(uint32 i = 0; i < 2; i++)
	{
		m_laserTrackBuilder[i] = new LaserTrackBuilder(g_gl, i, trackWidth, laserWidth);
		m_laserTrackBuilder[i]->laserTextureSize = laserTexture->GetSize();
		m_laserTrackBuilder[i]->laserEntryTextureSize = laserTailTextures[0]->GetSize();
		m_laserTrackBuilder[i]->laserExitTextureSize = laserTailTextures[1]->GetSize();
		m_laserTrackBuilder[i]->laserBorderPixels = 12;
		m_laserTrackBuilder[i]->laserLengthScale = trackLength / viewRange;
		m_laserTrackBuilder[i]->Reset(); // Also initializes the track builder
	}

	// Generate simple planes for the playfield track and elements
	trackMesh = MeshGenerators::Quad(g_gl, Vector2(-trackWidth * 0.5f, 0.0f), Vector2(trackWidth, trackLength));
	trackTickMesh = MeshGenerators::Quad(g_gl, Vector2(-buttonTrackWidth * 0.5f, 0.0f), Vector2(buttonTrackWidth, trackTickLength));
	centeredTrackMesh = MeshGenerators::Quad(g_gl, Vector2(-0.5f, -0.5f), Vector2(1.0f, 1.0f));

	return success;
}
void Track::Tick(class BeatmapPlayback& playback, float deltaTime)
{
	const TimingPoint& currentTimingPoint = playback.GetCurrentTimingPoint();
	if(&currentTimingPoint != m_lastTimingPoint)
	{
		m_laserTrackBuilder[0]->Reset();
		m_laserTrackBuilder[1]->Reset();
		m_lastTimingPoint = &currentTimingPoint;
	}

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
	int32 startBeat = 0;
	uint32 numBeats = playback.CountBeats(m_lastMapTime, currentTime - m_lastMapTime, startBeat, 4);
	m_lastMapTime = currentTime;
	if(numBeats > 0)
	{
		objectGlow = 1.0f;
	}
	else
	{
		objectGlow -= 7.0f * deltaTime;
		if(objectGlow < 0.0f)
			objectGlow = 0.0f;
	}

	// Perform laser track cache cleanup, etc.
	for(uint32 i = 0; i < 2; i++)
	{
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
		float currentObjectGlow = active ? objectGlow : 0.0f;
		if(mobj->button.index < 4) // Normal button
		{
			width = buttonWidth;
			xposition = buttonTrackWidth * -0.5f + width * mobj->button.index;
			length = buttonLength;
			params.SetParameter("mainTex", isHold ? buttonHoldTexture : buttonTexture);
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
			params.SetParameter("objectGlow", currentObjectGlow);
		}

		Vector3 buttonPos = Vector3(xposition, trackLength * position, 0.02f);

		Transform buttonTransform;
		buttonTransform *= Transform::Translation(buttonPos);
		float scale = 1.0f;
		if(isHold) // Hold Note?
		{
			scale = (playback.DurationToBarDistance(mobj->hold.duration) / viewRange) / length  * trackLength;
		}
		buttonTransform *= Transform::Scale({ 1.0f, scale, 1.0f });
		rq.Draw(buttonTransform, mesh, mat, params);
	}
	else if(obj->type == ObjectType::Laser) // Draw laser
	{
		LaserObjectState* laser = (LaserObjectState*)obj;

		// Draw segment function
		auto DrawSegment = [&](Mesh mesh, Texture texture)
		{
			MaterialParameterSet laserParams;

			// Make not yet hittable lasers slightly glowing
			if((laser->GetRoot()->time + Scoring::goodHitTime) > playback.GetLastTime())
				laserParams.SetParameter("objectGlow", 0.2f);
			else
				laserParams.SetParameter("objectGlow", active ? objectGlow : 0.0f);
			laserParams.SetParameter("mainTex", texture);

			// Get the length of this laser segment
			Transform laserTransform;
			laserTransform *= Transform::Translation(Vector3{ 0.0f, trackLength * position, 
				0.007f + 0.003f * laser->index }); // Small amount of elevation

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

	DrawSprite(rq, Vector3(0.0f, 0.0f, 0.0f), barSize, scoreBarTexture, Color::White, 5.0f);

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
		objectSize.y = laserPointerTexture->CalculateHeight(objectSize.x);
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
void Track::DrawCombo(RenderQueue& rq, uint32 score, Color color, float scale)
{
	if(score == 0)
		return;
	Vector<Mesh> meshes;
	while(score > 0)
	{
		uint32 c = score % 10;
		meshes.Add(comboSpriteMeshes[c]);
		score -= c;
		score /= 10;
	}
	const float charWidth = trackWidth * 0.15f * scale;
	const float seperation = charWidth * 0.7f;
	float size = (float)(meshes.size()-1) * seperation;
	float halfSize = size * 0.5f;

	MaterialParameterSet params;
	params.SetParameter("mainTex", comboSpriteSheet);
	params.SetParameter("color", color);
	for(uint32 i = 0; i < meshes.size(); i++)
	{
		float xpos = -halfSize + seperation * (meshes.size()-1-i);
		Transform t = Transform::Translation({ xpos, 0.3f, -0.004f});
		t *= Transform::Scale({charWidth, charWidth, 1.0f});
		rq.Draw(t, meshes[i], spriteMaterial, params);
	}
}

TimedEffect* Track::AddEffect(TimedEffect* effect)
{
	m_hitEffects.Add(effect);
	effect->track = this;
	return effect;
}
void Track::ClearEffects()
{
	for(auto it = m_hitEffects.begin(); it != m_hitEffects.end(); it++)
	{
		delete *it;
	}
	m_hitEffects.clear();
}

float Track::GetButtonPlacement(uint32 buttonIdx)
{
	if(buttonIdx < 4)
		return buttonIdx * buttonWidth - (buttonWidth * 1.5f);
	else
		return (buttonIdx - 4) * fxbuttonWidth - (fxbuttonWidth * 0.5f);
}


#include "stdafx.h"
#include "Game.hpp"
#include "Track.hpp"

TimedEffect::TimedEffect(float duration)
{
	Reset(duration);
}
void TimedEffect::Reset(float duration)
{
	this->duration = duration; 
	time = duration;
}
void TimedEffect::Tick(float deltaTime)
{
	time -= deltaTime;
}

ButtonHitEffect::ButtonHitEffect(uint32 buttonCode, Color color) : TimedEffect(0.2f), buttonCode(buttonCode), color(color)
{
	assert(buttonCode < 6);
}
void ButtonHitEffect::Draw(class RenderQueue& rq)
{
	float x = 0.0f;
	float w = track->buttonWidth;
	if(buttonCode < 4)
	{
		w = track->buttonWidth;
		x = (-track->buttonWidth * 1.5f) + w * buttonCode;
	}
	else
	{
		w = track->buttonWidth * 2.0f;
		x = -track->buttonWidth + w * (buttonCode - 4);
	}

	Vector2 hitEffectSize = Vector2(w * 1.2f, 0.0f);
	hitEffectSize.y = track->scoreHitTexture->CalculateHeight(hitEffectSize.x) * track->perspectiveHeightScale;
	Color c = color.WithAlpha(GetRate());
	track->DrawSprite(rq, Vector3(x, 0.05f + hitEffectSize.y * 0.5f, -0.03f), hitEffectSize, track->scoreHitTexture, c);
}

ButtonHitRatingEffect::ButtonHitRatingEffect(uint32 buttonCode, ScoreHitRating rating) : TimedEffect(0.3f), buttonCode(buttonCode), rating(rating)
{
	assert(buttonCode < 6);
	if(rating == ScoreHitRating::Miss)
		Reset(0.4f);
}
void ButtonHitRatingEffect::Draw(class RenderQueue& rq)
{
	float x = 0.0f;
	float w = track->buttonWidth;
	float y = 0.0f;
	if(buttonCode < 4)
	{
		w = track->buttonWidth;
		x = (-track->buttonWidth * 1.5f) + w * buttonCode;
		y = 0.80f;
	}
	else
	{
		w = track->buttonWidth * 2.0f;
		x = -track->buttonWidth + w * (buttonCode - 4);
		y = 1.0f;
	}

	float iScale = 1.0f;
	uint32 on = 1;
	if(rating == ScoreHitRating::Miss) // flicker
		on = (uint32)floorf(GetRate() * 6.0f) % 2;
	else if(rating == ScoreHitRating::Perfect)
		iScale = cos(GetRate() * 12.0f) * 0.5f + 1.0f;

	if(on == 1)
	{
		Texture hitTexture = track->scoreHitTextures[(size_t)rating];
		Vector2 hitEffectSize = Vector2(track->buttonWidth, 0.0f);
		hitEffectSize.y = hitTexture->CalculateHeight(hitEffectSize.x) * track->perspectiveHeightScale;

		// Fade out
		Color c = Color::White.WithAlpha(GetRate());
		// Intensity scale
		Utility::Reinterpret<Vector3>(c) *= iScale;

		track->DrawSprite(rq, Vector3(x, y + hitEffectSize.y * 0.5f, -0.03f), hitEffectSize, hitTexture, c, 0.0f);
	}
}
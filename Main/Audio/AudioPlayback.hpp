#pragma once
#include "Beatmap.hpp"
#include "AudioEffects.hpp"
#include "bass.h"

/* 
	Handles playback of map audio
	keeps track of the state of effects
*/
class AudioPlayback : Unique
{
public:
	AudioPlayback();
	~AudioPlayback();
	// Loads audio for beatmap
	// Map path is the map's root path
	bool Init(Beatmap& beatmap, const String& mapPath);

	// Updates effects
	void Tick(class BeatmapPlayback& playback, float deltaTime);

	// Play from start or continue
	void Play();
	void Advance(MapTime ms);
	MapTime GetPosition() const;
	void SetPosition(MapTime time);
	// Pause the playback
	void TogglePause();
	bool IsPaused() const { return m_paused; }
	// Enables or disables the fx track
	void SelectFXTrackEnabled(bool active);
	// Returns the track to apply effects to can be the nofx audio if there is no fx audio track
	HCHANNEL GetFXTrack();

	// NoFX and FX track for the current map
	HCHANNEL audioClips[2];

	// Sets either button effect 0 or 1
	void SetEffect(uint8 index, EffectType type, EffectParam param);

	// Sets the effect to be used for lasers
	void SetLaserEffect(LaserEffectType type);

	// The input which controls the laser filter amount
	void SetLaserFilterInput(float input);

private:
	void m_SetLaserEffectParameter(float input);

	bool m_paused = false;
	bool m_fxTrackActive = false;

	float m_lastLaserInput;
	LaserEffectType m_laserEffectType = LaserEffectType::PeakingFilter;

	uint32 m_buttonEffectParams[2] = { 0 };
	EffectType m_buttonEffectTypes[2] = { EffectType::None };
};
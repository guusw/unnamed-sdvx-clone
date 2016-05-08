#pragma once
#include "Beatmap.hpp"
#include "AudioEffects.hpp"
#include "AudioStream.hpp"

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

	// Sets either button effect 0 or 1
	void SetEffect(uint8 index, EffectType type, EffectParam param);

	// Sets the effect to be used for lasers
	void SetLaserEffect(LaserEffectType type);

	// The input which controls the laser filter amount
	void SetLaserFilterInput(float input);

private:
	class DSP* m_InitDSP(LaserEffectType type);
	void m_CleanupDSP(class DSP*& ptr);
	void m_SetLaserEffectParameter(float input);
	AudioStream m_music;
	bool m_paused = false;

	float m_lastLaserInput;
	LaserEffectType m_laserEffectType = LaserEffectType::PeakingFilter;
	class DSP* m_laserDSP;

	uint32 m_buttonEffectParams[2] = { 0 };
	EffectType m_buttonEffectTypes[2] = { EffectType::None };
};
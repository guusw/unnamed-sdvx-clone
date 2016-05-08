#include "stdafx.h"
#include "AudioPlayback.hpp"
#include "Beatmap.hpp"
#include "Audio.hpp"
#include "DSP.hpp"

AudioPlayback::AudioPlayback()
{
}
AudioPlayback::~AudioPlayback()
{
}
bool AudioPlayback::Init(class Beatmap& beatmap, const String& mapPath)
{
	m_lastLaserInput = FLT_MAX;

	const BeatmapSettings& mapSettings = beatmap.GetMapSettings();
	String audioPath = mapPath + "\\" + mapSettings.audioNoFX;
	WString audioPathUnicode = Utility::ConvertToUnicode(audioPath);
	if(!Path::FileExists(audioPath))
	{
		Logf("Audio file for beatmap does not exists at: \"%s\"", Logger::Error, audioPath);
		return false;
	}
	
	m_music = g_audio->CreateStream(audioPath);

	if(!m_music)
	{
		Logf("Failed to load any audio for beatmap \"%s\"", Logger::Error, audioPath);
		return false;
	}

	return true;
}
void AudioPlayback::Tick(class BeatmapPlayback& playback, float deltaTime)
{

}
void AudioPlayback::Play()
{
	m_music->Play();
}
void AudioPlayback::Advance(MapTime ms)
{
	m_music->SetPosition(m_music->GetPosition() + ms);
}
MapTime AudioPlayback::GetPosition() const
{
	return m_music->GetPosition();
}
void AudioPlayback::SetPosition(MapTime time)
{
	m_music->SetPosition(time);
}
void AudioPlayback::TogglePause()
{
	if(m_paused)
	{
		m_music->Play();
	}
	else
	{
		m_music->Pause();
	}
	m_paused = !m_paused;
}
void AudioPlayback::SetLaserFilterInput(float input)
{
	if(input == m_lastLaserInput)
		return;

	if(input != 0.0f)
	{
		// Create DSP
		if(!m_laserDSP)
		{
			m_laserDSP = m_InitDSP(m_laserEffectType);
		}

		// Set params
		m_SetLaserEffectParameter(input);
	}
	else
	{
		m_CleanupDSP(m_laserDSP);
	}

	m_lastLaserInput = input;
}
DSP* AudioPlayback::m_InitDSP(LaserEffectType type)
{
	DSP* ret = nullptr;
	switch(type)
	{
	default:
		ret = new BQFDSP();
		ret->priority = 0;
		break;
	}
	m_music->AddDSP(ret);

	return ret;
}
void AudioPlayback::m_CleanupDSP(DSP*& ptr)
{
	if(ptr)
	{
		delete ptr;
		ptr = nullptr;
	}
}

void AudioPlayback::m_SetLaserEffectParameter(float input)
{
	switch(m_laserEffectType)
	{
	default:
		float gain = 10.0f;
		if(input < 0.2f) // Fade in
			gain = input / 0.2f * gain;
		((BQFDSP*)m_laserDSP)->SetPeaking(3.0f, 100.0f + input * 8000.0f, gain);
		break;
	}
}


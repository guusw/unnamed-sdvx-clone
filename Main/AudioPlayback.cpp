#include "stdafx.h"
#include "AudioPlayback.hpp"
#include "Beatmap.hpp"
#include "bass_fx.h"

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
	String audioPath1 = mapPath + "\\" + mapSettings.audioFX;
	WString audioPathUnicode = Utility::ConvertToUnicode(audioPath);
	WString audioPathUnicode1 = Utility::ConvertToUnicode(audioPath1);
	if(Path::FileExists(audioPath))
		audioClips[0] = BASS_StreamCreateFile(false, *audioPathUnicode, 0, 0, BASS_UNICODE | BASS_SAMPLE_SOFTWARE | BASS_SAMPLE_FX);
	//if(Path::FileExists(audioPath1))
	//	audioClips[1] = BASS_StreamCreateFile(false, *audioPathUnicode1, 0, 0, BASS_UNICODE);

	if(audioClips[0] == 0)
	{
		int32 bassError = BASS_ErrorGetCode();
		Logf("Failed to load any audio for beatmap \"%s\" [%d]", Logger::Error, audioPath, bassError);
		return false;
	}

	//if(audioClips[1])
	//{
	//	// Link tracks
	//	BASS_ChannelSetLink(audioClips[0], audioClips[1]);
	//}

	SelectFXTrackEnabled(true);
	return true;
}
void AudioPlayback::Tick(class BeatmapPlayback& playback, float deltaTime)
{

}
void AudioPlayback::Play()
{
	BASS_ChannelPlay(audioClips[0], false);

	// Initially set fx track enabled
	m_fxTrackActive = false;
	SelectFXTrackEnabled(true);
}
void AudioPlayback::Advance(MapTime ms)
{
	QWORD bytePos = BASS_ChannelGetPosition(audioClips[0], BASS_POS_BYTE);
	double playbackPosition = BASS_ChannelBytes2Seconds(audioClips[0], bytePos);
	playbackPosition += (double)ms / 1000.0;
	BASS_ChannelSetPosition(audioClips[0], BASS_ChannelSeconds2Bytes(audioClips[0], playbackPosition), BASS_POS_BYTE);
	if(audioClips[1])
		BASS_ChannelSetPosition(audioClips[1], BASS_ChannelSeconds2Bytes(audioClips[1], playbackPosition), BASS_POS_BYTE);
}
MapTime AudioPlayback::GetPosition() const
{
	QWORD bytePos = BASS_ChannelGetPosition(audioClips[0], BASS_POS_BYTE);
	double playbackPosition = BASS_ChannelBytes2Seconds(audioClips[0], bytePos);
	MapTime playbackPositionMs = (MapTime)(playbackPosition * 1000.0);
	return playbackPositionMs;
}
void AudioPlayback::SetPosition(MapTime time)
{
	double dstTime = (double)time / 1000.0;
	BASS_ChannelSetPosition(audioClips[0], BASS_ChannelSeconds2Bytes(audioClips[0], dstTime), BASS_POS_BYTE);
	if(audioClips[1])
		BASS_ChannelSetPosition(audioClips[1], BASS_ChannelSeconds2Bytes(audioClips[1], dstTime), BASS_POS_BYTE);
}
void AudioPlayback::TogglePause()
{
	if(m_paused)
	{
		BASS_ChannelPlay(audioClips[0], false);
	}
	else
	{
		BASS_ChannelPause(audioClips[0]);
	}
	m_paused = !m_paused;
}
void AudioPlayback::SelectFXTrackEnabled(bool active)
{
	if(m_fxTrackActive != active)
	{
		/*
		if(active && audioClips[1])
		{
			BASS_ChannelSetAttribute(audioClips[0], BASS_ATTRIB_VOL, 0.0f);
			BASS_ChannelSetAttribute(audioClips[1], BASS_ATTRIB_VOL, 1.0f);
		}
		else
		{
			BASS_ChannelSetAttribute(audioClips[0], BASS_ATTRIB_VOL, 1.0f);
			if(audioClips[1])
				BASS_ChannelSetAttribute(audioClips[1], BASS_ATTRIB_VOL, 0.0f);
		}
		*/

		m_fxTrackActive = active;
	}
}
HCHANNEL AudioPlayback::GetFXTrack()
{
	if(audioClips[1])
		return audioClips[1];
	return audioClips[0];
}
void AudioPlayback::SetLaserFilterInput(float input)
{
	if(input == m_lastLaserInput)
		return;
	m_lastLaserInput = input;
}
void AudioPlayback::m_SetLaserEffectParameter(float input)
{
}


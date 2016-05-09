#include "stdafx.h"
#include "AudioPlayback.hpp"
#include "BeatmapPlayback.hpp"
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
	m_laserEffectType = LaserEffectType::PeakingFilter;

	const BeatmapSettings& mapSettings = beatmap.GetMapSettings();
	String audioPath = mapPath + "\\" + mapSettings.audioNoFX;
	WString audioPathUnicode = Utility::ConvertToUnicode(audioPath);
	if(!Path::FileExists(audioPath))
	{
		Logf("Audio file for beatmap does not exists at: \"%s\"", Logger::Error, audioPath);
		return false;
	}
	
	m_music = g_audio->CreateStream(audioPath);
	m_music->SetVolume(0.75f);

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

void AudioPlayback::SetEffect(uint32 index, HoldObjectState* object, class BeatmapPlayback& playback)
{
	assert(index >= 0 && index <= 1);
	ClearEffect(index);

	// For Time based effects
	const TimingPoint* timingPoint = playback.GetTimingPointAt(object->time);
	// Duration of a single bar
	double barDelay = timingPoint->measure * timingPoint->beatDuration;

	DSP*& dsp = m_buttonDSPs[index];
	switch(object->effectType)
	{
	default:
	case EffectType::Bitcrush:
		dsp = new BitCrusherDSP();
		((BitCrusherDSP*)dsp)->period = object->effectParam;
		break;
	case EffectType::Gate:
	{
		GateDSP* gate = new GateDSP();
		double delay = (barDelay / object->effectParam) / 1000.0;
		gate->delay = (uint32)(delay * g_audio->GetSampleRate());
		dsp = gate;
		break;
	}
	case EffectType::TapeStop:
	{
		TapeStopDSP* ts = new TapeStopDSP();
		double speed = 1.0 - (double)object->effectParam / 100.0;
		double delay = speed * object->duration / 1000.0;
		ts->delay = (uint32)(delay * g_audio->GetSampleRate());
		dsp = ts;
		break;
	}
	case EffectType::Retrigger:
	{
		RetriggerDSP* re = new RetriggerDSP();
		double delay = (barDelay / object->effectParam) / 1000.0;
		re->delay = (uint32)(delay * g_audio->GetSampleRate());
		dsp = re;
		break;
	}
	}

	if(dsp)
	{
		m_music->AddDSP(dsp);
	}
}
void AudioPlayback::ClearEffect(uint32 index)
{
	assert(index >= 0 && index <= 1);
	m_CleanupDSP(m_buttonDSPs[index]);
}

void AudioPlayback::SetLaserEffect(LaserEffectType type)
{
	if(type == m_laserEffectType)
		return;
	if(type != m_laserEffectType)
	{
		m_CleanupDSP(m_laserDSP);
	}
	m_laserEffectType = type;
}

void AudioPlayback::SetLaserFilterInput(float input, bool active)
{
	if(active || (input != 0.0f))
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
}

void AudioPlayback::SetLaserEffectMix(float mix)
{
	m_laserEffectMix = mix;
}

DSP* AudioPlayback::m_InitDSP(LaserEffectType type)
{
	DSP* ret = nullptr;
	switch(type)
	{
	default:
	case LaserEffectType::Bitcrush:
		ret = new BitCrusherDSP();
		((BitCrusherDSP*)ret)->mix = 0.85f * m_laserEffectMix;
		break;
	case LaserEffectType::PeakingFilter:
	case LaserEffectType::HighPassFilter:
	case LaserEffectType::LowPassFilter:
		ret = new BQFDSP();
		break;
	}
	ret->priority = 0;
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
	assert(input >= 0.0f && input <= 1.0f);
	switch(m_laserEffectType)
	{
	default:
	case LaserEffectType::Bitcrush:
	{
		((BitCrusherDSP*)m_laserDSP)->period = (uint32)(input * (64 * m_laserEffectMix));
		((BitCrusherDSP*)m_laserDSP)->mix = 1.0f;
		break;
	}
	case LaserEffectType::PeakingFilter:
	{
		const float volumeFadeIn = 0.3f;
		float gain = 13.0f * m_laserEffectMix;
		if(input < volumeFadeIn) // Fade in
			gain = (input / volumeFadeIn) * gain;
		input *= input; // ^2 for slope
		float width = 1.0f + 2.0f * input;
		((BQFDSP*)m_laserDSP)->SetPeaking(width, 200.0f + input * 8000.0f, gain);
		break;
	}
	case LaserEffectType::LowPassFilter:
	{
		float freqMax = (float)g_audio->GetSampleRate() * 0.1f;
		float v = (1.0f - input);
		v *= v; // ^2 for slope
		float freq = 100.0f + freqMax  * v;
		((BQFDSP*)m_laserDSP)->SetLowPass(8.0f * m_laserEffectMix, freq);
		break;
	}
	case LaserEffectType::HighPassFilter:
	{
		float freqMax = (float)g_audio->GetSampleRate() * 0.1f;
		float v = input;
		v *= v; // ^2 for slope
		float freq =  100.0f + freqMax  * v;
		((BQFDSP*)m_laserDSP)->SetHighPass(8.0f * m_laserEffectMix, freq);
		break;
	}
	}
}


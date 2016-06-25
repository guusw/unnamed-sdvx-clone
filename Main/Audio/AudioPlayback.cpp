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
	m_music->SetVolume(1.0f);
	if(!m_music)
	{
		Logf("Failed to load any audio for beatmap \"%s\"", Logger::Error, audioPath);
		return false;
	}

	// Load FX track
	audioPath = mapPath + "\\" + mapSettings.audioFX;
	audioPathUnicode = Utility::ConvertToUnicode(audioPath);
	if(!audioPath.empty())
	{
		if(!Path::FileExists(audioPath))
		{
			Logf("FX audio for for beatmap does not exists at: \"%s\"", Logger::Error, audioPath);
			return false;
		}
		else
		{
			m_fxtrack = g_audio->CreateStream(audioPath);
			if(m_fxtrack)
			{
				// Initially mute normal track if fx is enabled
				m_music->SetVolume(0.0f);
			}
		}
	}

	return true;
}
void AudioPlayback::Tick(class BeatmapPlayback& playback, float deltaTime)
{

}
void AudioPlayback::Play()
{
	m_music->Play();
	if(m_fxtrack)
		m_fxtrack->Play();
}
void AudioPlayback::Advance(MapTime ms)
{
	SetPosition(GetPosition() + ms);
}
MapTime AudioPlayback::GetPosition() const
{
	return m_music->GetPosition();
}
void AudioPlayback::SetPosition(MapTime time)
{
	m_music->SetPosition(time);
	if(m_fxtrack)
		m_fxtrack->SetPosition(time);
}
void AudioPlayback::TogglePause()
{
	if(m_paused)
	{
		m_music->Play();
		if(m_fxtrack)
			m_fxtrack->Play();
	}
	else
	{
		m_music->Pause();
		if(m_fxtrack)
			m_fxtrack->Pause();
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
	double barDelay = timingPoint->numerator * timingPoint->beatDuration;

	DSP*& dsp = m_buttonDSPs[index];
	bool shouldAdd = false;
	switch(object->effectType)
	{
	case EffectType::Bitcrush:
	{
		BitCrusherDSP* bc = new BitCrusherDSP();
		m_GetDSPTrack()->AddDSP(bc);
		bc->SetPeriod(object->effectParam);
		dsp = bc;
		break;
	}
	case EffectType::Gate:
	{
		GateDSP* gate = new GateDSP();
		double delay = (barDelay / object->effectParam) / 1000.0;
		gate->SetLength((uint32)(delay * g_audio->GetSampleRate()));
		dsp = gate;
		m_GetDSPTrack()->AddDSP(dsp);
		break;
	}
	case EffectType::TapeStop:
	{
		TapeStopDSP* ts = new TapeStopDSP();
		m_GetDSPTrack()->AddDSP(ts);
		double speed = 1.0 - (double)object->effectParam / 100.0;
		double delay = speed * object->duration / 1000.0;
		ts->SetLength((uint32)(delay * g_audio->GetSampleRate()));
		dsp = ts;
		break;
	}
	case EffectType::Retrigger:
	{
		RetriggerDSP* re = new RetriggerDSP();
		double delay = (barDelay / object->effectParam) / 1000.0;
		re->SetLength((uint32)(delay * g_audio->GetSampleRate()));
		dsp = re;
		m_GetDSPTrack()->AddDSP(dsp);
		break;
	}
	case EffectType::Wobble:
	{
		WobbleDSP* wb = new WobbleDSP();
		double delay = (barDelay / object->effectParam) / 1000.0;
		wb->delay = (uint32)(delay * g_audio->GetSampleRate());
		dsp = wb;
		m_GetDSPTrack()->AddDSP(dsp);
		break;
	}
	case EffectType::Phaser:
	{
		PhaserDSP* phs = new PhaserDSP();
		double delay = (barDelay) / 1000.0;
		phs->delay = (uint32)(delay * g_audio->GetSampleRate());
		phs->dmin = 800.f;
		phs->dmax = 8000.f;
		phs->depth = 0.8f;
		phs->fb = 0.5f;
		phs->time = object->time;
		dsp = phs;
		m_GetDSPTrack()->AddDSP(dsp);
		break;
	}
	case EffectType::Flanger:
	{
		FlangerDSP* fl = new FlangerDSP();
		double delay = (barDelay) / 1000.0;
		fl->delay = (uint32)(delay * g_audio->GetSampleRate());
		fl->min = 2;
		fl->max = 40;
		fl->time = object->time;
		dsp = fl;
		m_GetDSPTrack()->AddDSP(dsp);
		break;
	}
	default:
		break;
	}

	if(dsp)
	{
		dsp->mix = 0;
	}
}

void AudioPlayback::SetEffectEnabled(uint32 index, bool enabled)
{
	assert(index >= 0 && index <= 1);
	if(m_buttonDSPs[index])
	{
		m_buttonDSPs[index]->mix = enabled ? 1.0f : 0.0f;
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
float AudioPlayback::GetLaserEffectMix() const
{
	return m_laserEffectMix;
}

AudioStream AudioPlayback::m_GetDSPTrack()
{
	if(m_fxtrack)
		return m_fxtrack;
	return m_music;
}

void AudioPlayback::SetFXTrackEnabled(bool enabled)
{
	if(!m_fxtrack)
		return;
	if(m_fxtrackEnabled != enabled)
	{
		if(enabled)
		{
			m_fxtrack->SetVolume(1.0f);
			m_music->SetVolume(0.0f);
		}
		else
		{
			m_fxtrack->SetVolume(0.0f);
			m_music->SetVolume(1.0f);
		}
	}
	m_fxtrackEnabled = enabled;
}

DSP* AudioPlayback::m_InitDSP(LaserEffectType type)
{
	DSP* ret = nullptr;
	switch(type)
	{
	case LaserEffectType::Bitcrush:
		ret = new BitCrusherDSP();
		((BitCrusherDSP*)ret)->mix = 0.85f * m_laserEffectMix;
		break;
	case LaserEffectType::PeakingFilter:
	case LaserEffectType::LowPassFilter:
	case LaserEffectType::HighPassFilter:
		ret = new BQFDSP();
		break;
	}

	if(ret)
	{
		ret->priority = 0;
		m_GetDSPTrack()->AddDSP(ret);
	}

	return ret;
}

void AudioPlayback::m_CleanupDSP(DSP*& ptr)
{
	if(ptr)
	{
		m_GetDSPTrack()->RemoveDSP(ptr);
		delete ptr;
		ptr = nullptr;
	}
}

void AudioPlayback::m_SetLaserEffectParameter(float input)
{
	assert(input >= 0.0f && input <= 1.0f);
	assert(m_laserDSP);

	switch(m_laserEffectType)
	{
	case LaserEffectType::Bitcrush:
	{
		((BitCrusherDSP*)m_laserDSP)->SetPeriod(input * 64 * m_laserEffectMix);
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
		float width = 1.0f + 1.0f * input; 
		((BQFDSP*)m_laserDSP)->SetPeaking(width, 200.0f + input * 8000.0f, gain);
		break;
	}
	case LaserEffectType::LowPassFilter:
	{
		float v = (1.0f - input);
		v *= v; // ^2 for slope
		float freq = 100.0f + 10000 * v;
		((BQFDSP*)m_laserDSP)->SetLowPass(2.0f + 4.0f * (1-v), freq);
		((BQFDSP*)m_laserDSP)->mix = m_laserEffectMix;
		break;
	}
	case LaserEffectType::HighPassFilter:
	{
		float v = input;
		v *= v; // ^2 for slope
		float freq =  100.0f + 4000 * v;
		((BQFDSP*)m_laserDSP)->SetHighPass(2.0f + 4.0f * v, freq);
		((BQFDSP*)m_laserDSP)->mix = m_laserEffectMix;
		break;
	}
	}
}


#include "stdafx.h"
#include "AudioPlayback.hpp"
#include <Beatmap/BeatmapPlayback.hpp>
#include <Beatmap/Beatmap.hpp>
#include <Audio/Audio.hpp>
#include <Audio/DSP.hpp>

AudioPlayback::AudioPlayback()
{
}
AudioPlayback::~AudioPlayback()
{
	m_CleanupDSP(m_buttonDSPs[0]);
	m_CleanupDSP(m_buttonDSPs[1]);
	m_CleanupDSP(m_laserDSP);
}
bool AudioPlayback::Init(class BeatmapPlayback& playback, const String& mapRootPath)
{
	// Cleanup exising DSP's
	m_currentHoldEffects[0] = nullptr;
	m_currentHoldEffects[1] = nullptr;
	m_CleanupDSP(m_buttonDSPs[0]);
	m_CleanupDSP(m_buttonDSPs[1]);
	m_CleanupDSP(m_laserDSP);

	m_playback = &playback;
	m_beatmap = &playback.GetBeatmap();
	m_beatmapRootPath = mapRootPath;
	assert(m_beatmap != nullptr);

	// Set default effect type
	SetLaserEffect(EffectType::PeakingFilter);

	const BeatmapSettings& mapSettings = m_beatmap->GetMapSettings();
	String audioPath = Path::Normalize(m_beatmapRootPath + Path::sep + mapSettings.audioNoFX);
	WString audioPathUnicode = Utility::ConvertToWString(audioPath);
	if(!Path::FileExists(audioPath))
	{
		Logf("Audio file for beatmap does not exists at: \"%s\"", Logger::Error, audioPath);
		return false;
	}
	m_music = g_audio->CreateStream(audioPath, true);
	if(!m_music)
	{
		Logf("Failed to load any audio for beatmap \"%s\"", Logger::Error, audioPath);
		return false;
	}
	m_music->SetVolume(1.0f);

	// Load FX track
	audioPath = Path::Normalize(m_beatmapRootPath + Path::sep + mapSettings.audioFX);
	audioPathUnicode = Utility::ConvertToWString(audioPath);
	if(!audioPath.empty())
	{
		if(!Path::FileExists(audioPath))
		{
			Logf("FX audio for for beatmap does not exists at: \"%s\"", Logger::Warning, audioPath);
		}
		else
		{
			m_fxtrack = g_audio->CreateStream(audioPath, true);
			if(m_fxtrack)
			{
				// Initially mute normal track if fx is enabled
				m_music->SetVolume(0.0f);
			}
		}
	}

	return true;
}
void AudioPlayback::Tick(float deltaTime)
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
bool AudioPlayback::HasEnded() const
{
	return m_music->HasEnded();
}
void AudioPlayback::SetEffect(uint32 index, HoldObjectState* object, class BeatmapPlayback& playback)
{
	// Don't use effects when using an FX track
	if(m_fxtrack.IsValid())
		return;

	assert(index >= 0 && index <= 1);
	m_CleanupDSP(m_buttonDSPs[index]);
	m_currentHoldEffects[index] = object;

	// For Time based effects
	const TimingPoint* timingPoint = playback.GetTimingPointAt(object->time);
	// Duration of a single bar
	double barDelay = timingPoint->numerator * timingPoint->beatDuration;

	DSP*& dsp = m_buttonDSPs[index];

	m_buttonEffects[index] = m_beatmap->GetEffect(object->effectType);
	dsp = m_buttonEffects[index].CreateDSP(m_GetDSPTrack().GetData(), *this);

	if(dsp)
	{
		m_buttonEffects[index].SetParams(dsp, *this, object);
		// Initialize mix value to previous value
		dsp->mix = m_effectMix[index];
	}
}
void AudioPlayback::SetEffectEnabled(uint32 index, bool enabled)
{
	assert(index >= 0 && index <= 1);
	m_effectMix[index] = enabled ? 1.0f : 0.0f;
	if(m_buttonDSPs[index])
	{
		m_buttonDSPs[index]->mix = m_effectMix[index];
	}
}
void AudioPlayback::ClearEffect(uint32 index, HoldObjectState* object)
{
	assert(index >= 0 && index <= 1);
	if(m_currentHoldEffects[index] == object)
	{
		m_CleanupDSP(m_buttonDSPs[index]);
		m_currentHoldEffects[index] = nullptr;
	}
}
void AudioPlayback::SetLaserEffect(EffectType type)
{
	if(type != m_laserEffectType)
	{
		m_CleanupDSP(m_laserDSP);
		m_laserEffectType = type;
		m_laserEffect = m_beatmap->GetFilter(type);
	}
}
void AudioPlayback::SetLaserFilterInput(float input, bool active)
{
	if(m_laserEffect.type != EffectType::None && (active || (input != 0.0f)))
	{
		// Create DSP
		if(!m_laserDSP)
		{
			// Don't use Bitcrush effects over FX track
			if(m_fxtrack.IsValid() && m_laserEffectType == EffectType::Bitcrush)
				return;

			m_laserDSP = m_laserEffect.CreateDSP(m_GetDSPTrack().GetData(), *this);
			if(!m_laserDSP)
			{
				Logf("Failed to create laser DSP with type %d", Logger::Warning, m_laserEffect.type);
				return;
			}
		}

		// Set params
		m_SetLaserEffectParameter(input);
		m_laserInput = input;
	}
	else
	{
		m_CleanupDSP(m_laserDSP);
		m_laserInput = 0.0f;
	}
}
float AudioPlayback::GetLaserFilterInput() const
{
	return m_laserInput;
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
BeatmapPlayback& AudioPlayback::GetBeatmapPlayback()
{
	return *m_playback;
}
const Beatmap& AudioPlayback::GetBeatmap() const
{
	return *m_beatmap;
}
const String& AudioPlayback::GetBeatmapRootPath() const
{
	return m_beatmapRootPath;
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
	if(!m_laserDSP)
		return;
	assert(input >= 0.0f && input <= 1.0f);

	// Mix for normal effects
	m_laserDSP->mix = m_laserEffectMix;

	// Mix float biquad filters, these are applied manualy by changing the filter parameters (gain,q,freq,etc.)
	float mix = m_laserEffectMix;
	if(input < 0.1f)
		mix *= input / 0.1f;

	switch(m_laserEffect.type)
	{
	case EffectType::Bitcrush:
	{
		BitCrusherDSP* bcDSP = (BitCrusherDSP*)m_laserDSP;
		bcDSP->SetPeriod((float)m_laserEffect.bitcrusher.reduction.Sample(input));
		break;
	}
	case EffectType::Echo:
	{
		EchoDSP* echoDSP = (EchoDSP*)m_laserDSP;
		echoDSP->feedback = m_laserEffect.echo.feedback.Sample(input);
		break;
	}
	case EffectType::PeakingFilter:
	{
		if(input > 0.8f)
			mix *= 1.0f - (input - 0.8f) / 0.2f;

		BQFDSP* bqfDSP = (BQFDSP*)m_laserDSP;
		bqfDSP->SetPeaking(m_laserEffect.peaking.q.Sample(input), m_laserEffect.peaking.freq.Sample(input), m_laserEffect.peaking.gain.Sample(input) * mix);
		break;
	}
	case EffectType::LowPassFilter:
	{
		BQFDSP* bqfDSP = (BQFDSP*)m_laserDSP;
		bqfDSP->SetLowPass(m_laserEffect.lpf.q.Sample(input) * mix + 0.1f, m_laserEffect.lpf.freq.Sample(input));
		break;
	}
	case EffectType::HighPassFilter:
	{
		BQFDSP* bqfDSP = (BQFDSP*)m_laserDSP;
		bqfDSP->SetHighPass(m_laserEffect.hpf.q.Sample(input)  * mix + 0.1f, m_laserEffect.hpf.freq.Sample(input));
		break;
	}
	case EffectType::PitchShift:
	{
		PitchShiftDSP* ps = (PitchShiftDSP*)m_laserDSP;
		ps->amount = m_laserEffect.pitchshift.amount.Sample(input);
		break;
	}
	}
}

GameAudioEffect::GameAudioEffect(const AudioEffect& other)
{
	*((AudioEffect*)this) = other;
}

#include "stdafx.h"
#include "AudioPlayback.hpp"
#include <Beatmap/BeatmapPlayback.hpp>
#include <Audio/DSP.hpp>
#include <Audio/Audio.hpp>

DSP* GameAudioEffect::CreateDSP(class AudioBase* audioTrack, AudioPlayback& playback)
{
	DSP* ret = nullptr;

	const TimingPoint& tp = playback.GetBeatmapPlayback().GetCurrentTimingPoint();
	double noteDuration = tp.GetWholeNoteLength();

	float filterInput = playback.GetLaserFilterInput();
	uint32 actualLength = duration.Sample(filterInput).Absolute(noteDuration);

	switch(type)
	{
	case EffectType::Bitcrush:
	{
		BitCrusherDSP* bcDSP = new BitCrusherDSP();
		audioTrack->AddDSP(bcDSP);
		bcDSP->SetPeriod((float)bitcrusher.reduction.Sample(filterInput));
		ret = bcDSP;
		break;
	}
	case EffectType::Echo:
	{
		EchoDSP* echoDSP = new EchoDSP();
		audioTrack->AddDSP(echoDSP);
		echoDSP->feedback = echo.feedback.Sample(filterInput);
		echoDSP->SetLength(actualLength);
		ret = echoDSP;
		break;
	}
	case EffectType::PeakingFilter:
	{
		BQFDSP* bqfDSP = new BQFDSP();
		audioTrack->AddDSP(bqfDSP);
		bqfDSP->SetPeaking(peaking.q.Sample(filterInput), peaking.freq.Sample(filterInput), peaking.gain.Sample(filterInput));
		ret = bqfDSP;
		break;
	}
	case EffectType::LowPassFilter:
	{
		CombinedFilterDSP* bqfDSP = new CombinedFilterDSP();
		audioTrack->AddDSP(bqfDSP);
		bqfDSP->SetLowPass(lpf.q.Sample(filterInput), lpf.freq.Sample(filterInput), lpf.peakQ.Sample(filterInput), lpf.gain.Sample(filterInput));
		ret = bqfDSP;
		break;
	}
	case EffectType::HighPassFilter:
	{
		CombinedFilterDSP* bqfDSP = new CombinedFilterDSP();
		audioTrack->AddDSP(bqfDSP);
		bqfDSP->SetHighPass(hpf.q.Sample(filterInput), hpf.freq.Sample(filterInput), hpf.peakQ.Sample(filterInput), hpf.gain.Sample(filterInput));
		ret = bqfDSP;
		break;
	}
	case EffectType::Gate:
	{
		GateDSP* gateDSP = new GateDSP();
		audioTrack->AddDSP(gateDSP);
		gateDSP->SetLength(actualLength);
		gateDSP->SetGating(gate.gate.Sample(filterInput));
		ret = gateDSP;
		break;
	}
	case EffectType::TapeStop:
	{
		TapeStopDSP* tapestopDSP = new TapeStopDSP();
		audioTrack->AddDSP(tapestopDSP);
		tapestopDSP->SetLength(actualLength);
		ret = tapestopDSP;
		break;
	}
	case EffectType::Retrigger:
	{
		RetriggerDSP* retriggerDSP = new RetriggerDSP();
		audioTrack->AddDSP(retriggerDSP);
		retriggerDSP->SetLength(actualLength);
		retriggerDSP->SetGating(retrigger.gate.Sample(filterInput));
		retriggerDSP->SetResetDuration(retrigger.reset.Sample(filterInput).Absolute(noteDuration));
		ret = retriggerDSP;
		break;
	}
	case EffectType::Wobble:
	{
		WobbleDSP* wb = new WobbleDSP();
		audioTrack->AddDSP(wb);
		wb->SetLength(actualLength);
		ret = wb;
		break;
	}
	case EffectType::Phaser:
	{
		PhaserDSP* phs = new PhaserDSP();
		audioTrack->AddDSP(phs);
		phs->SetLength(actualLength);
		phs->dmin = phaser.min.Sample(filterInput);
		phs->dmax = phaser.max.Sample(filterInput);
		phs->fb = phaser.feedback.Sample(filterInput);
		ret = phs;
		break;
	}
	case EffectType::Flanger:
	{
		FlangerDSP* fl = new FlangerDSP();
		audioTrack->AddDSP(fl);
		fl->SetLength(actualLength);
		fl->SetDelayRange(flanger.offset.Sample(filterInput),
			flanger.depth.Sample(filterInput));
		ret = fl;
		break;
	}
	}

	return ret;
}
void GameAudioEffect::SetParams(DSP* dsp, AudioPlayback& playback, HoldObjectState* object)
{
	const TimingPoint& tp = playback.GetBeatmapPlayback().GetCurrentTimingPoint();
	double noteDuration = tp.GetWholeNoteLength();

	switch(type)
	{
	case EffectType::Bitcrush:
	{
		BitCrusherDSP* bcDSP = (BitCrusherDSP*)dsp;
		bcDSP->SetPeriod((float)object->effectParams[0]);
		break;
	}
	case EffectType::Gate:
	{
		GateDSP* gateDSP = (GateDSP*)dsp;
		gateDSP->SetLength((uint32)(noteDuration / object->effectParams[0]));
		gateDSP->SetGating(0.75f);
		break;
	}
	case EffectType::TapeStop:
	{
		TapeStopDSP* tapestopDSP = (TapeStopDSP*)dsp;
		double speed = 1.0 - (double)object->effectParams[0] / 100.0;
		tapestopDSP->SetLength((uint32)(speed * (float)object->duration));
		break;
	}
	case EffectType::Retrigger:
	{
		RetriggerDSP* retriggerDSP = (RetriggerDSP*)dsp;
		retriggerDSP->SetLength((uint32)(noteDuration / object->effectParams[0]));
		retriggerDSP->SetGating(0.65f);
		break;
	}
	case EffectType::Wobble:
	{
		WobbleDSP* wb = (WobbleDSP*)dsp;
		wb->SetLength((uint32)(noteDuration / object->effectParams[0]));
		break;
	}
	case EffectType::Phaser:
	{
		PhaserDSP* phs = (PhaserDSP*)dsp;
		phs->time = object->time;
		break;
	}
	case EffectType::Flanger:
	{
		FlangerDSP* fl = (FlangerDSP*)dsp;
		double delay = (noteDuration) / 1000.0;
		fl->SetLength((uint32)(noteDuration / object->effectParams[0]));
		fl->SetDelayRange(2, 40);
		break;
	}
	}
}
void GameAudioEffect::UpdateDSP(DSP* dsp, AudioPlayback& playback, float filterInput)
{
	switch(type)
	{
	case EffectType::Bitcrush:
	{
		BitCrusherDSP* bcDSP = (BitCrusherDSP*)dsp;
		bcDSP->SetPeriod((float)bitcrusher.reduction.Sample(filterInput));
		break;
	}
	case EffectType::Echo:
	{
		EchoDSP* echoDSP = (EchoDSP*)dsp;
		echoDSP->feedback = echo.feedback.Sample(filterInput);
		break;
	}
	case EffectType::PeakingFilter:
	{
		BQFDSP* bqfDSP = (BQFDSP*)dsp;
		bqfDSP->SetPeaking(peaking.q.Sample(filterInput), peaking.freq.Sample(filterInput), peaking.gain.Sample(filterInput));
		break;
	}
	case EffectType::LowPassFilter:
	{
		CombinedFilterDSP* bqfDSP = (CombinedFilterDSP*)dsp;
		bqfDSP->SetLowPass(lpf.q.Sample(filterInput), lpf.freq.Sample(filterInput), lpf.peakQ.Sample(filterInput), lpf.gain.Sample(filterInput));
		break;
	}
	case EffectType::HighPassFilter:
	{
		CombinedFilterDSP* bqfDSP = (CombinedFilterDSP*)dsp;
		bqfDSP->SetHighPass(hpf.q.Sample(filterInput), hpf.freq.Sample(filterInput), hpf.peakQ.Sample(filterInput), hpf.gain.Sample(filterInput));
		break;
	}
	default:
		break;
	}
}
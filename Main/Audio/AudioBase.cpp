#include "stdafx.h"
#include "AudioBase.hpp"
#include "Audio.hpp"
#include "Audio_Impl.hpp"

DSP::~DSP()
{
	// Make sure this is removed from parent
	assert(!audioBase);
}

AudioBase::~AudioBase()
{
	// Check this to make sure the audio is not being destroyed while it is still registered
	assert(!audio);
	assert(DSPs.empty());
}
void AudioBase::ProcessDSPs(float*& out, uint32 numSamples)
{
	for(DSP* dsp : DSPs)
	{
		dsp->Process(out, numSamples);
	}
}
void AudioBase::AddDSP(DSP* dsp)
{
	audio->lock.lock();
	DSPs.AddUnique(dsp);
	// Sort by priority
	DSPs.Sort([](DSP* l, DSP* r)
	{
		if(l->priority == r->priority)
			return l < r;
		return l->priority < r->priority;
	});
	dsp->audioBase = this;
	dsp->audio = audio;
	audio->lock.unlock();
}
void AudioBase::RemoveDSP(DSP* dsp)
{
	audio->lock.lock();
	DSPs.Remove(dsp);
	assert(!DSPs.Contains(dsp));
	dsp->audioBase = nullptr;
	dsp->audio = nullptr;
	audio->lock.unlock();
}

void AudioBase::Deregister()
{
	// Remove from audio manager
	if(audio)
	{
		audio->Deregister(this);
	}

	// Unbind DSP's
	// It is safe to do here since the audio won't be rendered again after a call to deregister
	for(DSP* dsp : DSPs)
	{
		dsp->audioBase = nullptr;
	}
	DSPs.clear();
}

#pragma once

class DSP
{
public:
	virtual ~DSP();
	// Process <numSamples> amount of samples in stereo float format
	virtual void Process(float*& out, uint32 numSamples) = 0;

	uint32 priority = 0;
	class AudioBase* audioBase = nullptr;
	class Audio_Impl* audio = nullptr;
};

class AudioBase
{
public:
	virtual ~AudioBase();
	// Process <numSamples> amount of samples in stereo float format
	virtual void Process(float*& out, uint32 numSamples) = 0;
	void ProcessDSPs(float*& out, uint32 numSamples);
	// Adds a signal processor to the audio
	void AddDSP(DSP* dsp);
	// Removes a signal processor from the audio
	void RemoveDSP(DSP* dsp);

	void Deregister();

	Vector<DSP*> DSPs;
	class Audio_Impl* audio = nullptr;
};
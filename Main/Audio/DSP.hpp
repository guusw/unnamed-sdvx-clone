#pragma once
#include "AudioBase.hpp"

class PanDSP : public DSP
{
public:
	// -1 to 1 LR pan value
	float panning = 0.0f;
	virtual void Process(float*& out, uint32 numSamples);
};

// Biquad Filter
// Thanks to https://www.youtube.com/watch?v=FnpkBE4kJ6Q&list=WL&index=8 for the explanation
// Also http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt for the coefficient formulas
class BQFDSP : public DSP
{
public:
	float b0 = 1.0f;
	float b1 = 0.0f;
	float b2 = 0.0f;
	float a0 = 1.0f;
	float a1 = 0.0f;
	float a2 = 0.0f;

	virtual void Process(float*& out, uint32 numSamples);

	// Sets the filter to behave as a peaking filter
	void SetPeaking(float bandWidth, float freq, float gain);
	void SetLowPass(float q, float freq);
	void SetHighPass(float q, float freq);
	void SetBandPass(float bandWidth, float freq);

private:
	// Delayed samples
	static const uint32 order = 2;
	// FIR Delay buffers
	float zb[2][order];
	// IIR Delay buffers
	float za[2][order];
};

// Basic limiter
class LimiterDSP : public DSP
{
public:
	float releaseTime = 0.1f;
	virtual void Process(float*& out, uint32 numSamples);
private:
	float m_currentMaxVolume = 1.0f;
	float m_currentReleaseTimer = releaseTime;
};

class BitCrusherDSP : public DSP
{
public:
	// Duration of samples, 1 = disable
	uint32 period = 1;
	float mix = 0.8f;

	virtual void Process(float*& out, uint32 numSamples);
private:
	float m_sampleBuffer[2] = { 0.0f };
	uint32 m_currentDuration = 0;
};

class GateDSP : public DSP
{
public:
	// The amount of time for a single cycle
	uint32 delay = 0;
	float low = 0.1f;

	virtual void Process(float*& out, uint32 numSamples);
private:
	uint32 m_currentSample = 0;
};

class TapeStopDSP : public DSP
{
public:
	// Length of the effect
	uint32 delay = 0;

	virtual void Process(float*& out, uint32 numSamples);
private:
	Vector<float> m_sampleBuffer;
	float m_sampleIdx = 0.0f;
	uint32 m_lastSample = 0;
	uint32 m_currentSample = 0;
};

class RetriggerDSP : public DSP
{
public:
	// Length of the effect
	uint32 delay = 0;

	virtual void Process(float*& out, uint32 numSamples);
private:
	Vector<float> m_sampleBuffer;
	uint32 m_loops = 0;
	uint32 m_currentSample = 0;
};

class WobbleDSP : public BQFDSP
{
public:
	uint32 delay;
	virtual void Process(float*& out, uint32 numSamples);
private:
	uint32 m_currentSample = 0;
};
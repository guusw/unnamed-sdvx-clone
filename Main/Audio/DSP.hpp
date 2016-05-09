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

	float mix = 1.0f;

	virtual void Process(float*& out, uint32 numSamples);

	// Sets the filter to behave as a peaking filter
	void SetPeaking(float bandWidth, float freq, float gain);
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
	float releaseTime = 0.6f;
	virtual void Process(float*& out, uint32 numSamples);
private:
	float m_currentMaxVolume = 1.0f;
	float m_currentReleaseTimer = releaseTime;
};
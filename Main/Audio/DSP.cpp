#include "stdafx.h"
#include "DSP.hpp"
#include "AudioOutput.hpp"
#include "Audio_Impl.hpp"

void PanDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		if(panning > 0)
			out[i * 2 + 0] = out[i * 2 + 0] * (1.0f - panning);
		if(panning < 0)
			out[i * 2 + 1] = out[i * 2 + 1] * (1.0f + panning);
	}
}

void BQFDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 c = 0; c < 2; c++)
	{
		for(uint32 i = 0; i < numSamples; i++)
		{
			float& sample = out[i * 2 + c];
			float src = sample;

			//float filtered =
			//	src * (b0/a0) +
			//	zb[c][0] * (b1/a0) +
			//	zb[c][1] * (b2/a0);
			//
			//filtered -=
			//	za[c][0] * (a1/a0) -
			//	za[c][1] * (a2/a0);

			float filtered = 
				(b0 / a0) * src + 
				(b1 / a0) * zb[c][0] + 
				(b2 / a0) * zb[c][1] - 
				(a1 / a0) * za[c][0] - 
				(a2 / a0) * za[c][1];
			sample = filtered;

			// Shift delay buffers
			zb[c][1] = zb[c][0];
			zb[c][0] = src;

			// Feedback the calculated value into the IIR delay buffers
			za[c][1] = za[c][0];
			za[c][0] = sample;
		}
	}
}
void BQFDSP::SetPeaking(float bandWidth, float freq, float gain)
{
	// Sampling frequency
	double fs = audioBase->audio->output->GetSampleRate();
	double w0 = (2 * Math::pi * freq) / fs;
	double cw0 = cos(w0);
	double sw0 = sin(w0);
	double alpha = sin(w0)*sinh(log(2) / 2 * bandWidth * w0 / sin(w0));
	double A = sqrt(pow(10, (gain / 20)));

	b0 = 1 + (float)(alpha * A);
	b1 = -2 * (float)cos(w0);
	b2 = 1 - (float)(alpha*A);
	a0 = 1 + (float)(alpha / A);
	a1 = -2 * (float)cos(w0);
	a2 = 1 - (float)(alpha / A); 
}
void BQFDSP::SetBandPass(float bandWidth, float freq)
{
	double fs = audioBase->audio->output->GetSampleRate();
	double w0 = (2 * Math::pi * freq) / fs;
	double cw0 = cos(w0);
	double sw0 = sin(w0);
	float alpha = (float)(sin(w0)*sinh(log(2) / 2 * bandWidth * w0 / sin(w0)));

	b0 = alpha;
	b1 = 0;
	b2 = -alpha;
	a0 = 1 + alpha;
	a1 = -2 * (float)cos(w0);
	a2 = 1 - alpha;
}

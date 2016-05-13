#include "stdafx.h"
#include "DSP.hpp"
#include "AudioOutput.hpp"
#include "Audio_Impl.hpp"

void PanDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		if(panning > 0)
			out[i * 2 + 0] = (out[i * 2 + 0] * (1.0f - panning)) * mix + out[i * 2 + 0] * (1 - mix);
		if(panning < 0)
			out[i * 2 + 1] = (out[i * 2 + 1] * (1.0f + panning)) * mix + out[i * 2 + 1] * (1 - mix);
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

			float filtered = 
				(b0 / a0) * src + 
				(b1 / a0) * zb[c][0] + 
				(b2 / a0) * zb[c][1] - 
				(a1 / a0) * za[c][0] - 
				(a2 / a0) * za[c][1];

			// Shift delay buffers
			zb[c][1] = zb[c][0];
			zb[c][0] = src;

			// Feedback the calculated value into the IIR delay buffers
			za[c][1] = za[c][0];
			za[c][0] = filtered;

			sample = filtered * mix + sample * (1 - mix);
		}
	}
}
void BQFDSP::SetPeaking(float bandWidth, float freq, float gain)
{
	// Sampling frequency
	double fs = audio->GetSampleRate();
	double w0 = (2 * Math::pi * freq) / fs;
	double cw0 = cos(w0);
	double sw0 = sin(w0);
	float alpha = (float)(sin(w0)*sinh(log(2) / 2 * bandWidth * w0 / sin(w0)));
	double A = sqrt(pow(10, (gain / 20)));

	b0 = 1 + (float)(alpha * A);
	b1 = -2 * (float)cos(w0);
	b2 = 1 - (float)(alpha*A);
	a0 = 1 + (float)(alpha / A);
	a1 = -2 * (float)cos(w0);
	a2 = 1 - (float)(alpha / A); 
}

void BQFDSP::SetLowPass(float q, float freq)
{
	assert(freq > 0.0);

	// Sampling frequency
	double fs = audio->GetSampleRate();
	double w0 = (2 * Math::pi * freq) / fs;
	double cw0 = cos(w0);
	double sw0 = sin(w0);
	float alpha = (float)(sin(w0) / (2 * q));

	b0 = (float)((1 - cos(w0)) / 2);
	b1 = (float)(1 - cos(w0));
	b2 = (float)((1 - cos(w0)) / 2);
	a0 = 1 + alpha;
	a1 = (float)(-2 * cos(w0));
	a2 = 1 - alpha;

}
void BQFDSP::SetHighPass(float q, float freq)
{
	// Sampling frequency
	double fs = audio->GetSampleRate();
	assert(freq < fs);
	double w0 = (2 * Math::pi * freq) / fs;
	double cw0 = cos(w0);
	double sw0 = sin(w0);
	float alpha = (float)(sin(w0) / (2 * q));

	b0 = (float)((1 + cos(w0)) / 2);
	b1 = (float)-(1 + cos(w0));
	b2 = float((1 + cos(w0)) / 2);
	a0 = 1 + alpha;
	a1 = (float)(-2 * cos(w0));
	a2 = 1 - alpha;

}
void BQFDSP::SetBandPass(float bandWidth, float freq)
{
	// Sampling frequency
	double fs = audio->GetSampleRate();
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

void LimiterDSP::Process(float*& out, uint32 numSamples)
{
	float secondsPerSample = (float)audio->GetSecondsPerSample();
	for(uint32 i = 0; i < numSamples; i++)
	{
		float currentGain = 1.0f;
		if(m_currentReleaseTimer < releaseTime)
		{
			float t = (1.0f - m_currentReleaseTimer / releaseTime);
			currentGain = (1.0f / m_currentMaxVolume) * t + (1.0f - t);
		}
	
		float maxVolume = Math::Max(abs(out[i*2]), abs(out[i * 2 + 1]));
		out[i*2] *= currentGain;
		out[i*2+1] *= currentGain;
	
		float currentMax = 1.0f / currentGain;
		if(maxVolume > currentMax)
		{
			m_currentMaxVolume = maxVolume;
			m_currentReleaseTimer = 0.0f;
		}
		else
		{
			m_currentReleaseTimer += secondsPerSample;
		}
	}
}

void BitCrusherDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		m_currentDuration++;
		if(m_currentDuration > period)
		{
			m_sampleBuffer[0] = out[i * 2];
			m_sampleBuffer[1] = out[i*2+1];
			m_currentDuration = 0;
		}

		out[i * 2] = m_sampleBuffer[0] * mix + out[i * 2] * (1.0f - mix);
		out[i * 2 + 1] = m_sampleBuffer[1] * mix + out[i * 2+1] * (1.0f - mix);
	}
}

void GateDSP::Process(float*& out, uint32 numSamples)
{
	if(delay < 2)
		return;

	uint32 halfwayDelay = delay / 2;
	for(uint32 i = 0; i < numSamples; i++)
	{
		if(m_currentSample > halfwayDelay)
		{
			// Dim volume
			out[i * 2] *= low * mix + (1 - mix) * 1;
			out[i * 2 + 1] *= low * mix + (1 - mix) * 1;
		}

		m_currentSample++;
		m_currentSample %= delay;
	}
}
void TapeStopDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		float sampleRate = 1.0f - (float)m_currentSample / (float)delay;
		if(sampleRate == 0.0f)
		{
			// Mute
			out[i * 2] = 0.0f;
			out[i * 2+1] = 0.0f;
			continue;
		}

		// Store samples for later
		m_sampleBuffer.Add(out[i*2]);
		m_sampleBuffer.Add(out[i*2+1]);

		// The sample index into the buffer
		uint32 i2 = (uint32)floor(m_sampleIdx);
		out[i * 2] = m_sampleBuffer[i2 * 2] * mix + out[i * 2] * (1 - mix);
		out[i * 2 + 1] = m_sampleBuffer[i2 * 2 + 1] * mix + out[i * 2+1] * (1 - mix);

		// Increase index
		m_sampleIdx += sampleRate;
		m_currentSample++;
	}
}

void RetriggerDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		if(m_loops < 0)
		{
			// Store samples for later
			m_sampleBuffer.Add(out[i * 2]);
			m_sampleBuffer.Add(out[i * 2 + 1]);
		}

		// Sample from buffer
		out[i * 2] = m_sampleBuffer[m_currentSample*2] * mix + out[i * 2] * (1 - mix);
		out[i * 2 + 1] = m_sampleBuffer[m_currentSample*2+1] * mix + out[i * 2+1] * (1 - mix);
		
		// Increase index
		m_currentSample++;
		if(m_currentSample > delay)
		{
			m_currentSample -= delay;
			m_loops++;
		}
	}
}

void WobbleDSP::Process(float*& out, uint32 numSamples)
{
	float f = 1- abs(cos(Math::pi * ((float)m_currentSample / (float)delay)));
	float freq = 50.0f + f * audio->GetSampleRate() * 0.2f;
	SetLowPass(1.5f, freq);

	BQFDSP::Process(out, numSamples);

	m_currentSample += numSamples;
	m_currentSample %= delay;
}

void PhaserDSP::Process(float*& out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		float f = ((float)time / (float)delay) * Math::pi * 2.0f;

		//calculate and update phaser sweep lfo...
		float d = dmin + (dmax - dmin) * ((sin(f) + 1.0f) / 2.0f);
		d /= (float)audio->GetSampleRate();

		//calculate output per channel
		for(uint32 c = 0; c < 2; c++)
		{
			APF* filters1 = filters[c];

			//update filter coeffs
			float a1 = (1.f - d) / (1.f + d);
			for(int i = 0; i < 6; i++)
				filters1[i].a1 = a1;

			float filtered = filters1[0].Update(
				filters1[1].Update(
					filters1[2].Update(
						filters1[3].Update(
							filters1[4].Update(
								filters1[5].Update(out[i * 2 + c] + za[c] * fb))))));
			// Store filter feedback
			za[c] = filtered;

			// Final sample
			out[i * 2 + c] = out[i * 2 + c] + filtered * mix * depth;
		}

		time++;
	}
}
float PhaserDSP::APF::Update(float in)
{
	float y = in * -a1 + za;
	za = y * a1 + in;
	return y;
}

void FlangerDSP::Process(float*& out, uint32 numSamples)
{
	if(m_sampleBuffer.size() != (max*2))
		m_sampleBuffer.resize(max*2);

	float* data = m_sampleBuffer.data();

	for(uint32 i = 0; i < numSamples; i++)
	{
		float f = ((float)time / (float)delay) * Math::pi * 2.0f;
		uint32 d = (uint32)(min + ((max - 1) - min) * (sin(f) * 0.5f + 0.5f));


		// Shift samples
		float s0 = data[0];
		float s1 = data[1];
		for(uint32 i = 1; i < m_sampleBuffer.size() / 2; i++)
		{
			float t;
			t = data[i * 2];
			data[i * 2] = s0;
			s0 = t;
			t = data[i * 2+1];
			data[i * 2+1] = s1;
			s1 = t;
		}
		// Inject new sample
		data[0] = out[i*2];
		data[1] = out[i*2+1];

		// Apply delay
		out[i * 2] += data[d * 2] * mix + out[i * 2] * (1 - mix);
		out[i * 2 + 1] += data[d * 2+1] * mix + out[i * 2+1] * (1 - mix);

		time++;
	}
}

#include "stdafx.h"
#include "DSP.hpp"
#include "AudioOutput.hpp"
#include "Audio_Impl.hpp"

void PanDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		if(panning > 0)
			out[i * 2 + 0] = (out[i * 2 + 0] * (1.0f - panning)) * mix + out[i * 2 + 0] * (1 - mix);
		if(panning < 0)
			out[i * 2 + 1] = (out[i * 2 + 1] * (1.0f + panning)) * mix + out[i * 2 + 1] * (1 - mix);
	}
}

void BQFDSP::Process(float* out, uint32 numSamples)
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

			sample = filtered;
		}
	}
}
void BQFDSP::SetLowPass(float q, float freq, float sampleRate)
{
	// Limit q
	q = Math::Max(q, 0.01f);

	// Sampling frequency
	double w0 = (2 * Math::pi * freq) / sampleRate;
	double cw0 = cos(w0);
	float alpha = (float)(sin(w0) / (2 * q));

	b0 = (float)((1 - cw0) / 2);
	b1 = (float)(1 - cw0);
	b2 = (float)((1 - cw0) / 2);
	a0 = 1 + alpha;
	a1 = (float)(-2 * cw0);
	a2 = 1 - alpha;
}
void BQFDSP::SetLowPass(float q, float freq)
{
	SetLowPass(q, freq, (float)audio->GetSampleRate());
}
void BQFDSP::SetHighPass(float q, float freq, float sampleRate)
{
	// Limit q
	q = Math::Max(q, 0.01f);

	assert(freq < sampleRate);
	double w0 = (2 * Math::pi * freq) / sampleRate;
	double cw0 = cos(w0);
	float alpha = (float)(sin(w0) / (2 * q));

	b0 = (float)((1 + cw0) / 2);
	b1 = (float)-(1 + cw0);
	b2 = float((1 + cw0) / 2);
	a0 = 1 + alpha;
	a1 = (float)(-2 * cw0);
	a2 = 1 - alpha;
}
void BQFDSP::SetHighPass(float q, float freq)
{
	SetHighPass(q, freq, (float)audio->GetSampleRate());
}
void BQFDSP::SetPeaking(float q, float freq, float gain, float sampleRate)
{
	// Limit q
	q = Math::Max(q, 0.01f);

	double w0 = (2 * Math::pi * freq) / sampleRate;
	double cw0 = cos(w0);
	float alpha = (float)(sin(w0) / (2 * q));
	double A = pow(10, (gain / 40));

	b0 = 1 + (float)(alpha * A);
	b1 = -2 * (float)cw0;
	b2 = 1 - (float)(alpha*A);
	a0 = 1 + (float)(alpha / A);
	a1 = -2 * (float)cw0;
	a2 = 1 - (float)(alpha / A);
}
void BQFDSP::SetPeaking(float q, float freq, float gain)
{
	SetPeaking(q, freq, gain, (float)audio->GetSampleRate());
}

void LimiterDSP::Process(float* out, uint32 numSamples)
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
		out[i * 2] *= currentGain * 0.9f;
		out[i * 2 + 1] *= currentGain * 0.9f;
	
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

void BitCrusherDSP::SetPeriod(float period /*= 0*/)
{
	// Scale period with sample rate
	assert(audio);
	double f = audio->GetSampleRate() / 44100.0;
	m_increment = (uint32)((double)(1 << 16));
	m_period = (uint32)(f * period * (double)(1 << 16));
}
void BitCrusherDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		m_currentDuration += m_increment;
		if(m_currentDuration > m_period)
		{
			m_sampleBuffer[0] = out[i * 2];
			m_sampleBuffer[1] = out[i*2+1];
			m_currentDuration -= m_period;
		}

		out[i * 2] = m_sampleBuffer[0] * mix + out[i * 2] * (1.0f - mix);
		out[i * 2 + 1] = m_sampleBuffer[1] * mix + out[i * 2+1] * (1.0f - mix);
	}
}

void GateDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
	SetGating(m_gating);
}
void GateDSP::SetGating(float gating)
{
	float flength = (float)m_length;
	m_gating = gating;
	m_halfway = (uint32)(flength * gating);
	const float fadeDuration = Math::Min(0.05f, gating * 0.5f);
	m_fadeIn = (uint32)((float)m_halfway * fadeDuration);
	m_fadeOut = (uint32)((float)m_halfway * (1.0f - fadeDuration));
	m_currentSample = 0;
}

void GateDSP::Process(float* out, uint32 numSamples)
{
	if(m_length < 2)
		return;

	for(uint32 i = 0; i < numSamples; i++)
	{
		float c = 1.0f;
		if(m_currentSample < m_halfway)
		{
			// Fade out before silence
			if(m_currentSample > m_fadeOut)
				c = 1-(float)(m_currentSample - m_fadeOut) / (float)m_fadeIn;
		}
		else
		{
			uint32 t = m_currentSample - m_halfway;
			// Fade in again
			if(t > m_fadeOut)
				c = (float)(t - m_fadeOut) / (float)m_fadeIn;
			else
				c = 0.0f;
		}

		// Multiply volume
		c = (c * (1 - low) + low); // Range [low, 1]
		c = c * mix + (1.0f-mix);
		out[i * 2] *= c;
		out[i * 2 + 1] *= c;

		m_currentSample++;
		m_currentSample %= m_length;
	}
}

void TapeStopDSP::SetLength(uint32 length)
{
	assert(audio);

	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
	m_sampleBuffer.clear();
	m_sampleBuffer.reserve(length);
}
void TapeStopDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		float sampleRate = 1.0f - (float)m_currentSample / (float)m_length;
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

void RetriggerDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
	SetGating(m_gating);
}
void RetriggerDSP::SetResetDuration(uint32 resetDuration)
{
	float flength = (float)resetDuration / 1000.0f * (float)audio->GetSampleRate();
	m_resetDuration = (uint32)flength;
}
void RetriggerDSP::SetGating(float gating)
{
	m_gating = gating;
	m_gateLength = (uint32)((float)m_length * gating);
}
void RetriggerDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		if(m_loops == 0)
		{
			// Store samples for later
			if(m_currentSample > m_gateLength) // Additional gating
			{
				m_sampleBuffer.Add(0.0f);
				m_sampleBuffer.Add(0.0f);
			}
			else
			{
				m_sampleBuffer.Add(out[i * 2]);
				m_sampleBuffer.Add(out[i * 2 + 1]);
			}
		}

		// Sample from buffer
		out[i * 2] = m_sampleBuffer[m_currentSample*2] * mix + out[i * 2] * (1 - mix);
		out[i * 2 + 1] = m_sampleBuffer[m_currentSample*2+1] * mix + out[i * 2+1] * (1 - mix);
		
		// Increase index
		m_currentSample++;
		if(m_currentSample > m_length)
		{
			m_currentSample -= m_length;
			m_loops++;
		}
	}
}

void WobbleDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
}
void WobbleDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		float f = abs(2.0f * ((float)m_currentSample / (float)m_length) - 1.0f);
		f = 1.0f - pow(f, 1.5f) * 0.4f;
		float freq = (float)pow(22000.0f, f);
		SetLowPass(0.7f, freq);

		BQFDSP::Process(&out[i * 2], 1);
		m_currentSample++;
		m_currentSample %= m_length;
	}
}

void PhaserDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
}
void PhaserDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		float f = ((float)time / (float)m_length) * Math::pi * 2.0f;

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
			out[i * 2 + c] = out[i * 2 + c] + filtered * mix;
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

void FlangerDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
}
void FlangerDSP::SetDelayRange(uint32 min, uint32 max)
{
	assert(max > min);
	m_min = min;
	m_max = max;
	m_bufferLength = m_max * 2 + 8;
	m_sampleBuffer.resize(m_bufferLength);
}
void FlangerDSP::Process(float* out, uint32 numSamples)
{
	float* data = m_sampleBuffer.data();

	if(m_bufferLength <= 0)
		return;

	for(uint32 i = 0; i < numSamples; i++)
	{
		// Determine where we want to sample past samples
		float f = ((float)m_time / (float)m_length) * Math::pi * 2.0f;
		uint32 d = (uint32)(m_min + ((m_max - 1) - m_min) * (sin(f) * 0.5f + 0.5f));
		uint32 samplePos = (m_bufferOffset - d * 2) % m_bufferLength;


		// Inject new sample
		data[m_bufferOffset + 0] = out[i*2];
		data[m_bufferOffset + 1] = out[i*2+1];

		// Apply delay
		out[i * 2] = (data[samplePos] + out[i*2]) * 0.5f * mix +
			out[i * 2] * (1 - mix);
		out[i * 2 + 1] = (data[samplePos+1] + out[i*2+1]) * 0.5f * mix +
			out[i * 2+1] * (1 - mix);

		m_bufferOffset += 2;
		if(m_bufferOffset >= m_bufferLength)
			m_bufferOffset = 0;
		m_time++;
	}
}

void EchoDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_sampleBuffer.clear();
	m_bufferLength = (uint32)flength * 2;
	m_sampleBuffer.resize(m_bufferLength);
	memset(m_sampleBuffer.data(), 0, sizeof(float) * m_bufferLength);
	m_numLoops = 0;
}
void EchoDSP::Process(float* out, uint32 numSamples)
{
	float* data = m_sampleBuffer.data();

	for(uint32 i = 0; i < numSamples; i++)
	{
		// Insert new samples before current position
		size_t insertPos = (m_bufferOffset - 2) % m_bufferLength;

		float l0 = data[m_bufferOffset + 0];
		float l1 = data[m_bufferOffset + 1];

		if(m_numLoops > 0)
		{
			// Send echo to output
			out[i * 2] = l0 * mix;
			out[i * 2 + 1] = l1 * mix;
		}

		// Inject new sample
		data[insertPos + 0] = out[i * 2] * feedback;
		data[insertPos + 1] = out[i * 2 + 1] * feedback;

		m_bufferOffset += 2;
		if(m_bufferOffset >= m_bufferLength)
		{
			m_bufferOffset = 0;
			m_numLoops++;
		}
	}
}

void SidechainDSP::SetLength(uint32 length)
{
	float flength = (float)length / 1000.0f * (float)audio->GetSampleRate();
	m_length = (uint32)flength;
	m_time = 0;
}
void SidechainDSP::Process(float* out, uint32 numSamples)
{
	for(uint32 i = 0; i < numSamples; i++)
	{
		float r = (float)m_time / (float)m_length;
		float sampleGain = 1.0f - low * (1.0f-r);
		if(++m_time > m_length)
		{
			out[numSamples * 2 + 0] *= sampleGain;
			out[numSamples * 2 + 1] *= sampleGain;
			m_time = 0;
		}
	}
}

void CombinedFilterDSP::SetLowPass(float q, float freq, float peakQ, float peakGain)
{
	float sr = (float)audio->GetSampleRate();
	a.SetLowPass(q, freq, sr);
	peak.SetPeaking(peakQ, freq, peakGain, sr);
}
void CombinedFilterDSP::SetHighPass(float q, float freq, float peakQ, float peakGain)
{
	float sr = (float)audio->GetSampleRate();
	a.SetHighPass(q, freq, sr);
	peak.SetPeaking(peakQ, freq, peakGain, sr);
}
void CombinedFilterDSP::Process(float* out, uint32 numSamples)
{
	a.mix = mix;
	peak.mix = mix;
	a.Process(out, numSamples);
	peak.Process(out, numSamples);
}

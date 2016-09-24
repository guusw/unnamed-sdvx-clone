#include "stdafx.h"
#include "Audio.hpp"
#include "AudioStream.hpp"
#include "Audio_Impl.hpp"
#include "AudioOutput.hpp"
#include "DSP.hpp"

Audio* g_audio = nullptr;
Audio_Impl impl;

void Audio_Impl::Mix(float* data, uint32& numSamples)
{
#if _DEBUG
	static const uint32 guardBand = 1024;
#else
	static const uint32 guardBand = 0;
#endif

	// Per-Channel data buffer
	float* tempData = new float[m_sampleBufferLength * 2 + guardBand];
	uint32* guardBuffer = (uint32*)tempData + 2 * m_sampleBufferLength;
	double adv = GetSecondsPerSample();

	uint32 currentNumberOfSamples = 0;
	while(currentNumberOfSamples < numSamples)
	{
		// Generate new sample
		if(m_remainingSamples <= 0)
		{
			// Clear sample buffer storing a fixed amount of samples
			memset(m_sampleBuffer, 0, sizeof(float) * 2 * m_sampleBufferLength);

			// Render items
			lock.lock();
			for(auto& item : itemsToRender)
			{
				// Clearn per-channel data (and guard buffer in debug mode)
				memset(tempData, 0, sizeof(float) * (2 * m_sampleBufferLength + guardBand));
				item->Process(tempData, m_sampleBufferLength);
#if _DEBUG
				// Check for memory corruption
				for(uint32 i = 0; i < guardBand; i++)
				{
					assert(guardBuffer[i] == 0);
				}
#endif
				item->ProcessDSPs(tempData, m_sampleBufferLength);
#if _DEBUG
				// Check for memory corruption
				for(uint32 i = 0; i < guardBand; i++)
				{
					assert(guardBuffer[i] == 0);
				}
#endif

				// Mix into buffer and apply volume scaling
				for(uint32 i = 0; i < m_sampleBufferLength; i++)
				{
					m_sampleBuffer[i * 2 + 0] += tempData[i * 2] * item->GetVolume();
					m_sampleBuffer[i * 2 + 1] += tempData[i * 2 + 1] * item->GetVolume();
				}
			}

			// Process global DSPs
			for(auto dsp : globalDSPs)
			{
				dsp->Process(m_sampleBuffer, m_sampleBufferLength);
			}
			lock.unlock();

			// Apply volume levels
			for(uint32 i = 0; i < m_sampleBufferLength; i++)
			{
				m_sampleBuffer[i * 2 + 0] *= globalVolume;
				m_sampleBuffer[i * 2 + 1] *= globalVolume;
			}

			// Set new remaining buffer data
			m_remainingSamples = m_sampleBufferLength;
		}

		// Copy samples from sample buffer
		uint32 sampleOffset = m_sampleBufferLength - m_remainingSamples;
		uint32 maxSamples = Math::Min(numSamples - currentNumberOfSamples, m_remainingSamples);
		memcpy(data + currentNumberOfSamples * 2, m_sampleBuffer + sampleOffset * 2, sizeof(float) * maxSamples * 2);
		m_remainingSamples -= maxSamples;
		currentNumberOfSamples += maxSamples;
	}

	delete[] tempData;
}
void Audio_Impl::Start()
{
	m_sampleBuffer = new float[2 * m_sampleBufferLength];

	limiter = new LimiterDSP();
	limiter->audio = this;
	limiter->releaseTime = 0.2f;
	globalDSPs.Add(limiter);
	output->Start(this);
}
void Audio_Impl::Stop()
{
	output->Stop();
	delete limiter;
	globalDSPs.Remove(limiter);

	delete[] m_sampleBuffer;
	m_sampleBuffer = nullptr;
}
void Audio_Impl::Register(AudioBase* audio)
{
	lock.lock();
	itemsToRender.AddUnique(audio);
	audio->audio = this;
	lock.unlock();
}
void Audio_Impl::Deregister(AudioBase* audio)
{
	lock.lock();
	itemsToRender.Remove(audio);
	audio->audio = nullptr;
	lock.unlock();
}
uint32 Audio_Impl::GetSampleRate() const
{
	return output->GetSampleRate();
}
double Audio_Impl::GetSecondsPerSample() const
{
	return 1.0 / (double)GetSampleRate();
}

Audio::Audio()
{
	// Enforce single instance
	assert(g_audio == nullptr);
	g_audio = this;
}
Audio::~Audio()
{
	if(m_initialized)
	{
		impl.Stop();
		delete impl.output;
		impl.output = nullptr;
	}

	assert(g_audio == this);
	g_audio = nullptr;
}
bool Audio::Init()
{
	audioLatency = 0;

	impl.output = new AudioOutput();
	if(!impl.output->Init())
	{
		delete impl.output;
		impl.output = nullptr;
		return false;
	}

	impl.Start();

	return m_initialized = true;
}
void Audio::SetGlobalVolume(float vol)
{
	impl.globalVolume = vol;
}
uint32 Audio::GetSampleRate() const
{
	return impl.output->GetSampleRate();
}
class Audio_Impl* Audio::GetImpl()
{
	return &impl;
}

AudioStream Audio::CreateStream(const String& path, bool preload)
{
	return AudioStreamRes::Create(this, path, preload);
}
Sample Audio::CreateSample(const String& path)
{
	return SampleRes::Create(this, path);
}

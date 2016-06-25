#include "stdafx.h"
#include "Audio.hpp"
#include <Graphics/Window.hpp>
#include "AudioStream.hpp"
#include "Audio_Impl.hpp"
#include "DSP.hpp"

Audio* g_audio = nullptr;
Audio_Impl impl;

// Main mixer thread
void Audio_Impl::AudioThread()
{
	double f = 0.0;
	int32 sleepDuration = Math::Max(1, (int32)output->GetBufferLength() - 40);
	while(runAudioThread)
	{
		float* data;
		uint32 numSamples;
		if(output->Begin(data, numSamples))
		{
#if _DEBUG
			static const uint32 guardBand = 1024;
#else
			static const uint32 guardBand = 0;
#endif

			float* tempData = new float[numSamples * 2 + guardBand];
			uint32* guardBuffer = (uint32*)tempData + 2 * numSamples;
			double adv = GetSecondsPerSample();

			// Clear buffer
			memset(data, 0, sizeof(float) * 2 * numSamples);

			// Render items
			lock.lock();
			for(auto& item : itemsToRender)
			{
				memset(tempData, 0, sizeof(float) * (2 * numSamples + guardBand));
				item->Process(tempData, numSamples);
#if _DEBUG
				// Check for memory corruption
				for(uint32 i = 0; i < guardBand; i++)
				{
					assert(guardBuffer[i] == 0);
				}
#endif
				item->ProcessDSPs(tempData, numSamples);
#if _DEBUG
				// Check for memory corruption
				for(uint32 i = 0; i < guardBand; i++)
				{
					assert(guardBuffer[i] == 0);
				}
#endif

				// Mix into buffer and apply volume scaling
				for(uint32 i = 0; i < numSamples; i++)
				{
					data[i * 2 + 0] += tempData[i * 2] * item->GetVolume();
					data[i * 2 + 1] += tempData[i * 2 + 1] * item->GetVolume();
				}
			}
			lock.unlock();

			// Apply volume levels
			for(uint32 i = 0; i < numSamples; i++)
			{
				data[i * 2 + 0] *= globalVolume;
				data[i * 2 + 1] *= globalVolume;
			}	

			// Process global DSPs
			for(auto dsp : globalDSPs)
			{
				dsp->Process(data, numSamples);
			}

			f += adv * numSamples;
			output->End(numSamples);

			delete[] tempData;
		}
		Sleep(sleepDuration);
	}
}
void Audio_Impl::Start()
{
	limiter = new LimiterDSP();
	limiter->audio = this;
	limiter->releaseTime = 0.05f;
	globalDSPs.Add(limiter);

	impl.runAudioThread = true;
	impl.audioThread = thread(&Audio_Impl::AudioThread, &impl);
}
void Audio_Impl::Stop()
{
	// Join audio thread
	runAudioThread = false;
	if(audioThread.joinable())
		audioThread.join();

	delete limiter;
	globalDSPs.Remove(limiter);
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
bool Audio::Init(class Window& window)
{
	m_window = &window;
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

AudioStream Audio::CreateStream(const String& path)
{
	return AudioStreamRes::Create(this, path);
}
Sample Audio::CreateSample(const String& path)
{
	return SampleRes::Create(this, path);
}

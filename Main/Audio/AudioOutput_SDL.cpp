#include "stdafx.h"
#include "AudioOutput.hpp"
#include <thread>
using std::this_thread::yield;

#ifdef AUDIO_SDL
#include "SDL.h"
#include "SDL_audio.h"

/* SDL audio instance singleton*/
class SDLAudio
{
	SDLAudio()
	{
		SDL_SetMainReady();
		int r = SDL_InitSubSystem(SDL_INIT_AUDIO);
		assert(r == 0);
	}
public:
	~SDLAudio()
	{
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
	static SDLAudio& Main()
	{
		static SDLAudio sdl;
		return sdl;
	}
};

class AudioOutput_Impl
{
public:
	SDL_AudioSpec m_audioSpec = { 0 };
	SDL_AudioDeviceID m_deviceId = 0;
	uint32 m_samplesInQueue = 0;
	float* m_tempBuffer = nullptr;
	double m_bufferLength = 0;

	volatile bool m_audioThreadRun = true;
	volatile int32 m_bufferAvailable = 0;
	volatile float* m_bufferPtr;
	volatile uint32 m_bufferSamples;

public:
	AudioOutput_Impl()
	{
		SDLAudio::Main();
	}
	~AudioOutput_Impl()
	{
		CloseDevice();
	}
	void CloseDevice()
	{
		m_audioThreadRun = false;
		if(m_tempBuffer)
			delete[] m_tempBuffer;
		m_tempBuffer = nullptr;
		if(m_deviceId != 0)
			SDL_CloseAudioDevice(m_deviceId);
		m_deviceId = 0;
	}
	bool OpenDevice(const char* dev)
	{
		CloseDevice();

		SDL_AudioSpec desiredSpec = { 0 };
		desiredSpec.freq = 44100;
		desiredSpec.format = AUDIO_F32;
		desiredSpec.channels = 2;    /* 1 = mono, 2 = stereo */
		desiredSpec.samples = 1024;  /* Good low-latency value for callback */
		desiredSpec.callback = (SDL_AudioCallback)&AudioOutput_Impl::FillBuffer;
		desiredSpec.userdata = this;

		m_deviceId = SDL_OpenAudioDevice(dev, 0, &desiredSpec, &m_audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
		if(m_deviceId == 0 || m_deviceId < 2)
			return false;

		SDL_PauseAudioDevice(m_deviceId, 0);

		m_bufferLength = (double)m_audioSpec.samples / (double)m_audioSpec.freq;
		m_tempBuffer = new float[m_audioSpec.samples*m_audioSpec.channels];
		m_audioThreadRun = true;
		return true;
	}
	bool Init()
	{
		OpenDevice(nullptr);
		return true;
	}
	bool Begin(float*& buffer, uint32_t& numSamples)
	{
		if(m_bufferAvailable == 0)
			return false;
		buffer = (float*)m_bufferPtr;
		numSamples = m_bufferSamples;
		return true;
	}
	void End(uint32_t numSamples)
	{
		m_bufferAvailable = 0;
	}
	static void SDLCALL FillBuffer(AudioOutput_Impl* self, float* data, int len)
	{
		self->m_bufferSamples = (uint32)(len / (4 * self->m_audioSpec.channels));
		self->m_bufferPtr = data;
		assert(self->m_bufferAvailable == 0);
		self->m_bufferAvailable = 1;
		while(self->m_bufferAvailable > 0 && self->m_audioThreadRun)
		{
			std::this_thread::yield();
		}
	}
};


AudioOutput::AudioOutput()
{
	m_impl = new AudioOutput_Impl();
}
AudioOutput::~AudioOutput()
{
	delete m_impl;
}
bool AudioOutput::Init()
{
	return m_impl->Init();
}
bool AudioOutput::Begin(float*& buffer, uint32_t& numSamples)
{
	return m_impl->Begin(buffer, numSamples);
}
void AudioOutput::End(uint32_t numSamples)
{
	m_impl->End(numSamples);
}
uint32_t AudioOutput::GetNumChannels() const
{
	return m_impl->m_audioSpec.channels;
}
uint32_t AudioOutput::GetSampleRate() const
{
	return m_impl->m_audioSpec.freq;
}
double AudioOutput::GetBufferLength() const
{
	return m_impl->m_bufferLength;
}

#endif
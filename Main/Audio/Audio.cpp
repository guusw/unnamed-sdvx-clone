#include "stdafx.h"
#include "Audio.hpp"
#include "Window.hpp"
#include "bass.h"
#include "bass_fx.h"

Audio* g_audio = nullptr;

Audio::Audio()
{
	// Enforce single instance
	assert(g_audio == nullptr);
	g_audio = this;
}
Audio::~Audio()
{
	// Cleanup BASS
	if(m_initialized)
	{
		BASS_Free();
	}

	assert(g_audio == this);
	g_audio = nullptr;
}
bool Audio::Init(class Window& window)
{
	m_window = &window;

	// Enumerate audio devices
	Vector<BASS_DEVICEINFO> devices;
	BASS_DEVICEINFO devInfo;
	while(BASS_GetDeviceInfo((uint32)devices.size(), &devInfo))
	{
		devices.Add(devInfo);	
	}

	if(devices.size() < 2)
	{
		Log("No audio devices found", Logger::Error);
		return false;
	}
	bool queryLatency = false;
	DWORD flags = 0;
	if(queryLatency)
		flags |= BASS_DEVICE_LATENCY;
	if(!BASS_Init(1, 44100, flags, (HWND)m_window->Handle(), nullptr))
	{
		Logf("Failed to open audio device \"%s\"", Logger::Error, devices[1].name);
	}

	// BFX init
	DWORD bfxVersion = BASS_FX_GetVersion();
	assert(HIWORD(bfxVersion) == BASSVERSION);

	if(queryLatency)
	{
		BASS_INFO bassInfo;
		BASS_GetInfo(&bassInfo);
		audioLatency = bassInfo.latency;
	}
	else
	{
		/// TODO: Store latency in config file
		audioLatency = 32;
	}
	Logf("Detected audio device latency: %d", Logger::Normal, audioLatency);

	return m_initialized = true;
}

void Audio::SetGlobalVolume(float vol)
{
	assert(vol >= 0.0f && vol <= 1.0f);
	DWORD dwVol = (DWORD)(vol * 10000);
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, dwVol);
}

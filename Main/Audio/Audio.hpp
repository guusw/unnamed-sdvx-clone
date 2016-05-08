#pragma once
#include "AudioStream.hpp"

extern class Audio* g_audio;

class Audio : Unique
{ 
public:
	Audio();
	~Audio();
	bool Init(class Window& window);
	void SetGlobalVolume(float vol);

	// Opens a stream at location
	AudioStream CreateStream(const String& path);

	// Private
	class Audio_Impl* GetImpl();

	int64 audioLatency;

private:
	class Window* m_window;
	bool m_initialized = false;
};
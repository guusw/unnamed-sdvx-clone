#pragma once
#include "AudioStream.hpp"
#include "Sample.hpp"

extern class Audio* g_audio;

class Audio : Unique
{ 
public:
	Audio();
	~Audio();
	bool Init(class Window& window);
	void SetGlobalVolume(float vol);

	// Opens a stream at path
	AudioStream CreateStream(const String& path);
	// Open a wav file at path
	Sample CreateSample(const String& path);

	uint32 GetSampleRate() const;

	// Private
	class Audio_Impl* GetImpl();

	int64 audioLatency;

private:
	class Window* m_window;
	bool m_initialized = false;
};
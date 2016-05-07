#pragma once

extern class Audio* g_audio;

class Audio : Unique
{ 
public:
	Audio();
	~Audio();
	bool Init(class Window& window);
	void SetGlobalVolume(float vol);

	int64 audioLatency;

private:
	class Window* m_window;
	bool m_initialized = false;
};
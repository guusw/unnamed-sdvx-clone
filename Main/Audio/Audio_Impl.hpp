#pragma once
#include "AudioOutput.hpp"
#include "AudioBase.hpp"

// Threading
#include <thread>
#include <mutex>
using std::thread;
using std::mutex;

class Audio_Impl
{
public:
	void Start();
	void Stop();
	void AudioThread();
	// Registers an AudioBase to be rendered
	void Register(AudioBase* audio);
	// Removes an AudioBase so it is no longer rendered
	void Deregister(AudioBase* audio);

	mutex lock;
	Vector<AudioBase*> itemsToRender;
	Vector<DSP*> globalDSPs;
	thread audioThread;
	bool runAudioThread = false;
	AudioOutput* output = nullptr;
};
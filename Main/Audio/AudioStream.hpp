#pragma once
#include "AudioBase.hpp"

class AudioStreamRes : public AudioBase
{
public:
	static Ref<AudioStreamRes> Create(class Audio* audio, const String& path);
	virtual ~AudioStreamRes() = default;
public:
	// Starts playback of the stream or continues a paused stream
	virtual void Play() = 0;
	virtual void Pause() = 0;

	// Gets the playback position in millisecond
	virtual int32 GetPosition() const = 0;
	// Sets the playback position in milliseconds
	virtual void SetPosition(int32 pos) = 0;

	// Stream volume from 0-1
	virtual void SetVolume(float vol) = 0;
};

typedef Ref<AudioStreamRes> AudioStream;
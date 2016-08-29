#pragma once
#include "AudioBase.hpp"

/*
	Audio stream object, currently only supports .ogg format
	The data is pre-loaded into memory and streamed from there
*/
class AudioStreamRes : public AudioBase
{
public:
	static Ref<AudioStreamRes> Create(class Audio* audio, const String& path, bool preload);
	virtual ~AudioStreamRes() = default;
public:
	// Starts playback of the stream or continues a paused stream
	virtual void Play() = 0;
	virtual void Pause() = 0;

	virtual bool HasEnded() const = 0;

	// Gets the playback position in millisecond
	virtual int32 GetPosition() const = 0;
	// Sets the playback position in milliseconds
	// negative time alowed, which will produce no audio for a certain amount of time
	virtual void SetPosition(int32 pos) = 0;
};

typedef Ref<AudioStreamRes> AudioStream;
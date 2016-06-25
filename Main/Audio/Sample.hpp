#pragma once
#include "AudioBase.hpp"

/*
	Audio sample, only supports wav files in signed 16 bit stereo or mono
*/
class SampleRes : public AudioBase
{
public:
	static Ref<SampleRes> Create(class Audio* audio, const String& path);
	virtual ~SampleRes() = default;

public:
	// Plays this sample from the start
	virtual void Play() = 0;
};

typedef Ref<SampleRes> Sample;
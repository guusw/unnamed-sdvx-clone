#pragma once

struct tWAVEFORMATEX;

/*
	Low level audio output
*/
class AudioOutput : public Unique
{
public:
	AudioOutput();
	~AudioOutput();

	bool Init();

	// Starts filling of the output buffer
	// Always call End after calling begin
	// The number of available samples is returned in 'numSamples'
	bool Begin(float*& buffer, uint32_t& numSamples);
	// Call this after you're done filling the output buffer
	void End(uint32_t numSamples);

	uint32_t GetNumChannels() const;
	uint32_t GetSampleRate() const;

	// The actual length of the buffer in seconds
	double GetBufferLength() const;

private:
	class AudioOutput_Impl* m_impl;
};
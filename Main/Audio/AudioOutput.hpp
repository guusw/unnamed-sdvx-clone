#pragma once

struct tWAVEFORMATEX;
class AudioOutput
{
public:
	AudioOutput();
	~AudioOutput();

	bool Init();

	// Starts filling of the output buffer
	// Always call End after calling Being
	bool Begin(float*& buffer, uint32_t& numSamples);
	// Call this after you're done filling the output buffer
	void End(uint32_t numSamples);

	uint32_t GetNumChannels() const;
	uint32_t GetSampleRate() const;

	// The actual length of the buffer in seconds
	double GetBufferLength() const;

private:
	AudioOutput& operator=(const AudioOutput&) = delete;
	AudioOutput(const AudioOutput&) = delete;

	//HWAVEOUT m_handle = nullptr;
	struct IAudioClient* m_audioClient = nullptr;
	struct IAudioRenderClient* m_audioRenderClient = nullptr;
	struct IMMDevice* m_device = nullptr;
	tWAVEFORMATEX* m_format;

	// The output wave buffer
	uint32_t m_numBufferFrames;

	double m_bufferLength;
};
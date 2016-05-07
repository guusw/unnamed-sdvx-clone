#pragma once

class Output
{
public:
	Output();
	~Output();

	bool Init();

	// Starts filling of the output buffer
	// Always call End after calling Being
	bool Begin(float*& buffer, uint32_t& numSamples);
	// Call this after you're done filling the output buffer
	void End(uint32_t numSamples);

	uint32_t GetNumChannels() const 
	{
		return m_format->nChannels;
	}
	uint32_t GetSampleRate() const
	{
		return m_format->nSamplesPerSec;
	}


private:
	Output& operator=(const Output&) = delete;
	Output(const Output&) = delete;

	//HWAVEOUT m_handle = nullptr;
	struct IAudioClient* m_audioClient = nullptr;
	struct IAudioRenderClient* m_audioRenderClient = nullptr;
	struct IMMDevice* m_device = nullptr;
	WAVEFORMATEX* m_format;

	// The output wave buffer
	uint32_t m_numBufferFrames;
};
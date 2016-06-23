#include "stdafx.h"
#include "Sample.hpp"
#include "Audio_Impl.hpp"
#include "Audio.hpp"

// Fixed point format for resampling
static uint64 fp_sampleStep = 1ull << 48;

struct WavHeader
{
	char id[4];
	uint32 nLength;

	bool operator ==(const char* rhs) const
	{
		return strncmp(id, rhs, 4) == 0;
	}
	bool operator !=(const char* rhs) const
	{
		return !(*this == rhs);
	}
};

struct WavFormat
{
	uint16 nFormat;
	uint16 nChannels;
	uint32 nSampleRate;
	uint32 nByteRate;
	uint16 nBlockAlign;
	uint16 nBitsPerSample;
};

BinaryStream& operator <<(BinaryStream& stream, WavHeader& hdr)
{
	stream.SerializeObject(hdr.id);
	stream << hdr.nLength;
	return stream;
}
BinaryStream& operator <<(BinaryStream& stream, WavFormat& fmt)
{
	stream.SerializeObject(fmt);
	return stream;
}

class Sample_Impl : public SampleRes
{
public:
	Audio* m_audio;
	Buffer m_pcm;
	WavFormat m_format = { 0 };

	mutex m_lock;

	// Resampling values
	uint64 m_sampleStep = 0;
	uint64 m_sampleStepIncrement = 0;

	uint64 m_playbackPointer = 0;
	uint64 m_length = 0;
	bool m_playing = false;
	float m_volume = 1.0f;

public:
	~Sample_Impl()
	{
		Deregister();
	}
	virtual void Play() override
	{
		m_lock.lock();
		m_playing = true;
		m_playbackPointer = 0;
		m_lock.unlock();
	}
	virtual void SetVolume(float volume) override
	{
		m_volume = volume;
	}
	bool Init(const String& path)
	{
		File file;
		if(!file.OpenRead(path))
			return false;
		FileReader stream = FileReader(file);

		WavHeader riff;
		stream << riff;
		if(riff != "RIFF")
			return false;

		char riffType[4];
		stream.SerializeObject(riffType);
		if(strncmp(riffType, "WAVE", 4) != 0)
			return false;

		while(stream.Tell() < stream.GetSize())
		{
			WavHeader chunkHdr;
			stream << chunkHdr;
			if(chunkHdr == "fmt ")
			{
				stream << m_format;
				//Logf("Sample format: %s", Logger::Info, (m_format.nFormat == 1) ? "PCM" : "Unknown");
				//Logf("Channels: %d", Logger::Info, m_format.nChannels);
				//Logf("Sample rate: %d", Logger::Info, m_format.nSampleRate);
				//Logf("Bps: %d", Logger::Info, m_format.nBitsPerSample);
			}
			else if(chunkHdr == "data") // data Chunk
			{
				// validate header
				if(m_format.nFormat != 1)
					return false;
				if(m_format.nChannels > 2 || m_format.nChannels == 0)
					return false;
				if(m_format.nBitsPerSample != 16)
					return false;

				// Read data
				m_length = chunkHdr.nLength / sizeof(short);
				m_pcm.resize(chunkHdr.nLength);
				stream.Serialize(m_pcm.data(), chunkHdr.nLength);
			}
			else
			{
				stream.Skip(chunkHdr.nLength);
			}
		}

		// Calculate the sample step if the rate is not the same as the output rate
		double sampleStep = (double)m_format.nSampleRate / (double)m_audio->GetSampleRate();
		m_sampleStepIncrement = (uint64)(sampleStep * (double)fp_sampleStep);

		return true;
	}
	virtual void Process(float* out, uint32 numSamples) override
	{
		if(!m_playing)
			return;

		m_lock.lock();
		if(m_format.nChannels == 2)
		{
			// Mix stereo sample
			for(uint32 i = 0; i < numSamples; i++)
			{
				if(m_playbackPointer >= m_length)
				{
					// Playback ended
					m_playing = false;
					break;
				}

				int16* src = ((int16*)m_pcm.data()) + m_playbackPointer;
				out[i * 2] = (float)src[0] / (float)0x7FFF * m_volume;
				out[i * 2 + 1] = (float)src[1] / (float)0x7FFF * m_volume;

				// Increment source sample with resampling
				m_sampleStep += m_sampleStepIncrement;
				while(m_sampleStep >= fp_sampleStep)
				{
					m_playbackPointer += 2;
					m_sampleStep -= fp_sampleStep;
				}
			}
		}
		else 
		{
			// Mix mono sample
			for(uint32 i = 0; i < numSamples; i++)
			{
				if(m_playbackPointer >= m_length)
				{
					// Playback ended
					m_playing = false;
					break;
				}

				int16* src = ((int16*)m_pcm.data()) + m_playbackPointer;
				out[i * 2] = (float)src[0] / (float)0x7FFF * m_volume;
				out[i * 2 + 1] = (float)src[0] / (float)0x7FFF * m_volume;

				// Increment source sample with resampling
				m_sampleStep += m_sampleStepIncrement;
				while(m_sampleStep >= fp_sampleStep)
				{
					m_playbackPointer += 1;
					m_sampleStep -= fp_sampleStep;
				}
			}
		}
		m_lock.unlock();
	}
};

Sample SampleRes::Create(Audio* audio, const String& path)
{
	Sample_Impl* res = new Sample_Impl();
	res->m_audio = audio;

	if(!res->Init(path))
	{
		delete res;
		return Sample();
	}

	audio->GetImpl()->Register(res);

	return Sample(res);
}

#include "stdafx.h"
#include "AudioStreamBase.hpp"
extern "C"
{
	#include "minimp3.h"
}

class AudioStreamMP3_Impl : public AudioStreamBase
{
	mp3_decoder_t* m_decoder;
	size_t m_mp3dataOffset = 0;
	size_t m_mp3dataLength = 0;
	int32 m_mp3samplePosition = 0;
	int32 m_samplingRate = 0;
	uint8* m_dataSource = 0;

	Map<int32, size_t> m_frameIndices;
	uint32 m_largetsFrameIndex;

	bool m_firstFrame = true;

public:
	~AudioStreamMP3_Impl()
	{
		Deregister();
		mp3_done(m_decoder);
	}
	bool Init(Audio* audio, const String& path, bool preload)
	{
		if(!AudioStreamBase::Init(audio, path, true)) // Always preload for now
			return false;

		// Always use preloaded data
		m_mp3dataLength = Reader().GetSize();
		m_dataSource = m_data.data();

		// Scan MP3 frame offsets
		uint32 sampleOffset = 0;
		for(size_t i = 0; i < m_mp3dataLength;)
		{
			if(m_dataSource[i] == 0xFF)
			{
				if(i + 1 > m_mp3dataLength)
					continue;
				if((m_dataSource[i + 1] & 0xE0) == 0xE0) // Frame Sync
				{
					uint8 version = (m_dataSource[i+1] & 0x18) >> 3;
					uint8 layer = (m_dataSource[i + 1] & 0x06) >> 1;
					bool crc = (m_dataSource[i + 1] & 0x01) != 0;
					uint8 bitrateIndex = (m_dataSource[i + 2] & 0xF0) >> 4;
					uint8 rateIndex = (m_dataSource[i + 2] & 0x0C) >> 2;
					bool paddingEnabled = ((m_dataSource[i + 2] & 0x02) >> 1) != 0;
					uint8 channelFlags = ((m_dataSource[i + 3] & 0xC0) >> 6);
					if(bitrateIndex == 0xF || rateIndex > 2) // bad
					{
						i++;
						continue;
					}

					uint8 channels = ((channelFlags & 0x3) == 0x3) ? 1 : 2;

					uint32 linearVersion = version == 0x03 ? 0 : 1; // Version 1/2
					uint32 bitrate = mp3_bitrate_tab[linearVersion][bitrateIndex] * 1000;
					uint32 sampleRate = mp3_freq_tab[rateIndex];
					uint32 padding = paddingEnabled ? 1 : 0;

					uint32 frameLength = 144 * bitrate / sampleRate + padding;
					if(frameLength == 0)
					{
						i++;
						continue;
					}
					
					i += frameLength;
					uint32 frameSamples = (linearVersion == 0) ? 1152 : 576;
					m_frameIndices.Add((int32)sampleOffset, i);
					sampleOffset += frameSamples;
					continue; // Skip header
				}
			}
			i++;
		}

		// No mp3 frames found
		if(m_frameIndices.empty())
		{
			Logf("No valid mp3 frames found in file \"%s\"", Logger::Warning, path);
			return false;
		}

		SetPosition_Internal(0);

		// Total sample
		m_samplesTotal = sampleOffset;

		m_decoder = (mp3_decoder_t*)mp3_create();
		int32 r = DecodeData_Internal();
		if(r <= 0)
			return false;

		return true;
	}
	virtual void SetPosition_Internal(int32 pos)
	{
		auto it = m_frameIndices.lower_bound(pos);
		if(it == m_frameIndices.end())
		{
			--it; // Take the last frame
		}
		if(it != m_frameIndices.end())
		{
			m_mp3samplePosition = it->first;
			m_mp3dataOffset = it->second;
		}
	}
	virtual int32 GetStreamPosition_Internal()
	{
		return m_mp3samplePosition;
	}
	virtual int32 GetStreamRate_Internal()
	{
		return (int32)m_samplingRate;
	}
	virtual int32 DecodeData_Internal()
	{
		int16 buffer[MP3_MAX_SAMPLES_PER_FRAME];
		mp3_info_t info;
		int32 readData = 0;
		while(true)
		{
			readData = mp3_decode(m_decoder, (uint8*)m_dataSource + m_mp3dataOffset, (int)(m_mp3dataLength - m_mp3dataOffset), buffer, &info);
			m_mp3dataOffset += readData;
			if(m_mp3dataOffset >= m_mp3dataLength) // EOF
				return -1;
			if(readData <= 0)
				return -1;
			if(info.audio_bytes >= 0)
				break;
		}

		int32 samplesGotten = info.audio_bytes / (info.channels * sizeof(short));
		m_mp3samplePosition += samplesGotten;

		if(m_firstFrame)
		{
			m_bufferSize = MP3_MAX_SAMPLES_PER_FRAME / 2;
			InitSampling(m_samplingRate = info.sample_rate);
			m_firstFrame = false;
		}

		// Copy data to read buffer
		for(int32 i = 0; i < samplesGotten; i++)
		{
			if(info.channels == 1)
			{
				m_readBuffer[0][i] = (float)buffer[i] / (float)0x7FFF;
				m_readBuffer[1][i] = m_readBuffer[0][i];
			}
			else if(info.channels == 2)
			{
				m_readBuffer[0][i] = (float)buffer[i * 2 + 0] / (float)0x7FFF;
				m_readBuffer[1][i] = (float)buffer[i * 2 + 1] / (float)0x7FFF;
			}
		}
		m_currentBufferSize = samplesGotten;
		m_remainingBufferData = samplesGotten;
		return samplesGotten;
	}
};

class AudioStreamRes* CreateAudioStream_mp3(class Audio* audio, const String& path, bool preload)
{
	AudioStreamMP3_Impl* impl = new AudioStreamMP3_Impl();
	if(!impl->Init(audio, path, preload))
	{
		delete impl;
		impl = nullptr;
	}
	return impl;
}

#include "stdafx.h"
#include "Audio.hpp"
#include "AudioStream.hpp"
#include "Audio_Impl.hpp"
#include <vorbis/vorbisfile.h>

// Fixed point format for resampling
static uint64 fp_sampleStep = 1ull << 48;

class AudioStreamOGG_Impl : public AudioStreamRes
{
	Audio* m_audio;
	File m_file;
	Buffer m_data;
	MemoryReader m_memoryReader;
	FileReader m_fileReader;
	bool m_preloaded = false;
	BinaryStream& Reader()
	{
		return m_preloaded ? (BinaryStream&)m_memoryReader : (BinaryStream&)m_fileReader;
	}

	mutex m_lock;
	OggVorbis_File m_ovf = { 0 };
	ov_callbacks callbacks =
	{
		(decltype(ov_callbacks::read_func))&AudioStreamOGG_Impl::m_Read,
		(decltype(ov_callbacks::seek_func))&AudioStreamOGG_Impl::m_Seek,
		nullptr, // Close
		(decltype(ov_callbacks::tell_func))&AudioStreamOGG_Impl::m_Tell,
	};

	vorbis_info* m_info;
	float** m_readBuffer = nullptr;
	const uint32 m_bufferSize = 4096;
	uint32 m_numChannels = 0;
	uint32 m_currentBufferSize = 0;
	uint32 m_remainingBufferData = 0;

	int64 m_samplePos = 0;

	// Resampling values
	uint64 m_sampleStep = 0;
	uint64 m_sampleStepIncrement = 0;

	Timer m_deltaTimer;
	Timer m_streamTimer;
	double m_streamTimeOffset = 0.0f;
	double m_offsetCorrection = 0.0f;
	bool m_paused = false;
	bool m_playing = false;

	float m_volume = 0.8f;
public:
	~AudioStreamOGG_Impl()
	{
		Deregister();
	}
	bool Init(Audio* audio, const String& path, bool preload)
	{
		if(!m_file.OpenRead(path))
			return false;

		if(preload)
		{
			m_data.resize(m_file.GetSize());
			m_file.Read(m_data.data(), m_data.size());
			m_memoryReader = MemoryReader(m_data);
			m_preloaded = preload;
		}
		else
		{
			m_fileReader = FileReader(m_file);
			m_preloaded = false;
		}

		int32 r = ov_open_callbacks(this, &m_ovf, 0, 0, callbacks);
		if(r != 0)
		{
			Logf("ov_open_callbacks failed with code %d", Logger::Error, r);
			return false;
		}

		vorbis_comment* comments = ov_comment(&m_ovf, 0);
		m_info = ov_info(&m_ovf, 0);
		if(!m_info)
			return false;

		// Calculate the sample step if the rate is not the same as the output rate
		double sampleStep = (double)m_info->rate / (double)audio->GetSampleRate();
		m_sampleStepIncrement = (uint64)(sampleStep * (double)fp_sampleStep);
		double stepCheck = (double)m_sampleStepIncrement / (double)fp_sampleStep;

		long numStreams = ov_streams(&m_ovf);

		m_numChannels = m_info->channels;
		m_readBuffer = new float*[m_numChannels];
		for(uint32 c = 0; c < m_numChannels; c++)
		{
			m_readBuffer[c] = new float[m_bufferSize];
		}

		return true;
	}
	virtual void Play() override
	{
		if(!m_playing)
		{
			m_playing = true;
		}
		if(m_paused)
		{
			m_paused = false;
			RestartTiming();
		}
	}
	virtual void Pause() override
	{
		if(!m_paused)
		{
			// Store time the stream was paused
			m_streamTimeOffset = m_streamTimer.SecondsAsDouble();
			m_paused = true;
		}
		else
		{
			m_paused = false;
			RestartTiming();
		}
	}

	uint64 SecondsToSamples(double s) const
	{
		return (uint64)(s * (double)m_info->rate);
	}
	double SamplesToSeconds(int64 s) const
	{
		return (double)s / (double)m_info->rate;
	}
	double GetPositionSeconds() const
	{
		double samplePosTime = SamplesToSeconds(m_samplePos);
		if(m_paused || m_samplePos < 0)
			return samplePosTime;
		else
		{
			double ret = m_streamTimeOffset + m_streamTimer.SecondsAsDouble() - m_offsetCorrection;
			if((ret - samplePosTime) > 0.2f) // Prevent time from running of when the application freezes
				return samplePosTime;
			return ret;
		}
	}
	virtual int32 GetPosition() const override
	{
		return (int32)(GetPositionSeconds() * 1000.0);
	}

	virtual void SetPosition(int32 pos) override
	{
		m_lock.lock();
		m_remainingBufferData = 0;
		m_samplePos = SecondsToSamples((double)pos / 1000.0);
		ov_pcm_seek(&m_ovf, m_samplePos);
		m_lock.unlock();
	}

	virtual void Process(float* out, uint32 numSamples) override
	{
		if(!m_playing || m_paused)
			return;

		m_lock.lock();

		uint32 outCount = 0;
		while(outCount < numSamples)
		{
			if(m_remainingBufferData > 0)
			{
				uint32 idxStart = (m_currentBufferSize - m_remainingBufferData);
				uint32 readOffset = 0; // Offset from the start to read from
				for(uint32 i = 0; outCount < numSamples && readOffset < m_remainingBufferData; i++)
				{
					if(m_samplePos < 0)
					{
						out[outCount * 2] = 0.0f;
						out[outCount * 2 + 1] = 0.0f;
					}
					else
					{
						out[outCount * 2] = m_readBuffer[0][idxStart + readOffset];
						out[outCount * 2 + 1] = m_readBuffer[1][idxStart + readOffset];
					}
					outCount++;

					// Increment source sample with resampling
					m_sampleStep += m_sampleStepIncrement;
					while(m_sampleStep >= fp_sampleStep)
					{
						m_sampleStep -= fp_sampleStep;
						if(m_samplePos >= 0)
							readOffset++;
						m_samplePos++;
					}
				}
				m_remainingBufferData -= readOffset;
			}

			if(outCount >= numSamples)
				break;

			// Read more data
			float** readBuffer;
			int32 r = ov_read_float(&m_ovf, &readBuffer, m_bufferSize, 0);
			if(r > 0)
			{
				// Copy data to read buffer
				for(int32 i = 0; i < r; i++)
				{
					m_readBuffer[0][i] = readBuffer[0][i];
					m_readBuffer[1][i] = readBuffer[1][i];
				}
				m_currentBufferSize = r;
				m_remainingBufferData = r;
			}
			else if(r == 0)
			{
				// EOF
				m_playing = false;
				break;
			}
			else
			{
				// Error
				m_playing = false;
				Logf("Ogg Stream error %d", Logger::Warning, r);
				break;
			}
		}

		// Store timing info
		if(m_samplePos > 0)
		{
			m_samplePos = (uint32)ov_pcm_tell(&m_ovf) - m_remainingBufferData;
			double timingDelta = GetPositionSeconds() - SamplesToSeconds(m_samplePos);

			// This is to stabilize the running timer with the actual audio stream, the delta is added for 50% as an offset to this timer
			if(abs(timingDelta) > 0.002)
			{
				ResyncTiming(timingDelta);
			}
		}

		m_lock.unlock();
	}

	void ResyncTiming(double delta)
	{
		if(abs(delta) > 1.5)
			RestartTiming();
		else
		{
			double syncAmount = delta * 0.3;
			Logf("Resyncing timing, %f", Logger::Warning, syncAmount);
			m_offsetCorrection += syncAmount;
		}
	}
	void RestartTiming()
	{
		m_streamTimeOffset = SamplesToSeconds(m_samplePos); // Add audio latency to this offset
		m_samplePos = 0;
		m_streamTimer.Restart();
		m_offsetCorrection = 0.0f;
	}

private:
	static size_t m_Read(void* ptr, size_t size, size_t nmemb, AudioStreamOGG_Impl* self)
	{
		return self->Reader().Serialize(ptr, nmemb*size);
	}
	static int m_Seek(AudioStreamOGG_Impl* self, int64 offset, int whence)
	{
		if(whence == SEEK_SET)
			self->Reader().Seek((size_t)offset);
		else if(whence == SEEK_CUR)
			self->Reader().Skip((size_t)offset);
		else if(whence == SEEK_END)
			self->Reader().SeekReverse((size_t)offset);
		else
			assert(false);
		return 0;
	}
	static long m_Tell(AudioStreamOGG_Impl* self)
	{
		return (long)self->Reader().Tell();
	}
};

Ref<AudioStreamRes> AudioStreamRes::Create(class Audio* audio, const String& path, bool preload)
{
	AudioStreamOGG_Impl* impl = new AudioStreamOGG_Impl();
	if(!impl->Init(audio, path, preload))
	{
		delete impl;
		return AudioStream();
	}

	audio->GetImpl()->Register(impl);

	return AudioStream(impl);
}

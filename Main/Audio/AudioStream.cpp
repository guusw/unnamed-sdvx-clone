#include "stdafx.h"
#include "Audio.hpp"
#include "AudioStream.hpp"
#include "Audio_Impl.hpp"
#include <vorbis/vorbisfile.h>


class AudioStreamOGG_Impl : public AudioStreamRes
{
	Audio* m_audio;
	File m_file;
	Buffer m_data;
	MemoryReader m_reader;

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

	uint64 m_samplePos = 0;
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
	bool Init(Audio* audio, const String& path)
	{
		if(!m_file.OpenRead(path))
			return false;
		m_data.resize(m_file.GetSize());
		m_file.Read(m_data.data(), m_data.size());
		m_reader = MemoryReader(m_data);
		
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
	double SamplesToSeconds(uint64 s) const
	{
		return (double)s / (double)m_info->rate;
	}
	double GetPositionSeconds() const
	{
		double samplePosTime = SamplesToSeconds(m_samplePos);;
		if(m_paused)
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
		ov_pcm_seek(&m_ovf, SecondsToSamples((double)pos / 1000.0));
		m_lock.unlock();
	}
	virtual void SetVolume(float vol) override
	{
		m_volume = vol;
	}
	virtual void Process(float*& out, uint32 numSamples) override
	{
		if(!m_playing || m_paused)
			return;

		m_lock.lock();

		uint32 outCount = 0;
		while(outCount < numSamples)
		{
			if(m_remainingBufferData > 0)
			{
				uint32 readBufferData = Math::Min(m_remainingBufferData, numSamples-outCount);
				uint32 idxStart = (m_currentBufferSize - m_remainingBufferData);
				for(uint32 i = 0; i < readBufferData; i++)
				{
					out[outCount * 2] = m_readBuffer[0][idxStart + i] * m_volume;
					out[outCount * 2 + 1] = m_readBuffer[1][idxStart + i] * m_volume;
					outCount++;
				}
				m_remainingBufferData -= readBufferData;
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
		m_samplePos = (uint32)ov_pcm_tell(&m_ovf) - m_remainingBufferData;
		double timingDelta = GetPositionSeconds() - SamplesToSeconds(m_samplePos);

		// This is to stabilize the running timer with the actual audio stream, the delta is added for 50% as an offset to this timer 
		if(abs(timingDelta) > 0.001)
		{
			ResyncTiming(timingDelta);
		}

		m_lock.unlock();
	}
	void ResyncTiming(double delta)
	{
		if(abs(delta) > 1.5)
			RestartTiming();
		else
			m_offsetCorrection += delta * 0.5;
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
		return self->m_reader.Serialize(ptr, nmemb*size);
	}
	static int m_Seek(AudioStreamOGG_Impl* self, int64 offset, int whence)
	{
		if(whence == SEEK_SET)
			self->m_reader.Seek((size_t)offset);
		else if(whence == SEEK_CUR)
			self->m_reader.Skip((size_t)offset);
		else if(whence == SEEK_END)
			self->m_reader.SeekReverse((size_t)offset);
		else
			assert(false);
		return 0;
	}
	static long m_Tell(AudioStreamOGG_Impl* self)
	{
		return (long)self->m_reader.Tell();
	}
};

AudioStream AudioStreamRes::Create(Audio* audio, const String& path)
{
	AudioStreamOGG_Impl* impl = new AudioStreamOGG_Impl();
	if(!impl->Init(audio, path))
	{
		delete impl;
		return AudioStream();
	}

	audio->GetImpl()->Register(impl);

	return AudioStream(impl);
}
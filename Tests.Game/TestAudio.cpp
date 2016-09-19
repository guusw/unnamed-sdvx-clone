#include <Shared/Shared.hpp>
#include <Tests/Tests.hpp>
#include <Audio/Audio.hpp>
#include <Audio/DSP.hpp>
#include <float.h>
#include "TestMusicPlayer.hpp"

#include <thread>
using namespace std;

static String testSamplePath = Path::Normalize("audio/laser_slam1.wav");

// Test for music player
static String testSongPath = Path::Normalize("songs/CHNLDiVR/mix1.ogg");
static uint32 testSongOffset = 180000;

Test("Audio.Playback")
{
	Audio* audio = new Audio();
	TestEnsure(audio->Init());

	Sample testSample = audio->CreateSample(testSamplePath);
	TestEnsure(testSample.IsValid());

	testSample->SetVolume(1.0f);

	Timer t;
	float time = FLT_MAX;
	while(true)
	{
		float deltaTime = t.SecondsAsFloat();
		t.Restart();
		time += deltaTime;
		t.Restart();
		if(time > 3.0f)
		{
			Logf("Playing sample", Logger::Info);
			testSample->Play();
			time = 0.0;
		}
		this_thread::sleep_for(chrono::milliseconds(5));
	}

	delete audio;
}

Test("Audio.Music.LPF")
{
	printf("\n");

	class MusicPlayer : public TestMusicPlayer
	{
		BQFDSP* filter;
		float filterSetting = 0.0f;
		float sweepDir = 1.0f;
	public:
		virtual void Init(const String& songPath, uint32 startOffset) override
		{
			TestMusicPlayer::Init(songPath, startOffset);

			filter = new BQFDSP();
			song->AddDSP(filter);
			filter->SetLowPass(1.0f, 500.0f);
		}
		virtual void Update(float dt) override
		{
			// Sweeping filter settings
			const float filterSweepStart = 200.0f;
			const float filterSweepEnd = 10000.0f;
			const float filterSweepQStart = 1.0f;
			const float filterSweepQEnd = 1.0f;

			for(int32 i = 0; i < 64; i++)
				printf("\b\0\b");

			// Increment filter
			filterSetting += dt * 0.25f * sweepDir;
			filterSetting = Math::Clamp(filterSetting, 0.0f, 1.0f);
			if(filterSetting >= 1.0f)
				sweepDir = -1.0f;
			else if(filterSetting <= 0.0f)
				sweepDir = 1.0f;

			float sweepInput = pow(filterSetting, 2.0f);
			float sweepFreq = filterSweepStart + (filterSweepEnd - filterSweepStart) * sweepInput;
			float sweepQ = filterSweepQStart + (filterSweepQEnd - filterSweepQStart) * sweepInput;

			int32 playbackTime = song->GetPosition();
			printf("%08d > f:%f q:%f", playbackTime, sweepFreq, sweepQ);

			filter->SetLowPass(sweepQ, sweepFreq);
		}
	};

	MusicPlayer mp;
	mp.Init(testSongPath, testSongOffset);
	mp.Run();
}

Test("Audio.Music.Peaking")
{
	printf("\n");

	class MusicPlayer : public TestMusicPlayer
	{
		BQFDSP* filter;
		float filterSetting = 0.0f;
		float sweepDir = 1.0f;
	public:
		virtual void Init(const String& songPath, uint32 startOffset) override
		{
			TestMusicPlayer::Init(songPath, startOffset);

			filter = new BQFDSP();
			song->AddDSP(filter);
			filter->SetLowPass(1.0f, 500.0f);
		}
		virtual void Update(float dt) override
		{
			// Sweeping filter settings
			const float filterSweepStart = 80.0f;
			const float filterSweepEnd = 5000.0f;
			const float filterSweepBwStart = 1.5f;
			const float filterSweepBwEnd = 3.0f;
			const float filterSweepAmpStart = 30.0f;
			const float filterSweepAmpEnd = 20.0f;

			for(int32 i = 0; i < 64; i++)
				printf("\b\0\b");

			// Increment filter
			filterSetting += dt * 0.5f * sweepDir;
			filterSetting = Math::Clamp(filterSetting, 0.0f, 1.0f);
			if(filterSetting >= 1.0f)
				sweepDir = -1.0f;
			else if(filterSetting <= 0.0f)
				sweepDir = 1.0f;

			float sweepInput = pow(filterSetting, 2.0f);
			float sweepFreq = filterSweepStart + (filterSweepEnd - filterSweepStart) * sweepInput;
			float sweepBw = filterSweepBwStart + (filterSweepBwEnd - filterSweepBwStart) * sweepInput;
			float sweepAmp = filterSweepAmpStart + (filterSweepAmpEnd - filterSweepAmpStart) * sweepInput;

			int32 playbackTime = song->GetPosition();
			printf("%08d > f:%f bw:%f amp:%f", playbackTime, sweepFreq, sweepBw, sweepAmp);

			filter->SetPeaking(sweepBw, sweepFreq, sweepAmp);
		}
	};

	MusicPlayer mp;
	mp.Init(testSongPath, testSongOffset);
	mp.Run();
}

Test("Audio.Music.Echo")
{
	class MusicPlayer : public TestMusicPlayer
	{
		BQFDSP* filter;
		float filterSetting = 0.0f;
	public:
		virtual void Init(const String& songPath, uint32 startOffset) override
		{
			TestMusicPlayer::Init(songPath, startOffset);

			Echo* echo = new Echo();
			song->AddDSP(echo);
			echo->SetLength(3000);
			echo->feedback = 0.4f;
		}
		virtual void Update(float dt) override
		{
		}
	};

	MusicPlayer mp;
	mp.Init(testSongPath, testSongOffset);
	mp.Run();
}

Test("Audio.Music.Flanger")
{
	class MusicPlayer : public TestMusicPlayer
	{
		BQFDSP* filter;
		float filterSetting = 0.0f;
	public:
		virtual void Init(const String& songPath, uint32 startOffset) override
		{
			TestMusicPlayer::Init(songPath, startOffset);

			FlangerDSP* fl = new FlangerDSP();
			song->AddDSP(fl);
			fl->SetDelayRange(10, 120);
			fl->delay = 24100;
			fl->mix = 1.0f;
		}
		virtual void Update(float dt) override
		{
		}
	};

	MusicPlayer mp;
	mp.Init(testSongPath, testSongOffset);
	mp.Run();
}

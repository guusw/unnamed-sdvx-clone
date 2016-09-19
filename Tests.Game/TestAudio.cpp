#include <Shared/Shared.hpp>
#include <Tests/Tests.hpp>
#include <Audio/Audio.hpp>
#include <Audio/DSP.hpp>
#include <thread>

using namespace std::this_thread;
using namespace std::chrono;

static String testSamplePath = Path::Normalize("audio/laser_slam1.wav");
static String testSongPath = Path::Normalize("songs/Yggdrasil/Yggdrasil.ogg");
static uint32 testSongOffset = 0;

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
		sleep_for(milliseconds(5));
	}

	delete audio;
}

Test("Audio.Music.LPF")
{
	Audio* audio = new Audio();
	TestEnsure(audio->Init());

	AudioStream testStream = audio->CreateStream(testSongPath);
	TestEnsure(testStream.IsValid());

	testStream->SetVolume(1.0f);
	testStream->Play();
	testStream->SetPosition(testSongOffset);

	BQFDSP* filter = new BQFDSP();
	testStream->AddDSP(filter);
	filter->SetLowPass(1.0f, 500.0f);

	float filterSetting = 0.0f;

	printf("\n");

	// Sweeping filter settings
	const float filterSweepStart = 200.0f;
	const float filterSweepEnd = 10000.0f;
	const float filterSweepQStart = 1.0f;
	const float filterSweepQEnd = 1.0f;
	float sweepDir = 1.0f;

	Timer t;
	while(!testStream->HasEnded())
	{
		float deltaTime = t.SecondsAsFloat();
		t.Restart();

		for(int32 i = 0; i < 64; i++)
			printf("\b\0\b");

		// Increment filter
		filterSetting += deltaTime * 0.25f * sweepDir;
		filterSetting = Math::Clamp(filterSetting, 0.0f, 1.0f);
		if(filterSetting >= 1.0f)
			sweepDir = -1.0f;
		else if(filterSetting <= 0.0f)
			sweepDir = 1.0f;

		float sweepInput = pow(filterSetting, 2.0f);
		float sweepFreq = filterSweepStart + (filterSweepEnd - filterSweepStart) * sweepInput;
		float sweepQ = filterSweepQStart + (filterSweepQEnd - filterSweepQStart) * sweepInput;

		int32 playbackTime = testStream->GetPosition();
		printf("%08d > f:%f q:%f", playbackTime, sweepFreq, sweepQ);

		filter->SetLowPass(sweepQ, sweepFreq);

		sleep_for(milliseconds(5));
	}
	delete audio;
}

Test("Audio.Music.Peaking")
{
	Audio* audio = new Audio();
	TestEnsure(audio->Init());

	AudioStream testStream = audio->CreateStream(testSongPath);
	TestEnsure(testStream.IsValid());

	testStream->SetVolume(1.0f);
	testStream->Play();
	testStream->SetPosition(testSongOffset);

	BQFDSP* pfilter = new BQFDSP();
	testStream->AddDSP(pfilter);

	float filterSetting = 0.0f;

	printf("\n");

	// Sweeping filter settings
	const float filterSweepStart = 80.0f;
	const float filterSweepEnd = 5000.0f;
	const float filterSweepBwStart = 1.5f;
	const float filterSweepBwEnd = 3.0f;
	const float filterSweepAmpStart = 30.0f;
	const float filterSweepAmpEnd = 20.0f;
	float sweepDir = 1.0f;

	Timer t;
	while(!testStream->HasEnded())
	{
		float deltaTime = t.SecondsAsFloat();
		t.Restart();

		for(int32 i = 0; i < 64; i++)
			printf("\b\0\b");

		// Increment filter
		filterSetting += deltaTime * 0.5f * sweepDir;
		filterSetting = Math::Clamp(filterSetting, 0.0f, 1.0f);
		if(filterSetting >= 1.0f)
			sweepDir = -1.0f;
		else if(filterSetting <= 0.0f)
			sweepDir = 1.0f;

		float sweepInput = pow(filterSetting, 2.0f);
		float sweepFreq = filterSweepStart + (filterSweepEnd - filterSweepStart) * sweepInput;
		float sweepBw = filterSweepBwStart + (filterSweepBwEnd - filterSweepBwStart) * sweepInput;
		float sweepAmp = filterSweepAmpStart + (filterSweepAmpEnd - filterSweepAmpStart) * sweepInput;

		int32 playbackTime = testStream->GetPosition();
		printf("%08d > f:%f bw:%f amp:%f", playbackTime, sweepFreq, sweepBw, sweepAmp);

		pfilter->SetPeaking(sweepBw, sweepFreq, sweepAmp);

		sleep_for(milliseconds(5));
	}
	delete audio;
}

Test("Audio.Music.Echo")
{
	Audio* audio = new Audio();
	TestEnsure(audio->Init());

	AudioStream testStream = audio->CreateStream(testSongPath);
	TestEnsure(testStream.IsValid());

	testStream->SetVolume(1.0f);
	testStream->Play();
	testStream->SetPosition(testSongOffset);

	Echo* echo = new Echo();
	testStream->AddDSP(echo);
	echo->SetLength(3000);
	echo->feedback = 0.4f;

	Timer t;
	while(!testStream->HasEnded())
	{
		float deltaTime = t.SecondsAsFloat();
		t.Restart();

		for(int32 i = 0; i < 64; i++)
			printf("\b\0\b");

		int32 playbackTime = testStream->GetPosition();
		printf("%08d >", playbackTime);

		sleep_for(milliseconds(5));
	}
	delete audio;
}

Test("Audio.Music.Flanger")
{
	Audio* audio = new Audio();
	TestEnsure(audio->Init());

	AudioStream testStream = audio->CreateStream(testSongPath);
	TestEnsure(testStream.IsValid());

	testStream->SetVolume(1.0f);
	testStream->Play();
	testStream->SetPosition(testSongOffset);

	FlangerDSP* fl = new FlangerDSP();
	testStream->AddDSP(fl);
	fl->SetDelayRange(10, 120);
	fl->delay = 24100;
	fl->mix = 1.0f;

	Timer t;
	while(!testStream->HasEnded())
	{
		float deltaTime = t.SecondsAsFloat();
		t.Restart();

		for(int32 i = 0; i < 64; i++)
			printf("\b\0\b");

		int32 playbackTime = testStream->GetPosition();
		printf("%08d >", playbackTime);

		sleep_for(milliseconds(5));
	}
	delete audio;
}
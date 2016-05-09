#include "stdafx.h"
#include "Audioclient.h"
#include "Mmdeviceapi.h"
#include "comdef.h"
#include "Functiondiscoverykeys_devpkey.h"
#include "AudioOutput.hpp"

#define REFTIME_NS (100)
#define REFTIMES_PER_MICROSEC (1000/REFTIME_NS)
#define REFTIMES_PER_MILLISEC (REFTIMES_PER_MICROSEC * 1000)
#define REFTIMES_PER_SEC  (REFTIMES_PER_MILLISEC * 1000)
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = nullptr; }

static uint32_t freq = 44100;
static uint32_t channels = 2;
static uint32_t numBuffers = 2;
static uint32_t bufferLength = 10;
static REFERENCE_TIME bufferDuration = (REFERENCE_TIME)(bufferLength * REFTIMES_PER_MILLISEC);

AudioOutput::AudioOutput()
{
}

bool AudioOutput::Init()
{
	// Initialize the WASAPI device enumerator
	HRESULT res;
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	IMMDeviceEnumerator* deviceEnumerator;
	CoInitialize(nullptr);
	res = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&deviceEnumerator);
	if(res != S_OK)
		throw _com_error(res);

	// Find an appropriate audio device
	IMMDeviceCollection* devices;
	res = deviceEnumerator->EnumAudioEndpoints(EDataFlow::eAll, DEVICE_STATE_ACTIVE, &devices);
	if(res != S_OK)
		throw _com_error(res);

	PROPVARIANT property;
	PropVariantInit(&property);
	char tmpStr[1024];
	uint32_t numDevices = 0;
	devices->GetCount(&numDevices);
	for(uint32_t i = 0; i < numDevices; i++)
	{
		SAFE_RELEASE(m_device);
		devices->Item(i, &m_device);

		// Get Device Properties(Name, etc.)
		IPropertyStore* props;
		m_device->OpenPropertyStore(STGM_READ, &props);
		props->GetValue(PKEY_Device_FriendlyName, &property);
		wcstombs(tmpStr, property.pwszVal, sizeof(tmpStr) - 1);
		props->Release();

		Logf("Audio Device found [%d]-> %s", Logger::Info, i, tmpStr);

		break;
	}
	PropVariantClear(&property);


	SAFE_RELEASE(deviceEnumerator);

	// Obtain audio client
	res = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
	if(res != S_OK)
		throw _com_error(res);

	// Aquire format and initialize device for shared mode
	res = m_audioClient->GetMixFormat(&m_format);
	if(m_format->nChannels != 2)
	{
		Log("Couldn't open stereo device", Logger::Error);
		return false;
	}

	// Init client
	res = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
		bufferDuration, 0, m_format, nullptr);
	if(res != S_OK)
		return false;

	// Get the audio render client
	res = m_audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_audioRenderClient);
	if(res != S_OK)
		return false;

	// Get the number of buffer frames
	m_audioClient->GetBufferSize(&m_numBufferFrames);

	m_bufferLength = (double)m_numBufferFrames / (double)m_format->nSamplesPerSec;

	res = m_audioClient->Start();
	return true;
}
AudioOutput::~AudioOutput()
{
	HRESULT res = m_audioClient->Stop();

	CoTaskMemFree(m_format);
	SAFE_RELEASE(m_device);
	SAFE_RELEASE(m_audioClient);
	SAFE_RELEASE(m_audioRenderClient);
}
bool AudioOutput::Begin(float*& buffer, uint32_t& numSamples)
{
	// See how much buffer space is available.
	uint32_t numFramesPadding;
	m_audioClient->GetCurrentPadding(&numFramesPadding);
	numSamples = m_numBufferFrames - numFramesPadding;

	if(numSamples > 0)
	{
		// Grab all the available space in the shared buffer.
		m_audioRenderClient->GetBuffer(numSamples, (BYTE**)&buffer);
		return true;
	}
	return false;
}
void AudioOutput::End(uint32_t numSamples)
{
	if(numSamples > 0)
	{
		m_audioRenderClient->ReleaseBuffer(numSamples, 0);
	}
}
uint32_t AudioOutput::GetNumChannels() const
{
	return m_format->nChannels;
}
uint32_t AudioOutput::GetSampleRate() const
{
	return m_format->nSamplesPerSec;
}
double AudioOutput::GetBufferLength() const
{
	return m_bufferLength;
}
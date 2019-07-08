#include "stdafx.h"
#include "audio.h"

CLiveAudio::CLiveAudio(CAudioDataProvider *pProvider) :
	CAudio(pProvider, TYPE_LIVE),
	m_OutputUnit(0),
	m_DeviceId(0)
{

}

CLiveAudio::~CLiveAudio()
{
	Terminate();
}

void CLiveAudio::Init(const AudioStreamBasicDescription &format, AudioDeviceID deviceId)
{
	Terminate();
	AudioUnit outputUnit = 0;
	UInt32 size;

	// Create Unit
	AudioComponentDescription outputDescription = {0};
	outputDescription.componentType = kAudioUnitType_Output;
	outputDescription.componentSubType = kAudioUnitSubType_HALOutput; // kAudioUnitSubType_DefaultOutput
	outputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;

	AudioComponent outputComponent = AudioComponentFindNext(NULL, &outputDescription);
	CheckResult(AudioComponentInstanceNew(outputComponent, &outputUnit));

	// Set format
	CheckResult(AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat,
									 kAudioUnitScope_Input, 0, &format, sizeof(format)));

	// Callback
	AURenderCallbackStruct callbackStruct;
	callbackStruct.inputProc = RenderCallback;
	callbackStruct.inputProcRefCon = this;
	CheckResult(AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback,
									 kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct)));

	// Find output device if not known
	if (deviceId == kAudioDeviceUnknown) {
		AudioObjectPropertyAddress propertyAddress;
		propertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
		propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
		propertyAddress.mElement = kAudioObjectPropertyElementMaster;

		size = sizeof(AudioDeviceID);
		CheckResult(AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress,
											   0, NULL, &size, &deviceId));
	}

	// Attach output to device
	CheckResult(AudioUnitSetProperty(outputUnit, kAudioOutputUnitProperty_CurrentDevice,
									 kAudioUnitScope_Global, 0, &deviceId, sizeof(AudioDeviceID)));

	CheckResult(AudioUnitInitialize(outputUnit));

	m_OutputUnit = outputUnit;
	m_DeviceId = deviceId;
}

void CLiveAudio::Terminate()
{
	Stop();
	AudioUnitUninitialize(m_OutputUnit);
	AudioComponentInstanceDispose(m_OutputUnit);
	m_DeviceId = 0;
	m_OutputUnit = 0;
}

void CLiveAudio::Start()
{
	CheckResult(AudioOutputUnitStart(m_OutputUnit));
}

void CLiveAudio::Stop()
{
	AudioOutputUnitStop(m_OutputUnit);
}

void CLiveAudio::CheckResult(OSErr err)
{
	if (err != noErr) throw CAudioException();
}

OSStatus CLiveAudio::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags	*ioActionFlags,
									const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber,
									UInt32 inNumberFrames,AudioBufferList *ioData)
{
	CLiveAudio *pThis = (CLiveAudio *)inRefCon;

	for (INTPTR ipBuffer=0; ipBuffer < ioData->mNumberBuffers; ipBuffer++) {
		INTPTR ipDataSize = ioData->mBuffers[ipBuffer].mDataByteSize;
		char *pData = (char *)ioData->mBuffers[ipBuffer].mData;
		INTPTR ipReceived = (pThis && pThis->m_pProvider ? pThis->m_pProvider->GetAudio(pData, ipDataSize) : 0);
		if (ipReceived < ipDataSize) {
			memset(pData + ipReceived, 0, ipDataSize - ipReceived);
		}
	}

	return noErr;
}

//////////////////////////////////

CFileAudio::CFileAudio(CAudioDataProvider *pProvider) :
	CAudio(pProvider, TYPE_FILE),
	m_FileRef(0),
	m_FileId(0),
	m_bOwner(false)
{

}

CFileAudio::~CFileAudio()
{
	Terminate();
}

void CFileAudio::Init(const AudioStreamBasicDescription &format, ExtAudioFileRef fileRef, bool bOwner)
{
	m_FileRef = fileRef;
	m_bOwner = bOwner;

	UInt32 size = sizeof(AudioFileID);
	ExtAudioFileGetProperty(m_FileRef, kExtAudioFileProperty_AudioFile,
							&size, &m_FileId);

	// Try to avoid updating the file size after each write, as this tends to be
	// a very costly operation.
	AudioFileOptimize(m_FileId);
	UInt32 defer = 0;
	AudioFileSetProperty(m_FileId, kAudioFilePropertyDeferSizeUpdates,
						 sizeof(UInt32), &defer);
	ExtAudioFileSetProperty(m_FileRef, kExtAudioFileProperty_ClientDataFormat,
							sizeof(AudioStreamBasicDescription), &format);
}

void CFileAudio::Terminate()
{
	if (m_FileId) {
		AudioFileOptimize(m_FileId);
		if (m_bOwner) {
			ExtAudioFileDispose(m_FileRef);
		}
	}

	m_FileRef = 0;
	m_FileId = 0;
	m_bOwner = false;
}

void CFileAudio::Flush()
{
	if (!m_pProvider) return;
	CFSData Data;
	Data.SetSize(1024);

	AudioBufferList buffer;
	buffer.mNumberBuffers = 1;
	buffer.mBuffers[0].mNumberChannels = 1;
	buffer.mBuffers[0].mDataByteSize = Data.GetSize();
	buffer.mBuffers[0].mData = Data.GetData();

	// detect byte order;
	unsigned short us = 0x1234;
	unsigned char *pus = (unsigned char *)&us;
	bool bLE = pus[0] > pus[1];

	for (;;) {
		INTPTR ipCount = m_pProvider->GetAudio(Data.GetData(), Data.GetSize()) / 2;
		if (!ipCount) break;

		if (bLE) {
			char *pBytes = (char *)Data.GetData();
			char cTemp;
			for (INTPTR ip = 0; ip < ipCount; ip++) {
				cTemp = pBytes[ip * 2];
				pBytes[ip * 2] = pBytes[ip * 2 + 1];
				pBytes[ip * 2 + 1] = cTemp;
			}
		}

		ExtAudioFileWrite(m_FileRef, (UInt32)ipCount, &buffer);
	}
}

#pragma once

#import <AudioToolbox/AudioToolbox.h>

class CAudioException : public CFSException {

};

class CAudioDataProvider {
public:
	virtual INTPTR GetAudio(void *pBuf, INTPTR ipSize) = 0;
};

class CAudio {
public:
	enum AUDIOTYPE { TYPE_NONE, TYPE_LIVE, TYPE_FILE };

	CAudio(CAudioDataProvider *pProvider, AUDIOTYPE Type) :
		m_pProvider(pProvider),
		m_Type(Type) { }
	virtual ~CAudio() { };

	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual void Flush() = 0;

	virtual int GetType() const { return m_Type; }

protected:
	AUDIOTYPE m_Type;
	CAudioDataProvider *m_pProvider;
};

class CLiveAudio : public CAudio {
public:
	CLiveAudio(CAudioDataProvider *pProvider);
	virtual ~CLiveAudio();

	void Init(const AudioStreamBasicDescription &format, AudioDeviceID deviceId = kAudioDeviceUnknown);
	void Terminate();

	void Start();
	void Stop();

	void Flush() { }

	AudioDeviceID GetDeviceId() const { return m_DeviceId; }
	
protected:
	void CheckResult(OSErr err);

	static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags	*ioActionFlags,
								   const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber,
								   UInt32 inNumberFrames,AudioBufferList *ioData);

	AudioUnit m_OutputUnit;
	AudioDeviceID m_DeviceId;
};

class CFileAudio : public CAudio {
public:
	CFileAudio(CAudioDataProvider *pProvider);
	virtual ~CFileAudio();

	void Init(const AudioStreamBasicDescription &format, ExtAudioFileRef fileRef, bool bOwner);
	void Terminate();
	
	void Start() { }
	void Stop() { }

	void Flush();

	ExtAudioFileRef GetFileRef() const { return m_FileRef; }

protected:
	ExtAudioFileRef m_FileRef;
	AudioFileID m_FileId;
	bool m_bOwner;
};

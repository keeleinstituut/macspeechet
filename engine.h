#pragma once

#include "speech.h"
#include "textprocessor.h"
#include "audio.h"
#include "pool.h"

class CMacEngine :
	public CSpeechEngine,
	public CFSThread,
	public CAudioDataProvider
{
public:
	CMacEngine();
	virtual ~CMacEngine();

	void Init();
	long InitVoice(CFBundleRef bundle);
	
	long Start(CFStringRef pText, bool bNoInterrupt, bool bPreflight);
	long Pause(int iWhere);
	long Resume();
	long Stop(int iWhere);

	long GetProperty(CFStringRef pProperty, CFTypeRef *pObject);
	long SetProperty(CFStringRef pProperty, CFTypeRef pObject);

protected:
	enum ENGINE_STATE { STATE_STOP, STATE_PAUSE, STATE_PLAY };

	void ResetProperties();
	
	CFSClassArray<CFragment> ProcessText(CFSWString szText, INTPTR ipDelta);
	CFSClassArray<CFragment> ProcessCommands(CFSWString szCommands);

	void CreateLiveAudio(AudioDeviceID audioDeviceId);
	void RequestState(ENGINE_STATE eNewState, int iWhere, bool bWait);

	void CheckRequest(int iWhere);
	void ResetActiveSentence();
	void RunDoneCallbacks();
	virtual int Run();

	AudioStreamBasicDescription GetAudioFormat();
	INTPTR GetAudio(void *pBuf, INTPTR ipSize);
	
protected:
	CFSArray<CMorphInfo> Analyze(const CFSWString &szWord);

	ENGINE_STATE m_eRequestState;
	int m_iRequestStateWhere;
	ENGINE_STATE m_eWorkerState;
	bool m_bExit;

	CTextProcessor m_TextProcessor;
	static CLinguisticPool m_LinguisticPool;

	// Main -> Worker thread
	CFStringRef m_pText;
	CFSClassArray<CFSClassArray<CFragment> > m_Sentences;

	// Worker <-> Audio thread
	CAudio *m_pAudioOutput;
	CFSMutex m_AudioMutex;
	CFSClassArray<CFragment> m_ActiveSentence;
	INTPTR m_ipAudioFragment, m_ipAudioFragmentOffset;

	// API properties
	CFSWString m_szCommandOpen, m_szCommandClose;
	float m_fSpeechRate;
	float m_fVolume;

	SRefCon m_pRefCon;
	SpeechTextDoneProcPtr m_pTextDoneCallback;	// Callback to call when we no longer need the input text
	SpeechDoneProcPtr m_pSpeechDoneCallback;	// Callback to call when we're done
	SpeechSyncProcPtr m_pSyncCallback;			// Callback to call for sync embedded command
	SpeechWordCFProcPtr m_pWordCallback;		// Callback to call for each word
	SpeechErrorCFProcPtr m_ErrorCallback;		// Callback to call for async error (?)
	SpeechPhonemeProcPtr m_PhonemeCallback;		// Smth with phonemes...
};

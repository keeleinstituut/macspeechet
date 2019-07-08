#include "stdafx.h"
#include "engine.h"
#include "typeconvert.h"

CLinguisticPool CMacEngine::m_LinguisticPool;

CMacEngine::CMacEngine() :
	m_eRequestState(STATE_STOP),
	m_iRequestStateWhere(kImmediate),
	m_eWorkerState(STATE_STOP),
	m_bExit(false),
	m_pAudioOutput(NULL),
	m_ipAudioFragment(-1),
	m_pText(NULL),
	m_szCommandOpen(L"[["),
	m_szCommandClose(L"]]"),
	m_pRefCon(NULL),
	m_pTextDoneCallback(NULL),
	m_pSpeechDoneCallback(NULL),
	m_pSyncCallback(NULL),
	m_pWordCallback(NULL),
	m_ErrorCallback(NULL),
	m_PhonemeCallback(NULL)
{
	ResetProperties();
	CFSThread::Create(NULL);
}

CMacEngine::~CMacEngine()
{
	RequestState(STATE_STOP, kImmediate, true);
	if (m_pAudioOutput) delete m_pAudioOutput;
	m_bExit = true;
	CFSThread::WaitForEnd();
}

void CMacEngine::Init()
{
	CreateLiveAudio(kAudioDeviceUnknown);
}

void CMacEngine::CreateLiveAudio(AudioDeviceID audioDeviceId)
{
	if (m_pAudioOutput) delete m_pAudioOutput;
	m_pAudioOutput = NULL;
	CLiveAudio *pLiveAudio = new CLiveAudio(this);
	pLiveAudio->Init(GetAudioFormat(), audioDeviceId);
	m_pAudioOutput = pLiveAudio;
}

void CMacEngine::ResetProperties()
{
	m_fSpeechRate = 150.0f;
	m_fVolume = 1.0f;
}

long CMacEngine::InitVoice(CFBundleRef bundle)
{
	if (!bundle) return voiceNotFound;
	CFSFileName szVoice = GetResourceFile(bundle, CFSTR("voice.htsvoice"));

	// fill in voice additional info
	CFDictionaryRef voiceAttributes = (CFDictionaryRef)CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("VoiceAttributes"));
	if (voiceAttributes && CFGetTypeID(voiceAttributes) != CFDictionaryGetTypeID()) return voiceNotFound;

	CVoiceSettings VoiceSettings;
	VoiceSettings.speed = 0.01 * GetDictionaryLong(voiceAttributes, CFSTR("VoiceSpeed"), 100);
	VoiceSettings.ht = 0.01 * GetDictionaryLong(voiceAttributes, CFSTR("VoiceHalfTone"), 0);
	VoiceSettings.gvw1 = 0.01 * GetDictionaryLong(voiceAttributes, CFSTR("VoiceWeight1"), 100);
	VoiceSettings.gvw2 = 0.01 * GetDictionaryLong(voiceAttributes, CFSTR("VoiceWeight2"), 100);
	TRACE("OpenVoice %s %f %f %f %f\n", (const char *)szVoice, VoiceSettings.speed, VoiceSettings.ht, VoiceSettings.gvw1, VoiceSettings.gvw2);

	OpenVoice(szVoice, VoiceSettings);

	return noErr;
}

CFSClassArray<CFragment> CMacEngine::ProcessText(CFSWString szText, INTPTR ipSrcOffset)
{
	TRACE("ProcessText(%s)\n", (const char *)FSStrWtoA(szText));
	CFSClassArray<CFragment> Fragments;
	CFSClassArray<CFragment> Fragments2 = m_TextProcessor.SplitText(szText);

	for (INTPTR ip = 0; ip < Fragments2.GetSize(); ip++) {
		CFragment &Fragment = Fragments2[ip];
		Fragment.m_ipStartPos += ipSrcOffset;
		Fragment.m_eAction = SPVA_Speak;

		CFSWString szCleanWord = m_TextProcessor.GetCleanWord(Fragment.m_szText);
		if (!szCleanWord.GetLength()) continue;
		Fragment.m_Morph.m_szWord = szCleanWord;

		Fragment.m_Morph.m_MorphInfo = Analyze(Fragment.m_Morph.m_szWord);
		Fragments.AddItem(Fragment);
	}

	return Fragments;
}

CFSClassArray<CFragment> CMacEngine::ProcessCommands(CFSWString szCommands)
{
	TRACE("ProcessCommands(%s)\n", (const char *)FSStrWtoA(szCommands));
	CFSClassArray<CFragment> Fragments;
	CFSWStringArray Commands;
	FSStrSplit(szCommands, L';', Commands);
	for (INTPTR ipCommand = 0; ipCommand < Commands.GetSize(); ipCommand++) {
		CFSWString szCommand = Commands[ipCommand];
		szCommand.Trim();
		szCommand.Replace(L"  ", L" ", CFSWString::REPLACE_ALL | CFSWString::REPLACE_CONT);
		CFSWStringArray Arguments;
		FSStrSplit(szCommand, L' ', Arguments);
		if (Arguments.GetSize()) {
			CFragment Fragment;
			Fragment.m_ipStartPos = -1;
			Fragment.m_eAction = SPVA_Command;
			Fragment.m_szText = Arguments[0];
			Arguments.RemoveItem(0);
			Fragment.m_Arguments = Arguments;
			Fragments.AddItem(Fragment);
		}
	}

	for (INTPTR ip = 0; ip < Fragments.GetSize(); ip++) {
		CFragment &Fragment = Fragments[ip];
		bool bRemove = false;

		if (Fragment.m_szText == L"cmnt") { // Comment
			Fragments.SetSize(ip);
			break;
		} else if (Fragment.m_szText == L"dlim") { // Command delimiter
			Fragment.ArgumentAdjustString(m_szCommandOpen, 0);
			Fragment.ArgumentAdjustString(m_szCommandClose, 1);
			bRemove = true;
		} // ... todo

		if (bRemove) {
			Fragments.RemoveItem(ip);
			ip--;
		}
	}

	return Fragments;
}

long CMacEngine::Start(CFStringRef pText, bool bNoInterrupt, bool bPreflight)
{
	TRACE("CMacEngine::Start('%s', %d, %d)\n",
		  (const char *)FSStrWtoA(GetString(pText), FSCP_UTF8), (int)bNoInterrupt, (int)bPreflight);

	if (bNoInterrupt && m_eRequestState != STATE_STOP) {
		return synthNotReady;
	}
	RequestState(STATE_STOP, kImmediate, true);

	CFSWString szText = GetString(pText);
	CFSClassArray<CFragment> Fragments;
	INTPTR ipSegmentStart = 0;
	for (;;) {
		INTPTR ipOpenPos = -1;
		INTPTR ipClosePos = -1;
		if (m_szCommandOpen.GetLength()) {
			ipOpenPos = szText.Find(m_szCommandOpen, ipSegmentStart);
		}
		if (ipOpenPos == -1) break;
		if (m_szCommandClose.GetLength()) {
			ipClosePos = szText.Find(m_szCommandClose, ipOpenPos + m_szCommandOpen.GetLength());
		}
		if (ipClosePos == -1) break;

		Fragments += ProcessText(szText.Mid(ipSegmentStart, ipOpenPos - ipSegmentStart), ipSegmentStart);
		Fragments += ProcessCommands(szText.Mid(ipOpenPos + m_szCommandOpen.GetLength(), ipClosePos - ipOpenPos - m_szCommandOpen.GetLength()));
		ipSegmentStart = ipClosePos + m_szCommandClose.GetLength();
	}
	Fragments += ProcessText(szText.Mid(ipSegmentStart), ipSegmentStart);

	// Second round of commands
	bool bSkip = false;
	bool bSpellOut = false;
	for (INTPTR ip = 0; ip < Fragments.GetSize(); ip++) {
		CFragment &Fragment = Fragments[ip];
		if (Fragment.m_eAction == SPVA_Command) {
			CFSWString szMode;
			if (Fragment.m_szText == L"ctxt") {
				Fragment.ArgumentAdjustString(szMode, 0);
				bSkip = (szMode == L"TSKP" || szMode == L"WSKP");
			} else if (Fragment.m_szText == L"char") { // kSpeechCharacterModeProperty
				Fragment.ArgumentAdjustString(szMode, 0);
				bSpellOut = (szMode == L"LTRL");
			}
		} else {
			if (bSkip) {
				Fragments.RemoveItem(ip);
				ip--;
				continue;
			}
			if (bSpellOut && Fragment.m_eAction == SPVA_Speak) {
				Fragment.m_eAction = SPVA_SpellOut;
			}
		}
	}

	// Remove extra whitespaces
	CFragment::FRAGMENTTYPE ePrevType = CFragment::TYPE_OTHER;
	for (INTPTR ip = 0; ip < Fragments.GetSize(); ip++) {
		if (Fragments[ip].IsSpeakAction()) continue;
		if (Fragments[ip].m_eType == CFragment::TYPE_SPACE && ePrevType == CFragment::TYPE_SPACE) {
			Fragments.RemoveItem(ip);
			ip--;
		} else {
			ePrevType = Fragments[ip].m_eType;
		}
	}

	ASSERT(m_eWorkerState == STATE_STOP);
	m_pText = pText;
	m_Sentences = m_TextProcessor.CreateSentences(Fragments);

	RequestState((bPreflight ? STATE_PAUSE : STATE_PLAY), kImmediate, true);
	return noErr;
}

long CMacEngine::Pause(int iWhere)
{
	RequestState(STATE_PAUSE, iWhere, false);
	return noErr;
}

long CMacEngine::Resume()
{
	if (m_eRequestState == STATE_PAUSE) {
		RequestState(STATE_PLAY, kImmediate, false);
	}
	return noErr;
}

long CMacEngine::Stop(int iWhere)
{
	RequestState(STATE_STOP, iWhere, false);
	return noErr;
}

long CMacEngine::GetProperty(CFStringRef pProperty, CFTypeRef *pObject)
{
	if (!CFStringCompare(pProperty, kSpeechInputModeProperty, 0)) {
		*pObject = CFRetain(kSpeechModeText); // ???
	} else if (!CFStringCompare(pProperty, kSpeechCharacterModeProperty, 0)) {
		*pObject = CFRetain(kSpeechModeNormal); // ???
	} else if (!CFStringCompare(pProperty, kSpeechNumberModeProperty, 0)) {
		*pObject = CFRetain(kSpeechModeNormal); // ???
	} else if (!CFStringCompare(pProperty, kSpeechRateProperty, 0)) {
		*pObject = NewFloat(m_fSpeechRate);
	} else if (!CFStringCompare(pProperty, kSpeechVolumeProperty, 0)) {
		*pObject = NewFloat(m_fVolume);
	} else if (!CFStringCompare(pProperty, kSpeechPitchBaseProperty, 0)) {
		*pObject = NewFloat(0.0f);
	} else if (!CFStringCompare(pProperty, kSpeechPitchModProperty, 0)) {
		*pObject = NewFloat(0.0f);
	} else if (!CFStringCompare(pProperty, kSpeechStatusProperty, 0)) {
		CFTypeRef statusKeys[4];
		CFTypeRef statusValues[4];

		statusKeys[0]	= kSpeechStatusOutputBusy;
		statusValues[0]	= NewInt(m_eWorkerState == STATE_PLAY ? 1 : 0);
		statusKeys[1]	= kSpeechStatusOutputPaused;
		statusValues[1]	= NewInt(m_eWorkerState == STATE_PAUSE ? 1 : 0);
		statusKeys[2]	= kSpeechStatusNumberOfCharactersLeft;
		statusValues[2]	= NewInt(0);
		statusKeys[3]	= kSpeechStatusPhonemeCode;
		statusValues[3]	= NewInt(0);

		*pObject = CFDictionaryCreate(NULL, statusKeys, statusValues, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFRelease(statusValues[0]);
		CFRelease(statusValues[1]);
		CFRelease(statusValues[2]);
		CFRelease(statusValues[3]);
//	} else if (!CFStringCompare(pProperty, CFSTR("aunt"), 0)) { /* soAudioUnit */
//		/*
//		 * If we're used from within an audio unit, we can safely assume that
//		 * one of the three following property calls is issued before anything
//		 * else could happen that would cause a sound channel to be created.
//		 */
//		[self createSoundChannel:YES];
//		*pObject = newPtr([audioOutput getSourceUnit]);
//	} else if (!CFStringCompare(pProperty, CFSTR("augr"), 0)) { /* soAudioGraph */
//		[self createSoundChannel:YES];
//		*pObject = newPtr([audioOutput getSourceGraph]);
//	} else if (!CFStringCompare(pProperty, CFSTR("offl"), 0)) { /* soOfflineMode */
//		[self createSoundChannel:YES];
//		*pObject = newInt([audioOutput offlineProcessing]);
	} else {
		TRACE("Unhandled GetProperty '%s'\n", (const char *)FSStrWtoA(GetString(pProperty), FSCP_UTF8));
		return siUnknownInfoType;
	}

	return noErr;
}

long CMacEngine::SetProperty(CFStringRef pProperty, CFTypeRef pObject)
{
	if (!CFStringCompare(pProperty, kSpeechResetProperty, 0)) {
		ResetProperties();
	} else if (!CFStringCompare(pProperty, kSpeechRateProperty, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberFloatType, &m_fSpeechRate);
	} else if (!CFStringCompare(pProperty, kSpeechVolumeProperty, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberFloatType, &m_fVolume);
	} else if (!CFStringCompare(pProperty, kSpeechPitchBaseProperty, 0)) {
		// Ignore
	} else if (!CFStringCompare(pProperty, kSpeechPitchModProperty, 0)) {
		// Ignore
	} else if (!CFStringCompare(pProperty, kSpeechRefConProperty, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_pRefCon);
	} else if (!CFStringCompare(pProperty, kSpeechOutputToFileURLProperty, 0)) {
		TRACE("OutputToFileURL %p\n", pObject);
		if (pObject) {
			ExtAudioFileRef newFileRef = 0;
			AudioStreamBasicDescription format = GetAudioFormat();
			format.mFormatFlags |= kAudioFormatFlagIsBigEndian; // Does not support LE
			if (!ExtAudioFileCreateWithURL((CFURLRef)pObject, kAudioFileAIFFType,
										   &format, NULL, kAudioFileFlags_EraseFile, &newFileRef))
			{
				RequestState(STATE_STOP, kImmediate, true);
				if (m_pAudioOutput) delete m_pAudioOutput;
				m_pAudioOutput = NULL;
				CFileAudio *pFileAudio = new CFileAudio(this);
				pFileAudio->Init(GetAudioFormat(), newFileRef, true);
				m_pAudioOutput = pFileAudio;
			}
		} else if (!m_pAudioOutput || m_pAudioOutput->GetType() != CAudio::TYPE_LIVE) {
			RequestState(STATE_STOP, kImmediate, true);
			CreateLiveAudio(kAudioDeviceUnknown);
		}
	} else if (!CFStringCompare(pProperty, kSpeechOutputToExtAudioFileProperty, 0)) {
		TRACE("OutputToExtAudioFile %p\n", pObject);
		ExtAudioFileRef newFileRef = 0;
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &newFileRef);
		if (pObject) {
			if (!m_pAudioOutput || m_pAudioOutput->GetType() != CAudio::TYPE_FILE ||
				newFileRef != ((CFileAudio *)m_pAudioOutput)->GetFileRef())
			{
				RequestState(STATE_STOP, kImmediate, true);
				if (m_pAudioOutput) delete m_pAudioOutput;
				m_pAudioOutput = NULL;
				CFileAudio *pFileAudio = new CFileAudio(this);
				AudioStreamBasicDescription format = GetAudioFormat();
				format.mFormatFlags |= kAudioFormatFlagIsBigEndian; // Does not support LE
				pFileAudio->Init(format, newFileRef, false);
				m_pAudioOutput = pFileAudio;
			}
		} else if (!m_pAudioOutput || m_pAudioOutput->GetType() != CAudio::TYPE_LIVE) {
			RequestState(STATE_STOP, kImmediate, true);
			CreateLiveAudio(kAudioDeviceUnknown);
		}
	} else if (!CFStringCompare(pProperty, kSpeechOutputToAudioDeviceProperty, 0)) {
		TRACE("OutputToAudioDevice %p\n", pObject);
		AudioDeviceID newAudioDevice = 0;
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberSInt32Type, &newAudioDevice);
		if (!m_pAudioOutput || m_pAudioOutput->GetType() != CAudio::TYPE_LIVE ||
			newAudioDevice != ((CLiveAudio *)m_pAudioOutput)->GetDeviceId())
		{
			RequestState(STATE_STOP, kImmediate, true);
			CreateLiveAudio(newAudioDevice);
		}
	} else if (!CFStringCompare(pProperty, CFSTR("offl"), 0)) { // soOfflineMode
		TRACE("OutputOffline %p\n", pObject);
		return siUnknownInfoType;
//		SInt8 offline;
//		CFNumberGetValue((CFNumberRef)object, kCFNumberSInt8Type, &offline);
//		[self createSoundChannel:YES];
//		[audioOutput setOfflineProcessing:offline];
	} else if (!CFStringCompare(pProperty, kSpeechSpeechDoneCallBack, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_pSpeechDoneCallback);
	} else if (!CFStringCompare(pProperty, kSpeechSyncCallBack, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_pSyncCallback);
	} else if (!CFStringCompare(pProperty, kSpeechWordCFCallBack, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_pWordCallback);
	} else if (!CFStringCompare(pProperty, kSpeechTextDoneCallBack, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_pTextDoneCallback);
	} else if (!CFStringCompare(pProperty, kSpeechErrorCFCallBack, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_ErrorCallback);
	} else if (!CFStringCompare(pProperty, kSpeechPhonemeCallBack, 0)) {
		CFNumberGetValue((CFNumberRef)pObject, kCFNumberLongType, &m_PhonemeCallback);
	} else {
		TRACE("Unhandled SetProperty '%s'\n", (const char *)FSStrWtoA(GetString(pProperty), FSCP_UTF8));
		return siUnknownInfoType;
	}

	return noErr;
}

void CMacEngine::RequestState(ENGINE_STATE newState, int iWhere, bool bWait)
{
	do {
		m_iRequestStateWhere = iWhere;
		m_eRequestState = newState;
		if (!bWait) return;
		while (m_eRequestState != m_eWorkerState) usleep(5 * 1000);
	} while (m_eRequestState != newState && m_eRequestState != STATE_STOP);
}

CFSArray<CMorphInfo> CMacEngine::Analyze(const CFSWString &szWord)
{
	CSpeechLinguistic *pLinguistic = NULL;
	try {
		pLinguistic = m_LinguisticPool.GetLinguistic();
		CFSArray<CMorphInfo> MorphInfos = pLinguistic->Analyze(szWord);
		m_LinguisticPool.ReleaseLinguistic(pLinguistic);
		return MorphInfos;
	} catch(...) {
		if (pLinguistic) delete pLinguistic;
		throw CFSException();
	}
}

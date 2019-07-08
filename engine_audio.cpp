#include "stdafx.h"
#include "engine.h"

AudioStreamBasicDescription CMacEngine::GetAudioFormat()
{
	AudioStreamBasicDescription format;
	format.mSampleRate       = 48000;
	format.mFormatID         = kAudioFormatLinearPCM;
	format.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	format.mBitsPerChannel   = 2 * 8;
	format.mChannelsPerFrame = 1;
	format.mBytesPerFrame    = 2 * format.mChannelsPerFrame;
	format.mFramesPerPacket  = 1;
	format.mBytesPerPacket   = format.mBytesPerFrame * format.mFramesPerPacket;
	format.mReserved         = 0;

	return format;
}

INTPTR CMacEngine::GetAudio(void *pBuf, INTPTR ipSize)
{
	if (m_eWorkerState != STATE_PLAY) return 0;

	CFSAutoLock Autolock(&m_AudioMutex);
	if (m_ipAudioFragment < 0) return 0;
	if (m_ipAudioFragment >= m_ActiveSentence.GetSize()) return 0;

	INTPTR ipResult = 0;
	for (; m_ipAudioFragment < m_ActiveSentence.GetSize(); m_ipAudioFragment++, m_ipAudioFragmentOffset = 0) {
		CFragment &Fragment = m_ActiveSentence[m_ipAudioFragment];
		if (Fragment.m_Audio.GetSize()) {
			INTPTR ipAvailable = FSMIN(ipSize - ipResult, Fragment.m_Audio.GetSize() - m_ipAudioFragmentOffset);
			memcpy((char *)pBuf + ipResult, (const char *)Fragment.m_Audio.GetData() + m_ipAudioFragmentOffset, ipAvailable);
			// TRACE("GetAudio %ld %ld %ld\n", m_ipAudioFragment, m_ipAudioFragmentOffset, ipAvailable);
			ipResult += ipAvailable;
			m_ipAudioFragmentOffset += ipAvailable;
			if (ipResult >= ipSize) return ipResult;
		}
	}
	return ipResult;
}

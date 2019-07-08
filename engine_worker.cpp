#include "stdafx.h"
#include "engine.h"
#include "typeconvert.h"

void CMacEngine::CheckRequest(int iWhere) {
	switch (m_eRequestState) {
		case STATE_STOP:
			if (m_eWorkerState != STATE_STOP && m_iRequestStateWhere <= iWhere) {
				if (m_pAudioOutput) m_pAudioOutput->Stop();
				ResetActiveSentence();
				m_Sentences.Cleanup();
				RunDoneCallbacks();
				m_eWorkerState = STATE_STOP;
			}
			break;
		case STATE_PAUSE:
			if (m_eWorkerState != STATE_PAUSE && m_iRequestStateWhere <= iWhere) {
				if (m_pAudioOutput) m_pAudioOutput->Stop();
				m_eWorkerState = STATE_PAUSE;
			}
			break;
		case STATE_PLAY:
			if (m_eWorkerState != STATE_PLAY) {
				if (m_pAudioOutput) {
					m_pAudioOutput->Start();
					m_eWorkerState = STATE_PLAY;
				} else {
					m_Sentences.Cleanup();
					m_eWorkerState = STATE_STOP;
				}
			}
			break;
	}
}

void CMacEngine::ResetActiveSentence()
{
	CFSAutoLock Autolock(&m_AudioMutex);
	m_ActiveSentence.Cleanup();
	m_ipAudioFragment = -1;
}

void CMacEngine::RunDoneCallbacks()
{
	if (m_pTextDoneCallback) { // Deprecated callback, not using actually
		const void *pNextBuf = NULL;
		unsigned long ulByteLen = 0;
		SInt32 iControlFlags= 0;
		TRACE("TextDoneCallback\n");
		(*m_pTextDoneCallback)((SpeechChannel)this, m_pRefCon, &pNextBuf, &ulByteLen, &iControlFlags);
	}

	if (m_pSpeechDoneCallback) {
		TRACE("SpeechDoneCallback\n");
		(*m_pSpeechDoneCallback)((SpeechChannel)this, m_pRefCon);
	}
}

int CMacEngine::Run()
{
	TRACE("CMacEngine::Run()\n");
	while (!m_bExit) {
		CheckRequest(kEndOfSentence);
		if (m_eWorkerState == STATE_STOP || m_eWorkerState == STATE_PAUSE) {
			usleep(10 * 1000); // 10ms
			continue;
		}

		if (!m_Sentences.GetSize()) {
			m_pText = NULL;
			m_eWorkerState = STATE_STOP;
			m_eRequestState = STATE_STOP;
			RunDoneCallbacks();
			continue;
		}

		// Process sentence
		TRACE("Disambiguate sentence\n");
		{
			CFSAutoLock Autolock(&m_AudioMutex);
			m_ipAudioFragment = -1;
			m_ActiveSentence = m_Sentences[0];
			m_Sentences.RemoveItem(0);
		}

		try {
			CFSArray<CMorphInfos> Analysis, Disambiguated;
			// Disambiguate sentence
			for (INTPTR ip2 = 0; ip2 < m_ActiveSentence.GetSize(); ip2++) {
				if (!m_ActiveSentence[ip2].IsSpeakAction()) continue;
				switch (m_ActiveSentence[ip2].m_eType) {
					case CFragment::TYPE_WORD:
					case CFragment::TYPE_PUNCTUATION:
					case CFragment::TYPE_SYMBOL:
						Analysis.AddItem(m_ActiveSentence[ip2].m_Morph);
				}
			}

			CDisambiguator *pDisambiguator = NULL;
			try {
				CDisambiguator *pDisambiguator = m_LinguisticPool.GetDisambiguator();
				Disambiguated = pDisambiguator->Disambiguate(Analysis);
				m_LinguisticPool.ReleaseDisambiguator(pDisambiguator);
			} catch (...) {
				if (pDisambiguator) delete pDisambiguator;
				throw CFSException();
			}

			if (Disambiguated.GetSize() != Analysis.GetSize()) {
				throw CLinguisticException(CLinguisticException::MAINDICT);
			}
			INTPTR ipDisamb = 0;
			for (INTPTR ip2 = 0; ip2 < m_ActiveSentence.GetSize(); ip2++) {
				if (!m_ActiveSentence[ip2].IsSpeakAction()) continue;
				switch (m_ActiveSentence[ip2].m_eType) {
					case CFragment::TYPE_WORD:
					case CFragment::TYPE_PUNCTUATION:
					case CFragment::TYPE_SYMBOL:
						m_ActiveSentence[ip2].m_Morph = Disambiguated[ipDisamb++];
				}
			}

			CheckRequest(kEndOfWord);
			if (m_eWorkerState == STATE_STOP) continue;

			TRACE("Create audio\n");
			float fVolume;
			if (m_fVolume <= 0) fVolume = -144.0;
			else fVolume = FSMINMAX(20 * log10(m_fVolume), -144, 0);
			CreateAudio(m_ActiveSentence, fVolume, FSMINMAX(m_fSpeechRate / 150.0, 0.35, 3.0));

			/* for (INTPTR ip2 = 0; ip2 < m_ActiveSentence.GetSize(); ip2++) {
				CFragment &Fragment = m_ActiveSentence[ip2];
				if (Fragment.m_eAction != SPVA_Command) continue;

				if (Fragment.m_szText == L"slnc") { // Silence for N ms
					int iMs = 0;
					Fragment.ArgumentAdjustInt(iMs, 0);
					if (iMs > 0) {
						Fragment.m_Audio.SetSize(96 * iMs);
						memset(Fragment.m_Audio.GetData(), 0, Fragment.m_Audio.GetSize());
					}
				}
			} */

		} catch (...) {
			TRACE("ERROR during disambiguation or CreateAudio\n");
			continue;
		}

		TRACE("Output audio\n");
		try {
			// Output sentence
			{
				CFSAutoLock Autolock(&m_AudioMutex);
				m_ipAudioFragment = m_ipAudioFragmentOffset = 0;
			}

			if (!m_pAudioOutput) {
				ResetActiveSentence();
				continue;
			}
			TRACE("Flush (%d)\n", m_pAudioOutput->GetType());
			m_pAudioOutput->Flush();

			INTPTR ipWorkerFragment = -1;
			for (;;) {
				CheckRequest(kEndOfWord);
				if (m_eWorkerState == STATE_STOP) break;

				INTPTR ipAudioFragment;
				{
					CFSAutoLock Autolock(&m_AudioMutex);
					ipAudioFragment = m_ipAudioFragment;
				}

				if (ipWorkerFragment >= ipAudioFragment) {
					usleep(1000); // 1ms
					continue;
				}

				ipWorkerFragment++;
				if (ipWorkerFragment >= m_ActiveSentence.GetSize()) break;

				const CFragment &Fragment = m_ActiveSentence[ipWorkerFragment];
				if (m_pWordCallback && m_pText && Fragment.m_Audio.GetSize() && Fragment.m_ipStartPos >= 0) {
					CFRange WordRange = CFRangeMake(Fragment.m_ipStartPos, Fragment.m_szText.GetLength());
					TRACE("WordCallback(%zd, %zd)\n", WordRange.location, WordRange.length);
					(*m_pWordCallback)((SpeechChannel)this, m_pRefCon, m_pText, WordRange);
				}
				if (Fragment.m_eAction == SPVA_Command) {
					if (Fragment.m_szText == L"sync" && m_pSyncCallback) {
						int iArg = 0;
						Fragment.ArgumentAdjustInt(iArg, 0);
						TRACE("SyncCallback(%d)\n", iArg);
						(*m_pSyncCallback)((SpeechChannel)this, m_pRefCon, iArg);
					}
				}
			}

			// End of sentence, cleanup
			ResetActiveSentence();
		} catch (...) {
			TRACE("ERROR during output\n");
		}

	}
	TRACE("CMacEngine::Run END\n");
	return 0;
}

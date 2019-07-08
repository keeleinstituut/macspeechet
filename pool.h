#pragma once

#include "speech.h"

class CLinguisticPool {
public:
	CLinguisticPool();
	~CLinguisticPool();

	CSpeechLinguistic *GetLinguistic() { return GetSmth(m_Linguistics, m_szLinguisticDct); }
	void ReleaseLinguistic(CSpeechLinguistic *pLinguistic) { ReleaseSmth(m_Linguistics, pLinguistic); }

	CDisambiguator *GetDisambiguator() { return GetSmth(m_Disambiguators, m_szDisambiguatorDct); }
	void ReleaseDisambiguator(CDisambiguator *pDisambiguator) { ReleaseSmth(m_Disambiguators, pDisambiguator); }

protected:
	template <class TYPE> TYPE *GetSmth(CFSArray<TYPE *> &Array, CFSFileName szFileName)
	{
		{
			CFSAutoLock AutoLock(&m_Mutex);
			if (Array.GetSize() > 0) {
				INTPTR ipPos = Array.GetSize() - 1;
				TYPE *pSmth = Array[ipPos];
				Array.RemoveItem(ipPos);
				return pSmth;
			}
		}

		TYPE *pSmth = new TYPE();
		try {
			pSmth->Open(szFileName);
			return pSmth;
		} catch(...) {
			delete pSmth;
			throw;
		}
	}

	template <class TYPE> void ReleaseSmth(CFSArray<TYPE *> &Array, TYPE *pSmth)
	{
		CFSAutoLock AutoLock(&m_Mutex);
		if (Array.GetSize() >= 2) delete pSmth;
		else Array.AddItem(pSmth);
		// TRACE("Pool size %ld\n", Array.GetSize());
	}

	CFSArray<CSpeechLinguistic *> m_Linguistics;
	CFSArray<CDisambiguator *> m_Disambiguators;
	CFSMutex m_Mutex;

	CFSFileName m_szLinguisticDct, m_szDisambiguatorDct;
};

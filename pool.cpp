#include "stdafx.h"
#include "pool.h"
#include "typeconvert.h"

// Mac is very inefficient with Engine creation, so using pool to spped things up

CLinguisticPool::CLinguisticPool()
{
	CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("ee.ranivagi.speech.synthesis.Estonian"));
	if (bundle) {
		m_szLinguisticDct = GetResourceFile(bundle, CFSTR("et.dct"));
		m_szDisambiguatorDct = GetResourceFile(bundle, CFSTR("et3.dct"));
	}
}

CLinguisticPool::~CLinguisticPool()
{
	for (INTPTR ip = 0; ip < m_Linguistics.GetSize(); ip++) {
		delete m_Linguistics[ip];
	}
	for (INTPTR ip = 0; ip < m_Disambiguators.GetSize(); ip++) {
		delete m_Disambiguators[ip];
	}
}


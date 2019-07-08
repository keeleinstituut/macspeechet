#pragma once

#include "lib/proof/proof.h"
#include "lib/hts/HTS_engine.h"
#include "htssyn/htssyn.h"
#include "fragment.h"

class CSpeechLinguistic : public CLinguistic {
public:
	CFSArray<CMorphInfo> Analyze(const CFSWString &szWord);
};

class CAudioFragment {
public:
	CAudioFragment(INTPTR ipWeight, CFragment *pFragment) :
		m_ipWeight(ipWeight),
		m_pFragment(pFragment)
	{ }

	INTPTR m_ipWeight;
	CFragment *m_pFragment;
};

class CVoiceSettings {
public:
	CVoiceSettings() :
		speed(1.0),
		ht(0.0),
		gvw1(1.0),
		gvw2(1.0)
	{ }

	double speed;
	double ht;
	double gvw1;
	double gvw2;
};

class CSpeechEngine {
public:
	CSpeechEngine();
	virtual ~CSpeechEngine();

	void OpenVoice(const CFSFileName &szVoice, const CVoiceSettings &Settings);
	void CreateAudio(CFSClassArray<CFragment> &Sentence, double fVolume, double fSpeed);

protected:
	void CreateLabels(CFSClassArray<CFragment> &Sentence, CFSAStringArray &Labels, CFSArray<CAudioFragment> &AudioFragments);
	void AddAlternativeWords(CPhrase &Phrase, CFSWString szText, CFragment *pFragment);

	virtual CFSArray<CMorphInfo> Analyze(const CFSWString &szWord) = 0;

	HTS_Engine m_HTS;
	CFSMutex m_HTSMutex;

	double m_fVoiceSpeed;
};

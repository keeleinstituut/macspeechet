#include "stdafx.h"
#include "speech.h"

// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~£¥¤¢§©°±¼½¾«»ÄÕÖÜäõöüŠšŽžΑΒΓΔαβγδ–—―‘’‚“”„‹›€

static bool IsWord(const CFSWString &str) {
	if (str.GetLength() <= 1) return FALSE;
	bool bVowel = FALSE;
	for (INTPTR ip = 0; ip < str.GetLength(); ip++) {
		if (htssyn::is_vowel(str[ip])) {
			bVowel = true;
			continue;
		}
		if (FSIsLetter(str[ip])) continue;
		return false;
	}
	return bVowel;
}

enum CT { CT_NONE, CT_SPACE, CT_LETTER, CT_NUMBER, CT_OTHER };
static CT GetCharType(wchar_t ch) {
	if (!ch) return CT_NONE;
	if (ch == L' ') return CT_SPACE;
	if (FSIsNumber(ch)) return CT_NUMBER;
	if (FSIsLetter(ch)) return CT_LETTER;
	return CT_OTHER;
}

static CFSWString GetNumberText(const CFSWString &szText, INTPTR maxSignificant = 36) {
	CFSWString szNumber;
	CFSWString szResult;

	for (INTPTR ip = 0; ip < szText.GetLength(); ip++) {
		if (FSIsNumber(szText[ip])) szNumber += szText[ip];
	}
	if (!szText.GetLength()) return szResult;
	bool bOneByOne = (szText[0] == L'0');
	if (!bOneByOne) {
		INTPTR ipSignificant = 0;
		for (INTPTR ip = szNumber.GetLength(); ip > 0; ip--) {
			if (szNumber[ip - 1] != L'0') {
				ipSignificant = ip;
				break;
			}
		}
		bOneByOne = (ipSignificant > maxSignificant);
	}

	static CFSWString OneNames[] = {
		L"null", L"üks", L"kaks", L"kolm", L"neli", L"viis", L"kuus", L"seitse", L"kaheksa", L"üheksa"
	};
	static CFSWString TeenNames[] = {
		L"kümme", L"üksteist", L"kaksteist", L"kolmteist", L"neliteist", L"viisteist", L"kuusteist", L"seitseteist", L"kaheksateist", L"üheksateist"
	};
	static CFSWString RankNames[] = {
		L"", L"tuhat", L"miljon", L"biljon", L"triljon", L"kvadriljon", L"kvintiljon", L"sekstiljon", L"septiljon", L"oktiljon", L"noniljon", L"detsiljon"
	};

	if (bOneByOne) {
		for (INTPTR ip = 0; ip < szNumber.GetLength(); ip++) {
			szResult += OneNames[szNumber[ip] - L'0'] + L" ";
		}
	}
	else {
		INTPTR ipCursor = szNumber.GetLength();
		for (INTPTR ipRank = 0; ipCursor > 0; ipRank++) {
			int iOnes = szNumber[--ipCursor] - L'0';
			int iTens = (ipCursor > 0 ? szNumber[--ipCursor] - L'0' : 0);
			int iHundreds (ipCursor > 0 ? szNumber[--ipCursor] - L'0' : 0);

			CFSWString szBlock;
			if (iHundreds) {
				szBlock += OneNames[iHundreds] + L"sada ";
			}
			if (iTens == 1) {
				szBlock += TeenNames[iOnes] + L" ";
				iOnes = 0;
			}
			else if (iTens > 1) {
				szBlock += OneNames[iTens] + L"kümmend ";
			}
			if (iOnes) {
				szBlock += OneNames[iOnes] + L" ";
			}

			if (ipRank > 0 && szBlock.GetLength()) {
				CFSWString szRank = (ipRank < sizeof(RankNames) / sizeof(RankNames[0]) ? RankNames[ipRank] : L"midagiljon");
				if (ipRank > 1 && (iHundreds > 0 || iTens > 0 || iOnes > 1)) szRank += L"it";
				szBlock += szRank + L" ";
			}

			if (szBlock.GetLength()) {
				szResult = szBlock + szResult;
			}
		}
	}
	szResult.Trim();
	return szResult;
}

static CFSWString CountPunctuationText(INTPTR ipCount, const CFSWString &szText)
{
	CFSWString szCount;
	szCount.Format(L"%ld", (long)ipCount);
	return GetNumberText(szCount) + L" " + szText;
}

static CFSWString GetPunctuationText(const CFSWString &szText, CFragment::FRAGMENTTYPE eNextType)
{
	//Skipping
	//L"-\x2013\x2014\x2015"
	//L"(){}[]\x00AB\x00BB"
	//L"'\x2018\x2019\x201a\x2039\x203a" // Single quotes
	//L"\"\x201c\x201d\x201e\x00ab\x00bb" // Double quotes

	if (eNextType == CFragment::TYPE_WORD) {
		if (szText.GetLength() == 1) {
			switch (szText[0]) {
			case L'.': return L"punkt";
			case L',': return L"koma";
			case L':': return L"koolon";
			case L';': return L"semikoolon";
			case L'!': return L"hüüumärk";
			case L'?': return L"küsimärk";
			}
		}
	}

	if (szText.GetLength() > 1) {
		INTPTR ipPoint = 0;
		INTPTR ipExp = 0;
		INTPTR ipQuestion = 0;
		for (INTPTR ip = 0; ip < szText.GetLength(); ip++) {
			switch (szText[ip]) {
			case L'.': ipPoint++; break;
			case L'!': ipExp++; break;
			case L'?': ipQuestion++; break;
			}
		}
		if (ipExp > 0 && ipQuestion > 0) return L"hüüuküsimärk";
		if (ipPoint > 0) return CountPunctuationText(ipPoint, L"punkti");
		if (ipExp > 0) return CountPunctuationText(ipExp, L"hüüumärki");
		if (ipQuestion > 0) return CountPunctuationText(ipQuestion, L"küsimärki");
	}

	return L"";
}

static CFSWString GetCharacterText(wchar_t ch) {
	ch = FSToLower(ch);
	switch (ch) {
	case L'a': return L"aa";
	case L'b': return L"pee";
	case L'c': return L"tse";
	case L'd': return L"tee";
	case L'e': return L"ee";
	case L'f': return L"ef";
	case L'g': return L"kee";
	case L'h': return L"haa";
	case L'i': return L"ii";
	case L'j': return L"jott";
	case L'k': return L"kaa";
	case L'l': return L"ell";
	case L'm': return L"emm";
	case L'n': return L"enn";
	case L'o': return L"oo";
	case L'p': return L"pee";
	case L'q': return L"kuu";
	case L'r': return L"er";
	case L's': return L"ess";
	case L'š': return L"shaa";
	case L'ž': return L"shee";
	case L't': return L"tee";
	case L'u': return L"uu";
	case L'v': return L"vee";
	case L'w': return L"kaksisvee";
	case L'õ': return L"õõ";
	case L'ä': return L"ää";
	case L'ö': return L"öö";
	case L'ü': return L"ü";
	case L'x': return L"iks";
	case L'y': return L"igrek";
	case L'z': return L"tsett";

	case L'1': return L"üks";
	case L'2': return L"kaks";
	case L'3': return L"kolm";
	case L'4': return L"neli";
	case L'5': return L"viis";
	case L'6': return L"kuus";
	case L'7': return L"seitse";
	case L'8': return L"kaheksa";
	case L'9': return L"üheksa";
	case L'0': return L"null";

	case L'.': return L"punkt";
	case L'!': return L"hüüumärk";
	case L'?': return L"küsimärk";
	case L',': return L"koma";
	case L':': return L"koolon";
	case L';': return L"semikoolon";
	case L'-':
	case L'\x2013':
	case L'\x2014':
	case L'\x2015':
		return L"kriips";
	case L'_': return L"alakriips";
	case L'*': return L"tärn";
	case L'@': return L"ätt";
	case L'#': return L"trellid";
	case L'$': return L"dollarit";
	case L'€': return L"eurot";
	case L'£': return L"naela";
	case L'¥': return L"jeeni";
	case L'¤': return L"raha";
	case L'¢': return L"senti";
	case L'%': return L"protsenti";
	case L'&': return L"änd";
	case L'/': return L"kaldkriips";
	case L'|': return L"püstkriips";
	case L'\\': return L"tagurpidi kaldkriips";
	case L'=': return L"võrdub";
	case L'+': return L"pluss";
	case L'~': return L"tilde";
	case L'`': return L"graavis";
	case L'^': return L"katus";
	case L'§': return L"paragrahv";
	case L'©': return L"kopirait";
	case L'°': return L"kraadi";
	case L'±': return L"pluss miinus";
	case L'¼': return L"veerand";
	case L'½': return L"pool";
	case L'¾': return L"kolmveerand";

	case L'(': return L"avav sulg";
	case L'[': return L"avav nurksulg";
	case L'{': return L"avav loogsulg";
	case L')': return L"lõpetav sulg";
	case L']': return L"lõpetav nurksulg";
	case L'}': return L"lõpetav loogsulg";
	case L'<': return L"väiksem kui";
	case L'>': return L"suurem kui";

	case L'\'':
	case L'\x2018':
	case L'\x2019':
	case L'\x201a':
	case L'\x2039':
	case L'\x203a':
		return L"ülakoma";
	case L'"':
	case L'\x201c':
	case L'\x201d':
	case L'\x201e':
	case L'\x00ab':
	case L'\x00bb':
		return L"jutumärk";

	case L'α': return L"alfa";
	case L'β': return L"beta";
	case L'γ': return L"gamma";
	case L'δ': return L"delta";
	}

	return L"";
}

static CFSWString GetSymbolText(const CFSWString &szText)
{
	if (szText.GetLength() == 1) return GetCharacterText(szText[0]);
	return L"";
}

///////////////////////////////////////////////////////////

CFSArray<CMorphInfo> CSpeechLinguistic::Analyze(const CFSWString &szWord)
{
	try {
		if (!m_pMorph) {
			throw CLinguisticException(CLinguisticException::MAINDICT);
		}
		CFSArray<CMorphInfo> Results;
		if (szWord.IsEmpty()) {
			return Results;
		}

		m_pMorph->Clr();
		m_pMorph->SetMaxTasand();
		// Flags are overriden
		MRF_FLAGS_BASE_TYPE Flags = MF_MRF | MF_POOLITA | MF_KR6NKSA | MF_PIKADVALED | MF_LYHREZH | MF_VEEBIAADRESS;
		m_pMorph->SetFlags(Flags);

		m_pMorph->Set1(szWord);
		LYLI Lyli;
		if (!m_pMorph->Flush(Lyli)) return Results;
		Lyli.ptr.pMrfAnal->StrctKomadLahku();
		for (int i = 0; i<Lyli.ptr.pMrfAnal->idxLast; i++) {
			CMorphInfo MorphInfo1;
			MRFTULtoMorphInfo(MorphInfo1, *(*Lyli.ptr.pMrfAnal)[i]);
			Results.AddItem(MorphInfo1);
		}

		return Results;
	}
	catch (const VEAD&) {
		throw CLinguisticException(CLinguisticException::MAINDICT, CLinguisticException::UNDEFINED);
	}
}

///////////////////////////////////////////////////////////

CSpeechEngine::CSpeechEngine()
{
	HTS_Engine_initialize(&m_HTS);
	m_fVoiceSpeed = 1.0;
	// HTS_Engine_set_audio_buff_size(&m_HTS, 1024 * 1024)
}

CSpeechEngine::~CSpeechEngine()
{
	HTS_Engine_clear(&m_HTS);
}

void CSpeechEngine::OpenVoice(const CFSFileName &szVoice, const CVoiceSettings &Settings)
{
	CFSAutoLock AutoLock(&m_HTSMutex);

	CFSAString szaVoice = FSStrTtoA(szVoice);
	char *pVoice = (char *)(const char *)szaVoice;
	if (!HTS_Engine_load(&m_HTS, &pVoice, 1)) {
		throw CFSFileException(CFSFileException::OPEN);
	}

	// Fixed settings
	HTS_Engine_set_sampling_frequency(&m_HTS, (size_t)48000);
	HTS_Engine_set_phoneme_alignment_flag(&m_HTS, FALSE);
	HTS_Engine_set_msd_threshold(&m_HTS, 1, 0.5);
	HTS_Engine_set_fperiod(&m_HTS, (size_t)240);
	HTS_Engine_set_alpha(&m_HTS, 0.55);
	HTS_Engine_set_beta(&m_HTS, 0.0);

	// Overridable settings
	m_fVoiceSpeed = Settings.speed;
	HTS_Engine_set_speed(&m_HTS, m_fVoiceSpeed);
	HTS_Engine_add_half_tone(&m_HTS, Settings.ht);
	HTS_Engine_set_gv_weight(&m_HTS, 0, Settings.gvw1);
	HTS_Engine_set_gv_weight(&m_HTS, 1, Settings.gvw2);
}

void CSpeechEngine::AddAlternativeWords(CPhrase &Phrase, CFSWString szText, CFragment *pFragment)
{
	CFSWStringArray Alternatives;
	FSStrSplit(szText, L' ', Alternatives);
	for (INTPTR ipAlt = 0; ipAlt < Alternatives.GetSize(); ipAlt++) {
		CFSArray<CMorphInfo> Altmi = Analyze(Alternatives[ipAlt]);
		CWord Word;
		if (Altmi.GetSize() > 0) {
			bool bFound = false;
			if (pFragment->m_Morph.m_MorphInfo.GetSize() > 0) {
				CMorphInfo Origmi = pFragment->m_Morph.m_MorphInfo[0];
				for (INTPTR ip = 0; ip < Altmi.GetSize(); ip++) {
					if (Altmi[ip].m_cPOS == Origmi.m_cPOS) {
						Word.mi = Altmi[ip];
						bFound = true;
						break;
					}
				}
			}
			if (!bFound) Word.mi = Altmi[0];
		}
		else Word.mi.m_szRoot = Alternatives[ipAlt];
		Word.ref = pFragment;
		Phrase.word_vector.AddItem(Word);
	}
}

#define NEW_PHRASE \
	if (Phrase.word_vector.GetSize() > 0) { \
		Utterance.phr_vector.AddItem(Phrase); \
		Phrase = CPhrase(); \
	}

void CSpeechEngine::CreateLabels(CFSClassArray<CFragment> &Sentence, CFSAStringArray &Labels, CFSArray<CAudioFragment> &AudioFragments)
{
	// Filter out speak actions
	CFSArray<CFragment *> SpeakFragments;
	for (INTPTR ipFrag = 0; ipFrag < Sentence.GetSize(); ipFrag++) {
		CFragment &Fragment = Sentence[ipFrag];
		if (Fragment.m_eType == CFragment::TYPE_SPACE && !SpeakFragments.GetSize()) continue;
		if (!Fragment.IsSpeakAction()) continue;
		SpeakFragments.AddItem(&Fragment);
	}
	while (SpeakFragments.GetSize() && SpeakFragments[SpeakFragments.GetSize() - 1]->m_eType == CFragment::TYPE_SPACE) {
		SpeakFragments.RemoveItem(SpeakFragments.GetSize() - 1);
	}
	if (!SpeakFragments.GetSize()) return;

	CUtterance Utterance;
	CPhrase Phrase;
	CFragment Empty;

	for (INTPTR ipFrag = 0; ipFrag < SpeakFragments.GetSize(); ipFrag++) {
		// Split phrases
		CFragment *pFragment = SpeakFragments[ipFrag];
		const CFragment *pPrevFragment = (ipFrag > 0 ? SpeakFragments[ipFrag - 1] : &Empty);
		const CFragment *pNextFragment = (ipFrag < SpeakFragments.GetSize() - 1 ? SpeakFragments[ipFrag + 1] : &Empty);

		bool bSkip = false;
		if (pFragment->m_eType == CFragment::TYPE_PUNCTUATION &&
			pFragment->m_szText == L":" &&
			pNextFragment->m_eType == CFragment::TYPE_SPACE)
		{
			NEW_PHRASE;
			bSkip = true;
		}
		else if (pFragment->m_eType == CFragment::TYPE_PUNCTUATION &&
			pFragment->m_szText.GetLength() == 1 &&
			pFragment->m_szText.FindOneOf(L",;?!") >= 0 &&
			(pNextFragment->m_eType == CFragment::TYPE_SPACE ||
				pNextFragment->m_eType == CFragment::TYPE_PUNCTUATION &&
				pNextFragment->m_szText.GetLength() == 1 &&
				pNextFragment->m_szText.FindOneOf(L"\"\x201c\x201d\x201f\x00bb\x0022") >= 0 &&
				ipFrag + 2 < SpeakFragments.GetSize() &&
				SpeakFragments[ipFrag + 2]->m_eType == CFragment::TYPE_SPACE)
			)
		{
			NEW_PHRASE;
			bSkip = true;
		}
		else if (pFragment->m_eType == CFragment::TYPE_WORD &&
			pPrevFragment->m_eType == CFragment::TYPE_SPACE &&
			pNextFragment->m_eType == CFragment::TYPE_SPACE &&
			htssyn::is_conju(pFragment->m_szText))
		{
			NEW_PHRASE;
		}
		else if (pFragment->m_eType == CFragment::TYPE_PUNCTUATION &&
			pFragment->m_szText.FindOneOf(L"([{") >= 0)
		{
			NEW_PHRASE;
			if (pFragment->m_eAction == SPVA_Speak) {
				AddAlternativeWords(Phrase, L"sulgudes", pFragment);
				NEW_PHRASE;
			}
			bSkip = true;
		}
		else if (pFragment->m_eType == CFragment::TYPE_PUNCTUATION &&
			pFragment->m_szText.FindOneOf(L")]}") >= 0)
		{
			NEW_PHRASE;
			bSkip = true;
		}
		else if (pFragment->m_eType == CFragment::TYPE_PUNCTUATION &&
			pFragment->m_szText.GetLength() == 1 &&
			pFragment->m_szText.FindOneOf(L"-\x2013\x2014\x2015") >= 0 &&
			(pPrevFragment->m_eType == CFragment::TYPE_WORD && pNextFragment->m_eType == CFragment::TYPE_SPACE ||
				pPrevFragment->m_eType == CFragment::TYPE_SPACE && pNextFragment->m_eType == CFragment::TYPE_WORD))
		{
			NEW_PHRASE;
			bSkip = true;
		}

		// Split words
		if (pFragment->m_eType == CFragment::TYPE_WORD ||
			pFragment->m_eType == CFragment::TYPE_SYMBOL ||
			pFragment->m_eType == CFragment::TYPE_PUNCTUATION)
		{
			if (pFragment->m_eAction == SPVA_Speak && !bSkip) {
				if (pFragment->m_eType == CFragment::TYPE_PUNCTUATION) {
					CFSWString zsText = GetPunctuationText(pFragment->m_szText, pNextFragment->m_eType);
					if (zsText.GetLength()) AddAlternativeWords(Phrase, zsText, pFragment);
				}
				else if (pFragment->m_eType == CFragment::TYPE_SYMBOL) {
					CFSWString zsText = GetSymbolText(pFragment->m_szText);
					if (zsText.GetLength()) AddAlternativeWords(Phrase, zsText, pFragment);
				}
				else {
					CFSWString Abbrevation = htssyn::replace_abbreviation(pFragment->m_szText);
					if (Abbrevation.GetLength()) AddAlternativeWords(Phrase, Abbrevation, pFragment);
					else if (IsWord(pFragment->m_szText)) {
						CWord Word;
						if (pFragment->m_Morph.m_MorphInfo.GetSize() > 0) Word.mi = pFragment->m_Morph.m_MorphInfo[0];
						else Word.mi.m_szRoot = pFragment->m_Morph.m_szWord;
						Word.ref = pFragment;
						Phrase.word_vector.AddItem(Word);
					}
					else {
						CFSWString szSubStr;
						CT CharType = CT_NONE;
						for (INTPTR ipChar = 0; ipChar <= pFragment->m_szText.GetLength(); ipChar++) {
							wchar_t ch = pFragment->m_szText[ipChar];
							CT CharType1 = GetCharType(ch);
							if (CharType1 == CT_SPACE) CharType1 = CharType;
							if (CharType1 != CharType && szSubStr.GetLength()) {
								if (CharType == CT_NUMBER) {
									CFSWString szNumber = GetNumberText(szSubStr, (szSubStr.GetLength() == pFragment->m_szText.GetLength() ? 9 : 3));
									if (szNumber.GetLength()) AddAlternativeWords(Phrase, szNumber, pFragment);
								}
								else if (CharType == CT_LETTER) {
									if (IsWord(szSubStr)) {
										AddAlternativeWords(Phrase, szSubStr, pFragment);
									}
									else {
										for (INTPTR ipChar = 0; ipChar < szSubStr.GetLength(); ipChar++) {
											CFSWString szSpelling = GetCharacterText(szSubStr[ipChar]);
											if (szSpelling.GetLength()) AddAlternativeWords(Phrase, szSpelling, pFragment);
										}
									}
								}
								szSubStr.Empty();
							}
							CharType = CharType1;
							szSubStr += ch;
							if (CharType == CT_OTHER) {
								CFSWString Alternative = GetCharacterText(ch);
								if (Alternative.GetLength()) AddAlternativeWords(Phrase, Alternative, pFragment);
								szSubStr.Empty();
							}
						}
					}
				}
			}
			else if (pFragment->m_eAction == SPVA_SpellOut) {
				for (INTPTR ipChar = 0; ipChar < pFragment->m_szText.GetLength(); ipChar++) {
					CFSWString szSpelling = GetCharacterText(pFragment->m_szText[ipChar]);
					if (szSpelling.GetLength()) AddAlternativeWords(Phrase, szSpelling, pFragment);
				}
			}
		}
	}
	NEW_PHRASE;

	Utterance.Process();
	Utterance.CreateLabels(Labels);

	// Guesstimate word borders
	INTPTR ipSpread = 30; // Starting silence
	for (INTPTR ipFrag = 0; ipFrag < Utterance.phr_vector.GetSize(); ipFrag++) {
		const CPhrase &Phrase = Utterance.phr_vector[ipFrag];

		for (INTPTR ipWord = 0; ipWord < Phrase.word_vector.GetSize(); ipWord++) {
			const CWord &Word = Phrase.word_vector[ipWord];
			INTPTR ipWeight = Word.GetWeight();
			if (Word.ref) {
				if (ipSpread > 0) {
					if (AudioFragments.GetSize() > 0) {
						AudioFragments[AudioFragments.GetSize() - 1].m_ipWeight += ipSpread / 2;
						ipSpread -= ipSpread / 2;
					}
					ipWeight += ipSpread;
					ipSpread = 0;
				}
				if (AudioFragments.GetSize() > 0 && AudioFragments[AudioFragments.GetSize() - 1].m_pFragment == Word.ref) {
					AudioFragments[AudioFragments.GetSize() - 1].m_ipWeight += ipWeight;
				}
				else {
					AudioFragments.AddItem(CAudioFragment(ipWeight, (CFragment *)Word.ref));
				}
			}
			else {
				ipSpread += ipWeight;
			}
		}
	}
	ipSpread += 30; // Ending silence
	if (AudioFragments.GetSize() > 0) {
		AudioFragments[AudioFragments.GetSize() - 1].m_ipWeight += ipSpread;
	}
}

void CSpeechEngine::CreateAudio(CFSClassArray<CFragment> &Sentence, double fVolume, double fSpeed)
{
	CFSArray<CAudioFragment> AudioFragments;
	CFSAStringArray Labels;

	// Calculate weights and create HTS labels
	CreateLabels(Sentence, Labels, AudioFragments);
	if (!Labels.GetSize()) return;

	// Synthesize audio
	char **pLabelArray = new char*[Labels.GetSize()];
	for (INTPTR ip = 0; ip < Labels.GetSize(); ip++) {
		TRACE(FSTSTR("%s\n"), (const TCHAR *)FSStrAtoT(Labels[ip]));
		pLabelArray[ip] = (char *)(const char *)Labels[ip];
	}

	CFSData Audio;
	short *pAudioBuf = NULL;
	INTPTR ipSamples = 0;
	{
		CFSAutoLock AutoLock(&m_HTSMutex);

		HTS_Engine_set_volume(&m_HTS, fVolume);
		HTS_Engine_set_speed(&m_HTS, m_fVoiceSpeed * fSpeed);
		HTS_Boolean bResult = HTS_Engine_synthesize_from_strings(&m_HTS, pLabelArray, Labels.GetSize());
		delete[] pLabelArray;
		if (!bResult) throw CFSException();

		ipSamples = HTS_Engine_get_nsamples(&m_HTS);
		Audio.SetSize(ipSamples * sizeof(short));
		pAudioBuf = (short *)Audio.GetData();
		for (INTPTR ip = 0; ip < ipSamples; ip++) {
			double fValue = HTS_Engine_get_generated_speech(&m_HTS, ip);
			short sValue;
			if (fValue >= 32767.0) sValue = 32767;
			else if (fValue <= -32768.0) sValue = -32768;
			else sValue = (short)round(fValue);
			pAudioBuf[ip] = sValue;
		}

		 HTS_Engine_refresh(&m_HTS);
	}

	// Split autio to fragments
	INTPTR ipTotalWeight = 0;
	for (INTPTR ip = 0; ip < AudioFragments.GetSize(); ip++) {
		ipTotalWeight += AudioFragments[ip].m_ipWeight;
	}

	INTPTR ipWeightOffset = 0;
	for (INTPTR ip = 0; ip < AudioFragments.GetSize(); ip++) {
		INTPTR ipStartSample = ((INT64)ipWeightOffset) * ipSamples / ipTotalWeight;
		INTPTR ipEndSample = ((INT64)(ipWeightOffset + AudioFragments[ip].m_ipWeight)) * ipSamples / ipTotalWeight;
		INTPTR ipSampleCount = ipEndSample - ipStartSample;
		if (ipSampleCount <= 0) continue;

		CFSData &FragmentAudio = AudioFragments[ip].m_pFragment->m_Audio;
		FragmentAudio.SetSize(ipSampleCount * sizeof(short));
		memcpy(FragmentAudio.GetData(), pAudioBuf + ipStartSample, ipSampleCount * sizeof(short));

		// Adjust fragment volume
		int iVolume = AudioFragments[ip].m_pFragment->m_iVolume;
		if (iVolume < 100) {
			short *pSamples = (short *)FragmentAudio.GetData();
			for (INTPTR ipSample = 0; ipSample < ipSampleCount; ipSample++) {
				pSamples[ipSample] = (short)(iVolume * pSamples[ipSample] / 100);
			}
		}

		ipWeightOffset += AudioFragments[ip].m_ipWeight;
	}
}

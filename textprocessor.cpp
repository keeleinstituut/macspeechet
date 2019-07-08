#include "stdafx.h"
#include "textprocessor.h"

static bool ParseNumber(CFSWString szText, INTPTR &ipDigits) {
	ipDigits = 0;
	for (INTPTR ip = 0; ip < szText.GetLength(); ip++) {
		if (!FSIsNumber(szText[ip])) return false;
	}
	ipDigits = szText.GetLength();
	return true;
}

static bool TPIsWordChar(wchar_t Char) {
	static const wchar_t *s_Additional = L"%\x2030\x2031\x00b0\x00ad";
	// permil, pertho; degree; opthyph; 
	return FSIsLetter(Char) || FSIsNumber(Char) || FSStrChr(s_Additional, Char) != 0;
}

static bool TPIsWordPunctuation(const CFSWString &szPunctuation) {
	return (szPunctuation.GetLength() == 1 && FSStrChr(
		L"-\x2013\x2014\x2015" // Dashes
		L".()/"
		L"'\x2019" // apostrophes
		, szPunctuation[0]) != 0);
}

static bool TPIsPunctuation(wchar_t Char) {
	return (FSStrChr(
		L".,;:!?-\x2013\x2014\x2015"
		L"(){}[]"
		L"'\x2018\x2019\x201a\x2039\x203a" // Single quotes
		L"\"\x201c\x201d\x201e\x00ab\x00bb" // Double quotes
		, Char) != 0);
}

static bool TPIsSymbol(wchar_t Char) {
	return (FSStrChr(
		L"$€£¥¤¢"
		L"*+/|\\=_<>"
		L"~`@#^&§©°±¼½¾"
		, Char) != 0);
}

static inline bool TPIsTrue(wchar_t /*Char*/) {
	return true;
}

#define TESTFOR(TestFunction, TestType) \
else if (TestFunction(szText[ip])) { \
	if (Fragment.m_eType == TestType) { \
		Fragment.m_szText += szText[ip]; \
	} else { \
		if (Fragment.m_szText.GetLength()) Fragments.AddItem(Fragment); \
		Fragment = CFragment(ip, szText.Mid(ip, 1), TestType); \
	} \
}

CFSClassArray<CFragment> CTextProcessor::SplitText(CFSWString szText) const
{
	CFSClassArray<CFragment> Fragments;
	CFragment Fragment;

	for (INTPTR ip = 0; ip < szText.GetLength(); ip++) {
		if (0) {}
		TESTFOR(TPIsWordChar, CFragment::TYPE_WORD)
		TESTFOR(FSIsSpace, CFragment::TYPE_SPACE)
		TESTFOR(TPIsPunctuation, CFragment::TYPE_PUNCTUATION)
		TESTFOR(TPIsSymbol, CFragment::TYPE_SYMBOL)
		TESTFOR(TPIsTrue, CFragment::TYPE_UNKNOWN)
	}

	if (Fragment.m_szText.GetLength()) Fragments.AddItem(Fragment);
	CombineFragments(Fragments);
	return Fragments;
}

void CTextProcessor::CombineFragments(CFSClassArray<CFragment> &Fragments) const
{
	// Separate punctuation
	for (INTPTR ip = 0; ip < Fragments.GetSize(); ip++) {
		if (Fragments[ip].m_eType == CFragment::TYPE_PUNCTUATION) {
			for (INTPTR ip2 = 1; ip2 < Fragments[ip].m_szText.GetLength(); ip2++) {
				bool bSplit = false;
				if (FSStrChr(L"!?", Fragments[ip].m_szText[ip2])) {
					bSplit = (FSStrChr(L"!?", Fragments[ip].m_szText[ip2 - 1]) == 0);
				}
				else if (FSStrChr(L".-\x2013\x2014\x2015", Fragments[ip].m_szText[ip2])) {
					bSplit = (Fragments[ip].m_szText[ip2] != Fragments[ip].m_szText[ip2 - 1]);
				}
				else {
					bSplit = true;
				}
				if (bSplit) {
					Fragments.InsertItem(ip + 1, CFragment(Fragments[ip].m_ipStartPos + ip2, Fragments[ip].m_szText.Mid(ip2), CFragment::TYPE_PUNCTUATION));
					Fragments[ip].m_szText = Fragments[ip].m_szText.Left(ip2);
				}
			}
		}
	}

	// Split all symbols
	for (INTPTR ip = 0; ip < Fragments.GetSize(); ip++) {
		if (Fragments[ip].m_eType == CFragment::TYPE_SYMBOL && Fragments[ip].m_szText.GetLength() > 1) {
			Fragments.InsertItem(ip + 1, CFragment(Fragments[ip].m_ipStartPos + 1, Fragments[ip].m_szText.Mid(1), CFragment::TYPE_SYMBOL));
			Fragments[ip].m_szText = Fragments[ip].m_szText.Left(1);
		}
	}

	// Combine numbers
	for (INTPTR ip = 0; ip < Fragments.GetSize() - 2; ip++) {
		INTPTR ipLen = 0;
		if (ParseNumber(Fragments[ip].m_szText, ipLen)) {
			bool bCombine = (ipLen <= 3);
			INTPTR ipNumEnd = ip;
			for (INTPTR ip2 = ip + 2; ip2 < Fragments.GetSize(); ip2 += 2) {
				if (Fragments[ip2 - 1].m_eType == CFragment::TYPE_SPACE && Fragments[ip2 - 1].m_szText == L" " &&
					ParseNumber(Fragments[ip2].m_szText, ipLen)) {
					ipNumEnd = ip2;
					if (ipLen != 3) bCombine = false;
				}
				else break;
			}
			if (ipNumEnd > ip) {
				if (bCombine) {
					while (ipNumEnd > ip) {
						Fragments[ip].m_szText += Fragments[ip + 1].m_szText;
						Fragments.RemoveItem(ip + 1);
						ipNumEnd--;
					}
				}
				else {
					ip = ipNumEnd;
				}
			}
		}
	}

	// TODO: maybe?
	// Combine URLs
}

CFSWString CTextProcessor::GetCleanWord(const CFSWString &szText) const
{
	CFSWString szResult = szText;
	szResult.Remove(FSWSTR('\x001F')); // Old soft hyph???
	szResult.Remove(FSWSTR('\x00AD')); // Soft hyphen
	return szResult;
}

CFSClassArray<CFSClassArray<CFragment> > CTextProcessor::CreateSentences(CFSClassArray<CFragment> &Fragments) const
{
	static const wchar_t *s_StartOfSentencePunctuation =
		L"({["
		L"\x2018\x2019\x201a\x201b\x2039\x0027" // Single quotes L
		L"\x201c\x201d\x201e\x201f\x00ab\x0022"; // Double quotes L
	static const wchar_t *s_EndOfSentencePunctuation =
		L")}]"
		L"\x2018\x2019\x201b\x203a\x0027" // Single quotes R
		L"\x201c\x201d\x201f\x00bb\x0022"; // Double quotes R
	static const wchar_t *s_EndOfSentencePunctuationMust =
		L".!?";

	CFSClassArray<CFSClassArray<CFragment> > Sentences;

AGAIN:
	for (INTPTR ip = 1; ip < Fragments.GetSize(); ip++) {
		INTPTR ipSentence = ip;

		const CFragment &Fragment = Fragments[ipSentence];
		if (!Fragment.IsSpeakAction()) continue;
		if (Fragment.m_eType != CFragment::TYPE_WORD) continue;


		// Check capitalization
		INTPTR ipFirstLetter = -1;
		for (INTPTR ip2 = 0; ip2<Fragment.m_szText.GetLength(); ip2++) {
			if (TPIsWordChar(Fragment.m_szText[ip2])) {
				ipFirstLetter = ip2;
				break;
			}
		}

		if (ipFirstLetter == -1) continue; // Should never happen
		if (FSToLower(Fragment.m_szText[ipFirstLetter]) == Fragment.m_szText[ipFirstLetter]) continue;

		bool bExplicitCapital = true;
		for (INTPTR ip2 = 0; bExplicitCapital && ip2 < Fragment.m_Morph.m_MorphInfo.GetSize(); ip2++) {
			bExplicitCapital = (FSToUpper(Fragment.m_Morph.m_MorphInfo[ip2].m_szRoot[0]) != Fragment.m_Morph.m_MorphInfo[ip2].m_szRoot[0]);
		}

		// Optional sentence start punctuation
		INTPTR ipTry;
		while ((ipTry = FindSpeakActionFragment(Fragments, ipSentence, -1)) >= 0 && Fragments[ipTry].m_eType == CFragment::TYPE_PUNCTUATION) {
			if (Fragments[ipTry].m_szText.GetLength() == 1 && FSStrChr(s_StartOfSentencePunctuation, Fragments[ipTry].m_szText[0])) {
				ipSentence = ipTry;
			}
			else {
				break;
			}
		}

		// Whitespace
		INTPTR ipWhiteSpace = -1;
		ipTry = ipSentence;
		while ((ipTry = FindSpeakActionFragment(Fragments, ipTry, -1)) >= 0 && Fragments[ipTry].m_eType == CFragment::TYPE_SPACE) {
			ipWhiteSpace = ipTry;
		}
		if (ipWhiteSpace == -1) continue;

		// Last sentence end punctuation
		INTPTR ipPunctuation = -1;
		ipTry = ipWhiteSpace;
		bool bEndPunctuation = true;
		bool bEndPunctuationMust = false;

		while ((ipTry = FindSpeakActionFragment(Fragments, ipTry, -1)) >= 0 && Fragments[ipTry].m_eType == CFragment::TYPE_PUNCTUATION) {
			ipPunctuation = ipTry;
			for (INTPTR ip2 = 0; bEndPunctuation && ip2 < Fragments[ipPunctuation].m_szText.GetLength(); ip2++) {
				if (FSStrChr(s_EndOfSentencePunctuationMust, Fragments[ipPunctuation].m_szText[ip2])) {
					bEndPunctuationMust = true;
				}
				else {
					bEndPunctuation = (FSStrChr(s_EndOfSentencePunctuation, Fragments[ipPunctuation].m_szText[ip2]) != 0);
				}
			}
		}
		if (!bEndPunctuationMust || !bEndPunctuation) continue;

		// Last word
		INTPTR ipLastWord = FindSpeakActionFragment(Fragments, ipPunctuation, -1);
		if (ipLastWord == -1) continue;
		if (Fragments[ipLastWord].m_eType != CFragment::TYPE_WORD) continue;
		bool bExplicitPunctuation = true;
		CMorphInfo InfoNumber; InfoNumber.m_cPOS = 'N'; // Number
		CMorphInfo InfoAbbr; InfoAbbr.m_cPOS = 'Y'; // Abbrevation
		if (Match(Fragments[ipLastWord], InfoNumber) != MATCH_NO || Match(Fragments[ipLastWord], InfoAbbr) != MATCH_NO) {
			bExplicitPunctuation = false;
		}

		if (!(bExplicitCapital || bExplicitPunctuation)) continue;

		// Found it...
		CFSClassArray<CFragment> Sentence;
		INTPTR ipLastInclude = FindSpeakActionFragment(Fragments, ipSentence, -1); // lease breakpoint etc to the next sentence
		for (INTPTR ip2 = 0; ip2 <= ipLastInclude; ip2++) {
			Sentence.AddItem(Fragments[0]);
			Fragments.RemoveItem(0);
		}
		Sentences.AddItem(Sentence);
		goto AGAIN;
	}

	if (Fragments.GetSize()) {
		Sentences.AddItem(Fragments);
	}

	//for (INTPTR ip = 0; ip < Sentences.GetSize(); ip++) {
	//	TRACE(FSTSTR("--- %d\n"), (int)ip);
	//	for (INTPTR ip2 = 0; ip2 < Sentences[ip].GetSize(); ip2++) {
	//		TRACE(FSTSTR(" %s - %d\n"), (const TCHAR *)FSStrWtoT(Sentences[ip][ip2].m_szText), (int)Sentences[ip][ip2].m_eType);
	//	}
	//}

	return Sentences;
}

INTPTR CTextProcessor::FindSpeakActionFragment(const CFSClassArray<CFragment> &Fragments, INTPTR ipPos, INTPTR ipDelta) const
{
	if (ipDelta > 0) {
		for (INTPTR ip = ipPos + 1; ip < Fragments.GetSize(); ip++) {
			if (!Fragments[ip].IsSpeakAction()) continue;
			ipDelta--;
			if (ipDelta == 0) return ip;
		}
		return -1;
	}
	else if (ipDelta < 0) {
		for (INTPTR ip = ipPos - 1; ip >= 0; ip--) {
			if (!Fragments[ip].IsSpeakAction()) continue;
			ipDelta++;
			if (ipDelta == 0) return ip;
		}
		return -1;
	}
	else {
		return ipPos;
	}
}

CTextProcessor::MATCHRESULT CTextProcessor::Match(const CFragment &Fragment, const CMorphInfo &MorfInfo) const
{
	bool bMatchOne = false;
	bool bMatchAll = true;
	for (INTPTR ip = 0; ip < Fragment.m_Morph.m_MorphInfo.GetSize(); ip++) {
		bool bMatch = true;
		if (bMatch && MorfInfo.m_cPOS) {
			bMatch = Fragment.m_Morph.m_MorphInfo[ip].m_cPOS == MorfInfo.m_cPOS;
		}
		if (bMatch && !MorfInfo.m_szForm.IsEmpty()) {
			bMatch = Fragment.m_Morph.m_MorphInfo[ip].m_szForm == MorfInfo.m_szForm;
		}
		if (bMatch && !MorfInfo.m_szEnding.IsEmpty()) {
			bMatch = Fragment.m_Morph.m_MorphInfo[ip].m_szEnding == MorfInfo.m_szEnding;
		}
		if (bMatch && !MorfInfo.m_szClitic.IsEmpty()) {
			bMatch = Fragment.m_Morph.m_MorphInfo[ip].m_szClitic == MorfInfo.m_szClitic;
		}
		if (bMatch && !MorfInfo.m_szRoot.IsEmpty()) {
			bMatch = Fragment.m_Morph.m_MorphInfo[ip].m_szRoot == MorfInfo.m_szRoot;
		}
		bMatchOne |= bMatch;
		bMatchAll &= bMatch;
	}
	if (!bMatchOne) {
		return MATCH_NO;
	}
	else if (!bMatchAll) {
		return MATCH_ONE;
	}
	else {
		return MATCH_ALL;
	}
}
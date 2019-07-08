#pragma once

#include "fragment.h"

class CTextProcessor {
public:
	CFSClassArray<CFragment> SplitText(CFSWString szText) const;
	CFSWString GetCleanWord(const CFSWString &szText) const;
	CFSClassArray<CFSClassArray<CFragment> > CreateSentences(CFSClassArray<CFragment> &Fragments) const;

protected:
	enum MATCHRESULT { MATCH_NO, MATCH_ONE, MATCH_ALL } ;

	void CombineFragments(CFSClassArray<CFragment> &Fragments) const;
	INTPTR FindSpeakActionFragment(const CFSClassArray<CFragment> &Fragments, INTPTR ipPos, INTPTR ipDelta) const;
	MATCHRESULT Match(const CFragment &Fragment, const CMorphInfo &MorfInfo) const;
};
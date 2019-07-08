#pragma once

#include "lib/proof/proof.h"

#if defined __APPLE__
enum SPVACTIONS {
	SPVA_None,
	SPVA_Speak,
	SPVA_SpellOut,
	SPVA_Command
};
#endif // __APPLE__

class CFragment {
public:
	enum FRAGMENTTYPE {
		TYPE_UNKNOWN,
		TYPE_WORD,
		TYPE_PUNCTUATION,
		TYPE_SYMBOL,
		TYPE_SPACE,
		TYPE_OTHER
	};

	CFragment() :
		m_eType(TYPE_UNKNOWN),
#if defined __APPLE__
#else
		m_lParam(0),
#endif
//		m_iRate(0),
		m_iVolume(100),
		m_ipStartPos(0)
	{}

	CFragment(INTPTR ipStartPos, const CFSWString &szText, FRAGMENTTYPE eType) :
		m_eType(eType),
#if defined __APPLE__
#else
		m_lParam(0),
#endif
//		m_iRate(0),
		m_iVolume(100),
		m_ipStartPos(ipStartPos),
		m_szText(szText)
	{}

	bool IsSpeakAction() const {
		return (m_eAction == SPVA_Speak || m_eAction == SPVA_SpellOut);
	}

	SPVACTIONS m_eAction;
	FRAGMENTTYPE m_eType;

#if defined __APPLE__
	bool ArgumentAdjustString(CFSWString &szValue, INTPTR ipArgument) const;
	template <class TYPE> bool ArgumentAdjustInt(TYPE &iValue, INTPTR ipArgument) const;
	template <class TYPE> bool ArgumentAdjustFloat(TYPE &fValue, INTPTR ipArgument) const;
	CFSWStringArray m_Arguments;
#else
	LPARAM m_lParam;
#endif
//	int m_iRate;
	int m_iVolume;

	INTPTR m_ipStartPos;
	CFSWString m_szText;
	CMorphInfos m_Morph;

	CFSData m_Audio;
};

#if defined __APPLE__
template <class TYPE>
bool CFragment::ArgumentAdjustInt(TYPE &iValue, INTPTR ipArgument) const
{
	if (ipArgument >= m_Arguments.GetSize()) return false;
	CFSWString szNewValue = m_Arguments[ipArgument];

	wchar_t cRelative = 0;
	TYPE iNewValue = 0;
	szNewValue.MakeLower();
	if (szNewValue[0] == L'+' || szNewValue[0] == L'-') {
		cRelative = szNewValue[0];
		szNewValue = szNewValue.Mid(1);
	}
	int iRadix = 10;
	if (szNewValue.StartsWith(L"0x")) {
		iRadix = 16;
		szNewValue = szNewValue.Mid(2);
	}
	else if (szNewValue.StartsWith(L"$")) {
		iRadix = 16;
		szNewValue = szNewValue.Mid(1);
	}
	iNewValue = (TYPE)wcstol(szNewValue, NULL, iRadix);
	switch (cRelative) {
		case L'+':
			iValue += iNewValue;
			break;
		case L'-':
			iValue -= iNewValue;
			break;
		default:
			iValue = iNewValue;
			break;
	}
	return true;
}

template <class TYPE>
bool CFragment::ArgumentAdjustFloat(TYPE &fValue, INTPTR ipArgument) const
{
	if (ipArgument >= m_Arguments.GetSize()) return false;
	CFSWString szNewValue = m_Arguments[ipArgument];

	wchar_t cRelative = 0;
	TYPE fNewValue = 0;
	if (szNewValue[0] == L'+' || szNewValue[0] == L'-') {
		cRelative = szNewValue[0];
		szNewValue = szNewValue.Mid(1);
	}
	fNewValue = (TYPE)wcstod(szNewValue, NULL);
	switch (cRelative) {
		case L'+':
			fValue += fNewValue;
			break;
		case L'-':
			fValue -= fNewValue;
			break;
		default:
			fValue = fNewValue;
			break;
	}
	return true;
}
#endif // __APPLE__

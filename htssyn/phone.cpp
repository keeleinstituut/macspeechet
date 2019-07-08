#include "htssyn.h"

CFSWString CPhone::gpos(wchar_t c) {
	CFSWString res = L"z";
	if (!c) return res;
	if (::FSStrChr(L"DGIKXY", c)) return L"d";
	if (::FSStrChr(L"ACHNOPSU", c)) return L"a";
	if (::FSStrChr(L"V", c)) return L"v";
	if (::FSStrChr(L"J", c)) return L"j";
	return res;
}

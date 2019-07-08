#include "htssyn.h"

namespace htssyn {

	bool is_lvowel(wchar_t c) {
		return c && FSStrChr(L"aeiouõäöüy", c);
	}

	bool is_uvowel(wchar_t c) {
		return c && FSStrChr(L"AEIOUÕÄÖÜY", c);
	}

	bool is_vowel(wchar_t c) {
		return (is_lvowel(c) || is_uvowel(c));
	}

	bool is_lconsonant(wchar_t c) {
		return c && FSStrChr(L"bcdfghjklmnpqrsšžtvwxz", c);
	}

	bool is_uconsonant(wchar_t c) {
		return c && FSStrChr(L"BCDFGHJKLMNPQRSŠŽTVWXZ", c);
	}

	bool is_consonant(wchar_t c) {
		return (is_lconsonant(c) || is_uconsonant(c));
	}

	bool is_char(wchar_t c) {
		return (is_vowel(c) || is_consonant(c));
	}

	bool can_palat(wchar_t c) {
		return c && FSStrChr(L"DLNST", FSToUpper(c));
	}

	bool is_conju(CFSWString s) {
		return (s == L"ja" || s == L"ning" || s == L"ega" || s == L"ehk" || s == L"või");
	}

}

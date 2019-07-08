#pragma once

namespace htssyn {

	bool is_lvowel(wchar_t c);
	bool is_uvowel(wchar_t c);
	bool is_vowel(wchar_t c);
	bool is_lconsonant(wchar_t c);
	bool is_uconsonant(wchar_t c);
	bool is_consonant(wchar_t c);
	bool is_char(wchar_t c);

	bool can_palat(wchar_t c);
	bool is_conju(CFSWString s);

}

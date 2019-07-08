#include "htssyn.h"

static CFSWString make_char_string(const CFSWString &str) {
	CFSWString result;
	for (INTPTR ip = 0; ip < str.GetLength(); ip++) {
		wchar_t c = htssyn::replace_fchar(str[ip]);
		if (!c) c = str[ip];
		if (htssyn::is_char(c)) result += c;
	}
	return result;
}

static wchar_t shift_pattern(wchar_t c) {
	if (!c) return c;
	if (c == L'j') return L'j';
	if (c == L'h') return L'h';
	if (c == L'v') return L'v';
	if (FSStrChr(L"sS", c)) return L's';
	if (FSStrChr(L"lmnrLN", c)) return L'L';
	if (FSStrChr(L"kptfšT", c)) return L'Q';
	if (htssyn::is_vowel(c)) return L'V';
	if (htssyn::is_consonant(c)) return L'C';
	return c;
}

static INTPTR pattern_lookup(const CFSWString &s) {
	static const INTPTR pattern_array_size = 55;
	static const CFSWString pattern[pattern_array_size] = {
		L"VV#", L"VVQ#", L"VVQQ#", L"VVC#", L"VVss#", L"VVsQ#", L"VVQs#", L"VVLQ#", L"VVLQQ#", L"VVCC#", L"VVCCC#", L"V#", L"VC#", L"VLQ#", L"VLQQ#", L"VLh#", L"VCC#", L"VLss#", L"VLsQ#", L"VLhv#", L"VLQC#", L"VCCC#", L"VCCCC#",
		L"VV-Q", L"VV-", L"VVs-s", L"VVs-Q", L"VVL-Q", L"VVQ-L", L"VVQ-j", L"VVQ-", L"VVQQ-", L"VVC-", L"VVQs-", L"VVsQ-", L"VVLQ-", L"VVLQ-Q", L"VVLQQ-", L"VVCC-", L"VVCCC-", L"VL-Q", L"VL-h", L"VC-", L"VLQ-", L"VLQ-Q", L"VLQ-s", L"VLs-", L"VLh-", L"VCC-", L"VLsQ-", L"VLQC-", L"VCCC-", L"VLQCC-", L"VCCCC-", L"VC-C"
	};
	static const INTPTR shift[pattern_array_size] = { 2, 2, 3, 2, 3, 2, 3, 2, 4, 2, 2, 1, 2, 2, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 5, 2, 2, 3, 3, 2, 3, 3, 2, 4, 4, 2, 2, 2, 4, 2, 2, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 2 };

	INTPTR res = -1;
	for (INTPTR i = 0; i < (pattern_array_size - 1); i++) {
		if (s == pattern[i]) {
			res = shift[i];
		}
	}
	return res;
}

static CFSWString simplify_pattern(const CFSWString &s) {
	CFSWString res;
	for (INTPTR i = 0; i < s.GetLength(); i++) {
		wchar_t c = s[i];
		if (FSStrChr(L"jhvsLQ", c)) res += L"C";
		else res += c;
	}
	return res;
}

static CFSWString the_shift(CFSWString s) {
	/*
	On mingi võimalus, et lihtsustus tuleb teha kahes astmes. LQ-ta ja LQ-ga (vt shift_pattern). Kõik
	seotud sellega, et pole	vältenihutusreeglitest lõpuni aru saanud. Eksisteerib Mihkla versioon ja
	ametlik versioon. Tänud	Mihklale, kes kala asemel annab tattninale õnge, see õpetab ujuma.
	Maadlesin õngega pikalt.
	*/

	CFSWString res;
	CFSWString code;
	INTPTR pos = 0;
	INTPTR i = 0;
	INTPTR x;

	while (s.GetLength() > 0) {
		wchar_t c = s[0];
		s.Delete(0, 1);
		if (htssyn::is_uvowel(c)) {
			c = FSToLower(c);
			code += shift_pattern(c);
			res += c;
			pos = i;
		}
		else
			if (c == '-' && code.GetLength() > 0) {
				res += c;
				code += c;
				CFSWString t_code = code;
				t_code += shift_pattern(s[0]);

				x = pattern_lookup(t_code); //orig üle silbipiiri
				if (x > -1) {
					x += pos;
					if (x > res.GetLength()) { // kui kargab järgmisse silpi
						x = x - res.GetLength();
						s.Insert(x, L":");
					}
					else
						res.Insert(x, L":");
					i++;
				}
				else {
					t_code = simplify_pattern(t_code);
					x = pattern_lookup(t_code); //liht üle silbipiiri
					if (x > -1) {
						x += pos;
						if (x > res.GetLength()) { // kui kargab järgmisse silpi
							x = x - res.GetLength();
							s.Insert(x, L":");
						}
						else
							res.Insert(x, L":");
						i++;
					}
					else {
						x = pattern_lookup(code); //orig 
						if (x > -1) {
							x += pos;
							res.Insert(x, L":");
							i++;
						}
						else {
							code = simplify_pattern(code);
							x = pattern_lookup(code); //liht
							if (x > -1) {
								x += pos;
								res.Insert(x, L":");
								i++;
							}
						}
					}
				}

				code.Empty();
			}
			else {
				res += c;
				if (code.GetLength() > 0) {
					code += shift_pattern(c);
				}
			}
			i++;
	} //while

	  // sõna lõpus
	if (code.GetLength() > 0) {
		code += L"#";
		//imelik koht ainult "lonksu" pärast
		if (code.GetLength() > 3 && code.StartsWith(L"VLQ") && FSStrChr(L"shvj", code[3])) {
			code = L"VLQC#";
		}
		x = pattern_lookup(code);
		if (x > -1) {
			x += pos;
			res.Insert(x, L":");
		}
		else {
			code = simplify_pattern(code);
			x = pattern_lookup(code);
			if (x > -1) {
				x += pos;
				res.Insert(x, L":");
			}
		}
		code.Empty();
	}
	return res;
}

static CFSWString chars_to_phones_part_I(CFSWString s) {
	/*	müüa -> müia siia?
	Kuna vältemärgi nihutamise reeglid on ehitatud selliselt, et kohati kasutatakse
	foneeme ja kohati ei, siis tuleb täht->foneem teisendus teha kahes jaos.
	Palataliseerimine põhineb ideel, et palataliseeritud foneemi ümbruses ei saa
	olla palataliseeruvaid mittepalataliseeritud foneeme :D
	*/
	CFSWString res;
	for (INTPTR i = 0; i < s.GetLength(); i++) {
		wchar_t c = s[i];
		if (c == L']') {
			if (i >= 1 && res.GetLength() >= 1) {
				res[res.GetLength() - 1] = FSToUpper(s[i - 1]);

				//vaatab taha; pole kindel, et kas on vajalik
				if (i >= 2 && res.GetLength() >= 2) {
					wchar_t t = FSToUpper(s[i - 2]);
					if (htssyn::can_palat(t)) {
						res[res.GetLength() - 2] = t;

						if (i >= 3) {
							t = FSToUpper(s[i - 3]);
							if (htssyn::can_palat(t)) {
								res[res.GetLength() - 2] = t; // !!! kas siin peaks olema - 3 ?
							}
						}
					}
				}
			}

			//vaatab ette					
			wchar_t t = FSToUpper(s[i + 1]);
			if (htssyn::can_palat(t)) {
				s[i + 1] = t;
				t = FSToUpper(s[i + 2]);
				if (htssyn::can_palat(t)) {
					s[i + 2] = t;
				}
			}
		}
		else if (c == L'<') {
			s[i + 1] = FSToUpper(s[i + 1]);
		}
		else if (c == L'?') {
		}//Ebanormaalne rõhk. Pärast vaatab, mis teha
		else if (c == L'x') res += L"ks";
		else if (c == L'y') res += L"i";
		else if (c == L'w') res += L"v";
		else if (c == L'z') {
			if (s[i + 1] == L'z') {
				s[i + 1] = L's';
				res += L"t";
			}
			else
				res += L"s";
		}
		else if (c == L'c') {
			if (s[i + 1] == L'e' || s[i + 1] == L'i' || s[i + 1] == L'<') {
				res += L"ts";
			}
			else res += L"k";
		}
		else if (i > 0 && c == L'ü' && htssyn::is_vowel(s[i + 1]) && s[i - 1] == L'ü') res += L"i";
		else if (c == L'q') {
			if (htssyn::is_vowel(s[i + 1])) {
				res += L"k";
				s[i + 1] = L'v';
			}
			else res += L"k";
		}
		else res += c;
	}
	return res;
}

static CFSWString syllabify2(const CFSWString &s) {
	CFSWString res;
	bool vowel_exist = false;
	for (INTPTR i = 0; i < s.GetLength(); i++) {
		if (htssyn::is_vowel(s[i])) vowel_exist = true;
		else if (vowel_exist) { // i > 0
			if (htssyn::is_consonant(s[i]) && htssyn::is_vowel(s[i + 1])) res += '-';
			else if (htssyn::is_vowel(s[i]) && htssyn::is_vowel(s[i - 1]) && s[i] == s[i + 1]) res += '-';
		}
		res += s[i];
	}
	return res;
}

static CFSWString word_to_syls(const CFSWString &word) {
	CFSWString s = chars_to_phones_part_I(word);
	s = make_char_string(s);
	s = syllabify2(s);
	s = the_shift(s);
	return s;
}

// et oleks lihtsam 2sid tagasi lülitada ja saada 1 astmlised rõhud
static INTPTR extra_stress(const CFSArray<CSyl> &sv, INTPTR size) {
	if (size == 1) return 0;
	for (INTPTR i = 1; i < size; i++)
		for (INTPTR i1 = 0; i1 < sv[i].syl.GetLength(); i1++) {
			if (sv[i].syl[i1] == ':') return i;
			if (i1 > 0 && htssyn::is_vowel(sv[i].syl[i1]) && htssyn::is_vowel(sv[i].syl[i1 - 1])) return i;
		}
	return 0;
}

static void add_stress2(CFSArray<CSyl> &sv, INTPTR wp) {
	/* Kõige radikaalsem rõhutus siiani.
	* wp = kui on liitsõna esimene liige siis on seal pearõhk.
	*/
	INTPTR main_stress = 2;
	INTPTR stress = 1;
	INTPTR size = sv.GetSize();
	INTPTR stress_type = extra_stress(sv, size);
	if (stress_type == 0) {
		for (INTPTR i = 0; i < size; i++) {
			if (i % 2 == 0) {
				if ((i == 0) && (wp == 0))
					sv[i].stress = main_stress;
				else
					sv[i].stress = stress;
			}
		}
		if (size > 1) sv[size - 1].stress = 0;
	}
	else {
		if (wp == 0)
			sv[stress_type].stress = main_stress;
		else
			sv[stress_type].stress = stress;

		//esimene pool
		if (stress_type == 1) sv[0].stress = 0;
		else
			for (INTPTR i = stress_type - 1; i >= 0; i--)
				if (i % 2 == 0)
					sv[i].stress = stress;

		if ((stress_type % 2 != 0) && (stress_type > 1))
			sv[stress_type - 1].stress = 0;

		//teine pool

		INTPTR lopp = size - stress_type;

		if (lopp > 3) {
			for (INTPTR i = stress_type + 1; i < size; i++)
				if (i % 2 != 0) sv[i].stress = stress;

			sv[size - 1].stress = 0;
		}
	}
}

///////////////////////////////////////////////////////////

INTPTR CWord::GetWeight() const {
	INTPTR ipRes = 0;
	for (INTPTR ipSyl = 0; ipSyl < syl_vector.GetSize(); ipSyl++) {
		const CSyl &Syl = syl_vector[ipSyl];
		for (INTPTR ipPhone = 0; ipPhone < Syl.phone_vector.GetSize(); ipPhone++) {
			const CPhone &Phone = Syl.phone_vector[ipPhone];
			if (Phone.phone == L"pau") ipRes += 30;
			else ipRes += 10;
		}
	}
	return ipRes;
//	return GetPhoneCount() * 10;
}

void CWord::CreateSyls(const CFSWString &root) {
	CFSWStringArray c_words;
	FSStrSplit(root, L'_', c_words);

	for (INTPTR cw = 0; cw < c_words.GetSize(); cw++) {
		CFSWString s = word_to_syls(c_words[cw]);

		//MINGI MUSTRITE ERROR paindliKkus
		s.Replace(L"K", L"k", 1);
		s.Replace(L"R", L"r", 1);
		s.Replace(L"V", L"v", 1);

		CFSWStringArray temp_arr;
		FSStrSplit(s, L'-', temp_arr);
		CFSArray<CSyl> sv_temp;
		for (INTPTR ip = 0; ip < temp_arr.GetSize(); ip++) {
			CSyl ss;
			ss.syl = temp_arr[ip];
			sv_temp.AddItem(ss);
		}
		add_stress2(sv_temp, cw);
		syl_vector += sv_temp;
	}
}

void CWord::Process() {
	CFSWString root = mi.m_szRoot + make_char_string(mi.m_szEnding) + mi.m_szClitic;
	root.MakeLower();
	// sidesõnad + ei välteta
	if ((CFSWString(mi.m_cPOS) == L"J") || (root == L"<ei")) root.Replace(L"<", L"", CFSWString::REPLACE_ALL);

	CreateSyls(root);

	for (INTPTR ip = 0; ip < syl_vector.GetSize(); ip++) {
		syl_vector[ip].Process();
	}
}

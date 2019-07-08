#pragma once

#include "../lib/proof/proof.h"
#include "mappings.h"
#include "utils.h"

#define COUNT_CHILDREN(Function, Vector) \
	INTPTR Function() const { \
		INTPTR res = 0; \
		for (INTPTR ip = 0; ip < Vector.GetSize(); ip++) { \
			res += Vector[ip].Function(); \
		} \
		return res; \
	}

class CPhone {
public:
	CPhone() {
		syl_p = word_p = phr_p = utt_p = 0;
		p6 = p7 = a1 = a3 = b1 = b3 = c1 = c3 = b4 = b5 = b6 = b7 = d2 = e2 = f2 = e3 = e4 = g1 = g2 = h1 = h2 = i1 = i2 = h3 = h4 = j1 = j2 = j3 = 0;
	}

	static CFSWString gpos(wchar_t c);

	CFSWString phone;
	CFSWString lpos, pos, rpos;
	INTPTR syl_p, word_p, phr_p, utt_p;
	INTPTR p6, p7, a1, a3, b1, b3, c1, c3, b4, b5, b6, b7, d2, e2, f2, e3, e4, g1, g2, h1, h2, i1, i2, h3, h4, j1, j2, j3;
};

class CSyl {
public:
	CSyl() {
		stress = 0;
		word_p = phr_p = 0;
	}

	void Process();

	INTPTR GetPhoneCount() const {
		return phone_vector.GetSize();
	}

	CFSWString syl;
	INTPTR stress;
	CFSArray<CPhone> phone_vector;
	INTPTR word_p, phr_p;
	//INTPTR utt_p;
};

class CWord {
public:
	CWord() {
		weight = 0;
		phr_p = 0;
		ref = NULL;
	}

	void Process();

	INTPTR GetSylCount() const {
		return syl_vector.GetSize();
	}
	COUNT_CHILDREN(GetPhoneCount, syl_vector);

	INTPTR GetWeight() const;

protected:
	void CreateSyls(const CFSWString &root);

public:
	CMorphInfo mi;
	CFSArray<CSyl> syl_vector;
	INTPTR weight;
	void *ref;
	INTPTR phr_p;
	//INTPTR utt_p;
};

class CPhrase {
public:
	void Process();

	INTPTR GetWordCount() const {
		return word_vector.GetSize();
	}
	COUNT_CHILDREN(GetSylCount, word_vector)

	//CFSWString s;
	CFSArray<CWord> word_vector;
	//INTPTR utt_p;
};

class CUtterance {
public:
	void Process();
	void CreateLabels(CFSAStringArray &Labels) const;

	INTPTR GetPhraseCount() const {
		return phr_vector.GetSize();
	}
	COUNT_CHILDREN(GetWordCount, phr_vector);
	COUNT_CHILDREN(GetSylCount, phr_vector);

protected:
	void FillPositions();
	void FillPhones();
	void AddPauses();

public:
	//CFSWString s;
	CFSArray<CPhrase> phr_vector;
};
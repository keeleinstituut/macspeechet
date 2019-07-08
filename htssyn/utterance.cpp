#include "htssyn.h"

void CUtterance::FillPositions() {
	INTPTR pho_utt_pos = 1;
	for (INTPTR i_phr = 0; i_phr < phr_vector.GetSize(); i_phr++) {
		CPhrase &phr = phr_vector[i_phr];

		INTPTR syl_phr_p = 1;
		INTPTR pho_phr_pos = 1;
		for (INTPTR i_word = 0; i_word < phr.word_vector.GetSize(); i_word++) {
			CWord &word = phr.word_vector[i_word];
			word.phr_p = i_word + 1;

			INTPTR pho_word_pos = 1;
			for (INTPTR i_syl = 0; i_syl < word.syl_vector.GetSize(); i_syl++) {
				CSyl &syl = word.syl_vector[i_syl];
				syl.word_p = i_syl + 1;
				syl.phr_p = syl_phr_p++;

				for (INTPTR i_pho = 0; i_pho < syl.phone_vector.GetSize(); i_pho++) {
					CPhone &pho = syl.phone_vector[i_pho];
					pho.syl_p = i_pho + 1;
					pho.word_p = pho_word_pos++;
					pho.phr_p = pho_phr_pos++;
					pho.utt_p = pho_utt_pos++;
				}
			}
		}
	}
}

void CUtterance::FillPhones() {
	for (INTPTR i_phr = 0; i_phr < phr_vector.GetSize(); i_phr++) {
		CPhrase &phr = phr_vector[i_phr];
		const CPhrase *lphr = (i_phr > 0 ? &phr_vector[i_phr - 1] : NULL);
		const CPhrase *rphr = (i_phr < phr_vector.GetSize() - 1 ? &phr_vector[i_phr + 1] : NULL);

		for (INTPTR i_word = 0; i_word < phr.word_vector.GetSize(); i_word++) {
			CWord &word = phr.word_vector[i_word];
			const CWord *lword = (i_word > 0 ? &phr.word_vector[i_word - 1] : NULL);
			const CWord *rword = (i_word < phr.word_vector.GetSize() - 1 ? &phr.word_vector[i_word + 1] : NULL);

			if (!lword && lphr && lphr->word_vector.GetSize() > 0) {
				lword = &lphr->word_vector[lphr->word_vector.GetSize() - 1];
			}
			if (!rword && rphr && rphr->word_vector.GetSize() > 0) {
				rword = &rphr->word_vector[0];
			}

			for (INTPTR i_syl = 0; i_syl < word.syl_vector.GetSize(); i_syl++) {
				CSyl &syl = word.syl_vector[i_syl];
				const CSyl *lsyl = (i_syl > 0 ? &word.syl_vector[i_syl - 1] : NULL);
				const CSyl *rsyl = (i_syl <  word.syl_vector.GetSize() - 1 ? &word.syl_vector[i_syl + 1] : NULL);

				if (!lsyl && lword && lword->syl_vector.GetSize() > 0) {
					lsyl = &lword->syl_vector[lword->syl_vector.GetSize() - 1];
				}
				if (!rsyl && rword && rword->syl_vector.GetSize() > 0) {
					rsyl = &rword->syl_vector[0];
				}

				for (INTPTR i_pho = 0; i_pho < syl.phone_vector.GetSize(); i_pho++) {
					CPhone &pho = syl.phone_vector[i_pho];

					pho.p6 = pho.syl_p;
					pho.p7 = syl.GetPhoneCount() - pho.syl_p + 1;
					if (lsyl) {
						pho.a1 = lsyl->stress;
						pho.a3 = lsyl->GetPhoneCount();
					}
					pho.b1 = syl.stress;
					pho.b3 = syl.GetPhoneCount();
					pho.b4 = syl.word_p;
					pho.b5 = word.GetSylCount() - pho.b4 + 1;
					pho.b6 = syl.phr_p;
					pho.b7 = phr.GetSylCount() - pho.b6 + 1;
					if (rsyl) {
						pho.c1 = rsyl->stress;
						pho.c3 = rsyl->GetPhoneCount();
					}
					pho.d2 = (lword ? lword->GetSylCount() : 0);
					pho.e2 = word.GetSylCount();
					pho.f2 = (rword ? rword->GetSylCount() : 0);
					pho.e3 = word.phr_p;
					pho.e4 = phr.GetWordCount() - pho.e3 + 1;

					pho.g1 = (lphr ? lphr->GetSylCount() : 0);
					pho.g2 = (lphr ? lphr->GetWordCount() : 0);

					pho.h1 = phr.GetSylCount();
					pho.h2 = phr.GetWordCount();

					pho.i1 = (rphr ? rphr->GetSylCount() : 0);
					pho.i2 = (rphr ? rphr->GetWordCount() : 0);

					pho.h3 = i_phr + 1;
					pho.h4 = GetPhraseCount() - pho.h3 + 1;

					pho.j1 = GetSylCount();
					pho.j2 = GetWordCount();
					pho.j3 = GetPhraseCount();

					pho.lpos = (lword ? CPhone::gpos(lword->mi.m_cPOS) : L"0");
					pho.pos = CPhone::gpos(word.mi.m_cPOS);
					pho.rpos = (rword ? CPhone::gpos(rword->mi.m_cPOS) : L"0");
				}
			}
		}
	}
}

void CUtterance::AddPauses() {
	CPhone phs;
	phs.syl_p = -2;
	phs.word_p = -2;
	phs.phr_p = -2;
	phs.utt_p = -2;
	phs.p6 = -1;
	phs.p7 = -1;
	phs.a1 = 0;
	phs.a3 = 0;
	phs.b1 = -1;
	phs.b3 = -1;
	phs.b4 = -1;
	phs.b5 = -1;
	phs.b6 = -1;
	phs.b7 = -1;
	phs.c1 = 0;
	phs.c3 = 0;
	phs.lpos = L"0";
	phs.d2 = 0;
	phs.pos = L"0";
	phs.e2 = -1;
	phs.e3 = -1;
	phs.e4 = -1;
	phs.rpos = L"0";
	phs.f2 = 0;
	phs.g1 = 0;
	phs.g2 = 0;
	phs.h1 = -1;
	phs.h2 = -1;
	phs.h3 = 1;
	phs.h4 = 1;
	phs.i1 = 0;
	phs.i2 = 0;
	phs.j1 = 1;
	phs.j2 = 1;
	phs.j3 = 1;

	CSyl ss;
	ss.word_p = -2;
	ss.phr_p = -2;
	//ss.utt_p = -2;
	ss.stress = 0;

	CWord ws;
	ws.phr_p = -2;
	//ws.utt_p = -2;

	CPhrase ps;
	//ps.utt_p = -2;

	phs.phone = L"pau";
	ss.phone_vector.AddItem(phs);
	ws.syl_vector.AddItem(ss);
	ps.word_vector.AddItem(ws);

	for (INTPTR i = phr_vector.GetSize(); i >= 0; i--) {
		phr_vector.InsertItem(i, ps);
	}
}

void CUtterance::Process() {
	for (INTPTR ip = 0; ip < phr_vector.GetSize(); ip++) {
		phr_vector[ip].Process();
	}
	FillPositions();
	FillPhones();
	AddPauses();
}

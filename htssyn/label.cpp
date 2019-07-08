#include "htssyn.h"

static CFSAString its(INTPTR i) {
	if (i < 0) return "x";
	CFSAString s;
	s.Format("%d", (int)i);
	return s;
}

void CUtterance::CreateLabels(CFSAStringArray &Labels) const {
	CPhone x;
	x.phone = L"x";

	CFSClassArray<CPhone> pa;
	pa.AddItem(x);
	pa.AddItem(x);
	for (INTPTR i_phr = 0; i_phr < phr_vector.GetSize(); i_phr++) {
		const CPhrase &phrase = phr_vector[i_phr];
		for (INTPTR i_word = 0; i_word < phrase.word_vector.GetSize(); i_word++) {
			const CWord &word = phrase.word_vector[i_word];
			for (INTPTR i_syl = 0; i_syl < word.syl_vector.GetSize(); i_syl++) {
				const CSyl syl = word.syl_vector[i_syl];
				for (INTPTR i_pho = 0; i_pho < syl.phone_vector.GetSize(); i_pho++) {
					pa.AddItem(syl.phone_vector[i_pho]);
				}
			}
		}
	}
	pa.AddItem(x);
	pa.AddItem(x);

	static const CFSWString context_signs = L"^-+=@_"; //5
	for (INTPTR i = 2; i < pa.GetSize() - 2; i++) {
		
		// todo: eemalda rida, kui ", nagu" häälefailis korda saab
		if (pa[i - 1].phone == L"pau" && pa[i].phone == L"n" && pa[i + 1].phone == L"a" && pa[i + 2].phone == L"g") pa[i - 1].phone = L"u"; // fraasialguse 'nagu' välistamine
		
		if (pa[i].j1 == 1) { // üksikute ühesilbiliste sõnade prarameetrite hääldusparandused
			pa[i].j1 = 2;
			if (pa[i].phone != L"pau") {
				pa[i].b5 = 2;
				pa[i].b7 = 2;
				pa[i].c3 = 2;
				pa[i].e2 = 2;
				pa[i].h1 = 2;
			}
		}
		
		CFSWString ws =
			pa[i - 2].phone + context_signs[0] +
			pa[i - 1].phone + context_signs[1] +
			pa[i].phone + context_signs[2] +
			pa[i + 1].phone + context_signs[3] +
			pa[i + 2].phone + context_signs[4];
		CFSAString s = FSStrWtoA(ws, FSCP_UTF8);

		s += its(pa[i].p6) + "-" + its(pa[i].p7);
		s += "/A:";
		s += its(pa[i].a1) + "_" + its(pa[i].a3);
		s += "/B:";
		s += its(pa[i].b1) + "-" + its(pa[i].b3);
		s += "@";
		s += its(pa[i].b4) + "-" + its(pa[i].b5);
		s += "&";
		s += its(pa[i].b6) + "-" + its(pa[i].b7);
		s += "/C:";
		s += its(pa[i].c1) + "+" + its(pa[i].c3);
		s += "/D:";
		s += FSStrWtoA(pa[i].lpos, FSCP_UTF8) + "_" + its(pa[i].d2);
		s += "/E:";
		s += FSStrWtoA(pa[i].pos, FSCP_UTF8) + "+" + its(pa[i].e2);
		s += "@";
		s += its(pa[i].e3) + "+" + its(pa[i].e4);
		s += "/F:";
		s += FSStrWtoA(pa[i].rpos, FSCP_UTF8) + "_" + its(pa[i].f2);
		s += "/G:";
		s += its(pa[i].g1) + "_" + its(pa[i].g2);
		s += "/H:";
		s += its(pa[i].h1) + "=" + its(pa[i].h2);
		s += "^";
		s += its(pa[i].h3) + "=" + its(pa[i].h4); //ei lähe küsimusega pärsi kokku, võib olla ka 0
		s += "/I:";
		s += its(pa[i].i1) + "=" + its(pa[i].i2);
		s += "/J:";
		if (pa[i].phone == L"pau") {
			if (i == 2) s += its(pa[i + 1].j1) + "+" + its(pa[i + 1].j2) + "-" + its(pa[i + 1].j3);
			else s += its(pa[i - 1].j1) + "+" + its(pa[i - 1].j2) + "-" + its(pa[i - 1].j3);
		}
		else {
			s += its(pa[i].j1) + "+" + its(pa[i].j2) + "-" + its(pa[i].j3);
		}

		Labels.AddItem(s);
	}

}
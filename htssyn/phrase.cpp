#include "htssyn.h"

void CPhrase::Process() {
	for (INTPTR ip = 0; ip < word_vector.GetSize(); ip++) {
		word_vector[ip].Process();
	}
}

#include "stdafx.h"
#include "fragment.h"

bool CFragment::ArgumentAdjustString(CFSWString &szValue, INTPTR ipArgument) const
{
	if (ipArgument >= m_Arguments.GetSize()) return false;
	CFSWString szNewValue = m_Arguments[ipArgument];

	szValue = szNewValue;
	return true;
}

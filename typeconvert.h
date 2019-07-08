#pragma once

CFNumberRef NewFloat(float fValue);
CFNumberRef NewInt(int iValue);
CFNumberRef NewLong(long lValue);
CFNumberRef NewPtr(void *pValue);
CFStringRef NewString(const CFSWString &szString);

CFSWString GetString(CFStringRef string);

bool GetDictionaryBool(CFDictionaryRef dictionary, CFStringRef key, bool def);
long GetDictionaryLong(CFDictionaryRef dictionary, CFStringRef key, long def);

CFSAString GetResourceFile(CFBundleRef bundle, CFStringRef fileName);

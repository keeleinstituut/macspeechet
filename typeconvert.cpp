#include <CoreFoundation/CoreFoundation.h>
#include "lib/fsc/fsc.h"
#include "typeconvert.h"

// FS to CF

CFNumberRef NewFloat(float fValue)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &fValue);
}

CFNumberRef NewInt(int iValue)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &iValue);
}

CFNumberRef NewLong(long lValue)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &lValue);
}

CFNumberRef NewPtr(void *pValue)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &pValue);
}

CFStringRef NewString(const CFSWString &szString)
{
	CFSData Data;
	Data.SetSize(szString.GetLength() * sizeof(UniChar));
	UniChar *pBuf = (UniChar *)Data.GetData();
	for (INTPTR ip = 0; ip < szString.GetLength(); ip++) {
		pBuf[ip] = (UniChar)szString[ip];
	}
	return CFStringCreateWithCharacters(kCFAllocatorDefault, pBuf, szString.GetLength());
}

// CF to FS

CFSWString GetString(CFStringRef string)
{
	CFSWString szResult;
	if (string) {
		INTPTR ipLength = CFStringGetLength(string);
		const UniChar *pChars = CFStringGetCharactersPtr(string);
		CFSData Data;
		if (!pChars) {
			Data.SetSize(ipLength * sizeof(UniChar));
			UniChar *pBuf = (UniChar *)Data.GetData();
			CFStringGetCharacters(string, CFRangeMake(0, ipLength), pBuf);
			pChars = pBuf;
		}
		for (INTPTR ip = 0; ip < ipLength; ip++) {
			szResult += (wchar_t)(pChars[ip] ? pChars[ip] : 1); // change possible 0-chars to 1
		}
	}
	return szResult;
}

bool GetDictionaryBool(CFDictionaryRef dictionary, CFStringRef key, bool def)
{
	if (!dictionary) return def;
	CFBooleanRef boolRef = (CFBooleanRef)CFDictionaryGetValue(dictionary, key);
	if (boolRef && CFGetTypeID(boolRef) == CFBooleanGetTypeID()) {
		return CFBooleanGetValue(boolRef);
	}
	return def;
}

long GetDictionaryLong(CFDictionaryRef dictionary, CFStringRef key, long def)
{
	if (!dictionary) return def;
	CFNumberRef numberRef = (CFNumberRef)CFDictionaryGetValue(dictionary, key);
	if (numberRef && CFGetTypeID(numberRef) == CFNumberGetTypeID()) {
		long realValue = 0;
		if (CFNumberGetValue(numberRef, kCFNumberLongType, &realValue)) {
			return realValue;
		}
	}
	return def;
}

CFSFileName GetResourceFile(CFBundleRef bundle, CFStringRef fileName)
{
	CFSFileName szResult;
	CFURLRef resourcesUrl = CFBundleCopyResourceURL(bundle, fileName, NULL, NULL);
	if (resourcesUrl) {
		CFStringRef path = CFURLCopyFileSystemPath(resourcesUrl, kCFURLPOSIXPathStyle);
		if (path) {
			szResult = FSStrWtoA(GetString(path), FSCP_UTF8);
			CFRelease(path);
		}
		CFRelease(resourcesUrl);
	}
	return szResult;
}

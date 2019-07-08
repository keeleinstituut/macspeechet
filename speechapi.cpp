#include "stdafx.h"
#include "engine.h"
#include "typeconvert.h"

typedef void * SpeechChannelIdentifier;

#define FUNCTION_HEADER \
CMacEngine *pEngine = (CMacEngine *)ssr; \
if (!pEngine) return noSynthFound; \
try {

#define FUNCTION_FOOTER \
} catch(...) { \
	TRACE("!!! Unexpected exception\n"); \
	return -1; \
}

extern "C" {

	// Try to release all resources that would require this bundle to remain in memory.
	// The SEWillUnloadBundle function is required to be implemented by synthesizers that can be loaded and unloaded on-the-fly
	// from a location outside the standard directories in which synthesizers are found automatically. This function is called
	// prior to the synthesizer's bundle being unloaded, usually as a result of the client calling SpeechSynthesisUnregisterModuleURL.
	//
	// When called, the synthesizer should remove any run loops and threads created by the bundle so that its code can be removed
	// from memory and the executable file closed. If the synthesizer was successful in preparing for unloading, then return 0 (zero);
	// otherwise, return -1.
	long SEWillUnloadBundle()
	{
		TRACE("SEWillUnloadBundle\n");
		return noErr; // | -1
	}

	/* Open channel - called from NewSpeechChannel, passes back in *ssr a unique SpeechChannelIdentifier value of your choosing. */
	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	synthOpenFailed		-241	Could not open another speech synthesizer channel
	long SEOpenSpeechChannel(SpeechChannelIdentifier *ssr)
	{
		TRACE("SEOpenSpeechChannel\n");
		CMacEngine *pEngine = NULL;
		try {
			if (!ssr) return synthOpenFailed;
			pEngine = new CMacEngine();
			pEngine->Init();
			*ssr = (SpeechChannelIdentifier)pEngine;
			TRACE("Created %p\n", pEngine);
			return (pEngine ? noErr : synthOpenFailed);
		} catch (...) {
			delete pEngine;
			return synthOpenFailed;
		}
	}

	/* Set the voice to be used for the channel. Voice type guaranteed to be compatible with above spec */
	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	//	voiceNotFound		-244	Voice resource not found
	long SEUseVoice(SpeechChannelIdentifier ssr, VoiceSpec *voice, CFBundleRef inVoiceSpecBundle)
	{
		TRACE("SEUseVoice(%p)\n", ssr);
		FUNCTION_HEADER;

		long result = pEngine->InitVoice(inVoiceSpecBundle);
		TRACE("SEUseVoice End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

	/* Close channel */
	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SECloseSpeechChannel(SpeechChannelIdentifier ssr)
	{
		TRACE("SECloseSpeechChannel(%p)\n", ssr);
		FUNCTION_HEADER;

		delete pEngine;
		TRACE("SECloseSpeechChannel End 0\n");
		return noErr;

		FUNCTION_FOOTER;
	}

	/* Analogous to corresponding speech synthesis API calls, except for details noted below */


	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter. Your application passed an invalid parameter for dialog options.
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SEStopSpeechAt(SpeechChannelIdentifier ssr, unsigned long whereToStop)
	{
		TRACE("SEStopSpeechAt(%p, %ld)\n", ssr, whereToStop);
		FUNCTION_HEADER;

		long result = pEngine->Stop((int)whereToStop);
		TRACE("SEStopSpeechAt End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter. Your application passed an invalid parameter for dialog options.
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SEPauseSpeechAt(SpeechChannelIdentifier ssr, unsigned long whereToPause)
	{
		TRACE("SEPauseSpeechAt(%p, %ld)\n", whereToPause);
		FUNCTION_HEADER;

		long result = pEngine->Pause((int)whereToPause);
		TRACE("SEPauseSpeechAt End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SEContinueSpeech(SpeechChannelIdentifier ssr)
	{
		TRACE("SEContinueSpeech(%p)\n", ssr);
		FUNCTION_HEADER;

		long result = pEngine->Resume();
		TRACE("SEContinueSpeech End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

	// Must also be able to parse and handle the embedded commands defined in Inside Macintosh: Speech
	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter.
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	//	synthNotReady		-242	Speech synthesizer is still busy speaking
	long SESpeakCFString(SpeechChannelIdentifier ssr, CFStringRef text, CFDictionaryRef options)
	{
		TRACE("SESpeakCFString(%p)\n", ssr);
		FUNCTION_HEADER;

		long result = pEngine->Start(text,
			GetDictionaryBool(options, kSpeechNoSpeechInterrupt, false),
			GetDictionaryBool(options, kSpeechPreflightThenPause, false)
		);
		// kSpeechNoEndingProsody not handled
		TRACE("SESpeakCFString End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter. Your application passed an invalid parameter for dialog options.
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SECopyPhonemesFromText(SpeechChannelIdentifier ssr, CFStringRef text, CFStringRef *phonemes)
	{
		TRACE("SECopyPhonemesFromText(%p)\n", ssr);
		FUNCTION_HEADER;

		// Not implemented
		TRACE("SECopyPhonemesFromText End %ld\n", (long)paramErr);
		return paramErr; //[(MorseSynthesizer *)ssr copyPhonemes:text result:phonemes];

		FUNCTION_FOOTER;
	}

	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter. Your application passed an invalid parameter for dialog options.
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	//	bufTooSmall			-243	Output buffer is too small to hold result
	//	badDictFormat		-246	Pronunciation dictionary format error
	long SEUseSpeechDictionary(SpeechChannelIdentifier ssr, CFDictionaryRef speechDictionary)
	{
		TRACE("SEUseSpeechDictionary(%p)\n", ssr);
		FUNCTION_HEADER;

		// Not implemented
		TRACE("SEUseSpeechDictionary End %ld\n", (long)paramErr);
		return paramErr;

		FUNCTION_FOOTER;
	}

	// Pass back the information for the designated speech channel and selector
	// This routine is required to support the following properties:
	// kSpeechStatusProperty
	// kSpeechErrorsProperty
	// kSpeechInputModeProperty
	// kSpeechCharacterModeProperty
	// kSpeechNumberModeProperty
	// kSpeechRateProperty
	// kSpeechPitchBaseProperty
	// kSpeechPitchModProperty
	// kSpeechVolumeProperty
	// kSpeechSynthesizerInfoProperty
	// kSpeechRecentSyncProperty
	// kSpeechPhonemeSymbolsProperty
	//
	// NOTE: kSpeechCurrentVoiceProperty is automatically handled by the API
	//

	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter. Your application passed an invalid parameter for dialog options.
	//	siUnknownInfoType	-231	Feature not implemented on synthesizer, Unknown type of information
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SECopySpeechProperty(SpeechChannelIdentifier ssr, CFStringRef property, CFTypeRef *object)
	{
//		TRACE("SECopySpeechProperty(%p, %s)\n", ssr, (const char *)FSStrWtoA(GetString(property), FSCP_UTF8));
		FUNCTION_HEADER;

		long result = pEngine->GetProperty(property, object);
//		TRACE("SECopySpeechProperty End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

	// Set the information for the designated speech channel and selector
	// This routine is required to support the following properties:
	// kSpeechCharacterModeProperty
	// kSpeechNumberModeProperty
	// kSpeechRateProperty
	// kSpeechPitchBaseProperty
	// kSpeechPitchModProperty
	// kSpeechVolumeProperty
	// kSpeechCommandDelimiterProperty
	// kSpeechResetProperty
	// kSpeechRefConProperty
	// kSpeechTextDoneCallBack
	// kSpeechSpeechDoneCallBack
	// kSpeechSyncCallBack
	// kSpeechPhonemeCallBack
	// kSpeechErrorCFCallBack
	// kSpeechWordCFCallBack
	// kSpeechOutputToFileURLProperty
	//
	// NOTE: Setting kSpeechCurrentVoiceProperty is automatically converted to a SEUseVoice call.
	//

	// This routine normally returns one of the following values:
	//	noErr				0		No error
	//	paramErr			-50		Invalid value passed in a parameter. Your application passed an invalid parameter for dialog options.
	//	siUnknownInfoType	-231	Feature not implemented on synthesizer, Unknown type of information
	//	noSynthFound		-240	Could not find the specified speech synthesizer
	long SESetSpeechProperty(SpeechChannelIdentifier ssr, CFStringRef property, CFTypeRef object)
	{
		TRACE("SESetSpeechProperty(%p, %s)\n", ssr, (const char *)FSStrWtoA(GetString(property), FSCP_UTF8));
		FUNCTION_HEADER;

		long result = pEngine->SetProperty(property, object);
		TRACE("SESetSpeechProperty End %ld\n", result);
		return result;

		FUNCTION_FOOTER;
	}

}

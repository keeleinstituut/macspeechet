#!/bin/sh

rm -rf /Library/Speech/Synthesizers/Estonian.SpeechSynthesizer
rm -rf /Library/Speech/Voices/Tonu.SpeechVoice
rm -rf /Library/Speech/Voices/Eva.SpeechVoice
cp -R ./DerivedData/Estonian/Build/Products/Debug/Estonian.SpeechSynthesizer /Library/Speech/Synthesizers/
cp -R ./DerivedData/Estonian/Build/Products/Debug/Tonu.SpeechVoice /Library/Speech/Voices/
cp -R ./DerivedData/Estonian/Build/Products/Debug/Eva.SpeechVoice /Library/Speech/Voices/
killall com.apple.speech.speechsynthesisd

echo "OK"

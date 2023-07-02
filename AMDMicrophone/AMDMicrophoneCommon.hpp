//
//  AMDMicrophoneCommon.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 01/07/2023.
//

#ifndef AMDMicrophoneCommon_h
#define AMDMicrophoneCommon_h

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>

#define LOG(fmt, ...) IOLog("%s: " fmt, this->getName(), ##__VA_ARGS__)

#define kAudioSampleRate         48000
#define kAudioNumChannels        2
#define kAudioSampleDepth        24
#define kAudioSampleWidth        32
#define kAudioBufferSampleFrames kAudioSampleRate / 2
#define kAudioSampleBufferSize   (kAudioBufferSampleFrames * kAudioNumChannels * (kAudioSampleDepth / 8))

#endif /* AMDMicrophoneCommon_h */

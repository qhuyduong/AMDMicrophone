//
//  AMDMicrophoneEngine.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#include "AMDMicrophoneEngine.hpp"

#include "AMDMicrophoneCommon.hpp"

#include <IOKit/IOTimerEventSource.h>
#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/audio/IOAudioDevice.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>

#define super IOAudioEngine

#define kAudioSampleRate 48000
#define kAudioNumChannels 2
#define kAudioSampleDepth 24
#define kAudioSampleWidth 32
#define kAudioBufferSampleFrames kAudioSampleRate / 2
#define kAudioSampleBufferSize (kAudioBufferSampleFrames * kAudioNumChannels * (kAudioSampleDepth / 8))

#define kAudioInterruptInterval 10000000 // 10ms = (1000 ms / 100 hz).
#define kAudioInterruptHZ 100

OSDefineMetaClassAndStructors(AMDMicrophoneEngine, IOAudioEngine);

IOAudioStream* AMDMicrophoneEngine::createNewAudioStream(IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize)
{
    IOAudioStream* audioStream;

    audioStream = new IOAudioStream;
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                2,
                kIOAudioStreamSampleFormatLinearPCM,
                kIOAudioStreamNumericRepresentationSignedInt,
                kAudioSampleDepth,
                kAudioSampleWidth,
                kIOAudioStreamAlignmentLowByte,
                kIOAudioStreamByteOrderLittleEndian,
                true,
                0
            };

            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);
            rate.fraction = 0;
            rate.whole = kAudioSampleRate;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            audioStream->setFormat(&format);
        }
    }

    return audioStream;
}

bool AMDMicrophoneEngine::init()
{
    if (!super::init(NULL)) {
        return false;
    }

    return true;
}

void AMDMicrophoneEngine::free()
{
    if (buffer) {
        IOFree(buffer, kAudioSampleBufferSize);
        buffer = NULL;
    }

    super::free();
}

bool AMDMicrophoneEngine::initHardware(IOService* provider)
{
    bool result = false;
    IOAudioSampleRate initialSampleRate;
    IOAudioControl* control;
    IOAudioStream* audioStream;

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDescription("AMD Digital Microphone");

    initialSampleRate.whole = kAudioSampleRate;
    initialSampleRate.fraction = 0;
    setSampleRate(&initialSampleRate);
    // Set the number of sample frames in each buffer
    setNumSampleFramesPerBuffer(kAudioBufferSampleFrames);
    setInputSampleLatency(kAudioSampleRate / kAudioInterruptHZ);

    control = IOAudioLevelControl::createVolumeControl(
        65535,
        0,
        65535,
        0,
        (22 << 16) + (32768),
        kIOAudioControlChannelIDAll,
        kIOAudioControlChannelNameAll,
        0,
        kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }
    addDefaultAudioControl(control);
    control->release();

    control = IOAudioToggleControl::createMuteControl(
        false,
        kIOAudioControlChannelIDAll,
        kIOAudioControlChannelNameAll,
        0,
        kIOAudioControlUsageInput);

    if (!control) {
        goto Done;
    }

    addDefaultAudioControl(control);
    control->release();

    buffer = (SInt16*)IOMalloc(kAudioSampleBufferSize);
    if (!buffer) {
        goto Done;
    }

    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, buffer, kAudioSampleBufferSize);
    if (!audioStream) {
        goto Done;
    }
    addAudioStream(audioStream);
    audioStream->release();

    result = true;
Done:

    return result;
}

void AMDMicrophoneEngine::stop(IOService* provider)
{
    super::stop(provider);
}

UInt32 AMDMicrophoneEngine::getCurrentSampleFrame()
{
    LOG("getCurrentSampleFrame()\n");
    return 0;
}

IOReturn AMDMicrophoneEngine::performAudioEngineStart()
{
    LOG("performAudioEngineStart()\n");

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performAudioEngineStop()
{
    LOG("performAudioEngineStop()\n");

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performFormatChange(IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate)
{
    LOG("peformFormatChange()\n");
    IOReturn result = kIOReturnSuccess;

    // Since we only allow one format, we only need to be concerned with sample rate changes
    // In this case, we only allow 2 sample rates - 44100 & 48000, so those are the only ones
    // that we check for
    if (newSampleRate) {
        switch (newSampleRate->whole) {
        case kAudioSampleRate:
            result = kIOReturnSuccess;
            break;
        default:
            result = kIOReturnUnsupported;
            LOG("Internal Error - unknown sample rate selected.\n");
            break;
        }
    }

    return result;
}

IOReturn AMDMicrophoneEngine::convertInputSamples(const void* sampleBuf, void* destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat* streamFormat, IOAudioStream* audioStream)
{
    UInt32 numSamplesLeft;
    float* floatDestBuf;
    SInt16* inputBuf;

    // Start by casting the destination buffer to a float *
    floatDestBuf = (float*)destBuf;
    // Determine the starting point for our input conversion
    inputBuf = &(((SInt16*)sampleBuf)[firstSampleFrame * streamFormat->fNumChannels]);

    // Calculate the number of actual samples to convert
    numSamplesLeft = numSampleFrames * streamFormat->fNumChannels;

    // Loop through each sample and scale and convert them
    while (numSamplesLeft > 0) {
        SInt16 inputSample;

        // Fetch the SInt16 input sample
        inputSample = *inputBuf;

        // Scale that sample to a range of -1.0 to 1.0, convert to float and store in the destination buffer
        // at the proper location
        if (inputSample >= 0) {
            *floatDestBuf = inputSample / 32767.0f;
        } else {
            *floatDestBuf = inputSample / 32768.0f;
        }

        // Move on to the next sample
        ++inputBuf;
        ++floatDestBuf;
        --numSamplesLeft;
    }

    return kIOReturnSuccess;
}
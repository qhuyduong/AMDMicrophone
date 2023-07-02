//
//  AMDMicrophoneEngine.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#include "AMDMicrophoneEngine.hpp"

#include "AMDMicrophoneCommon.hpp"
#include "AMDMicrophoneDevice.hpp"

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
                kAudioNumChannels,
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

bool AMDMicrophoneEngine::createControls()
{
    bool result = false;
    IOAudioControl* control;

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

    result = true;

Done:
    return result;
}

void AMDMicrophoneEngine::interruptOccured(OSObject* owner, IOTimerEventSource* sender)
{
    UInt64 thisTimeNS;
    uint64_t time;
    SInt64 diff;
    UInt32 bufferPosition;
    AMDMicrophoneEngine* audioEngine = (AMDMicrophoneEngine*)owner;

    if (!audioEngine || !sender)
        return;

    bufferPosition = audioEngine->interruptCount % (kAudioInterruptHZ / 2);
    if (bufferPosition == 0) {
        audioEngine->takeTimeStamp();
    }
    audioEngine->interruptCount++;

    clock_get_uptime(&time);
    absolutetime_to_nanoseconds(time, &thisTimeNS);
    diff = ((SInt64)audioEngine->nextTimeout - (SInt64)thisTimeNS);
    sender->setTimeoutUS((UInt32)(((SInt64)kAudioInterruptInterval + diff) / 1000));
    audioEngine->nextTimeout += kAudioInterruptInterval;
}

bool AMDMicrophoneEngine::init(AMDMicrophoneDevice* device)
{
    if (!super::init(NULL)) {
        return false;
    }

    audioDevice = device;

    return true;
}

void AMDMicrophoneEngine::free()
{
    if (interruptSource) {
        interruptSource->release();
        interruptSource = NULL;
    }

    if (dmaDescriptor) {
        dmaDescriptor->release();
        dmaDescriptor = NULL;
    }

    super::free();
}

bool AMDMicrophoneEngine::initHardware(IOService* provider)
{
    bool result = false;
    IOAudioSampleRate initialSampleRate;
    IOAudioStream* audioStream;
    IOWorkLoop* workLoop;

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDescription("AMD Digital Microphone");

    initialSampleRate.whole = kAudioSampleRate;
    initialSampleRate.fraction = 0;
    setSampleRate(&initialSampleRate);
    setNumSampleFramesPerBuffer(kAudioBufferSampleFrames);
    setInputSampleLatency(kAudioSampleRate / kAudioInterruptHZ);

    if (!createControls()) {
        goto Done;
    }

    dmaDescriptor = audioDevice->allocateDMADescriptor(kAudioSampleBufferSize);
    if (!dmaDescriptor) {
        LOG("ERROR: failed to allocate DMA buffer");
        goto Done;
    }

    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, dmaDescriptor->getBytesNoCopy(), kAudioSampleBufferSize);
    if (!audioStream) {
        goto Done;
    }
    addAudioStream(audioStream);
    audioStream->release();

    workLoop = getWorkLoop();
    if (!workLoop) {
        goto Done;
    }

    interruptSource = IOTimerEventSource::timerEventSource(this, AMDMicrophoneEngine::interruptOccured);
    if (!interruptSource) {
        goto Done;
    }

    if (workLoop->addEventSource(interruptSource) != kIOReturnSuccess) {
        goto Done;
    }

    result = true;

Done:
    return result;
}

void AMDMicrophoneEngine::stop(IOService* provider)
{
    if (interruptSource) {
        interruptSource->cancelTimeout();
        getWorkLoop()->removeEventSource(interruptSource);
    }
    super::stop(provider);
}

UInt32 AMDMicrophoneEngine::getCurrentSampleFrame()
{
    LOG("getCurrentSampleFrame()\n");

    UInt32 periodCount = (UInt32)interruptCount % (kAudioInterruptHZ / 2);
    UInt32 sampleFrame = periodCount * (kAudioSampleRate / kAudioInterruptHZ);
    return sampleFrame;
}

IOReturn AMDMicrophoneEngine::performAudioEngineStart()
{
    UInt64 time, timeNS;

    LOG("performAudioEngineStart()\n");
    interruptCount = 0;
    takeTimeStamp(false);
    interruptSource->setTimeoutUS(kAudioInterruptInterval / 1000);

    clock_get_uptime(&time);
    absolutetime_to_nanoseconds(time, &timeNS);

    nextTimeout = timeNS + kAudioInterruptInterval;

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performAudioEngineStop()
{
    LOG("performAudioEngineStop()\n");
    interruptSource->cancelTimeout();

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performFormatChange(IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate)
{
    LOG("peformFormatChange()\n");
    IOReturn result = kIOReturnSuccess;

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
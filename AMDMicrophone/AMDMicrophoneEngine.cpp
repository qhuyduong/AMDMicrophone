//
//  AMDMicrophoneEngine.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#include "AMDMicrophoneEngine.hpp"

#include "AMDMicrophoneCommon.hpp"
#include "AMDMicrophoneDevice.hpp"

#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/audio/IOAudioDevice.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>

#define super IOAudioEngine

OSDefineMetaClassAndStructors(AMDMicrophoneEngine, IOAudioEngine);

bool AMDMicrophoneEngine::createControls()
{
    bool result = false;
    IOAudioControl* control;

    control = IOAudioLevelControl::createVolumeControl(
        32768,
        0,
        65535,
        (-22 << 16) + (32768),
        0,
        kIOAudioControlChannelIDAll,
        kIOAudioControlChannelNameAll,
        0,
        kIOAudioControlUsageInput
    );
    if (!control)
        goto Done;

    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    addDefaultAudioControl(control);
    control->release();

    result = true;

Done:
    return result;
}

IOAudioStream* AMDMicrophoneEngine::createNewAudioStream(
    IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize
)
{
    IOAudioStream* audioStream;

    audioStream = new IOAudioStream;
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1))
            audioStream->release();
        else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                NUM_CHANNELS,
                kIOAudioStreamSampleFormatLinearPCM,
                kIOAudioStreamNumericRepresentationSignedInt,
                SAMPLE_DEPTH,
                SAMPLE_WIDTH,
                kIOAudioStreamAlignmentLowByte,
                kIOAudioStreamByteOrderLittleEndian,
                true,
                0
            };

            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);
            rate.fraction = 0;
            rate.whole = SAMPLE_RATE;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            audioStream->setFormat(&format);
        }
    }

    return audioStream;
}

IOReturn AMDMicrophoneEngine::gainChangeHandler(
    IOService* target, IOAudioControl* gainControl, SInt32 oldValue, SInt32 newValue
)
{
    AMDMicrophoneEngine* that = (AMDMicrophoneEngine*)target;

    if (!that)
        return kIOReturnBadArgument;

    that->gain = newValue;

    return kIOReturnSuccess;
}

bool AMDMicrophoneEngine::init(AMDMicrophoneDevice* device)
{
    if (!super::init(NULL))
        return false;

    audioDevice = device;

    return true;
}

void AMDMicrophoneEngine::free()
{
    super::free();
}

bool AMDMicrophoneEngine::initHardware(IOService* provider)
{
    bool result = false;
    IOAudioSampleRate initialSampleRate;
    IOAudioStream* audioStream;

    if (!super::initHardware(provider))
        goto Done;

    setDescription("AMD Digital Microphone");

    initialSampleRate.whole = SAMPLE_RATE;
    initialSampleRate.fraction = 0;
    setSampleRate(&initialSampleRate);
    setNumSampleFramesPerBuffer(NUM_FRAMES);

    if (!createControls())
        goto Done;

    audioStream = createNewAudioStream(
        kIOAudioStreamDirectionInput, audioDevice->dmaDescriptor->getBytesNoCopy(), BUFFER_SIZE
    );
    if (!audioStream)
        goto Done;
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

IOReturn AMDMicrophoneEngine::convertInputSamples(
    const void* sampleBuf, void* destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames,
    const IOAudioStreamFormat* streamFormat, IOAudioStream* audioStream
)
{
    UInt32 numSamplesLeft;
    float* floatDestBuf;
    SInt32* inputBuf32;
    UInt32 firstSample = firstSampleFrame * streamFormat->fNumChannels;
    const float kOneOverMaxSInt32Value = 1.0 / 2147483648.0f;

    floatDestBuf = (float*)destBuf;
    numSamplesLeft = numSampleFrames * streamFormat->fNumChannels;

    inputBuf32 = &(((SInt32*)sampleBuf)[firstSample]);
    while (numSamplesLeft-- > 0)
        *(floatDestBuf++) = (float)*(inputBuf32++) * kOneOverMaxSInt32Value * gain / 65535;

    return kIOReturnSuccess;
}

UInt32 AMDMicrophoneEngine::getCurrentSampleFrame()
{
    return (audioDevice->periodsCount * PERIOD_SIZE) / FRAME_SIZE;
}

IOReturn AMDMicrophoneEngine::performAudioEngineStart()
{
    takeTimeStamp(false);

    audioDevice->initRingBuffer(ACP_MEM_WINDOW_START, BUFFER_SIZE, PERIOD_SIZE);
    audioDevice->writel(0x0, ACP_WOV_PDM_NO_OF_CHANNELS);
    audioDevice->writel(ACP_PDM_DECIMATION_FACTOR, ACP_WOV_PDM_DECIMATION_FACTOR);
    audioDevice->startDMA();
    audioDevice->enableInterrupt();

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performAudioEngineStop()
{
    audioDevice->disableInterrupt();
    audioDevice->stopDMA();

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performFormatChange(
    IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate
)
{
    IOReturn result = kIOReturnSuccess;

    if (newSampleRate) {
        switch (newSampleRate->whole) {
        case SAMPLE_RATE:
            result = kIOReturnSuccess;
            break;
        default:
            result = kIOReturnUnsupported;
            break;
        }
    }

    return result;
}
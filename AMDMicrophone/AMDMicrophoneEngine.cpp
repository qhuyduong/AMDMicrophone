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

IOAudioStream* AMDMicrophoneEngine::createNewAudioStream(
    IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize
)
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
        kIOAudioControlUsageInput
    );
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
        kIOAudioControlUsageInput
    );

    if (!control) {
        goto Done;
    }

    addDefaultAudioControl(control);
    control->release();

    result = true;

Done:
    return result;
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
    super::free();
}

bool AMDMicrophoneEngine::initHardware(IOService* provider)
{
    bool result = false;
    IOAudioSampleRate initialSampleRate;
    IOAudioStream* audioStream;

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDescription("AMD Digital Microphone");

    initialSampleRate.whole = kAudioSampleRate;
    initialSampleRate.fraction = 0;
    setSampleRate(&initialSampleRate);
    setNumSampleFramesPerBuffer(kAudioNumSampleFrames);

    if (!createControls()) {
        goto Done;
    }

    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, audioDevice->dmaDescriptor->getBytesNoCopy(), BUFFER_SIZE);
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
    LOG("performAudioEngineStop()\n");

    audioDevice->disableInterrupt();
    audioDevice->stopDMA();

    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneEngine::performFormatChange(
    IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate
)
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
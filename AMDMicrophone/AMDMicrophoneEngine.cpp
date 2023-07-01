//
//  AMDMicrophoneEngine.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#include "AMDMicrophoneEngine.hpp"
#include "AMDMicrophoneCommon.hpp"

#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/audio/IOAudioDevice.h>

#define super IOAudioEngine
#define INITIAL_SAMPLE_RATE 44100
#define NUM_SAMPLE_FRAMES 16384
#define NUM_CHANNELS 1
#define BIT_DEPTH 16
#define BUFFER_SIZE (NUM_SAMPLE_FRAMES * NUM_CHANNELS * BIT_DEPTH / 8)

OSDefineMetaClassAndStructors(AMDMicrophoneEngine, IOAudioEngine);

IOAudioStream* AMDMicrophoneEngine::createNewAudioStream(IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize)
{
    IOAudioStream* audioStream;

    // For this sample device, we are only creating a single format and allowing 44.1KHz and 48KHz
    audioStream = new IOAudioStream;
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                2, // num channels
                kIOAudioStreamSampleFormatLinearPCM, // sample format
                kIOAudioStreamNumericRepresentationSignedInt, // numeric format
                BIT_DEPTH, // bit depth
                BIT_DEPTH, // bit width
                kIOAudioStreamAlignmentHighByte, // high byte aligned - unused because bit depth == bit width
                kIOAudioStreamByteOrderBigEndian, // big endian
                true, // format is mixable
                0 // driver-defined tag - unused by this driver
            };

            // As part of creating a new IOAudioStream, its sample buffer needs to be set
            // It will automatically create a mix buffer should it be needed
            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);

            // This device only allows a single format and a choice of 2 different sample rates
            rate.fraction = 0;
            rate.whole = 44100;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            rate.whole = 48000;
            audioStream->addAvailableFormat(&format, &rate, &rate);

            // Finally, the IOAudioStream's current format needs to be indicated
            audioStream->setFormat(&format);
        }
    }

    return audioStream;
}

bool AMDMicrophoneEngine::init()
{
    bool result = false;

    if (!super::init(NULL)) {
        goto Done;
    }

    result = true;
Done:
    return result;
}

void AMDMicrophoneEngine::free()
{
    LOG("free\n");

    if (buffer) {
        IOFree(buffer, BUFFER_SIZE);
        buffer = NULL;
    }

    super::free();
}

bool AMDMicrophoneEngine::initHardware(IOService* provider)
{
    bool result = false;
    IOAudioStream* audioStream;

    LOG("initHardware\n");

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDescription("AMD Digital Microphone Engine");

    // Allocate our input and output buffers - a real driver will likely need to allocate its buffers
    // differently
    buffer = (SInt16*)IOMalloc(BUFFER_SIZE);
    if (!buffer) {
        goto Done;
    }

    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, buffer, BUFFER_SIZE);
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

    // In order for the erase process to run properly, this function must return the current location of
    // the audio engine - basically a sample counter
    // It doesn't need to be exact, but if it is inexact, it should err towards being before the current location
    // rather than after the current location.  The erase head will erase up to, but not including the sample
    // frame returned by this function.  If it is too large a value, sound data that hasn't been played will be
    // erased.

    // Change to return the real value
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

    return kIOReturnSuccess;
}
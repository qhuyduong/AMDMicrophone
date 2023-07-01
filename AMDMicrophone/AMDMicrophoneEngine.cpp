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

    audioStream = new IOAudioStream;
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                1,
                kIOAudioStreamSampleFormatLinearPCM,
                kIOAudioStreamNumericRepresentationSignedInt,
                BIT_DEPTH,
                BIT_DEPTH,
                kIOAudioStreamAlignmentHighByte,
                kIOAudioStreamByteOrderBigEndian,
                true,
                0
            };

            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);

            rate.fraction = 0;
            rate.whole = 44100;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            rate.whole = 48000;
            audioStream->addAvailableFormat(&format, &rate, &rate);

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
    IOAudioSampleRate initialSampleRate;
    IOAudioStream* audioStream;

    LOG("initHardware\n");

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDescription("AMD Digital Microphone");

    initialSampleRate.whole = INITIAL_SAMPLE_RATE;
    initialSampleRate.fraction = 0;
    setSampleRate(&initialSampleRate);
    setNumSampleFramesPerBuffer(NUM_SAMPLE_FRAMES);

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
    LOG("stop()\n");

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

    takeTimeStamp();

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

// The function convertInputSamples() is responsible for converting from the hardware format
// in the input sample buffer to float samples in the destination buffer and scale the samples
// to a range of -1.0 to 1.0.  This function is guaranteed not to have the samples wrapped
// from the end of the buffer to the beginning.
// This function only needs to be implemented if the device has any input IOAudioStreams

// This implementation is very inefficient, but illustrates the conversion and scaling that needs to take place.

// The parameters are as follows:
//		sampleBuf - a pointer to the beginning of the hardware formatted sample buffer - this is the same buffer passed
//					to the IOAudioStream using setSampleBuffer()
//		destBuf - a pointer to the float destination buffer - this is the buffer that the CoreAudio.framework uses
//					its size is numSampleFrames * numChannels * sizeof(float)
//		firstSampleFrame - this is the index of the first sample frame to the input conversion on
//		numSampleFrames - the total number of sample frames to convert and scale
//		streamFormat - the current format of the IOAudioStream this function is operating on
//		audioStream - the audio stream this function is operating on
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
            *floatDestBuf = inputSample / 32767.0;
        } else {
            *floatDestBuf = inputSample / 32768.0;
        }

        // Move on to the next sample
        ++inputBuf;
        ++floatDestBuf;
        --numSamplesLeft;
    }

    return kIOReturnSuccess;
}
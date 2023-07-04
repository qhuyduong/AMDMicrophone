//
//  AMDMicrophoneEngine.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#ifndef AMDMicrophoneEngine_hpp
#define AMDMicrophoneEngine_hpp

#include <IOKit/audio/IOAudioEngine.h>

#define NUM_PERIODS 4
#define PERIOD_SIZE 8192
#define BUFFER_SIZE (PERIOD_SIZE * NUM_PERIODS)

#define SAMPLE_RATE  48000
#define NUM_CHANNELS 2
#define SAMPLE_DEPTH 32
#define SAMPLE_WIDTH 32
#define FRAME_SIZE   (NUM_CHANNELS * SAMPLE_WIDTH / 8)
#define NUM_FRAMES   (BUFFER_SIZE / FRAME_SIZE)

class AMDMicrophoneDevice;

class AMDMicrophoneEngine : public IOAudioEngine {
    OSDeclareDefaultStructors(AMDMicrophoneEngine);

    AMDMicrophoneDevice* audioDevice;

    IOAudioStream* createNewAudioStream(
        IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize
    );
    bool createControls();

public:
    bool init(AMDMicrophoneDevice* device);
    void free() override;

    bool initHardware(IOService* provider) override;
    void stop(IOService* provider) override;

    IOReturn convertInputSamples(
        const void* sampleBuf, void* destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames,
        const IOAudioStreamFormat* streamFormat, IOAudioStream* audioStream
    );
    UInt32 getCurrentSampleFrame() override;
    IOReturn performAudioEngineStart() override;
    IOReturn performAudioEngineStop() override;
    IOReturn performFormatChange(
        IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate
    ) override;
};

#endif /* AMDMicrophoneEngine_hpp */

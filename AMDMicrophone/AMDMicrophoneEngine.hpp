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
#define PERIOD_FRAMES (PERIOD_SIZE / FRAME_SIZE)
#define NUM_FRAMES   (BUFFER_SIZE / FRAME_SIZE)
#define MAX_VOLUME   100
#define INPUT_SOFTWARE_GAIN 3.0f
#define INPUT_EXPANDER_THRESHOLD 0.018f
#define INPUT_EXPANDER_FLOOR 0.18f
#define INPUT_EXPANDER_ATTACK 0.05f
#define INPUT_EXPANDER_RELEASE 0.001f
#define STARTUP_FADE_FRAMES (SAMPLE_RATE / 2)

class AMDMicrophoneDevice;

class AMDMicrophoneEngine : public IOAudioEngine {
    OSDeclareDefaultStructors(AMDMicrophoneEngine);

    AMDMicrophoneDevice* audioDevice;
    UInt32 volume = MAX_VOLUME;
    UInt32 startupFadeFrame = STARTUP_FADE_FRAMES;
    float expanderEnvelope = 0.0f;
    float expanderGain = 1.0f;

    bool createControls();
    IOAudioStream* createNewAudioStream(
        IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize
    );
    static IOReturn gainChangeHandler(
        IOService* target, IOAudioControl* gainControl, SInt32 oldValue, SInt32 newValue
    );

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

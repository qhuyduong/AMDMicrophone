//
//  AMDMicrophoneEngine.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#ifndef AMDMicrophoneEngine_hpp
#define AMDMicrophoneEngine_hpp

#include <IOKit/audio/IOAudioEngine.h>

class AMDMicrophoneDevice;
class IOTimerEventSource;

class AMDMicrophoneEngine : public IOAudioEngine {
    OSDeclareDefaultStructors(AMDMicrophoneEngine);

    AMDMicrophoneDevice* audioDevice;
    IOBufferMemoryDescriptor* dmaDescriptor;
    IOTimerEventSource* interruptSource;
    SInt64 nextTimeout;
    UInt32 interruptCount;

    IOAudioStream* createNewAudioStream(
        IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize
    );
    bool createControls();
    static void interruptOccured(OSObject* owner, IOTimerEventSource* sender);

public:
    bool init(AMDMicrophoneDevice* device);
    void free() override;

    bool initHardware(IOService* provider) override;
    void stop(IOService* provider) override;

    UInt32 getCurrentSampleFrame() override;
    IOReturn performAudioEngineStart() override;
    IOReturn performAudioEngineStop() override;
    IOReturn performFormatChange(
        IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate
    ) override;
    IOReturn convertInputSamples(
        const void* sampleBuf, void* destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat* streamFormat, IOAudioStream* audioStream
    ) override;
};

#endif /* AMDMicrophoneEngine_hpp */

//
//  AMDMicrophoneEngine.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#ifndef AMDMicrophoneEngine_hpp
#define AMDMicrophoneEngine_hpp

#include <IOKit/audio/IOAudioEngine.h>

class IOInterruptEventSource;
class IOFilterInterruptEventSource;

class AMDMicrophoneEngine : public IOAudioEngine {
    OSDeclareDefaultStructors(AMDMicrophoneEngine);

    IOFilterInterruptEventSource* interruptEventSource;
    SInt16* buffer;

    IOAudioStream* createNewAudioStream(IOAudioStreamDirection direction, void* sampleBuffer, UInt32 sampleBufferSize);

    void filterInterrupt(int index);
    static void interruptHandler(OSObject* owner, IOInterruptEventSource* source, int count);
    static bool interruptFilter(OSObject* owner, IOFilterInterruptEventSource* source);

public:
    bool init();
    void free() override;

    bool initHardware(IOService* provider) override;
    void stop(IOService* provider) override;

    UInt32 getCurrentSampleFrame() override;
    IOReturn performAudioEngineStart() override;
    IOReturn performAudioEngineStop() override;
    IOReturn performFormatChange(IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat, const IOAudioSampleRate* newSampleRate) override;
};

#endif /* AMDMicrophoneEngine_hpp */

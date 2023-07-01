//
//  AMDMicrophoneEngine.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#ifndef AMDMicrophoneEngine_hpp
#define AMDMicrophoneEngine_hpp

#include <IOKit/audio/IOAudioEngine.h>

#include "AMDMicrophoneDevice.hpp"

class IOFilterInterruptEventSource;
class IOInterruptEventSource;

class AMDMicrophoneEngine : public IOAudioEngine {
    OSDeclareDefaultStructors(AMDMicrophoneEngine);

    IOFilterInterruptEventSource* interruptEventSource;

public:
    bool initHardware(IOService* provider) override;
    UInt32 getCurrentSampleFrame() override;

    bool init();
    void free();
};

#endif /* AMDMicrophoneEngine_hpp */

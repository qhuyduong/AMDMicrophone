//
//  AMDMicrophoneDevice.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AMDMicrophoneDevice_hpp
#define AMDMicrophoneDevice_hpp

#include <IOKit/audio/IOAudioDevice.h>

class AMDMicrophoneEngine;
class IOInterruptEventSource;
class IOPCIDevice;

class AMDMicrophoneDevice : public IOAudioDevice {
    OSDeclareDefaultStructors(AMDMicrophoneDevice);

    IOPCIDevice* pciDevice;
    IOMemoryMap* deviceMap;
    AMDMicrophoneEngine* audioEngine;
    IOInterruptEventSource* interruptSource;

    bool createAudioEngine();
    int findMSIInterruptTypeIndex();
    static void interruptOccurred(OSObject* owner, IOInterruptEventSource* src, int intCount);

public:
    IOService* probe(IOService* provider, SInt32* score) override;
    bool initHardware(IOService* provider) override;
    void free() override;
};

#endif /* AMDMicrophoneDevice_hpp */

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

    friend class AMDMicrophoneEngine;

    AMDMicrophoneEngine* audioEngine;
    IOInterruptEventSource* interruptSource;
    IOPCIDevice* pciDevice;
    IOMemoryMap* baseAddrMap;
    IOVirtualAddress baseAddr;

    bool createAudioEngine();
    int findMSIInterruptTypeIndex();
    static void interruptOccurred(OSObject* owner, IOInterruptEventSource* src, int intCount);
    IOBufferMemoryDescriptor* allocateDMADescriptor(UInt32 size);

public:
    IOService* probe(IOService* provider, SInt32* score) override;
    bool initHardware(IOService* provider) override;
    void free() override;
};

#endif /* AMDMicrophoneDevice_hpp */

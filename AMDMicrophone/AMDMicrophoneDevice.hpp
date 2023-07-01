//
//  AMDMicrophoneDevice.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AMDMicrophoneDevice_hpp
#define AMDMicrophoneDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/audio/IOAudioDevice.h>
#include <IOKit/pci/IOPCIDevice.h>

#define LOG(fmt, ...) IOLog("%s: " fmt, this->getName(), ##__VA_ARGS__)

class AMDMicrophoneDevice : public IOAudioDevice {
    OSDeclareDefaultStructors(AMDMicrophoneDevice);

    friend class AMDMicrophoneEngine;

    IOPCIDevice* pciDevice;
    IOMemoryMap* deviceMap;

    virtual bool createAudioEngine();

public:
    IOService* probe(IOService* provider, SInt32* score) override;
    bool initHardware(IOService* provider) override;
    void free() override;
};

#endif /* AMDMicrophoneDevice_hpp */

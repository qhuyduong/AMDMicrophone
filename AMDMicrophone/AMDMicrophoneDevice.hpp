//
//  AMDMicrophoneDevice.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AMDMicrophoneDevice_hpp
#define AMDMicrophoneDevice_hpp

#include <IOKit/audio/IOAudioDevice.h>

class IOPCIDevice;

class AMDMicrophoneDevice : public IOAudioDevice {
    OSDeclareDefaultStructors(AMDMicrophoneDevice);

    friend class AMDMicrophoneEngine;

    IOPCIDevice* pciDevice;
    IOMemoryMap* deviceMap;

    bool createAudioEngine();

    static IOReturn gainChangeHandler(IOService* target, IOAudioControl* gainControl, SInt32 oldValue, SInt32 newValue);
    IOReturn gainChanged(IOAudioControl* gainControl, SInt32 oldValue, SInt32 newValue);

    static IOReturn inputMuteChangeHandler(IOService* target, IOAudioControl* muteControl, SInt32 oldValue, SInt32 newValue);
    IOReturn inputMuteChanged(IOAudioControl* muteControl, SInt32 oldValue, SInt32 newValue);

public:
    IOService* probe(IOService* provider, SInt32* score) override;
    bool initHardware(IOService* provider) override;
    void free() override;
};

#endif /* AMDMicrophoneDevice_hpp */

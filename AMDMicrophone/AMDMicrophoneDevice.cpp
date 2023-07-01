//
//  AMDMicrophoneDevice.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#include "AMDMicrophoneDevice.hpp"
#include "AMDMicrophoneCommon.hpp"
#include "AMDMicrophoneEngine.hpp"

#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/pci/IOPCIDevice.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(AMDMicrophoneDevice, IOAudioDevice);

bool AMDMicrophoneDevice::createAudioEngine()
{
    bool result = false;
    AMDMicrophoneEngine* audioEngine = NULL;
    IOAudioControl* control;

    LOG("createAudioEngine()\n");

    audioEngine = new AMDMicrophoneEngine;
    if (!audioEngine) {
        goto Done;
    }

    if (!audioEngine->init()) {
        goto Done;
    }

    // Create a left & right input gain control with an int range from 0 to 65535
    // and a db range from 0 to 22.5
    control = IOAudioLevelControl::createVolumeControl(65535, // Initial value
        0, // min value
        65535, // max value
        0, // min 0.0 in IOFixed
        (22 << 16) + (32768), // 22.5 in IOFixed (16.16)
        kIOAudioControlChannelIDDefaultLeft,
        kIOAudioControlChannelNameLeft,
        0, // control ID - driver-defined
        kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }

    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();

    control = IOAudioLevelControl::createVolumeControl(65535, // Initial value
        0, // min value
        65535, // max value
        0, // min 0.0 in IOFixed
        (22 << 16) + (32768), // max 22.5 in IOFixed (16.16)
        kIOAudioControlChannelIDDefaultRight, // Affects right channel
        kIOAudioControlChannelNameRight,
        0, // control ID - driver-defined
        kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }

    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();

    // Create an input mute control
    control = IOAudioToggleControl::createMuteControl(false, // initial state - unmuted
        kIOAudioControlChannelIDAll, // Affects all channels
        kIOAudioControlChannelNameAll,
        0, // control ID - driver-defined
        kIOAudioControlUsageInput);

    if (!control) {
        goto Done;
    }

    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)inputMuteChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();

    if (activateAudioEngine(audioEngine) != kIOReturnSuccess) {
        LOG("ERROR activateAudioEngine failed\n");
        goto Done;
    }

    result = true;

Done:
    if (audioEngine != NULL) {
        audioEngine->release();
    }

    return result;
}

IOReturn AMDMicrophoneDevice::gainChangeHandler(IOService* target, IOAudioControl* gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    AMDMicrophoneDevice* audioDevice;

    audioDevice = (AMDMicrophoneDevice*)target;
    if (audioDevice) {
        result = audioDevice->gainChanged(gainControl, oldValue, newValue);
    }

    return result;
}

IOReturn AMDMicrophoneDevice::gainChanged(IOAudioControl* gainControl, SInt32 oldValue, SInt32 newValue)
{
    LOG("gainChanged(%d, %d)\n", oldValue, newValue);

    if (gainControl) {
        LOG("\t-> Channel %d\n", gainControl->getChannelID());
    }

    // Add hardware gain change code here
    return kIOReturnSuccess;
}

IOReturn AMDMicrophoneDevice::inputMuteChangeHandler(IOService* target, IOAudioControl* muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    AMDMicrophoneDevice* audioDevice;

    audioDevice = (AMDMicrophoneDevice*)target;
    if (audioDevice) {
        result = audioDevice->inputMuteChanged(muteControl, oldValue, newValue);
    }

    return result;
}

IOReturn AMDMicrophoneDevice::inputMuteChanged(IOAudioControl* muteControl, SInt32 oldValue, SInt32 newValue)
{
    LOG("inputMuteChanged(%d, %d)\n", oldValue, newValue);

    // Add input mute change code here

    return kIOReturnSuccess;
}

IOService* AMDMicrophoneDevice::probe(IOService* provider, SInt32* score)
{
    pciDevice = OSDynamicCast(IOPCIDevice, provider);

    UInt8 revisionId = pciDevice->configRead8(kIOPCIConfigRevisionID);

    if (revisionId != 0x01) {
        LOG("Only Renoir is supported at the moment\n");
        return NULL;
    }

    return this;
}

bool AMDMicrophoneDevice::initHardware(IOService* provider)
{
    bool result = false;

    if (!super::initHardware(provider)) {
        goto Done;
    }

    deviceMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!deviceMap) {
        goto Done;
    }

    pciDevice->setMemoryEnable(true);

    setDeviceName("AMD Digital Microphone");
    setDeviceShortName("AMDMicrophone");
    setManufacturerName("AMD");

    if (!createAudioEngine()) {
        goto Done;
    }

    result = true;

Done:
    if (!result) {
        if (deviceMap) {
            deviceMap->release();
            deviceMap = NULL;
        }
    }

    return result;
}

void AMDMicrophoneDevice::free()
{
    if (deviceMap) {
        deviceMap->release();
        deviceMap = NULL;
    }

    super::free();
}
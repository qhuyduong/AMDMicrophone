//
//  AMDMicrophoneDevice.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#include "AMDMicrophoneDevice.hpp"
#include "AMDMicrophoneEngine.hpp"

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

    activateAudioEngine(audioEngine);
    audioEngine->release();

    result = true;

Done:
    if (!result && (audioEngine != NULL)) {
        audioEngine->release();
    }

    return result;
}

IOService* AMDMicrophoneDevice::probe(IOService* provider, SInt32* score)
{
    pciDevice = OSDynamicCast(IOPCIDevice, provider);

    UInt8 revisionId = pciDevice->configRead8(kIOPCIConfigRevisionID);

    if (revisionId == 0x01) {
        LOG("Renoir detected\n");
    } else {
        LOG("Only Renoir is supported!\n");
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
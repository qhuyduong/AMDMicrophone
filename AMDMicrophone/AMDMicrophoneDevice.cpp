//
//  AMDMicrophoneDevice.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#include "AMDMicrophoneDevice.hpp"

#include "AMDMicrophoneCommon.hpp"
#include "AMDMicrophoneEngine.hpp"

#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(AMDMicrophoneDevice, IOAudioDevice);

bool AMDMicrophoneDevice::createAudioEngine()
{
    bool result = false;

    LOG("createAudioEngine()\n");

    audioEngine = new AMDMicrophoneEngine;
    if (!audioEngine) {
        goto Done;
    }

    if (!audioEngine->init(this)) {
        goto Done;
    }

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

int AMDMicrophoneDevice::findMSIInterruptTypeIndex()
{
    IOReturn ret;
    int index, source = 0;
    for (index = 0;; index++) {
        int interruptType;
        ret = pciDevice->getInterruptType(index, &interruptType);
        if (ret != kIOReturnSuccess)
            break;
        if (interruptType & kIOInterruptTypePCIMessaged) {
            source = index;
            break;
        }
    }
    return source;
}

void AMDMicrophoneDevice::interruptOccurred(OSObject* owner, IOInterruptEventSource* src, int intCount)
{
    AMDMicrophoneDevice* me = (AMDMicrophoneDevice*)owner;

    // Start next DMA
}

IOBufferMemoryDescriptor* AMDMicrophoneDevice::allocateDMADescriptor(UInt32 size)
{
    return IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, kIODirectionIn, size, 4096);
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
    IOWorkLoop* workLoop;

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDeviceName("AMD Digital Microphone");
    setDeviceShortName("AMDMicrophone");
    setManufacturerName("AMD");
    setDeviceTransportType(kIOAudioDeviceTransportTypePCI);

    pciDevice->setBusMasterEnable(true);
    pciDevice->setIOEnable(true);
    pciDevice->setMemoryEnable(true);

    baseAddrMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!baseAddrMap) {
        goto Done;
    }
    baseAddr = baseAddrMap->getVirtualAddress();
    if (!baseAddr) {
        goto Done;
    }

    workLoop = getWorkLoop();
    if (!workLoop) {
        goto Done;
    }

    interruptSource = IOInterruptEventSource::interruptEventSource(
        this,
        (IOInterruptEventAction)&AMDMicrophoneDevice::interruptOccurred,
        provider,
        findMSIInterruptTypeIndex()
    );

    if (workLoop->addEventSource(interruptSource) != kIOReturnSuccess) {
        goto Done;
    }

    if (!createAudioEngine()) {
        goto Done;
    }

    result = true;

Done:
    if (!result) {
        free();
    }

    return result;
}

void AMDMicrophoneDevice::free()
{
    if (baseAddrMap) {
        baseAddrMap->release();
        baseAddrMap = NULL;
    }

    super::free();
}
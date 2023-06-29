//
//  AMDMicrophone.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#include "AMDMicrophone.hpp"

OSDefineMetaClassAndStructors(AMDMicrophone, IOService);

IOService* AMDMicrophone::probe(IOService* provider, SInt32* score)
{
    IOPCIDevice* pci = OSDynamicCast(IOPCIDevice, provider);

    UInt8 revisionId = pci->configRead8(kIOPCIConfigRevisionID);

    if (revisionId == 0x01) {
        LOG("Renoir detected!\n");
    } else {
        LOG("Only Renoir is supported!\n");
        return NULL;
    }

    return this;
}

bool AMDMicrophone::start(IOService* provider)
{
    if (!super::start(provider)) {
        return false;
    }
    this->pci = OSDynamicCast(IOPCIDevice, provider);
    this->pci->setBusMasterEnable(true);
    this->pci->setMemoryEnable(true);
    this->pci->setIOEnable(true);

    return true;
}
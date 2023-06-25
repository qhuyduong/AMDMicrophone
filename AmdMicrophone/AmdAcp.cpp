//
//  AmdAcp.cpp
//  AmdMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#include "AmdAcp.hpp"

OSDefineMetaClassAndStructors(AmdAcp, IOService);

bool AmdAcp::start(IOService* provider)
{
    LOG("Hello, world\n");

    this->pciDev = OSDynamicCast(IOPCIDevice, provider);
    if (this->pciDev == NULL) {
        return false;
    }

    UInt8 revisionId = this->pciDev->configRead8(kIOPCIConfigRevisionID);

    if (revisionId == 0x01) {
        LOG("Renoir detected!\n");
    } else {
        LOG("Only Renoir is supported!\n");
    }

    return true;
}

void AmdAcp::stop(IOService* provider)
{
    LOG("Bye, world!\n");
}

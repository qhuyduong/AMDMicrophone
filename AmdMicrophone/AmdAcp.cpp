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
    IOLog("Hello, world\n");

    return true;
}

void AmdAcp::stop(IOService* provider)
{
    IOLog("Bye, world\n");
}

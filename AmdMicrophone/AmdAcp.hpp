//
//  AmdAcp.hpp
//  AmdMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AmdAcp_hpp
#define AmdAcp_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>

#define LOG(...) IOLog("%s: " __VA_ARGS__, this->getName())

class AmdAcp : public IOService {
    OSDeclareDefaultStructors(AmdAcp);

private:
    IOPCIDevice* pciDev;

public:
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
};

#endif /* AmdAcp_hpp */

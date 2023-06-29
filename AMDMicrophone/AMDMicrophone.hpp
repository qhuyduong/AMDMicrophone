//
//  AMDMicrophone.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AMDMicrophone_hpp
#define AMDMicrophone_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>

#define LOG(...) IOLog("%s: " __VA_ARGS__, this->getName())
#define super IOService

class AMDMicrophone : public IOService {
    OSDeclareDefaultStructors(AMDMicrophone);

private:
    IOPCIDevice* pci;

public:
    virtual IOService* probe(IOService* provider, SInt32* score) override;
    virtual bool start(IOService* provider) override;
};

#endif /* AMDMicrophone_hpp */

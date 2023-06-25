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

class AmdAcp : public IOService {
    OSDeclareDefaultStructors(AmdAcp);

public:
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
};

#endif /* AmdAcp_hpp */

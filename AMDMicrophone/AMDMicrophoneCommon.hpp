//
//  AMDMicrophoneCommon.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 01/07/2023.
//

#ifndef AMDMicrophoneCommon_h
#define AMDMicrophoneCommon_h

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>

#define LOG(fmt, ...) IOLog("%s: " fmt, this->getName(), ##__VA_ARGS__)

#endif /* AMDMicrophoneCommon_h */

//
//  AMDMicrophoneEngine.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 30/06/2023.
//

#include "AMDMicrophoneEngine.hpp"

#define super IOAudioEngine

OSDefineMetaClassAndStructors(AMDMicrophoneEngine, IOAudioEngine);

bool AMDMicrophoneEngine::initHardware(IOService* provider)
{
    bool result = false;

    LOG("initHardware\n");

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDescription("AMD Digital Microphone Engine");

    result = true;
Done:

    return result;
}

bool AMDMicrophoneEngine::init()
{
    bool result = false;

    if (!super::init(NULL)) {
        goto Done;
    }

    result = true;
Done:

    return result;
}

void AMDMicrophoneEngine::free()
{
    LOG("free\n");

    super::free();
}
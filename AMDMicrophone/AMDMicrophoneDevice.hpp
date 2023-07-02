//
//  AMDMicrophoneDevice.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AMDMicrophoneDevice_hpp
#define AMDMicrophoneDevice_hpp

#include <IOKit/audio/IOAudioDevice.h>

#define ACP_POWER_ON_IN_PROGRESS 0x01
#define ACP_POWERED_OFF          0x02
#define PDM_DMA_STAT             0x10

#define ACP_PHY_BASE_ADDRESS   0x1240000
#define ACP_SOFT_RESET         0x1241000
#define ACP_CONTROL            0x1241004
#define ACP_EXTERNAL_INTR_ENB  0x1241800
#define ACP_EXTERNAL_INTR_CNTL 0x1241804
#define ACP_EXTERNAL_INTR_STAT 0x1241808
#define ACP_PGFSM_CONTROL      0x124141C
#define ACP_PGFSM_STATUS       0x1241420
#define ACP_CLKMUX_SEL         0x1241424

#define ACP_ERROR_MASK                        0x20000000
#define ACP_EXT_INTR_STAT_CLEAR_MASK          0xFFFFFFFF
#define ACP_PGFSM_CNTL_POWER_OFF_MASK         0x00
#define ACP_PGFSM_CNTL_POWER_ON_MASK          0x01
#define ACP_PGFSM_STATUS_MASK                 0x03
#define ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK 0x00010001
#define PDM_DMA_INTR_MASK                     0x10000

#define BIT(n)      (1UL << (n))
#define cpu_relax() asm volatile("rep; nop")

static inline UInt32 readl(IOVirtualAddress addr)
{
    return *(const volatile UInt32*)(addr - ACP_PHY_BASE_ADDRESS);
}

static inline void writel(UInt32 val, IOVirtualAddress addr)
{
    *(volatile UInt32*)(addr - ACP_PHY_BASE_ADDRESS) = val;
}

class AMDMicrophoneEngine;
class IOInterruptEventSource;
class IOPCIDevice;

class AMDMicrophoneDevice : public IOAudioDevice {
    OSDeclareDefaultStructors(AMDMicrophoneDevice);

    friend class AMDMicrophoneEngine;

    AMDMicrophoneEngine* audioEngine;
    IOInterruptEventSource* interruptSource;
    IOPCIDevice* pciDevice;
    IOMemoryMap* baseAddrMap;
    IOVirtualAddress baseAddr;

    bool createAudioEngine();
    int findMSIInterruptTypeIndex();
    static void interruptOccurred(OSObject* owner, IOInterruptEventSource* src, int intCount);
    IOBufferMemoryDescriptor* allocateDMADescriptor(UInt32 size);

    void disableInterrupts();
    void enableInterrupts();
    int powerOff();
    int powerOn();
    int reset();

public:
    IOService* probe(IOService* provider, SInt32* score) override;
    bool initHardware(IOService* provider) override;
    void stop(IOService* provider) override;
    void free() override;
};

#endif /* AMDMicrophoneDevice_hpp */

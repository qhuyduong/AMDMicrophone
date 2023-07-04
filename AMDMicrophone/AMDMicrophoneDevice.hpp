//
//  AMDMicrophoneDevice.hpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#ifndef AMDMicrophoneDevice_hpp
#define AMDMicrophoneDevice_hpp

#include <IOKit/audio/IOAudioDevice.h>

#define ACP_PHY_BASE_ADDRESS            0x1240000
#define ACP_AXI2AXI_ATU_PAGE_SIZE_GRP_1 0x1240C00
#define ACP_AXI2AXI_ATU_BASE_ADDR_GRP_1 0x1240C04
#define ACP_AXI2AXI_ATU_CTRL            0x1240C40
#define ACP_SOFT_RESET                  0x1241000
#define ACP_CONTROL                     0x1241004
#define ACP_EXTERNAL_INTR_ENB           0x1241800
#define ACP_EXTERNAL_INTR_CNTL          0x1241804
#define ACP_EXTERNAL_INTR_STAT          0x1241808
#define ACP_PGFSM_CONTROL               0x124141C
#define ACP_PGFSM_STATUS                0x1241420
#define ACP_CLKMUX_SEL                  0x1241424
#define ACP_WOV_PDM_ENABLE              0x1242C04
#define ACP_WOV_PDM_DMA_ENABLE          0x1242C08
#define ACP_WOV_RX_RINGBUFADDR          0x1242C0C
#define ACP_WOV_RX_RINGBUFSIZE          0x1242C10
#define ACP_WOV_RX_INTR_WATERMARK_SIZE  0x1242C20
#define ACP_WOV_PDM_FIFO_FLUSH          0x1242C24
#define ACP_WOV_PDM_NO_OF_CHANNELS      0x1242C28
#define ACP_WOV_PDM_DECIMATION_FACTOR   0x1242C2C
#define ACP_WOV_MISC_CTRL               0x1242C5C
#define ACP_WOV_CLK_CTRL                0x1242C60
#define ACP_SCRATCH_REG_0               0x1250000

#define ACP_ERROR_MASK                        0x20000000
#define ACP_EXT_INTR_STAT_CLEAR_MASK          0xFFFFFFFF
#define ACP_PDM_CLK_FREQ_MASK                 0x7
#define ACP_PDM_DMA_INTR_MASK                 0x10000
#define ACP_PGFSM_CNTL_POWER_OFF_MASK         0x0
#define ACP_PGFSM_CNTL_POWER_ON_MASK          0x1
#define ACP_PGFSM_STATUS_MASK                 0x3
#define ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK 0x10001
#define ACP_WOV_MISC_CTRL_MASK                0x10

#define ACP_COUNTER               20000
#define ACP_MEM_WINDOW_START      0x4000000
#define ACP_PAGE_SIZE_4K_ENABLE   0x2
#define ACP_PDM_DECIMATION_FACTOR 0x2
#define ACP_PDM_DISABLE           0x0
#define ACP_PDM_DMA_EN_STATUS     0x2
#define ACP_PDM_DMA_STAT          0x10
#define ACP_PDM_ENABLE            0x1
#define ACP_POWER_ON_IN_PROGRESS  0x1
#define ACP_POWERED_OFF           0x2
#define ACP_SRAM_PTE_OFFSET       0x2050000

#define BIT(n)           (1UL << (n))
#define cpu_relax()      asm volatile("rep; nop")
#define upper_32_bits(n) ((UInt32)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((UInt32)((n)&0xffffffff))

class AMDMicrophoneEngine;
class IOInterruptEventSource;
class IOPCIDevice;

class AMDMicrophoneDevice : public IOAudioDevice {
    OSDeclareDefaultStructors(AMDMicrophoneDevice);

    friend class AMDMicrophoneEngine;

    AMDMicrophoneEngine* audioEngine;
    IOInterruptEventSource* irqEventSource;
    IOPCIDevice* pciDevice;
    IOMemoryMap* baseAddrMap;
    IOVirtualAddress baseAddr;
    IOBufferMemoryDescriptor* dmaDescriptor;

    UInt32 readl(UInt32 reg);
    void writel(UInt32 val, UInt32 reg);

    bool createAudioEngine();
    int findMSIInterruptTypeIndex();
    static void interruptOccurred(OSObject* owner, IOInterruptEventSource* src, int intCount);

    int powerOff();
    int powerOn();
    int reset();

    bool checkDMAStatus();
    void configDMA();
    void enableClock();
    void initRingBuffer(UInt32 physAddr, UInt32 bufferSize, UInt32 watermarkSize);
    int startDMA();
    int stopDMA();

    void disableInterrupt();
    void enableInterrupt();

public:
    IOService* probe(IOService* provider, SInt32* score) override;
    bool initHardware(IOService* provider) override;
    void stop(IOService* provider) override;
    void free() override;
};

#endif /* AMDMicrophoneDevice_hpp */

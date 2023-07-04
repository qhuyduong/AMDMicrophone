//
//  AMDMicrophoneDevice.cpp
//  AMDMicrophone
//
//  Created by Huy Duong on 25/06/2023.
//

#include "AMDMicrophoneDevice.hpp"

#include "AMDMicrophoneCommon.hpp"
#include "AMDMicrophoneEngine.hpp"

#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(AMDMicrophoneDevice, IOAudioDevice);

UInt32 AMDMicrophoneDevice::readl(UInt32 reg)
{
    return *(const volatile UInt32*)(baseAddr + reg - ACP_PHY_BASE_ADDRESS);
}

void AMDMicrophoneDevice::writel(UInt32 val, UInt32 reg)
{
    *(volatile UInt32*)(baseAddr + reg - ACP_PHY_BASE_ADDRESS) = val;
}

bool AMDMicrophoneDevice::checkDMAStatus()
{
    bool pdmDMAStatus;
    UInt32 pdmEnable, pdmDMAEnable;

    pdmDMAStatus = false;
    pdmEnable = readl(ACP_WOV_PDM_ENABLE);
    pdmDMAEnable = readl(ACP_WOV_PDM_DMA_ENABLE);
    if ((pdmEnable & ACP_PDM_ENABLE) && (pdmDMAEnable & ACP_PDM_DMA_EN_STATUS))
        pdmDMAStatus = true;
    return pdmDMAStatus;
}

void AMDMicrophoneDevice::configDMA()
{
    UInt32 low, high, val;

    val = 0;

    /* Group Enable */
    writel(ACP_SRAM_PTE_OFFSET | BIT(31), ACP_AXI2AXI_ATU_BASE_ADDR_GRP_1);
    writel(ACP_PAGE_SIZE_4K_ENABLE, ACP_AXI2AXI_ATU_PAGE_SIZE_GRP_1);

    IOByteCount offset = 0;
    while (offset < dmaDescriptor->getLength()) {
        IOByteCount segmentLength = 0;
        addr64_t address = dmaDescriptor->getPhysicalSegment(offset, &segmentLength);

        low = lower_32_bits(address);
        high = upper_32_bits(address);

        writel(low, ACP_SCRATCH_REG_0 + val);
        high |= BIT(31);
        writel(high, ACP_SCRATCH_REG_0 + val + 4);
        offset += segmentLength;
        val += 8;
    }
}

void AMDMicrophoneDevice::disableInterrupt()
{
    writel(ACP_EXT_INTR_STAT_CLEAR_MASK, ACP_EXTERNAL_INTR_STAT);
    writel(0x0, ACP_EXTERNAL_INTR_ENB);
}

void AMDMicrophoneDevice::enableClock()
{
    UInt32 pdmClkEnable, pdmCtrl;

    pdmClkEnable = ACP_PDM_CLK_FREQ_MASK;
    writel(pdmClkEnable, ACP_WOV_CLK_CTRL);
    pdmCtrl = readl(ACP_WOV_MISC_CTRL);
    pdmCtrl |= ACP_WOV_MISC_CTRL_MASK;
    writel(pdmCtrl, ACP_WOV_MISC_CTRL);
}

void AMDMicrophoneDevice::enableInterrupt()
{
    writel(0x1, ACP_EXTERNAL_INTR_ENB);
    writel(ACP_PDM_DMA_INTR_MASK, ACP_EXTERNAL_INTR_CNTL);
}

UInt64 AMDMicrophoneDevice::getBytesCount()
{
    UInt32 low, high;
    UInt64 val;

    high = readl(ACP_WOV_RX_LINEARPOSITIONCNTR_HIGH);
    low = readl(ACP_WOV_RX_LINEARPOSITIONCNTR_LOW);

    val = ((UInt64)high << 32) | (UInt64)low;

    return val;
}

void AMDMicrophoneDevice::initRingBuffer(UInt32 physAddr, UInt32 bufferSize, UInt32 watermarkSize)
{
    writel(physAddr, ACP_WOV_RX_RINGBUFADDR);
    writel(bufferSize, ACP_WOV_RX_RINGBUFSIZE);
    writel(watermarkSize, ACP_WOV_RX_INTR_WATERMARK_SIZE);
    writel(0x1, ACP_AXI2AXI_ATU_CTRL);
}

int AMDMicrophoneDevice::powerOff()
{
    UInt32 val;
    int timeout;

    writel(ACP_PGFSM_CNTL_POWER_OFF_MASK, ACP_PGFSM_CONTROL);

    timeout = 0;
    while (++timeout < 500) {
        val = readl(ACP_PGFSM_STATUS);
        if ((val & ACP_PGFSM_STATUS_MASK) == ACP_POWERED_OFF)
            return 0;
        IODelay(1);
    }

    return kIOReturnTimeout;
}

int AMDMicrophoneDevice::powerOn()
{
    UInt32 val;
    int timeout;

    val = readl(ACP_PGFSM_STATUS);
    if (val == 0)
        return val;

    if ((val & ACP_PGFSM_STATUS_MASK) != ACP_POWER_ON_IN_PROGRESS) {
        writel(ACP_PGFSM_CNTL_POWER_ON_MASK, ACP_PGFSM_CONTROL);
    }

    timeout = 0;
    while (++timeout < 500) {
        val = readl(ACP_PGFSM_STATUS);
        if (!val)
            return 0;
        IODelay(1);
    }

    return kIOReturnTimeout;
}

int AMDMicrophoneDevice::reset()
{
    UInt32 val;
    int timeout;

    writel(1, ACP_SOFT_RESET);
    timeout = 0;
    while (++timeout < 500) {
        val = readl(ACP_SOFT_RESET);
        if (val & ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK)
            break;
        cpu_relax();
    }
    writel(0, ACP_SOFT_RESET);
    timeout = 0;
    while (++timeout < 500) {
        val = readl(ACP_SOFT_RESET);
        if (!val)
            return 0;
        cpu_relax();
    }
    return kIOReturnTimeout;
}

int AMDMicrophoneDevice::startDMA()
{
    UInt32 pdmEnable;
    UInt32 pdmDMAEnable;
    int timeout;

    pdmEnable = 0x1;
    pdmDMAEnable = 0x1;

    enableClock();
    writel(pdmEnable, ACP_WOV_PDM_ENABLE);
    writel(pdmDMAEnable, ACP_WOV_PDM_DMA_ENABLE);

    timeout = 0;
    while (++timeout < ACP_COUNTER) {
        pdmDMAEnable = readl(ACP_WOV_PDM_DMA_ENABLE);
        if ((pdmDMAEnable & 0x2) == ACP_PDM_DMA_EN_STATUS)
            return 0;
        IODelay(5);
    }

    return kIOReturnTimeout;
}

int AMDMicrophoneDevice::stopDMA()
{
    UInt32 pdmEnable;
    UInt32 pdmDMAEnable;
    int timeout;

    pdmEnable = readl(ACP_WOV_PDM_ENABLE);
    pdmDMAEnable = readl(ACP_WOV_PDM_DMA_ENABLE);
    if (pdmDMAEnable & 0x1) {
        pdmDMAEnable = 0x2;
        writel(pdmDMAEnable, ACP_WOV_PDM_DMA_ENABLE);
        timeout = 0;
        while (++timeout < ACP_COUNTER) {
            pdmDMAEnable = readl(ACP_WOV_PDM_DMA_ENABLE);
            if ((pdmDMAEnable & 0x2) == 0x0)
                break;
            IODelay(5);
        }
        if (timeout == ACP_COUNTER)
            return kIOReturnTimeout;
    }

    if (pdmEnable == ACP_PDM_ENABLE) {
        pdmEnable = ACP_PDM_DISABLE;
        writel(pdmEnable, ACP_WOV_PDM_ENABLE);
    }

    writel(0x1, ACP_WOV_PDM_FIFO_FLUSH);

    return 0;
}

bool AMDMicrophoneDevice::createAudioEngine()
{
    bool result = false;

    audioEngine = new AMDMicrophoneEngine;
    if (!audioEngine) {
        goto Done;
    }

    if (!audioEngine->init(this)) {
        goto Done;
    }

    if (activateAudioEngine(audioEngine) != kIOReturnSuccess) {
        goto Done;
    }

    result = true;

Done:
    if (audioEngine != NULL) {
        audioEngine->release();
    }

    return result;
}

int AMDMicrophoneDevice::findMSIInterruptTypeIndex()
{
    IOReturn ret;
    int index, source = 0;
    for (index = 0;; index++) {
        int interruptType;
        ret = pciDevice->getInterruptType(index, &interruptType);
        if (ret != kIOReturnSuccess)
            break;
        if (interruptType & kIOInterruptTypePCIMessaged) {
            source = index;
            break;
        }
    }
    return source;
}

void AMDMicrophoneDevice::interruptOccurred(OSObject* owner, IOInterruptEventSource* src, int intCount)
{
    AMDMicrophoneDevice* me = (AMDMicrophoneDevice*)owner;

    me->writel(ACP_EXT_INTR_STAT_CLEAR_MASK, ACP_EXTERNAL_INTR_STAT);
    me->audioEngine->takeTimeStamp();
}

IOService* AMDMicrophoneDevice::probe(IOService* provider, SInt32* score)
{
    pciDevice = OSDynamicCast(IOPCIDevice, provider);

    UInt8 revisionId = pciDevice->configRead8(kIOPCIConfigRevisionID);

    if (revisionId == 0x1) {
        LOG("AMD Digital Microphone for Renoir\n");
    } else {
        LOG("Only Renoir is supported at the moment\n");
        return NULL;
    }

    return this;
}

bool AMDMicrophoneDevice::initHardware(IOService* provider)
{
    bool result = false;
    IOWorkLoop* workLoop;

    if (!super::initHardware(provider)) {
        goto Done;
    }

    setDeviceName("AMD Digital Microphone");
    setDeviceShortName("AMDMicrophone");
    setManufacturerName("AMD");
    setDeviceTransportType(kIOAudioDeviceTransportTypePCI);

    pciDevice->setBusMasterEnable(true);
    pciDevice->setIOEnable(true);
    pciDevice->setMemoryEnable(true);

    baseAddrMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!baseAddrMap) {
        goto Done;
    }

    baseAddr = baseAddrMap->getVirtualAddress();
    if (!baseAddr) {
        goto Done;
    }

    dmaDescriptor = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, kIODirectionIn, BUFFER_SIZE);
    if (!dmaDescriptor) {
        goto Done;
    }

    workLoop = getWorkLoop();
    if (!workLoop) {
        goto Done;
    }

    irqEventSource = IOInterruptEventSource::interruptEventSource(
        this,
        (IOInterruptEventAction)&AMDMicrophoneDevice::interruptOccurred,
        provider,
        pciDevice->findPCICapability(kIOPCIMSICapability) ? findMSIInterruptTypeIndex() : 0
    );
    if (workLoop->addEventSource(irqEventSource) != kIOReturnSuccess) {
        goto Done;
    }
    irqEventSource->enable();

    if (!createAudioEngine()) {
        goto Done;
    }

    if (powerOn() != kIOReturnSuccess) {
        goto Done;
    }

    writel(0x1, ACP_CONTROL);
    if (reset() != kIOReturnSuccess) {
        goto Done;
    }
    writel(0x3, ACP_CLKMUX_SEL);

    configDMA();

    result = true;

Done:
    if (!result) {
        free();
    }

    return result;
}

void AMDMicrophoneDevice::stop(IOService* provider)
{
    irqEventSource->disable();
    if (reset() != kIOReturnSuccess) {
        return;
    }
    writel(0x0, ACP_CLKMUX_SEL);
    writel(0x0, ACP_CONTROL);
}

void AMDMicrophoneDevice::free()
{
    if (baseAddrMap) {
        baseAddrMap->release();
        baseAddrMap = NULL;
    }

    if (dmaDescriptor) {
        dmaDescriptor->release();
        dmaDescriptor = NULL;
    }

    super::free();
}
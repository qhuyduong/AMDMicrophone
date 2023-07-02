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

bool AMDMicrophoneDevice::createAudioEngine()
{
    bool result = false;

    LOG("createAudioEngine()\n");

    audioEngine = new AMDMicrophoneEngine;
    if (!audioEngine) {
        goto Done;
    }

    if (!audioEngine->init(this)) {
        goto Done;
    }

    if (activateAudioEngine(audioEngine) != kIOReturnSuccess) {
        LOG("ERROR activateAudioEngine failed\n");
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

    UInt32 val;

    val = readl(me->baseAddr + ACP_EXTERNAL_INTR_STAT);
    if (val & BIT(ACP_PDM_DMA_STAT)) {
        writel(BIT(ACP_PDM_DMA_STAT), me->baseAddr + ACP_EXTERNAL_INTR_STAT);
    }
}

void AMDMicrophoneDevice::disableInterrupts()
{
    writel(ACP_EXT_INTR_STAT_CLEAR_MASK, baseAddr + ACP_EXTERNAL_INTR_STAT);
    writel(0x00, baseAddr + ACP_EXTERNAL_INTR_ENB);
}

void AMDMicrophoneDevice::enableInterrupts()
{
    UInt32 extIntrCtrl;

    writel(0x01, baseAddr + ACP_EXTERNAL_INTR_ENB);
    extIntrCtrl = readl(baseAddr + ACP_EXTERNAL_INTR_CNTL);
    extIntrCtrl |= ACP_ERROR_MASK;
    writel(extIntrCtrl, baseAddr + ACP_EXTERNAL_INTR_CNTL);
}

int AMDMicrophoneDevice::powerOff()
{
    UInt32 val;
    int timeout;

    writel(ACP_PGFSM_CNTL_POWER_OFF_MASK, baseAddr + ACP_PGFSM_CONTROL);

    timeout = 0;
    while (++timeout < 500) {
        val = readl(baseAddr + ACP_PGFSM_STATUS);
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

    val = readl(baseAddr + ACP_PGFSM_STATUS);
    if (val == 0)
        return val;

    if ((val & ACP_PGFSM_STATUS_MASK) != ACP_POWER_ON_IN_PROGRESS) {
        writel(ACP_PGFSM_CNTL_POWER_ON_MASK, baseAddr + ACP_PGFSM_CONTROL);
    }

    timeout = 0;
    while (++timeout < 500) {
        val = readl(baseAddr + ACP_PGFSM_STATUS);
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

    writel(1, baseAddr + ACP_SOFT_RESET);
    timeout = 0;
    while (++timeout < 500) {
        val = readl(baseAddr + ACP_SOFT_RESET);
        if (val & ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK)
            break;
        cpu_relax();
    }
    writel(0, baseAddr + ACP_SOFT_RESET);
    timeout = 0;
    while (++timeout < 500) {
        val = readl(baseAddr + ACP_SOFT_RESET);
        if (!val)
            return 0;
        cpu_relax();
    }
    return kIOReturnTimeout;
}

bool AMDMicrophoneDevice::checkPDMDMAStatus()
{
    bool pdmDMAStatus;
    UInt32 pdmEnable, pdmDMAEnable;

    pdmDMAStatus = false;
    pdmEnable = readl(baseAddr + ACP_WOV_PDM_ENABLE);
    pdmDMAEnable = readl(baseAddr + ACP_WOV_PDM_DMA_ENABLE);
    if ((pdmEnable & ACP_PDM_ENABLE) && (pdmDMAEnable & ACP_PDM_DMA_EN_STATUS))
        pdmDMAStatus = true;
    return pdmDMAStatus;
}

void AMDMicrophoneDevice::disablePDMInterrupts()
{
    UInt32 extIntCtrl;

    extIntCtrl = readl(baseAddr + ACP_EXTERNAL_INTR_CNTL);
    extIntCtrl |= ~ACP_PDM_DMA_INTR_MASK;
    writel(extIntCtrl, baseAddr + ACP_EXTERNAL_INTR_CNTL);
}

void AMDMicrophoneDevice::enablePDMClock()
{
    UInt32 pdmClkEnable, pdmCtrl;

    pdmClkEnable = ACP_PDM_CLK_FREQ_MASK;
    writel(pdmClkEnable, baseAddr + ACP_WOV_CLK_CTRL);
    pdmCtrl = readl(baseAddr + ACP_WOV_MISC_CTRL);
    pdmCtrl |= ACP_WOV_MISC_CTRL_MASK;
    writel(pdmCtrl, baseAddr + ACP_WOV_MISC_CTRL);
}

void AMDMicrophoneDevice::enablePDMInterrupts()
{
    UInt32 extIntCtrl;

    extIntCtrl = readl(baseAddr + ACP_EXTERNAL_INTR_CNTL);
    extIntCtrl |= ACP_PDM_DMA_INTR_MASK;
    writel(extIntCtrl, baseAddr + ACP_EXTERNAL_INTR_CNTL);
}

void AMDMicrophoneDevice::initPDMRingBuffer(UInt32 physAddr, UInt32 bufferSize, UInt32 watermarkSize)
{
    writel(physAddr, baseAddr + ACP_WOV_RX_RINGBUFADDR);
    writel(bufferSize, baseAddr + ACP_WOV_RX_RINGBUFSIZE);
    writel(watermarkSize, baseAddr + ACP_WOV_RX_INTR_WATERMARK_SIZE);
    writel(0x01, baseAddr + ACP_AXI2AXI_ATU_CTRL);
}

int AMDMicrophoneDevice::startPDMDMA()
{
    UInt32 pdmEnable;
    UInt32 pdmDMAEnable;
    int timeout;

    pdmEnable = 0x01;
    pdmDMAEnable = 0x01;

    enablePDMClock();
    writel(pdmEnable, baseAddr + ACP_WOV_PDM_ENABLE);
    writel(pdmDMAEnable, baseAddr + ACP_WOV_PDM_DMA_ENABLE);

    timeout = 0;
    while (++timeout < ACP_COUNTER) {
        pdmDMAEnable = readl(baseAddr + ACP_WOV_PDM_DMA_ENABLE);
        if ((pdmDMAEnable & 0x02) == ACP_PDM_DMA_EN_STATUS)
            return 0;
        IODelay(5);
    }

    return kIOReturnTimeout;
}

int AMDMicrophoneDevice::stopPDMDMA()
{
    UInt32 pdmEnable;
    UInt32 pdmDMAEnable;
    int timeout;

    pdmEnable = readl(baseAddr + ACP_WOV_PDM_ENABLE);
    pdmDMAEnable = readl(baseAddr + ACP_WOV_PDM_DMA_ENABLE);
    if (pdmDMAEnable & 0x01) {
        pdmDMAEnable = 0x02;
        writel(pdmDMAEnable, baseAddr + ACP_WOV_PDM_DMA_ENABLE);
        timeout = 0;
        while (++timeout < ACP_COUNTER) {
            pdmDMAEnable = readl(baseAddr + ACP_WOV_PDM_DMA_ENABLE);
            if ((pdmDMAEnable & 0x02) == 0x00)
                break;
            IODelay(5);
        }
        if (timeout == ACP_COUNTER)
            return kIOReturnTimeout;
    }

    if (pdmEnable == ACP_PDM_ENABLE) {
        pdmEnable = ACP_PDM_DISABLE;
        writel(pdmEnable, baseAddr + ACP_WOV_PDM_ENABLE);
    }

    writel(0x01, baseAddr + ACP_WOV_PDM_FIFO_FLUSH);

    return 0;
}


IOService* AMDMicrophoneDevice::probe(IOService* provider, SInt32* score)
{
    pciDevice = OSDynamicCast(IOPCIDevice, provider);

    UInt8 revisionId = pciDevice->configRead8(kIOPCIConfigRevisionID);

    if (revisionId == 0x01) {
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

    dmaDescriptor = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, kIODirectionIn, kAudioSampleBufferSize);
    if (!dmaDescriptor) {
        LOG("ERROR: failed to allocate DMA buffer");
        goto Done;
    }

    workLoop = getWorkLoop();
    if (!workLoop) {
        goto Done;
    }

    interruptSource = IOInterruptEventSource::interruptEventSource(
        this,
        (IOInterruptEventAction)&AMDMicrophoneDevice::interruptOccurred,
        provider,
        findMSIInterruptTypeIndex()
    );
    if (workLoop->addEventSource(interruptSource) != kIOReturnSuccess) {
        goto Done;
    }

    if (!createAudioEngine()) {
        goto Done;
    }

    if (powerOn() != kIOReturnSuccess) {
        LOG("power on failed\n");
        goto Done;
    }

    writel(0x01, baseAddr + ACP_CONTROL);
    if (reset() != kIOReturnSuccess) {
        LOG("reset failed\n");
        goto Done;
    }
    writel(0x03, baseAddr + ACP_CLKMUX_SEL);
    enableInterrupts();

    result = true;

Done:
    if (!result) {
        free();
    }

    return result;
}

void AMDMicrophoneDevice::stop(IOService* provider)
{
    disableInterrupts();
    if (reset() != kIOReturnSuccess) {
        LOG("reset failed\n");
        return;
    }
    writel(0x00, baseAddr + ACP_CLKMUX_SEL);
    writel(0x00, baseAddr + ACP_CONTROL);
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
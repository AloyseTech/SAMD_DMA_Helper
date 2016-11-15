#ifndef _DMA_HELPER_
#define _DMA_HELPER_

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t btctrl;    //Block Transfer Control
  uint16_t btcnt;     //Block Transfer Count
  uint32_t srcaddr;   //Source Address
  uint32_t dstaddr;   //Destination Address
  uint32_t descaddr;  //Next Descriptor Address
} dmacdescriptor;

//Write Back Descriptor (x12)
volatile dmacdescriptor descriptor_wrb[12] __attribute__((aligned(16)));

//DMA Channels Descriptor (x12)
dmacdescriptor descriptor_section[12] __attribute__((aligned(16)));

//DMA Channel Interrupt Result (x12)
volatile uint32_t dmaChannelDone[12];

//DMA Channel Callback (x12)
void(*dma_callback[12])(void) = { NULL };

//DMAC already initialized
uint32_t dmac_initialized = 0;

//DMAC_Handler : Interrupt Service Routine :
//override DMAC_Handler (attribute is weak in Reset_Handler.c)
void DMAC_Handler() {
  uint8_t active_channel;

  //Disable global interrupts
  __disable_irq();

  //Get the triggering channel number
  active_channel = DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk;
  DMAC->CHID.reg = DMAC_CHID_ID(active_channel);

  //Get Channel Result Flags
  //DMAC_CHINTENCLR_TERR  = 0x01    =>Transfer Error
  //DMAC_CHINTENCLR_TCMPL = 0x02    =>Transfer Complete
  //DMAC_CHINTENCLR_SUSP  = 0x04    =>Transfer Suspended
  dmaChannelDone[active_channel] = DMAC->CHINTFLAG.reg;

  //Execute calback if registered
  if (dma_callback[active_channel] != NULL)
    dma_callback[active_channel]();

  //Clear interrupt flags
  DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TCMPL; // clear
  DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TERR;
  DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_SUSP;

  //Enable global interrupts
  __enable_irq();
}

void DMAC_Init()
{
  if (!dmac_initialized)
  {
    PM->AHBMASK.reg |= PM_AHBMASK_DMAC;
    PM->APBBMASK.reg |= PM_APBBMASK_DMAC;
    NVIC_EnableIRQ(DMAC_IRQn);

    DMAC->BASEADDR.reg = (uint32_t)descriptor_section;
    DMAC->WRBADDR.reg = (uint32_t)descriptor_wrb;
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);
  }
}

void DMAC_End()
{
  if (dmac_initialized)
  {
    DMAC->CTRL.bit.DMAENABLE = 0;        // disable DMA controller
    DMAC->CTRL.bit.SWRST = 1;           // reset all DMA registers
    while (DMAC->CTRL.bit.SWRST) {};    // wait for reset to complete
  }
}

#ifdef __cplusplus
}
#endif

#endif //_DMA_HELPER_

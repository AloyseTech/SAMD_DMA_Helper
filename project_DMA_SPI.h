#ifndef _DMA_SPI_
#define _DMA_SPI_

#include "Arduino.h"
#include "project_DMA_Helper.h"


class DMA_SPI
{
  public:
    DMA_SPI(Sercom *s, uint32_t txc, uint32_t rxc, uint8_t txtrig, uint8_t rxtrig)
    {
      sercom = s;
      chnltx = txc;
      chnlrx = rxc;
      txTrigger = txtrig;
      rxTrigger = rxtrig;
    }

    Sercom *sercom;
    uint32_t chnltx, chnlrx; // DMA channels
    uint8_t txTrigger, rxTrigger;
    dmacdescriptor descriptor __attribute__((aligned(16)));

    void registerCallbacks(void(*tx)(), void(*rx)())
    {
      dma_callback[chnltx] = tx;
      dma_callback[chnlrx] = rx;
    }
    void registerRXCallbacks(void(*rx)())
    {
      dma_callback[chnlrx] = rx;
    }
    void registerTXCallbacks(void(*tx)())
    {
      dma_callback[chnltx] = tx;
    }

    void init()
    {
      DMAC_Init();
      txsrc[0] = 0xff;
      xfr(txsrc, rxsink, 1);
      while (!dmaChannelDone[chnltx] || !dmaChannelDone[chnlrx]);
      disable();
    }


    void transfer(void *txdata, void *rxdata, size_t n)
    {
      xtype = DoTXRX;
      xfr(txdata, rxdata, n);
    }
    void read(void *data, size_t n)
    {
      xtype = DoRX;
      xfr(txsrc, data, n);
    }
    void write(void *data, size_t n)
    {
      xtype = DoTX;
      xfr(data, rxsink, n);
    }

    bool transferDone()
    {
      return (dmaChannelDone[chnltx] || dmaChannelDone[chnlrx]);
    }

    void disable()
    {
      __disable_irq();
      DMAC->CHID.reg = DMAC_CHID_ID(chnltx);   //disable DMA to allow lib SPI
      DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
      DMAC->CHID.reg = DMAC_CHID_ID(chnlrx);
      DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
      __enable_irq();
    }

  private:
    enum XfrType { DoTX, DoRX, DoTXRX };
    XfrType xtype;
    uint8_t rxsink[1], txsrc[1];
    void(*callbackTX)(void);
    void(*callbackRX)(void);

    void xfr(void *txdata, void *rxdata, size_t n)
    {
      __disable_irq();

      uint32_t temp_CHCTRLB_reg;

      // set up transmit channel
      DMAC->CHID.reg = DMAC_CHID_ID(chnltx);
      DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
      DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
      DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << chnltx));
      temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) |
                         DMAC_CHCTRLB_TRIGSRC(txTrigger) | DMAC_CHCTRLB_TRIGACT_BEAT;
      DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;
      DMAC->CHINTENSET.reg = DMAC_CHINTENSET_MASK; // enable all 3 interrupts
      dmaChannelDone[chnltx] = 0;
      descriptor.descaddr = 0;
      descriptor.dstaddr = (uint32_t)&sercom->SPI.DATA.reg;
      descriptor.btcnt = n;
      descriptor.srcaddr = (uint32_t)txdata;
      descriptor.btctrl = DMAC_BTCTRL_VALID;
      if (xtype != DoRX) {
        descriptor.srcaddr += n;
        descriptor.btctrl |= DMAC_BTCTRL_SRCINC;
      }
      //copy all the descriptor data at once
      memcpy(&descriptor_section[chnltx], &descriptor, sizeof(dmacdescriptor));

      // rx channel
      DMAC->CHID.reg = DMAC_CHID_ID(chnlrx);
      DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
      DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
      DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << chnlrx));
      temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) |
                         DMAC_CHCTRLB_TRIGSRC(rxTrigger) | DMAC_CHCTRLB_TRIGACT_BEAT;
      DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;
      DMAC->CHINTENSET.reg = DMAC_CHINTENSET_MASK; // enable all 3 interrupts
      dmaChannelDone[chnlrx] = 0;
      descriptor.descaddr = 0;
      descriptor.srcaddr = (uint32_t)&sercom->SPI.DATA.reg;
      descriptor.btcnt = n;
      descriptor.dstaddr = (uint32_t)rxdata;
      descriptor.btctrl = DMAC_BTCTRL_VALID;
      if (xtype != DoTX) {
        descriptor.dstaddr += n;
        descriptor.btctrl |= DMAC_BTCTRL_DSTINC;
      }
      memcpy(&descriptor_section[chnlrx], &descriptor, sizeof(dmacdescriptor));

      // start both channels
      DMAC->CHID.reg = DMAC_CHID_ID(chnltx);
      DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;
      DMAC->CHID.reg = DMAC_CHID_ID(chnlrx);
      DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

      __enable_irq();
    }
};

#endif //_DMA_SPI_


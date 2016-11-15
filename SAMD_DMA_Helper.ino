#include <SPI.h>
#include "project_DMA_SPI.h"


DMA_SPI DMASPI(SERCOM2, 0, 1, SERCOM2_DMAC_ID_TX, SERCOM2_DMAC_ID_RX);

uint8_t buffer[256] = {0};

void setup()
{
  while (!SerialUSB);

  SPI.begin();
  SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0));

  //DMA setup for SPI transaction, arduino SPI class init must have been called already
  DMASPI.init();

  DMASPI.registerTXCallbacks(callback);

  for (int i = 0; i < 128; i++)
  {
    for (int i = 0; i < 256; i++)
      buffer[i] = random(0xFF);
    while (!DMASPI.transferDone());
    DMASPI.write(buffer, 256);
  }
  //Allow standard SPI use again
  DMASPI.disable();


}

void loop()
{

}

static bool led_state = true;
void callback()
{
  //blink led during transfer to prove callbacks are working
  digitalWrite(LED_BUILTIN, led_state);
  led_state = !led_state;
  delayMicroseconds(50000);
}


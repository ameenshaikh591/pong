/*
Author: Ameen Shaikh

Source code for SPI.

Sets up SPI, and also implements the custom SPI transaction understood by the FPGA.
*/

#include "spi_driver.h"
#include <Arduino.h>

#define SCLK_PIN 4
#define MISO_PIN_UNUSED 5
#define MOSI_PIN 6
#define CS_PIN 7
#define SPI_CLK_HZ 100000

SPIClass* vspi = nullptr;

/*
Sets up SPI
1. Assigns pins
2. Instantiates a SPIClass object
*/
void SetupSPI() {
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, LOW); // CS pin is driven low initially

  vspi = new SPIClass(SPI);
  vspi->begin(SCLK_PIN, MISO_PIN_UNUSED, MOSI_PIN, CS_PIN);
  delay(250); // Some time to setup SPI
}

/*
Implements the custom SPI transaction understood by the FPGA
1. Drives CS HIGH (Alerts FPGA of incoming transaction)
2. Delay to allow FPGA to register CS going HIGH
3. Ten bytes of data are sent in the transaction
4. Optional delay
5. Drives CS LOW (Alerts FPGA that transaction has ended)
*/
void SPITransaction(SPIClass* spi_driver, game_state_t* g) {
  spi_driver->beginTransaction(SPISettings(SPI_CLK_HZ, MSBFIRST, SPI_MODE0));

  digitalWrite(CS_PIN, HIGH);
  delayMicroseconds(1);

  spi_driver->transfer((uint8_t)(g->ball_x & 0xFF));
  spi_driver->transfer((uint8_t)(g->ball_x >> 8));

  spi_driver->transfer((uint8_t)(g->ball_y & 0xFF));
  spi_driver->transfer((uint8_t)(g->ball_y >> 8));

  spi_driver->transfer((uint8_t)(g->paddle_l_y & 0xFF));
  spi_driver->transfer((uint8_t)(g->paddle_l_y >> 8));

  spi_driver->transfer((uint8_t)(g->paddle_r_y & 0xFF));
  spi_driver->transfer((uint8_t)(g->paddle_r_y >> 8));

  spi_driver->transfer(g->score_l);
  spi_driver->transfer(g->score_r);

  delayMicroseconds(1);
  digitalWrite(CS_PIN, LOW);

  spi_driver->endTransaction();
}
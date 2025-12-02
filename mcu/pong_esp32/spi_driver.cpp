#include "spi_driver.h"
#include <Arduino.h>

#define SCLK_PIN 4
#define MISO_PIN_UNUSED 5
#define MOSI_PIN 6
#define CS_PIN 7
#define SPI_CLK_HZ 100000

SPIClass* vspi = nullptr;

void SetupSPI() {
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, LOW);

  vspi = new SPIClass(SPI);
  vspi->begin(SCLK_PIN, MISO_PIN_UNUSED, MOSI_PIN, CS_PIN);
  delay(250); // Some time to set-up SPI
}

void SPITransaction(SPIClass* spi_driver, game_state_t* g) {
  spi_driver->beginTransaction(SPISettings(SPI_CLK_HZ, MSBFIRST, SPI_MODE0));

  digitalWrite(CS_PIN, HIGH); // ASSERT CS (active high)
  delayMicroseconds(1); // allow FPGA to detect CS high

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
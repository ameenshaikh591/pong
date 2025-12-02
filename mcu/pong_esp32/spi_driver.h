#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "game_types.h"

extern SPIClass* vspi;

void SetupSPI();
void SPITransaction(SPIClass* spi_driver, game_state_t* g);
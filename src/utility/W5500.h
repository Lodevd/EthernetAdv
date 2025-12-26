/*
 * Copyright 2018 Paul Stoffregen
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * Copyright 2025 Lode Van Dyck
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

// w5100.h contains private W5x00 hardware "driver" level definitions
// which are not meant to be exposed to other libraries or Arduino users

#ifndef	W5500_H_INCLUDED
#define	W5500_H_INCLUDED

#include <Arduino.h>
#include <SPI.h>
#include "W5x00.h"

// Safe for all chips
//#define SPI_ETHERNET_SETTINGS SPISettings(14000000, MSBFIRST, SPI_MODE0)

// Safe for W5200 and W5500, but too fast for W5100
// Uncomment this if you know you'll never need W5100 support.
//  Higher SPI clock only results in faster transfer to hosts on a LAN
//  or with very low packet latency.  With ordinary internet latency,
//  the TCP window size & packet loss determine your overall speed.
#define SPI_ETHERNET_SETTINGS SPISettings(30000000, MSBFIRST, SPI_MODE0)

// Arduino 101's SPI can not run faster than 8 MHz.
#if defined(ARDUINO_ARCH_ARC32)
#undef SPI_ETHERNET_SETTINGS
#define SPI_ETHERNET_SETTINGS SPISettings(8000000, MSBFIRST, SPI_MODE0)
#endif

class W5500Class: public W5x00Class {

  // Interface functions that need to be impelmented
  // -------------------------

public:

  W5500Class(SPIClass &spi, uint8_t sspin, uint8_t maxSockNum);

  W5500Class(SPIClass &spi, uint8_t sspin);

  uint8_t init(void);

  W5x00Linkstatus getLinkStatus();

  void beginTransaction();

  uint16_t SBASE(uint8_t socknum) {
    return socknum * SSIZE + 0x8000;
  }
  uint16_t RBASE(uint8_t socknum) {
    return socknum * SSIZE + 0xC000;
  }

  const bool hasOffsetAddressMapping(){return true;}

private:

  uint16_t write(uint16_t addr, const uint8_t *buf, uint16_t len);

  uint16_t read(uint16_t addr, uint8_t *buf, uint16_t len);

};

#endif
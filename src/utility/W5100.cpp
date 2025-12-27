/*
 * Copyright 2018 Paul Stoffregen
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <Arduino.h>
#include "EthernetAdv.h"
#include "w5100.h"

W5100Class::W5100Class(SPIClass &spi, uint8_t sspin, uint8_t maxSockNum){
	this->spi = &spi;
	ss_pin = sspin;
	_maxSockNum = 4; // Max for W5100
	if(maxSockNum < _maxSockNum){_maxSockNum = maxSockNum;}
}

W5100Class::W5100Class(SPIClass &spi, uint8_t sspin){
	this->spi = &spi;
	ss_pin = sspin;
	_maxSockNum = 4; // Max for W5100
}

// Chip dependant
uint8_t W5100Class::init(void)
{
	if (initialized) return 1;

	CH_BASE_MSB = 0x04;
	uint8_t i;

	// Many Ethernet shields have a CAT811 or similar reset chip
	// connected to W5100 or W5200 chips.  The W5200 will not work at
	// all, and may even drive its MISO pin, until given an active low
	// reset pulse!  The CAT811 has a 240 ms typical pulse length, and
	// a 400 ms worst case maximum pulse length.  MAX811 has a worst
	// case maximum 560 ms pulse length.  This delay is meant to wait
	// until the reset pulse is ended.  If your hardware has a shorter
	// reset time, this can be edited or removed.
	delay(560);
	//Serial.println("w5100 init");

	//SPI.begin();	This should be done outside of the class
	initSS();
	resetSS();
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
	
	// Try a soft reset, if this works a chip is present. 
	if (softReset()){
		// Buffer size based on number of sockets 
		if(_maxSockNum <= 1){
			SSIZE = 8192;
			writeTMSR(0x03);
			writeRMSR(0x03);
		}else if(_maxSockNum <= 2){
			SSIZE = 4096;
			writeTMSR(0x0A);
			writeRMSR(0x0A);
		}else{
			SSIZE = 2048;
			writeTMSR(0x55);
			writeRMSR(0x55);
		}
		SMASK = SSIZE - 1;

	// No hardware seems to be present.  Or it could be a W5200
	// that's heard other SPI communication if its chip select
	// pin wasn't high when a SD card or other SPI chip was used.
	} else {
		//Serial.println("no chip :-(");
		spi->endTransaction();
		return 0; // no known chip is responding :-(
	}
	spi->endTransaction();
	initialized = true;
	//Serial.println("w5100 Initialized");
	return 1; // successful init
}

W5x00Linkstatus W5100Class::getLinkStatus()
{
	return UNKNOWN;
}

// Chip dependant
uint16_t W5100Class::write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
	uint8_t cmd[8];

	for (uint16_t i=0; i<len; i++) {
		setSS();
		spi->transfer(0xF0);
		spi->transfer(addr >> 8);
		spi->transfer(addr & 0xFF);
		addr++;
		spi->transfer(buf[i]);
		resetSS();
	}
	return len;
}

// Chip dependant
uint16_t W5100Class::read(uint16_t addr, uint8_t *buf, uint16_t len)
{
	uint8_t cmd[4];

	for (uint16_t i=0; i < len; i++) {
		setSS();
		#if 1
		spi->transfer(0x0F);
		spi->transfer(addr >> 8);
		spi->transfer(addr & 0xFF);
		addr++;
		buf[i] = spi->transfer(0);
		#else
		cmd[0] = 0x0F;
		cmd[1] = addr >> 8;
		cmd[2] = addr & 0xFF;
		cmd[3] = 0;
		spi->transfer(cmd, 4); // TODO: why doesn't this work?
		buf[i] = cmd[3];
		addr++;
		#endif
		resetSS();
	}
	return len;
}

// Generic
void W5100Class::beginTransaction(){
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
}

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
#include "w5200.h"

// Generic
W5200Class::W5200Class(SPIClass &spi, uint8_t sspin, uint8_t maxSockNum){
	this->spi = &spi;
	ss_pin = sspin;
	_maxSockNum = 8; // Max for W5200
	if(maxSockNum < _maxSockNum){_maxSockNum = maxSockNum;}
}

W5200Class::W5200Class(SPIClass &spi, uint8_t sspin){
	this->spi = &spi;
	ss_pin = sspin;
	_maxSockNum = 8; // Max for W5200
}

// Chip dependant
uint8_t W5200Class::init(void)
{
	if (initialized) return 1;

	CH_BASE_MSB = 0x40;
	uint8_t i;

	// Many Ethernet shields have a CAT811 or similar reset chip
	// connected to W5200 or W5200 chips.  The W5200 will not work at
	// all, and may even drive its MISO pin, until given an active low
	// reset pulse!  The CAT811 has a 240 ms typical pulse length, and
	// a 400 ms worst case maximum pulse length.  MAX811 has a worst
	// case maximum 560 ms pulse length.  This delay is meant to wait
	// until the reset pulse is ended.  If your hardware has a shorter
	// reset time, this can be edited or removed.
	delay(560);
	//Serial.println("W5200 init");

	//SPI.begin();	This should be done outside of the class
	initSS();
	resetSS();
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);

	// Attempt W5200 detection first, because W5200 does not properly
	// reset its SPI state when CS goes high (inactive).  Communication
	// from detecting the other chips can leave the W5200 in a state
	// where it won't recover, unless given a reset pulse.
	
	// Try a soft reset, if this works a chip is present. 
	if (softReset()){
		// Buffer size based on number of sockets 
		if(_maxSockNum <= 1){
			SSIZE = 16384;
		}else if(_maxSockNum <= 2){
			SSIZE = 8192;
		}else if(_maxSockNum <= 4){
			SSIZE = 4096;
		}else{
			SSIZE = 2048;
		}
		SMASK = SSIZE - 1;

		for (i=0; i<_maxSockNum; i++) {
			writeSnRX_SIZE(i, SSIZE >> 10);
			writeSnTX_SIZE(i, SSIZE >> 10);
		}
		for (; i<8; i++) {
			writeSnRX_SIZE(i, 0);
			writeSnTX_SIZE(i, 0);
		}

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

W5x00Linkstatus W5200Class::getLinkStatus()
{
	uint8_t phystatus;

	// Initialization not possible
	if (!init()) return UNKNOWN;

	// Get status
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
	phystatus = readPSTATUS_W5200();
	spi->endTransaction();
	if (phystatus & 0x20) return LINK_ON;
	return LINK_OFF;
}

// Chip dependant
uint16_t W5200Class::write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
	uint8_t cmd[8];

	setSS();
	cmd[0] = addr >> 8;
	cmd[1] = addr & 0xFF;
	cmd[2] = ((len >> 8) & 0x7F) | 0x80;
	cmd[3] = len & 0xFF;
	spi->transfer(cmd, 4);
#ifdef SPI_HAS_TRANSFER_BUF
	spi->transfer(buf, NULL, len);
#else
	// TODO: copy 8 bytes at a time to cmd[] and block transfer
	for (uint16_t i=0; i < len; i++) {
		spi->transfer(buf[i]);
	}
#endif
	resetSS();
	return len;
}

// Chip dependant
uint16_t W5200Class::read(uint16_t addr, uint8_t *buf, uint16_t len)
{
	uint8_t cmd[4];

	setSS();
	cmd[0] = addr >> 8;
	cmd[1] = addr & 0xFF;
	cmd[2] = (len >> 8) & 0x7F;
	cmd[3] = len & 0xFF;
	spi->transfer(cmd, 4);
	memset(buf, 0, len);
	spi->transfer(buf, len);
	resetSS();

	return len;
}

// Generic
void W5200Class::beginTransaction(){
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
}

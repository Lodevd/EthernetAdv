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

#include <Arduino.h>
#include "EthernetAdv.h"
#include "w5500.h"

// Generic
W5500Class::W5500Class(SPIClass &spi, uint8_t sspin, uint8_t maxSockNum){
	this->spi = &spi;
	ss_pin = sspin;
	_maxSockNum = 8; // Max for W5500
	if(maxSockNum < _maxSockNum){_maxSockNum = maxSockNum;}
}

W5500Class::W5500Class(SPIClass &spi, uint8_t sspin){
	this->spi = &spi;
	ss_pin = sspin;
	_maxSockNum = 8; // Max for W5500
}

// Chip dependant
uint8_t W5500Class::init(void)
{
	if (_initialized) return 1;
	
	CH_BASE_MSB = 0x10;
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
	//Serial.println("w5500 init");

	//SPI.begin();	This should be done outside of the class
	initSS();
	resetSS();
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
	
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
	_initialized = true;
	//Serial.println("w5500 Initialized");
	return 1; // successful init
}

W5x00Linkstatus W5500Class::getLinkStatus()
{
	uint8_t phystatus;

	// Initialization not possible
	if (!init()) return UNKNOWN;

	// Get status
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
	phystatus = readPHYCFGR_W5500();
	spi->endTransaction();
	if (phystatus & 0x01) return LINK_ON;
	return LINK_OFF;
}

// Chip dependant
uint16_t W5500Class::write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
	uint8_t cmd[8];

	setSS();
	if (addr < 0x100) {
		// common registers 00nn
		cmd[0] = 0;
		cmd[1] = addr & 0xFF;
		cmd[2] = 0x04;
	} else if (addr < 0x8000) {
		// socket registers  10nn, 11nn, 12nn, 13nn, etc
		cmd[0] = 0;
		cmd[1] = addr & 0xFF;
		cmd[2] = ((addr >> 3) & 0xE0) | 0x0C;
	} else if (addr < 0xC000) {
		// transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
		//  10## #nnn nnnn nnnn
		cmd[0] = addr >> 8;
		cmd[1] = addr & 0xFF;

		if(_maxSockNum <= 1){
			cmd[2] = 0x14;  
		}else if(_maxSockNum <= 2){
			cmd[2] = ((addr >> 8) & 0x20) | 0x14; // 8K buffers
		}else if(_maxSockNum <= 4){
			cmd[2] = ((addr >> 7) & 0x60) | 0x14; // 4K buffers
		}else{
			cmd[2] = ((addr >> 6) & 0xE0) | 0x14; // 2K buffers
		}

	} else {
		// receive buffers
		cmd[0] = addr >> 8;
		cmd[1] = addr & 0xFF;

		if(_maxSockNum <= 1){
			cmd[2] = 0x1C;    
		}else if(_maxSockNum <= 2){
			cmd[2] = ((addr >> 8) & 0x20) | 0x1C; // 8K buffers
		}else if(_maxSockNum <= 4){
			cmd[2] = ((addr >> 7) & 0x60) | 0x1C; // 4K buffers
		}else{
			cmd[2] = ((addr >> 6) & 0xE0) | 0x1C; // 2K buffers
		}

	}
	if (len <= 5) {
		for (uint8_t i=0; i < len; i++) {
			cmd[i + 3] = buf[i];
		}
		spi->transfer(cmd, len + 3);
	} else {
		spi->transfer(cmd, 3);
#ifdef SPI_HAS_TRANSFER_BUF
		spi->transfer(buf, NULL, len);
#else
		// TODO: copy 8 bytes at a time to cmd[] and block transfer
		for (uint16_t i=0; i < len; i++) {
			spi->transfer(buf[i]);
		}
#endif
	}
	resetSS();

	return len;
}

// Chip dependant
uint16_t W5500Class::read(uint16_t addr, uint8_t *buf, uint16_t len)
{
	uint8_t cmd[4];

	setSS();
	if (addr < 0x100) {
		// common registers 00nn
		cmd[0] = 0;
		cmd[1] = addr & 0xFF;
		cmd[2] = 0x00;
	} else if (addr < 0x8000) {
		// socket registers  10nn, 11nn, 12nn, 13nn, etc
		cmd[0] = 0;
		cmd[1] = addr & 0xFF;
		cmd[2] = ((addr >> 3) & 0xE0) | 0x08;
	} else if (addr < 0xC000) {
		// transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
		//  10## #nnn nnnn nnnn
		cmd[0] = addr >> 8;
		cmd[1] = addr & 0xFF;

		if(_maxSockNum <= 1){
			cmd[2] = 0x10;                       // 16K buffers
		}else if(_maxSockNum <= 2){
			cmd[2] = ((addr >> 8) & 0x20) | 0x10; // 8K buffers
		}else if(_maxSockNum <= 4){
			cmd[2] = ((addr >> 7) & 0x60) | 0x10; // 4K buffers
		}else{
			cmd[2] = ((addr >> 6) & 0xE0) | 0x10; // 2K buffers
		}

	} else {
		// receive buffers
		cmd[0] = addr >> 8;
		cmd[1] = addr & 0xFF;
		
		if(_maxSockNum <= 1){
			cmd[2] = 0x18;                       // 16K buffers
		}else if(_maxSockNum <= 2){
			cmd[2] = ((addr >> 8) & 0x20) | 0x18; // 8K buffers
		}else if(_maxSockNum <= 4){
			cmd[2] = ((addr >> 7) & 0x60) | 0x18; // 4K buffers
		}else{
			cmd[2] = ((addr >> 6) & 0xE0) | 0x18; // 2K buffers
		}
	}
	spi->transfer(cmd, 3);
	memset(buf, 0, len);
	spi->transfer(buf, len);
	resetSS();
	return len;
}

// Generic
void W5500Class::beginTransaction(){
	spi->beginTransaction(SPI_ETHERNET_SETTINGS);
}

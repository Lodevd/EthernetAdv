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
#include "Ethernet.h"
#include "W5x00.h"

// Generic
// Soft reset the WIZnet chip, by writing to its MR register reset bit
uint8_t W5x00Class::softReset(void)
{
	uint16_t count=0;

	//Serial.println("WIZnet soft reset");
	// write to reset bit
	writeMR(0x80);
	// then wait for soft reset to complete
	do {
		uint8_t mr = readMR();
		//Serial.print("mr=");
		//Serial.println(mr, HEX);
		if (mr == 0) return 1;
		delay(1);
	} while (++count < 20);
	return 0;
}

// Generic
void W5x00Class::execCmdSn(SOCKET s, SockCMD _cmd)
{
	// Send command to socket
	writeSnCR(s, _cmd);
	// Wait for command to complete
	while (readSnCR(s)) ;
}

// Generic
void W5x00Class::endTransaction(){
	spi->endTransaction();
}

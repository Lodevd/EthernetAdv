# EthernetAdv Library for Arduino(c) #

> **Note:** 
> !!! This project is under construction, use at own risk !!!
> For now only W5500 is supported and only tested with ESP32S3. Like in the example below. 
> I'll try adding uno with W5100 in the near future.

This is a reworked version of the Arduino(c) Ethernet library.
The Ethernet library makes Ethernet communication using WIZnet(c) Ethernet chips W5100, W5200 and W5500.
This version aims to give more flexibility to the programmer.
The main advantages of this library are:
- Possibility for having multiple Ethernet interfaces.
- Flexible SPI bus assignement for each interface.
- Configurable amount of sockets on each interface. 

As you will see in the examples, gaining flexibility comes at the cost of (a little) more complex configuration.
You will need to know, what chip you are using, what SPI bus it is connected to and what pins you are using.

The original library can be found at:
https://www.arduino.cc/en/Reference/Ethernet

## Library diagram ##

In the original library there are main communication principles between the library classes, and the classes with the Ethernet chip. 
But there where not strictly implemented (annymore). During reworking I tried correcting the library to strictly match the diagram below. 

![Class diagram](https://raw.githubusercontent.com/Lodevd/EthernetAdv/refs/heads/master/docs/EthernetLibrary.svg)

## Example usage ##

### Uno, Mega with W5100 Shield ###
```C++

TODO();

```

### Compatibility with original library ###
```C++

// You can name your ethernet interface annyway you want.
// But if you want compatibility with other libraries using 
// the original Ethernet library you need to name it 'Ethernet' like this.

W5500Class chip(SPI,10);          // Use the W5100Class, W5200Class or W5500Class depending on the chip you are using. 
EthernetClass Ethernet(chip);      // For compatibility with the original library the name ' Ethernet' is important here. 

```

### ESP32S3 with W5500 Shield and dedicated pins ###
```C++

#include <SPI.h>
#include <Ethernet.h>       

// Ethernet interface settings
byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress myip(192, 168, 10, 25);

// SPI pins
#define ETH_SPI_CS   10       // Chip Select for W5500
#define ETH_SPI_MISO 13       // SPI2 (HSPI)
#define ETH_SPI_MOSI 11       // SPI2 (HSPI)
#define ETH_SPI_SCK 12        // SPI2 (HSPI)

// Class instances
SPIClass SPI2(HSPI);
W5500Class w5500(SPI2,ETH_SPI_CS);
EthernetClass eth(w5500);
EthernetClient client(eth);

void setup() {
    // Setup for the SPI bus must be added manualy.
    // This is different from the original library.
    SPI2.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
    // Setup for the W5500 chip
    w5500.init();
    // Setup for the Ethernet Interface
    eth.begin(mac, myip);
    // Allow the hardware to sort itself out
    delay(1500);
}

```

## License ##

Copyright (c) 2025 Lode Van Dyck. All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA


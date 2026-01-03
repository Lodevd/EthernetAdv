/*
  Link Status

  This sketch prints the Ethernet link status. When the
  Ethernet cable is connected the link status should go to "ON".
  NOTE: Only WIZnet W5200 and W5500 are capable of reporting
  the link status. W5100 will report "Unknown".
  Hardware:
   - Ethernet shield or equivalent board/shield with WIZnet W5200/W5500
  Written by Cristian Maglie
  Modified to use the EthernetAdv library 3 Jan 2026
  by Lode Van Dyck

  This example is public domain.
*/

#include <SPI.h>
#include <EthernetAdv.h>

#define W5100_CS_PIN 10
W5100Class w5100(SPI,W5100_CS_PIN);         // Use the W5100Class, W5200Class or W5500Class depending on the chip you are using. 
EthernetClass Ethernet(w5100);

void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH Shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  Serial.begin(9600);
}

void loop() {
  auto link = Ethernet.linkStatus();
  Serial.print("Link status: ");
  switch (link) {
    case Unknown:
      Serial.println("Unknown");
      break;
    case LinkON:
      Serial.println("ON");
      break;
    case LinkOFF:
      Serial.println("OFF");
      break;
  }
  delay(1000);
}

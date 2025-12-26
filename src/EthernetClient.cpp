/* Copyright 2018 Paul Stoffregen
 * Copyright 2025 Lode Van Dyck
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>
#include "EthernetAdv.h"
#include "Dns.h"

EthernetClient::EthernetClient(EthernetClass &ethernet){
	_timeout = 1000;
	_eth = & ethernet;
	_sockindex = _eth->maxSocketNum();
}

EthernetClient::EthernetClient(EthernetClass &ethernet, uint8_t s){
	_sockindex = s;
	_timeout = 1000;
	_eth = & ethernet;
}
 
int EthernetClient::connect(const char * host, uint16_t port)
{
	DNSClient dns(*_eth); // Look up the host first
	IPAddress remote_addr;

	if (_sockindex < _eth->maxSocketNum()) {
		if (_eth->socketStatus(_sockindex) != SnSR::CLOSED) {
			_eth->socketDisconnect(_sockindex); // TODO: should we call stop()?
		}
		_sockindex = _eth->maxSocketNum();
	}
	dns.begin(_eth->dnsServerIP());
	if (!dns.getHostByName(host, remote_addr)) return 0; // TODO: use _timeout
	return connect(remote_addr, port);
}

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
	if (_sockindex < _eth->maxSocketNum()) {
		if (_eth->socketStatus(_sockindex) != SnSR::CLOSED) {
			_eth->socketDisconnect(_sockindex); // TODO: should we call stop()?
		}
		_sockindex = _eth->maxSocketNum();
	}
#if defined(ESP8266) || defined(ESP32)
	if (ip == IPAddress((uint32_t)0) || ip == IPAddress(0xFFFFFFFFul)) return 0;
#else
	if (ip == IPAddress(0ul) || ip == IPAddress(0xFFFFFFFFul)) return 0;
#endif
	_sockindex = _eth->socketBegin(SnMR::TCP, 0);
	if (_sockindex >= _eth->maxSocketNum()) return 0;
	_eth->socketConnect(_sockindex, rawIPAddress(ip), port);
	uint32_t start = millis();
	while (1) {
		uint8_t stat = _eth->socketStatus(_sockindex);
		if (stat == SnSR::ESTABLISHED) return 1;
		if (stat == SnSR::CLOSE_WAIT) return 1;
		if (stat == SnSR::CLOSED) return 0;
		if (millis() - start > _timeout) break;
		delay(1);
	}
	_eth->socketClose(_sockindex);
	_sockindex = _eth->maxSocketNum();
	return 0;
}

int EthernetClient::availableForWrite(void)
{
	if (_sockindex >= _eth->maxSocketNum()) return 0;
	return _eth->socketSendAvailable(_sockindex);
}

size_t EthernetClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
	if (_sockindex >= _eth->maxSocketNum()) return 0;
	if (_eth->socketSend(_sockindex, buf, size)) return size;
	setWriteError();
	return 0;
}

int EthernetClient::available()
{
	if (_sockindex >= _eth->maxSocketNum()) return 0;
	return _eth->socketRecvAvailable(_sockindex);
	// TODO: do the WIZnet chips automatically retransmit TCP ACK
	// packets if they are lost by the network?  Someday this should
	// be checked by a man-in-the-middle test which discards certain
	// packets.  If ACKs aren't resent, we would need to check for
	// returning 0 here and after a timeout do another Sock_RECV
	// command to cause the WIZnet chip to resend the ACK packet.
}

int EthernetClient::read(uint8_t *buf, size_t size)
{
	if (_sockindex >= _eth->maxSocketNum()) return 0;
	return _eth->socketRecv(_sockindex, buf, size);
}

int EthernetClient::peek()
{
	if (_sockindex >= _eth->maxSocketNum()) return -1;
	if (!available()) return -1;
	return _eth->socketPeek(_sockindex);
}

int EthernetClient::read()
{
	uint8_t b;
	if (_eth->socketRecv(_sockindex, &b, 1) > 0) return b;
	return -1;
}

void EthernetClient::flush()
{
	while (_sockindex < _eth->maxSocketNum()) {
		uint8_t stat = _eth->socketStatus(_sockindex);
		if (stat != SnSR::ESTABLISHED && stat != SnSR::CLOSE_WAIT) return;
		if (_eth->socketSendAvailable(_sockindex) >= _eth->SSIZE()) return;
	}
}

void EthernetClient::stop()
{
	if (_sockindex >= _eth->maxSocketNum()) return;

	// attempt to close the connection gracefully (send a FIN to other side)
	_eth->socketDisconnect(_sockindex);
	unsigned long start = millis();

	// wait up to a second for the connection to close
	do {
		if (_eth->socketStatus(_sockindex) == SnSR::CLOSED) {
			_sockindex = _eth->maxSocketNum();
			return; // exit the loop
		}
		delay(1);
	} while (millis() - start < _timeout);

	// if it hasn't closed, close it forcefully
	_eth->socketClose(_sockindex);
	_sockindex = _eth->maxSocketNum();
}

uint8_t EthernetClient::connected()
{
	if (_sockindex >= _eth->maxSocketNum()) return 0;

	uint8_t s = _eth->socketStatus(_sockindex);
	return !(s == SnSR::LISTEN || s == SnSR::CLOSED || s == SnSR::FIN_WAIT ||
		(s == SnSR::CLOSE_WAIT && !available()));
}

uint8_t EthernetClient::status()
{
	if (_sockindex >= _eth->maxSocketNum()) return SnSR::CLOSED;
	return _eth->socketStatus(_sockindex);
}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.
bool EthernetClient::operator==(const EthernetClient& rhs)
{
	if (_sockindex != rhs._sockindex) return false;
	if (_sockindex >= _eth->maxSocketNum()) return false;
	if (rhs._sockindex >= _eth->maxSocketNum()) return false;
	return true;
}

// https://github.com/per1234/EthernetMod
// from: https://github.com/ntruchsess/Arduino-1/commit/937bce1a0bb2567f6d03b15df79525569377dabd
uint16_t EthernetClient::localPort()
{
	return _eth->localPort(_sockindex);
}

// https://github.com/per1234/EthernetMod
// returns the remote IP address: https://forum.arduino.cc/index.php?topic=82416.0
IPAddress EthernetClient::remoteIP()
{
	return _eth->remoteIP(_sockindex);
}

// https://github.com/per1234/EthernetMod
// from: https://github.com/ntruchsess/Arduino-1/commit/ca37de4ba4ecbdb941f14ac1fe7dd40f3008af75
uint16_t EthernetClient::remotePort()
{
	return _eth->remotePort(_sockindex);
}

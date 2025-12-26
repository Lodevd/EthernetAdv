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

EthernetServer::EthernetServer(EthernetClass &ethernet, uint16_t port){
	_port = port;
	_eth = &ethernet;
	_sockindex = _eth->maxSocketNum();
}

void EthernetServer::begin()
{
	_sockindex = _eth->socketBegin(SnMR::TCP, _port);
	if (_sockindex < _eth->maxSocketNum()) {
		if (_eth->socketListen(_sockindex)) {
		} else {
			_eth->socketDisconnect(_sockindex);
		}
	}
}

EthernetClient EthernetServer::available()
{
	bool listening = false;
	
	if(_sockindex < _eth->maxSocketNum()){
		uint8_t stat = _eth->socketStatus(_sockindex);
		if (stat == SnSR::ESTABLISHED || stat == SnSR::CLOSE_WAIT) {
			if (_eth->socketRecvAvailable(_sockindex) <= 0) {
				// remote host closed connection, our end still open
				if (stat == SnSR::CLOSE_WAIT) {
					_eth->socketDisconnect(_sockindex);
					// status becomes LAST_ACK for short time
				}
			}
		} else if (stat == SnSR::LISTEN) {
			listening = true;
		} else if (stat == SnSR::CLOSED) {
			_sockindex = _eth->maxSocketNum();
		}
	}

	if (!listening) begin();
	return EthernetClient(*this->_eth, _sockindex);
}

EthernetClient EthernetServer::accept()
{
	bool listening = false;
	uint8_t sockindex2 = _sockindex;

	if(_sockindex < _eth->maxSocketNum()){
		uint8_t stat = _eth->socketStatus(_sockindex);
		if (_sockindex == _eth->maxSocketNum() &&
			(stat == SnSR::ESTABLISHED || stat == SnSR::CLOSE_WAIT)) {
			// Return the connected client even if no data received.
			// Some protocols like FTP expect the server to send the
			// first data.
			_sockindex = _eth->maxSocketNum();
		} else if (stat == SnSR::LISTEN) {
			listening = true;
		} else if (stat == SnSR::CLOSED) {
			_sockindex = _eth->maxSocketNum();
			sockindex2 = _sockindex;
		}
	}

	if (!listening) begin();
	return EthernetClient(*this->_eth, sockindex2);
}

EthernetServer::operator bool()
{
	if(_sockindex < _eth->maxSocketNum()){
		if (_eth->socketStatus(_sockindex) == SnSR::LISTEN) {
			return true; // server is listening for incoming clients
		}
	}
	return false;
}

#if 0
void EthernetServer::statusreport()
{
	Serial.printf("EthernetServer, port=%d\n", _port);
	for (uint8_t i=0; i < _eth->maxSocketNum(); i++) {
		uint16_t port = server_port[i];
		uint8_t stat = _eth->socketStatus(i);
		const char *name;
		switch (stat) {
			case 0x00: name = "CLOSED"; break;
			case 0x13: name = "INIT"; break;
			case 0x14: name = "LISTEN"; break;
			case 0x15: name = "SYNSENT"; break;
			case 0x16: name = "SYNRECV"; break;
			case 0x17: name = "ESTABLISHED"; break;
			case 0x18: name = "FIN_WAIT"; break;
			case 0x1A: name = "CLOSING"; break;
			case 0x1B: name = "TIME_WAIT"; break;
			case 0x1C: name = "CLOSE_WAIT"; break;
			case 0x1D: name = "LAST_ACK"; break;
			case 0x22: name = "UDP"; break;
			case 0x32: name = "IPRAW"; break;
			case 0x42: name = "MACRAW"; break;
			case 0x5F: name = "PPPOE"; break;
			default: name = "???";
		}
		int avail = _eth->socketRecvAvailable(i);
		Serial.printf("  %d: port=%d, status=%s (0x%02X), avail=%d\n",
			i, port, name, stat, avail);
	}
}
#endif

size_t EthernetServer::write(uint8_t b)
{
	return write(&b, 1);
}

size_t EthernetServer::write(const uint8_t *buffer, size_t size)
{
	available();
	if(_sockindex < _eth->maxSocketNum()){
		if (_eth->socketStatus(_sockindex) == SnSR::ESTABLISHED) {
			_eth->socketSend(_sockindex, buffer, size);
		}
	}
	return size;
}

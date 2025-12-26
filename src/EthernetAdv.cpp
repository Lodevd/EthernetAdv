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
#include "Dhcp.h"

EthernetClass::EthernetClass(W5x00Class &w5x00){
	_w5x00 = &w5x00;
	//Create an array for the socket states just big enough for the number of sockets.
	socketState = new socketstate_t[_w5x00->maxSockNum()];
}

EthernetClass::~EthernetClass(){ 
	delete[] socketState; 
	delete _dhcp;
}

int EthernetClass::begin(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
	// When using this function you use DHCP
	if(_dhcp == nullptr){
		_dhcp = new DhcpClass(*this);
	}

	// Initialise the basic info
	if (_w5x00->init() == 0) return 0;
	_w5x00->beginTransaction();
	_w5x00->setMACAddress(mac);
	_w5x00->setIPAddress(IPAddress(0,0,0,0).raw_address());
	_w5x00->endTransaction();

	// Now try to get our config info from a DHCP server
	int ret = _dhcp->beginWithDHCP(mac, timeout, responseTimeout);
	if (ret == 1) {
		// We've successfully found a DHCP server and got our configuration
		// info, so set things accordingly
		_w5x00->beginTransaction();
		_w5x00->setIPAddress(_dhcp->getLocalIp().raw_address());
		_w5x00->setGatewayIp(_dhcp->getGatewayIp().raw_address());
		_w5x00->setSubnetMask(_dhcp->getSubnetMask().raw_address());
		_w5x00->endTransaction();
		_dnsServerAddress = _dhcp->getDnsServerIp();
		socketPortRand(micros());
	}
	return ret;
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip)
{
	// Assume the DNS server will be the machine on the same network as the local IP
	// but with last octet being '1'
	IPAddress dns = ip;
	dns[3] = 1;
	begin(mac, ip, dns);
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns)
{
	// Assume the gateway will be the machine on the same network as the local IP
	// but with last octet being '1'
	IPAddress gateway = ip;
	gateway[3] = 1;
	begin(mac, ip, dns, gateway);
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway)
{
	IPAddress subnet(255, 255, 255, 0);
	begin(mac, ip, dns, gateway, subnet);
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet)
{
	if (_w5x00->init() == 0) return;
	_w5x00->beginTransaction();
	_w5x00->setMACAddress(mac);
	_w5x00->setIPAddress(ip.raw_address());
	_w5x00->setGatewayIp(gateway.raw_address());
	_w5x00->setSubnetMask(subnet.raw_address());
	_w5x00->endTransaction();
	_dnsServerAddress = dns;
}

EthernetLinkStatus EthernetClass::linkStatus()
{
	switch (_w5x00->getLinkStatus()) {
		case UNKNOWN:  return Unknown;
		case LINK_ON:  return LinkON;
		case LINK_OFF: return LinkOFF;
		default:       return Unknown;
	}
}

int EthernetClass::maintain()
{
	int rc = DHCP_CHECK_NONE;
	if (_dhcp != NULL) {
		// we have a pointer to dhcp, use it
		rc = _dhcp->checkLease();
		switch (rc) {
		case DHCP_CHECK_NONE:
			//nothing done
			break;
		case DHCP_CHECK_RENEW_OK:
		case DHCP_CHECK_REBIND_OK:
			//we might have got a new IP.
			_w5x00->beginTransaction();
			_w5x00->setIPAddress(_dhcp->getLocalIp().raw_address());
			_w5x00->setGatewayIp(_dhcp->getGatewayIp().raw_address());
			_w5x00->setSubnetMask(_dhcp->getSubnetMask().raw_address());
			_w5x00->endTransaction();
			_dnsServerAddress = _dhcp->getDnsServerIp();
			break;
		default:
			//this is actually an error, it will retry though
			break;
		}
	}
	return rc;
}

void EthernetClass::MACAddress(uint8_t *mac_address)
{
	_w5x00->beginTransaction();
	_w5x00->getMACAddress(mac_address);
	_w5x00->endTransaction();
}

IPAddress EthernetClass::localIP()
{
	IPAddress ret;
	_w5x00->beginTransaction();
	_w5x00->getIPAddress(ret.raw_address());
	_w5x00->endTransaction();
	return ret;
}

IPAddress EthernetClass::subnetMask()
{
	IPAddress ret;
	_w5x00->beginTransaction();
	_w5x00->getSubnetMask(ret.raw_address());
	_w5x00->endTransaction();
	return ret;
}

IPAddress EthernetClass::gatewayIP()
{
	IPAddress ret;
	_w5x00->beginTransaction();
	_w5x00->getGatewayIp(ret.raw_address());
	_w5x00->endTransaction();
	return ret;
}

void EthernetClass::setMACAddress(const uint8_t *mac_address)
{
	_w5x00->beginTransaction();
	_w5x00->setMACAddress(mac_address);
	_w5x00->endTransaction();
}

void EthernetClass::setLocalIP(const IPAddress local_ip)
{
	_w5x00->beginTransaction();
	IPAddress ip = local_ip;
	_w5x00->setIPAddress(ip.raw_address());
	_w5x00->endTransaction();
}

void EthernetClass::setSubnetMask(const IPAddress subnet)
{
	_w5x00->beginTransaction();
	IPAddress ip = subnet;
	_w5x00->setSubnetMask(ip.raw_address());
	_w5x00->endTransaction();
}

void EthernetClass::setGatewayIP(const IPAddress gateway)
{
	_w5x00->beginTransaction();
	IPAddress ip = gateway;
	_w5x00->setGatewayIp(ip.raw_address());
	_w5x00->endTransaction();
}

void EthernetClass::setRetransmissionTimeout(uint16_t milliseconds)
{
	if (milliseconds > 6553) milliseconds = 6553;
	_w5x00->beginTransaction();
	_w5x00->setRetransmissionTime(milliseconds * 10);
	_w5x00->endTransaction();
}

void EthernetClass::setRetransmissionCount(uint8_t num)
{
	_w5x00->beginTransaction();
	_w5x00->setRetransmissionCount(num);
	_w5x00->endTransaction();
}

uint16_t EthernetClass::SSIZE(){
	return _w5x00->SSIZE;
}

uint8_t EthernetClass::maxSocketNum(){
	return _w5x00->maxSockNum();
}

// https://github.com/per1234/EthernetMod
// from: https://github.com/ntruchsess/Arduino-1/commit/937bce1a0bb2567f6d03b15df79525569377dabd
uint16_t EthernetClass::localPort(uint8_t socketIndex){
	if (socketIndex >= _w5x00->maxSockNum()) return 0;
	uint16_t port;
	_w5x00->beginTransaction();
	port = _w5x00->getLocalPort(socketIndex);
	_w5x00->endTransaction();
	return port;
}

// https://github.com/per1234/EthernetMod
// returns the remote IP address: https://forum.arduino.cc/index.php?topic=82416.0
IPAddress EthernetClass::remoteIP(uint8_t socketIndex){
	if (socketIndex >= _w5x00->maxSockNum()) return IPAddress((uint32_t)0);
	IPAddress remoteIP;
	_w5x00->beginTransaction();
	remoteIP = _w5x00->getRemoteIp(socketIndex);
	_w5x00->endTransaction();
	return remoteIP;
}

// https://github.com/per1234/EthernetMod
// from: https://github.com/ntruchsess/Arduino-1/commit/ca37de4ba4ecbdb941f14ac1fe7dd40f3008af75
uint16_t EthernetClass::remotePort(uint8_t socketIndex){
	if (socketIndex >= _w5x00->maxSockNum()) return 0;
	uint16_t port;
	_w5x00->beginTransaction();
	port = _w5x00->getRemotePort(socketIndex);
	_w5x00->endTransaction();
	return port;
}
/*
 *   Copyright (C) 2009-2014,2016,2020,2024,2025 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "P25Network.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CP25Network::CP25Network(unsigned short port, const std::string& callsign, bool debug) :
m_callsign(callsign),
m_socket4(port),
m_socket6(port),
m_debug(debug)
{
	assert(port > 0U);

	m_callsign.resize(10U, ' ');
}

CP25Network::~CP25Network()
{
}

bool CP25Network::open()
{
	LogInfo("Opening P25 network connection");

	sockaddr_storage addr4;
	addr4.ss_family = AF_INET;

	bool ret = m_socket4.open(addr4);
	if (!ret)
		return false;

	sockaddr_storage addr6;
	addr6.ss_family = AF_INET6;

	return m_socket6.open(addr6);
}

bool CP25Network::write(const unsigned char* data, unsigned int length, const sockaddr_storage& addr, unsigned int addrLen)
{
	assert(data != nullptr);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", data, length);

	switch (addr.ss_family) {
		case AF_INET:
			return m_socket4.write(data, length, addr, addrLen);
		case AF_INET6:
			return m_socket6.write(data, length, addr, addrLen);
		default:
			LogError("Unknown socket address family - %u", addr.ss_family);
			return false;
	}
}

bool CP25Network::poll(const sockaddr_storage& addr, unsigned int addrLen)
{
	unsigned char data[15U];

	data[0U] = 0xF0U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Poll Sent", data, 11U);

	switch (addr.ss_family) {
		case AF_INET:
			return m_socket4.write(data, 11U, addr, addrLen);
		case AF_INET6:
			return m_socket6.write(data, 11U, addr, addrLen);
		default:
			LogError("Unknown socket address family - %u", addr.ss_family);
			return false;
	}
}

bool CP25Network::unlink(const sockaddr_storage& addr, unsigned int addrLen)
{
	unsigned char data[15U];

	data[0U] = 0xF1U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Unlink Sent", data, 11U);

	switch (addr.ss_family) {
		case AF_INET:
			return m_socket4.write(data, 11U, addr, addrLen);
		case AF_INET6:
			return m_socket6.write(data, 11U, addr, addrLen);
		default:
			LogError("Unknown socket address family - %u", addr.ss_family);
			return false;
	}
}

unsigned int CP25Network::read(unsigned char* data, unsigned int length, sockaddr_storage& addr, unsigned int& addrLen)
{
	assert(data != nullptr);
	assert(length > 0U);

	int len = m_socket4.read(data, length, addr, addrLen);
	if (len <= 0)
		len = m_socket6.read(data, length, addr, addrLen);
	if (len <= 0)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Received", data, len);

	return len;
}

void CP25Network::close()
{
	m_socket4.close();
	m_socket6.close();

	LogInfo("Closing P25 network connection");
}

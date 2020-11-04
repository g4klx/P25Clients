/*
 *   Copyright (C) 2009-2014,2016,2020 by Jonathan Naylor G4KLX
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

CP25Network::CP25Network(unsigned int port, const std::string& callsign, bool debug) :
m_callsign(callsign),
m_socket(),
m_port(port),
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

	bool ret = m_socket.open(0, PF_INET, "", m_port);
	if (!ret)
		return false;

	return m_socket.open(1, PF_INET6, "", m_port);
}

bool CP25Network::write(const unsigned char* data, unsigned int length, const sockaddr_storage& addr, unsigned int addrLen)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", data, length);

	return m_socket.write(data, length, addr, addrLen);
}

bool CP25Network::poll(const sockaddr_storage& addr, unsigned int addrLen)
{
	unsigned char data[15U];

	data[0U] = 0xF0U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Poll Sent", data, 11U);

	return m_socket.write(data, 11U, addr, addrLen);
}

bool CP25Network::unlink(const sockaddr_storage& addr, unsigned int addrLen)
{
	unsigned char data[15U];

	data[0U] = 0xF1U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Unlink Sent", data, 11U);

	return m_socket.write(data, 11U, addr, addrLen);
}

unsigned int CP25Network::read(unsigned char* data, unsigned int length, sockaddr_storage& addr, unsigned int& addrLen)
{
	assert(data != NULL);
	assert(length > 0U);

	int len = m_socket.read(data, length, addr, addrLen);
	if (len <= 0)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Received", data, len);

	return len;
}

void CP25Network::close()
{
	m_socket.close();

	LogInfo("Closing P25 network connection");
}

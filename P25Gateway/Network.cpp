/*
 *   Copyright (C) 2009-2014,2016 by Jonathan Naylor G4KLX
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

#include "Network.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CNetwork::CNetwork(unsigned int port, const std::string& callsign, bool debug) :
m_callsign(callsign),
m_socket(port),
m_debug(debug)
{
	m_callsign.resize(10U, ' ');
}

CNetwork::~CNetwork()
{
}

bool CNetwork::open()
{
	LogInfo("Opening P25 network connection");

	return m_socket.open();
}

bool CNetwork::writeData(const unsigned char* data, unsigned int length, const in_addr& address, unsigned int port)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(port > 0U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", data, length);

	return m_socket.write(data, length, address, port);
}

bool CNetwork::writePoll(const in_addr& address, unsigned int port)
{
	assert(port > 0U);

	unsigned char data[15U];

	data[0U] = 0xF0U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Poll Sent", data, 11U);

	return m_socket.write(data, 11U, address, port);
}

bool CNetwork::writeUnlink(const in_addr& address, unsigned int port)
{
	assert(port > 0U);

	unsigned char data[15U];

	data[0U] = 0xF1U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Unlink Sent", data, 11U);

	return m_socket.write(data, 11U, address, port);
}

unsigned int CNetwork::readData(unsigned char* data, unsigned int length, in_addr& address, unsigned int& port)
{
	assert(data != NULL);
	assert(length > 0U);

	int len = m_socket.read(data, length, address, port);
	if (len <= 0)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Received", data, len);

	return len;
}

void CNetwork::close()
{
	m_socket.close();

	LogInfo("Closing P25 network connection");
}

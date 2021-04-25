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

#include "Network.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CNetwork::CNetwork(unsigned short port, bool debug) :
m_socket(port),
m_debug(debug)
{
}

CNetwork::~CNetwork()
{
}

bool CNetwork::open()
{
	LogInfo("Opening P25 network connection");

	return m_socket.open();
}

bool CNetwork::writeData(const unsigned char* data, unsigned int length, const sockaddr_storage& addr, unsigned int addrLen)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", data, length);

	return m_socket.write(data, length, addr, addrLen);
}

unsigned int CNetwork::readData(unsigned char* data, unsigned int length, sockaddr_storage& addr, unsigned int& addrLen)
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

void CNetwork::close()
{
	m_socket.close();

	LogInfo("Closing P25 network connection");
}

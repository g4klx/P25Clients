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

const unsigned int BUFFER_LENGTH = 200U;

CNetwork::CNetwork(unsigned int port) :
m_socket(port),
m_address(),
m_port(0U),
m_buffer(1000U, "P25 Network")
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

bool CNetwork::write(const unsigned char* data, unsigned int length)
{
	if (m_port == 0U)
		return true;

	assert(data != NULL);

	CUtils::dump(1U, "P25 Network Data Sent", data, length);

	return m_socket.write(data, length, m_address, m_port);
}

void CNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return;

	m_address.s_addr = address.s_addr;
	m_port = port;

	CUtils::dump(1U, "P25 Network Data Received", buffer, length);

	unsigned char l = length;
	m_buffer.addData(&l, 1U);

	m_buffer.addData(buffer, length);
}

unsigned int CNetwork::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_buffer.getData(&len, 1U);

	m_buffer.getData(data, len);

	return len;
}

void CNetwork::end()
{
	m_port = 0U;
}

void CNetwork::close()
{
	m_socket.close();

	LogInfo("Closing P25 network connection");
}

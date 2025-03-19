/*
 *   Copyright (C) 2009-2014,2016,2018,2020,2025 by Jonathan Naylor G4KLX
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

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CNetwork::CNetwork(unsigned short port) :
m_socket(port),
m_addr(),
m_addrLen(0U)
{
}

CNetwork::~CNetwork()
{
}

bool CNetwork::open()
{
	::fprintf(stdout, "Opening P25 network connection\n");

	return m_socket.open();
}

bool CNetwork::write(const unsigned char* data, unsigned int length)
{
	if (m_addrLen == 0U)
		return true;

	assert(data != nullptr);

	return m_socket.write(data, length, m_addr, m_addrLen);
}

unsigned int CNetwork::read(unsigned char* data)
{
	sockaddr_storage addr;
	unsigned int addrlen;
	int length = m_socket.read(data, BUFFER_LENGTH, addr, addrlen);
	if (length <= 0)
		return 0U;

	m_addr    = addr;
	m_addrLen = addrlen;

	if (data[0U] == 0xF0U) {			// A poll
		write(data, length);
		return 0U;
	} else if (data[0U] == 0xF1U) {	// An unlink
		// Nothing to do
		return 0U;
	} else {
		return length;
	}
}

void CNetwork::end()
{
	m_addrLen = 0U;
}

void CNetwork::close()
{
	m_socket.close();

	::fprintf(stdout, "Closing P25 network connection\n");
}

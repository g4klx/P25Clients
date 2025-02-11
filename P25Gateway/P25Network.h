/*
 *   Copyright (C) 2009-2014,2016,2020,2024 by Jonathan Naylor G4KLX
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

#ifndef	P25Network_H
#define	P25Network_H

#include "UDPSocket.h"

#include <cstdint>
#include <string>

class CP25Network {
public:
	CP25Network(unsigned short port, const std::string& callsign, bool debug);
	~CP25Network();

	bool open();

	bool write(const unsigned char* data, unsigned int length, const sockaddr_storage& addr, unsigned int addrLen);

	unsigned int read(unsigned char* data, unsigned int length, sockaddr_storage& addr, unsigned int& addrLen);

	bool poll(const sockaddr_storage& addr, unsigned int addrLen);

	bool unlink(const sockaddr_storage& addr, unsigned int addrLen);

	void close();

private:
	std::string m_callsign;
	CUDPSocket  m_socket4;
	CUDPSocket  m_socket6;
	bool        m_debug;
};

#endif

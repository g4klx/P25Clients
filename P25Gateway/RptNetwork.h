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

#ifndef	RptNetwork_H
#define	RptNetwork_H

#include "UDPSocket.h"
#include "Timer.h"

#include <cstdint>
#include <string>

class CRptNetwork {
public:
	CRptNetwork(unsigned short myPort, const sockaddr_storage& rptAddr, unsigned int rptAddrLen, const std::string& callsign, bool debug);
	~CRptNetwork();

	bool open();

	bool write(const unsigned char* data, unsigned int length);

	unsigned int read(unsigned char* data, unsigned int length);

	void clock(unsigned int ms);

	void close();

private:
	sockaddr_storage m_rptAddr;
	unsigned int     m_rptAddrLen;
	std::string      m_callsign;
	CUDPSocket       m_socket;
	bool             m_debug;
	CTimer           m_timer;

	bool writePoll();
};

#endif

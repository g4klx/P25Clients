/*
 *   Copyright (C) 2009-2014,2016,2020,2025 by Jonathan Naylor G4KLX
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

#include "RptNetwork.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CRptNetwork::CRptNetwork(unsigned short myPort, const sockaddr_storage& rptAddr, unsigned int rptAddrLen, const std::string& callsign, bool debug) :
m_rptAddr(rptAddr),
m_rptAddrLen(rptAddrLen),
m_callsign(callsign),
m_socket(myPort),
m_debug(debug),
m_timer(1000U, 5U)
{
	assert(myPort > 0U);
	assert(rptAddrLen > 0U);

	m_callsign.resize(10U, ' ');
}

CRptNetwork::~CRptNetwork()
{
}

bool CRptNetwork::open()
{
	LogInfo("Opening Rpt network connection");

	bool ret = m_socket.open(m_rptAddr);

	if (ret) {
		m_timer.start();
		return true;
	} else {
		return false;
	}
}

bool CRptNetwork::write(const unsigned char* data, unsigned int length)
{
	assert(data != nullptr);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "Rpt Network Data Sent", data, length);

	return m_socket.write(data, length, m_rptAddr, m_rptAddrLen);
}

bool CRptNetwork::writePoll()
{
	unsigned char data[15U];

	data[0U] = 0xF0U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "Rpt Network Poll Sent", data, 11U);

	return m_socket.write(data, 11U, m_rptAddr, m_rptAddrLen);
}

unsigned int CRptNetwork::read(unsigned char* data, unsigned int length)
{
	assert(data != nullptr);
	assert(length > 0U);

	sockaddr_storage addr;
	unsigned int addrLen = 0U;
	int len = m_socket.read(data, length, addr, addrLen);
	if (len <= 0)
		return 0U;

	if (!CUDPSocket::match(addr, m_rptAddr))
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "Rpt Network Data Received", data, len);

	return len;
}

void CRptNetwork::clock(unsigned int ms)
{
	m_timer.clock(ms);
	if (m_timer.isRunning() && m_timer.hasExpired()) {
		writePoll();
		m_timer.start();
	}
}

void CRptNetwork::close()
{
	m_timer.stop();

	m_socket.close();

	LogInfo("Closing Rpt network connection");
}

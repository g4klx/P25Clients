/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#if !defined(Reflectors_H)
#define	Reflectors_H

#include "UDPSocket.h"
#include "Timer.h"

#include <vector>
#include <string>

class CP25Reflector {
public:
	CP25Reflector() :
	m_id(),
	m_address(),
	m_port(0U)
	{
	}

	unsigned int m_id;
	in_addr      m_address;
	unsigned int m_port;
};

class CReflectors {
public:
	CReflectors(const std::string& hostsFile, unsigned int reloadTime);
	~CReflectors();

	void setParrot(const std::string& address, unsigned int port);

	bool load();

	CP25Reflector* find(unsigned int id);

	void clock(unsigned int ms);

private:
	std::string                 m_hostsFile;
	std::string                 m_parrotAddress;
	unsigned int                m_parrotPort;
	std::vector<CP25Reflector*> m_reflectors;
	CTimer                      m_timer;
};

#endif

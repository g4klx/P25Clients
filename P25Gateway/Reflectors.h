/*
*   Copyright (C) 2016,2018,2020 by Jonathan Naylor G4KLX
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

#include <nlohmann/json.hpp>

#include <vector>
#include <string>

class CP25Reflector {
public:
	CP25Reflector() :
	m_id(),
	m_addr_v4(),
	m_addrLen_v4(0U),
	m_addr_v6(),
	m_addrLen_v6(0U)
	{
	}

	unsigned int     m_id;
	sockaddr_storage m_addr_v4;
	unsigned int     m_addrLen_v4;
	sockaddr_storage m_addr_v6;
	unsigned int     m_addrLen_v6;
};

class CReflectors {
public:
	CReflectors(const std::string& hostsFile1, const std::string& hostsFile2, unsigned int reloadTime);
	~CReflectors();

	void setParrot(const std::string& address, unsigned short port);
	void setP252DMR(const std::string& address, unsigned short port);

	bool load();

	CP25Reflector* find(unsigned int id);

	void clock(unsigned int ms);

private:
	std::string  m_hostsFile1;
	std::string  m_hostsFile2;
	std::string  m_parrotAddress;
	unsigned short m_parrotPort;
	std::string  m_p252dmrAddress;
	unsigned short m_p252dmrPort;
	std::vector<CP25Reflector*> m_reflectors;
	CTimer       m_timer;

	void remove();
	void parse(const nlohmann::json& data);
};

#endif

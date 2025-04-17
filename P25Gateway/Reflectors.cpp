/*
*   Copyright (C) 2016,2018,2020,2025 by Jonathan Naylor G4KLX
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

#include "Reflectors.h"
#include "Log.h"

#include <fstream>

#include <algorithm>
#include <functional>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <cctype>

CReflectors::CReflectors(const std::string& hostsFile1, const std::string& hostsFile2, unsigned int reloadTime) :
m_hostsFile1(hostsFile1),
m_hostsFile2(hostsFile2),
m_parrotAddress(),
m_parrotPort(0U),
m_p252dmrAddress(),
m_p252dmrPort(0U),
m_reflectors(),
m_timer(1000U, reloadTime * 60U)
{
	if (reloadTime > 0U)
		m_timer.start();
}

CReflectors::~CReflectors()
{
	remove();
}

void CReflectors::setParrot(const std::string& address, unsigned short port)
{
	m_parrotAddress = address;
	m_parrotPort    = port;
}

void CReflectors::setP252DMR(const std::string& address, unsigned short port)
{
	m_p252dmrAddress = address;
	m_p252dmrPort    = port;
}

bool CReflectors::load()
{
	// Clear out the old reflector list
	remove();

	try {
		std::fstream f(m_hostsFile1);
		nlohmann::json data = nlohmann::json::parse(f);

		parse(data);
	}
	catch (...) {
		LogError("Unable to load/parse file %s", m_hostsFile1.c_str());
		return false;
	}

	try {
		std::fstream f(m_hostsFile2);
		nlohmann::json data = nlohmann::json::parse(f);

		parse(data);
	}
	catch (...) {
		LogWarning("Unable to load/parse file %s", m_hostsFile2.c_str());
	}

	size_t size = m_reflectors.size();
	LogInfo("Loaded %u P25 reflectors", size);

	// Add the Parrot entry
	if (m_parrotPort > 0U) {
		sockaddr_storage addr;
		unsigned int addrLen;
		if (CUDPSocket::lookup(m_parrotAddress, m_parrotPort, addr, addrLen) == 0) {
			CP25Reflector* refl = new CP25Reflector;
			refl->m_id = 10U;
			refl->m_addr_v4 = addr;
			refl->m_addrLen_v4 = addrLen;
			m_reflectors.push_back(refl);
			LogInfo("Loaded P25 parrot (TG%u)", refl->m_id);
		} else {
			LogWarning("Unable to resolve the address of the Parrot");
		}
	}

	// Add the P252DMR entry
	if (m_p252dmrPort > 0U) {
		sockaddr_storage addr;
		unsigned int addrLen;
		if (CUDPSocket::lookup(m_p252dmrAddress, m_p252dmrPort, addr, addrLen) == 0) {
			CP25Reflector* refl = new CP25Reflector;
			refl->m_id = 20U;
			refl->m_addr_v4 = addr;
			refl->m_addrLen_v4 = addrLen;
			m_reflectors.push_back(refl);
			LogInfo("Loaded P252DMR (TG%u)", refl->m_id);
		} else {
			LogWarning("Unable to resolve the address of P252DMR");
		}
	}

	size = m_reflectors.size();
	if (size == 0U)
		return false;

	return true;
}

CP25Reflector* CReflectors::find(unsigned int id)
{
	for (std::vector<CP25Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it) {
		if (id == (*it)->m_id)
			return *it;
	}

	return nullptr;
}

void CReflectors::clock(unsigned int ms)
{
	m_timer.clock(ms);

	if (m_timer.isRunning() && m_timer.hasExpired()) {
		load();
		m_timer.start();
	}
}

void CReflectors::remove()
{
	for (std::vector<CP25Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it)
		delete* it;

	m_reflectors.clear();
}

void CReflectors::parse(const nlohmann::json& data)
{
	bool hasData = data["reflectors"].is_array();
	if (!hasData)
		throw;

	nlohmann::json::array_t arr = data["reflectors"];
	for (const auto& it : arr) {
		unsigned int tg = it["designator"];
		if (tg > 0xFFFFU) {
			LogWarning("P25 Talkgroups can only be 16 bits. %u is too large", tg);
			continue;
		}

		unsigned short port = it["port"];

		sockaddr_storage addr_v4    = sockaddr_storage();
		unsigned int     addrLen_v4 = 0U;

		bool isNull = it["ipv4"].is_null();
		if (!isNull) {
			std::string ipv4 = it["ipv4"];
			if (!CUDPSocket::lookup(ipv4, port, addr_v4, addrLen_v4) == 0) {
				LogWarning("Unable to resolve the address of %s", ipv4.c_str());
				addrLen_v4 = 0U;
			}
		}

		sockaddr_storage addr_v6    = sockaddr_storage();
		unsigned int     addrLen_v6 = 0U;

		isNull = it["ipv6"].is_null();
		if (!isNull) {
			std::string ipv6 = it["ipv6"];
			if (!CUDPSocket::lookup(ipv6, port, addr_v6, addrLen_v6) == 0) {
				LogWarning("Unable to resolve the address of %s", ipv6.c_str());
				addrLen_v6 = 0U;
			}
		}

		if ((addrLen_v4 > 0U) || (addrLen_v6 > 0U)) {
			CP25Reflector* refl = new CP25Reflector;
			refl->m_id         = tg;
			refl->m_addr_v4    = addr_v4;
			refl->m_addrLen_v4 = addrLen_v4;
			refl->m_addr_v6    = addr_v6;
			refl->m_addrLen_v6 = addrLen_v6;
			m_reflectors.push_back(refl);
		}
	}
}

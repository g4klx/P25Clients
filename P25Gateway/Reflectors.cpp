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

#include "Reflectors.h"
#include "Log.h"

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
	for (std::vector<CP25Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it)
		delete *it;

	m_reflectors.clear();
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
	for (std::vector<CP25Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it)
		delete *it;

	m_reflectors.clear();

	FILE* fp = ::fopen(m_hostsFile1.c_str(), "rt");
	if (fp != NULL) {
		char buffer[100U];
		while (::fgets(buffer, 100U, fp) != NULL) {
			if (buffer[0U] == '#')
				continue;

			char* p1 = ::strtok(buffer, " \t\r\n");
			char* p2 = ::strtok(NULL,   " \t\r\n");
			char* p3 = ::strtok(NULL,   " \t\r\n");

			if (p1 != NULL && p2 != NULL && p3 != NULL) {
				std::string host  = std::string(p2);
				unsigned short port = (unsigned short)::atoi(p3);
				unsigned int tg = (unsigned int)::atoi(p1);

				if (tg > 0xFFFFU){
					LogWarning("P25 Talkgroups can only be 16 bits. %u is too large",tg);
					continue;
				}

				sockaddr_storage addr;
				unsigned int addrLen;
				if (CUDPSocket::lookup(host, port, addr, addrLen) == 0) {
					CP25Reflector* refl = new CP25Reflector;
					refl->m_id      = (unsigned int)::atoi(p1);
					refl->m_addr    = addr;
					refl->m_addrLen = addrLen;
					m_reflectors.push_back(refl);
				} else {
					LogWarning("Unable to resolve the address of %s", host.c_str());
				}
			}
		}

		::fclose(fp);
	}

	fp = ::fopen(m_hostsFile2.c_str(), "rt");
	if (fp != NULL) {
		char buffer[100U];
		while (::fgets(buffer, 100U, fp) != NULL) {
			if (buffer[0U] == '#')
				continue;

			char* p1 = ::strtok(buffer, " \t\r\n");
			char* p2 = ::strtok(NULL, " \t\r\n");
			char* p3 = ::strtok(NULL, " \t\r\n");

			if (p1 != NULL && p2 != NULL && p3 != NULL) {
				// Don't allow duplicate reflector ids from the secondary hosts file.
				unsigned int id = (unsigned int)::atoi(p1);
				if (find(id) == NULL) {
					std::string host  = std::string(p2);
					unsigned short port = (unsigned short)::atoi(p3);
					unsigned int tg = (unsigned int)::atoi(p1);

					if (tg > 0xFFFFU){
						LogWarning("P25 Talkgroups can only be 16 bits. %u is too large",tg);
						continue;
					}

					sockaddr_storage addr;
					unsigned int addrLen;
					if (CUDPSocket::lookup(host, port, addr, addrLen) == 0) {
						CP25Reflector* refl = new CP25Reflector;
						refl->m_id      = id;
						refl->m_addr    = addr;
						refl->m_addrLen = addrLen;
						m_reflectors.push_back(refl);
					} else {
						LogWarning("Unable to resolve the address of %s", host.c_str());
					}
				}
			}
		}

		::fclose(fp);
	}

	size_t size = m_reflectors.size();
	LogInfo("Loaded %u P25 reflectors", size);

	// Add the Parrot entry
	if (m_parrotPort > 0U) {
		sockaddr_storage addr;
		unsigned int addrLen;
		if (CUDPSocket::lookup(m_parrotAddress, m_parrotPort, addr, addrLen) == 0) {
			CP25Reflector* refl = new CP25Reflector;
			refl->m_id      = 10U;
			refl->m_addr    = addr;
			refl->m_addrLen = addrLen;
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
			refl->m_id      = 20U;
			refl->m_addr    = addr;
			refl->m_addrLen = addrLen;
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

	return NULL;
}

void CReflectors::clock(unsigned int ms)
{
	m_timer.clock(ms);

	if (m_timer.isRunning() && m_timer.hasExpired()) {
		load();
		m_timer.start();
	}
}

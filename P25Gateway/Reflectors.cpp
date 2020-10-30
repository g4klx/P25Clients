/*
*   Copyright (C) 2016,2018 by Jonathan Naylor G4KLX
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

void CReflectors::setParrot(const std::string& address, unsigned int port)
{
	m_parrotAddress = address;
	m_parrotPort    = port;
}

void CReflectors::setP252DMR(const std::string& address, unsigned int port)
{
	m_p252dmrAddress = address;
	m_p252dmrPort    = port;
}

void CReflectors::setP252PCM(const std::string& address, unsigned int port)
{
	m_p252pcmAddress = address;
	m_p252pcmPort    = port;
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
				std::string host = std::string(p2);

				in_addr address = CUDPSocket::lookup(host);
				if (address.s_addr != INADDR_NONE) {
					CP25Reflector* refl = new CP25Reflector;
					refl->m_id      = (unsigned int)::atoi(p1);
					refl->m_address = address;
					refl->m_port    = (unsigned int)::atoi(p3);

					m_reflectors.push_back(refl);
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
					std::string host = std::string(p2);

					in_addr address = CUDPSocket::lookup(host);
					if (address.s_addr != INADDR_NONE) {
						CP25Reflector* refl = new CP25Reflector;
						refl->m_id      = id;
						refl->m_address = address;
						refl->m_port    = (unsigned int)::atoi(p3);

						m_reflectors.push_back(refl);
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
		CP25Reflector* refl = new CP25Reflector;
		refl->m_id      = 10U;
		refl->m_address = CUDPSocket::lookup(m_parrotAddress);
		refl->m_port    = m_parrotPort;
		m_reflectors.push_back(refl);
		LogInfo("Loaded P25 parrot (TG%u)", refl->m_id);
	}
	
	// Add the P252DMR entry
	if (m_p252dmrPort > 0U) {
		CP25Reflector* refl = new CP25Reflector;
		refl->m_id      = 20U;
		refl->m_address = CUDPSocket::lookup(m_p252dmrAddress);
		refl->m_port    = m_p252dmrPort;
		m_reflectors.push_back(refl);
		LogInfo("Loaded P252DMR (TG%u)", refl->m_id);
	}
	
	//Add the P252PCM entry
	if (m_p252pcmPort > 0U) {
		CP25Reflector* refl = new CP25Reflector;
		refl->m_id      = 30U;
		refl->m_address = CUDPSocket::lookup(m_p252dmrAddress);
		refl->m_port    = m_p252pcmPort;
		m_reflectors.push_back(refl);
		LogInfo("Loaded P252PCM (TG%u)", refl->m_id);
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

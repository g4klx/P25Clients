/*
*   Copyright (C) 2016,2024 by Jonathan Naylor G4KLX
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

#if !defined(P25Gateway_H)
#define	P25Gateway_H

#include "P25Network.h"
#include "Reflectors.h"
#include "Voice.h"
#include "Timer.h"
#include "Conf.h"

#include <cstdio>
#include <string>
#include <vector>

#if !defined(_WIN32) && !defined(_WIN64)
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <WS2tcpip.h>
#endif

class CStaticTG {
public:
	unsigned int     m_tg;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
};

class CP25Gateway
{
public:
	CP25Gateway(const std::string& file);
	~CP25Gateway();

	int run();

private:
	CConf        m_conf;
	CVoice*      m_voice;
	CP25Network* m_remoteNetwork;
	unsigned int m_currentTG;
	unsigned int m_currentAddrLen;
	sockaddr_storage m_currentAddr;
	bool         m_currentIsStatic;
	CTimer       m_hangTimer;
	unsigned int m_rfHangTime;
	CReflectors* m_reflectors;	
	std::vector<CStaticTG> m_staticTGs;

	bool isVoiceBusy() const;

	void writeJSONStatus(const std::string& status);
	void writeJSONLinking(const std::string& reason, unsigned int tg);
	void writeJSONUnlinked(const std::string& reason);
	void writeJSONRelinking(unsigned int tg);

	void writeCommand(const std::string& command);

	static void onCommand(const unsigned char* command, unsigned int length);
};

#endif

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

#if !defined(SPEECH_H)
#define	SPEECH_H

#include "StopWatch.h"
#include "Network.h"
#include "Timer.h"

enum SPEECH_STATE {
	SS_NONE,
	SS_WAIT,
	SS_RUN
};

class CSpeech {
public:
	CSpeech(CNetwork& network, const in_addr& address, unsigned int port);
	~CSpeech();

	void announce(unsigned int id);

	void eof();

	void clock(unsigned int ms);

private:
	CNetwork&            m_network;
	in_addr              m_address;
	unsigned int         m_port;
	SPEECH_STATE         m_state;
	unsigned int         m_id;
	const unsigned char* m_speech;
	unsigned int         m_count;
	CTimer               m_timer;
	CStopWatch           m_stopWatch;
	unsigned int         m_pos;
	unsigned char        m_n;
};

#endif

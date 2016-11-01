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

#include "Speech.h"

#include "SpeechDefault.h"
#include "SpeechEU.h"
#include "SpeechNA.h"
#include "SpeechPacific.h"
#include "SpeechWW.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int SRC_ID = 10999U;

const unsigned char REC62[] = {
	0x62U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x35U, 0xA3U, 0x1AU, 0x65U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC6B[] = {
	0x6BU, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x35U, 0xA3U, 0x1AU, 0x64U, 0x9CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

CSpeech::CSpeech(CNetwork& network, const in_addr& address, unsigned int port) :
m_network(network),
m_address(address),
m_port(port),
m_state(SS_NONE),
m_id(0U),
m_speech(NULL),
m_count(0U),
m_timer(1000U, 1U),
m_stopWatch(),
m_pos(0U),
m_n(0x00U)
{
	assert(port > 0U);
}

CSpeech::~CSpeech()
{
}

void CSpeech::announce(unsigned int id)
{
	switch (id) {
	case 10100U:
		m_speech = SPEECH_WW;
		m_count  = SPEECH_WW_COUNT;
		m_id     = 10100U;
		break;
	case 10200U:
		m_speech = SPEECH_NA;
		m_count  = SPEECH_NA_COUNT;
		m_id     = 10200U;
		break;
	case 10300U:
		m_speech = SPEECH_EU;
		m_count  = SPEECH_EU_COUNT;
		m_id     = 10300U;
		break;
	case 10400U:
		m_speech = SPEECH_PACIFIC;
		m_count  = SPEECH_PACIFIC_COUNT;
		m_id     = 10400U;
		break;
	case 9999U:
		m_speech = SPEECH_DEFAULT;
		m_count  = SPEECH_DEFAULT_COUNT;
		m_id     = 9999U;
		break;
	default:
		m_speech = NULL;
		m_count  = 0U;
		m_id     = 0U;
		break;
	}
}

void CSpeech::eof()
{
	if (m_id == 0U)
		return;

	m_state = SS_WAIT;

	m_timer.start();
}

void CSpeech::clock(unsigned int ms)
{
	if (m_state == SS_WAIT) {
		m_timer.clock(ms);
		if (m_timer.isRunning() && m_timer.hasExpired()) {
			if (m_id > 0U) {
				m_state = SS_RUN;
				m_stopWatch.start();
				m_timer.stop();
				m_pos = 0U;
				m_n = 0x62U;
			} else {
				m_state = SS_NONE;
				m_timer.stop();
			}
		}
	}

	if (m_state == SS_RUN) {
		unsigned int n = m_stopWatch.elapsed() / 20U;
		while (m_pos < n) {
			unsigned char buffer[22U];

			switch (m_n) {
			case 0x62U:
				::memcpy(buffer, REC62, 22U);
				::memcpy(buffer + 10U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 22U, m_address, m_port);
				m_n = 0x63U;
				break;
			case 0x63U:
				::memset(buffer, 0x00U, 14U);
				buffer[0U]  = 0x63U;
				buffer[13U] = 0x7AU;
				::memcpy(buffer + 1U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 14U, m_address, m_port);
				m_n = 0x64U;
				break;
			case 0x64U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x64U;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x65U;
				break;
			case 0x65U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x65U;
				buffer[1U] = (m_id >> 16) & 0xFFU;
				buffer[2U] = (m_id >> 8) & 0xFFU;
				buffer[3U] = (m_id >> 0) & 0xFFU;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x66U;
				break;
			case 0x66U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U] = 0x66U;
				buffer[1U] = (SRC_ID >> 16) & 0xFFU;
				buffer[2U] = (SRC_ID >> 8) & 0xFFU;
				buffer[3U] = (SRC_ID >> 0) & 0xFFU;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x67U;
				break;
			case 0x67U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x67U;
				buffer[1U]  = 0x28U;			// XXX ???
				buffer[2U]  = 0xD6U;			// XXX ???
				buffer[3U]  = 0x58U;			// XXX ???
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x68U;
				break;
			case 0x68U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x68U;
				buffer[1U]  = 0xA0U;			// XXX ???
				buffer[2U]  = 0x81U;			// XXX ???
				buffer[3U]  = 0x9CU;			// XXX ???
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x69U;
				break;
			case 0x69U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x69U;
				buffer[1U]  = 0x0EU;			// XXX ???
				buffer[2U]  = 0x74U;			// XXX ???
				buffer[3U]  = 0xBCU;			// XXX ???
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x6AU;
				break;
			case 0x6AU:
				::memset(buffer, 0x00U, 16U);
				buffer[0U]  = 0x6AU;
				buffer[3U]  = 0x0EU;			// XXX ???
				buffer[15U] = (m_pos == (m_count - 1U)) ? 0x00U : 0x02U;
				::memcpy(buffer + 4U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 16U, m_address, m_port);
				m_n = 0x6BU;
				break;
			case 0x6BU:
				::memcpy(buffer, REC6B, 22U);
				::memcpy(buffer + 10U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 22U, m_address, m_port);
				m_n = 0x6CU;
				break;
			case 0x6CU:
				::memset(buffer, 0x00U, 14U);
				buffer[0U]  = 0x6CU;
				buffer[12U] = 0x02U;
				buffer[13U] = 0xF6U;
				::memcpy(buffer + 1U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 14U, m_address, m_port);
				m_n = 0x6DU;
				break;
			case 0x6DU:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x6DU;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x6EU;
				break;
			case 0x6EU:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x6EU;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x6FU;
				break;
			case 0x6FU:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x6FU;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x70U;
				break;
			case 0x70U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x70U;
				buffer[1U]  = 0x80U;
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x71U;
				break;
			case 0x71U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U]  = 0x71U;
				buffer[1U]  = 0xACU;			// XXX ???
				buffer[2U]  = 0xB8U;			// XXX ???
				buffer[3U]  = 0xA4U;			// XXX ???
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x72U;
				break;
			case 0x72U:
				::memset(buffer, 0x00U, 17U);
				buffer[0U] = 0x72U;
				buffer[1U] = 0x9BU;			// XXX ???
				buffer[2U] = 0xDCU;			// XXX ???
				buffer[3U] = 0x75U;			// XXX ???
				buffer[16U] = 0x02U;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x73U;
				break;
			default:
				::memset(buffer, 0x00U, 16U);
				buffer[0U]  = 0x73U;
				buffer[3U]  = 0x06U;			// XXX ???
				buffer[15U] = (m_pos == (m_count - 1U)) ? 0x00U : 0x02U;
				::memcpy(buffer + 4U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 16U, m_address, m_port);
				m_n = 0x62U;
				break;
			}

			if (m_pos == (m_count - 1U)) {
				m_state = SS_NONE;
				m_id = 0U;
				return;
			}

			m_pos++;
		}
	}
}

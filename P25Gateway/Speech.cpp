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
	0x62U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x1BU, 0x5AU, 0x1AU, 0x2BU, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC63[] = {
	0x63U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x7AU };

const unsigned char REC64[] = {
	0x64U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC65[] = {
	0x65U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC66[] = {
	0x66U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC67[] = {
	0x67U, 0xC4U, 0x52U, 0x9BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC68[] = {
	0x68U, 0x9AU, 0xECU, 0xBAU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC69[] = {
	0x69U, 0xB9U, 0xD8U, 0x16U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC6A[] = {
	0x6AU, 0x00U, 0x00U, 0x06U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC6B[] = {
	0x6BU, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x1BU, 0x5AU, 0x1AU, 0x2BU, 0xACU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC6C[] = {
	0x6CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xD6U };

const unsigned char REC6D[] = {
	0x6DU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC6E[] = {
	0x6EU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC6F[] = {
	0x6FU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC70[] = {
	0x70U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC71[] = {
	0x71U, 0xACU, 0xB8U, 0xA4U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC72[] = {
	0x72U, 0x9BU, 0xDCU, 0x75U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC73[] = {
	0x73U, 0x00U, 0x00U, 0x06U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U };

const unsigned char REC80[] = {
	0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U };

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
				::memcpy(buffer, REC63, 14U);
				::memcpy(buffer + 1U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 14U, m_address, m_port);
				m_n = 0x64U;
				break;
			case 0x64U:
				::memcpy(buffer, REC64, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x65U;
				break;
			case 0x65U:
				::memcpy(buffer, REC65, 17U);
				buffer[1U] = (m_id >> 16) & 0xFFU;
				buffer[2U] = (m_id >> 8) & 0xFFU;
				buffer[3U] = (m_id >> 0) & 0xFFU;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x66U;
				break;
			case 0x66U:
				::memcpy(buffer, REC66, 17U);
				buffer[1U] = (SRC_ID >> 16) & 0xFFU;
				buffer[2U] = (SRC_ID >> 8) & 0xFFU;
				buffer[3U] = (SRC_ID >> 0) & 0xFFU;
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x67U;
				break;
			case 0x67U:
				::memcpy(buffer, REC67, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x68U;
				break;
			case 0x68U:
				::memcpy(buffer, REC68, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x69U;
				break;
			case 0x69U:
				::memcpy(buffer, REC69, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x6AU;
				break;
			case 0x6AU:
				::memcpy(buffer, REC6A, 16U);
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
				::memcpy(buffer, REC6C, 14U);
				::memcpy(buffer + 1U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 14U, m_address, m_port);
				m_n = 0x6DU;
				break;
			case 0x6DU:
				::memcpy(buffer, REC6D, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x6EU;
				break;
			case 0x6EU:
				::memcpy(buffer, REC6E, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x6FU;
				break;
			case 0x6FU:
				::memcpy(buffer, REC6F, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x70U;
				break;
			case 0x70U:
				::memcpy(buffer, REC70, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x71U;
				break;
			case 0x71U:
				::memcpy(buffer, REC71, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x72U;
				break;
			case 0x72U:
				::memcpy(buffer, REC72, 17U);
				::memcpy(buffer + 5U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 17U, m_address, m_port);
				m_n = 0x73U;
				break;
			default:
				::memcpy(buffer, REC73, 16U);
				::memcpy(buffer + 4U, m_speech + (m_pos * 11U), 11U);
				m_network.writeData(buffer, 16U, m_address, m_port);
				m_n = 0x62U;
				break;
			}

			if (m_pos == (m_count - 1U)) {
				m_network.writeData(REC80, 17U, m_address, m_port);
				m_state = SS_NONE;
				m_id = 0U;
				return;
			}

			m_pos++;
		}
	}
}

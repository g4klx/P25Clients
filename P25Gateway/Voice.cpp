/*
*   Copyright (C) 2017,2018 by Jonathan Naylor G4KLX
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

#include "NXDNCRC.h"
#include "Voice.h"
#include "Log.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <sys/stat.h>

const unsigned char HEADER[] = { 0x83U, 0x01U, 0x10U, 0x00U, 0x0FU, 0x01U, 0x00U, 0x20U };
const unsigned char TRAILER[] = { 0x83U, 0x01U, 0x10U, 0x00U, 0x0FU, 0x08U, 0x00U, 0x20U };

const unsigned char SILENCE[] = { 0xF0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x78U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U };

const unsigned char NXDN_FRAME_LENGTH = 33U;

const unsigned int NXDN_FRAME_TIME = 80U;

const unsigned int SILENCE_LENGTH = 4U;
const unsigned int AMBE_LENGTH = 13U;

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CVoice::CVoice(const std::string& directory, const std::string& language, unsigned int srcId) :
m_language(language),
m_indxFile(),
m_ambeFile(),
m_srcId(srcId),
m_status(VS_NONE),
m_timer(1000U, 1U),
m_stopWatch(),
m_sent(0U),
m_ambe(NULL),
m_voiceData(NULL),
m_voiceLength(0U),
m_positions()
{
	assert(!directory.empty());
	assert(!language.empty());

#if defined(_WIN32) || defined(_WIN64)
	m_indxFile = directory + "\\" + language + ".indx";
	m_ambeFile = directory + "\\" + language + ".nxdn";
#else
	m_indxFile = directory + "/" + language + ".indx";
	m_ambeFile = directory + "/" + language + ".nxdn";
#endif

	// Approximately 10 seconds worth
	m_voiceData = new unsigned char[120U * NXDN_FRAME_LENGTH];
}

CVoice::~CVoice()
{
	for (std::unordered_map<std::string, CPositions*>::iterator it = m_positions.begin(); it != m_positions.end(); ++it)
		delete it->second;

	m_positions.clear();

	delete[] m_ambe;
	delete[] m_voiceData;
}

bool CVoice::open()
{
	FILE* fpindx = ::fopen(m_indxFile.c_str(), "rt");
	if (fpindx == NULL) {
		LogError("Unable to open the index file - %s", m_indxFile.c_str());
		return false;
	}

	struct stat statStruct;
	int ret = ::stat(m_ambeFile.c_str(), &statStruct);
	if (ret != 0) {
		LogError("Unable to stat the AMBE file - %s", m_ambeFile.c_str());
		::fclose(fpindx);
		return false;
	}

	FILE* fpambe = ::fopen(m_ambeFile.c_str(), "rb");
	if (fpambe == NULL) {
		LogError("Unable to open the AMBE file - %s", m_ambeFile.c_str());
		::fclose(fpindx);
		return false;
	}

	m_ambe = new unsigned char[statStruct.st_size];

	size_t sizeRead = ::fread(m_ambe, 1U, statStruct.st_size, fpambe);
	if (sizeRead != 0U) {
		char buffer[80U];
		while (::fgets(buffer, 80, fpindx) != NULL) {
			char* p1 = ::strtok(buffer, "\t\r\n");
			char* p2 = ::strtok(NULL, "\t\r\n");
			char* p3 = ::strtok(NULL, "\t\r\n");

			if (p1 != NULL && p2 != NULL && p3 != NULL) {
				std::string symbol  = std::string(p1);
				unsigned int start  = ::atoi(p2) * AMBE_LENGTH;
				unsigned int length = ::atoi(p3) * AMBE_LENGTH;

				CPositions* pos = new CPositions;
				pos->m_start = start;
				pos->m_length = length;

				m_positions[symbol] = pos;
			}
		}
	}

	::fclose(fpindx);
	::fclose(fpambe);

	LogInfo("Loaded the audio and index file for %s", m_language.c_str());

	return true;
}

void CVoice::linkedTo(unsigned int tg)
{
	char letters[10U];
	::sprintf(letters, "%u", tg);

	std::vector<std::string> words;
	if (m_positions.count("linkedto") == 0U) {
		words.push_back("linked");
		words.push_back("2");
	} else {
		words.push_back("linkedto");
	}

	for (unsigned int i = 0U; letters[i] != 0x00U; i++)
		words.push_back(std::string(1U, letters[i]));

	createVoice(tg, words);
}

void CVoice::unlinked()
{
	std::vector<std::string> words;
	words.push_back("notlinked");

	createVoice(9999U, words);
}

void CVoice::createVoice(unsigned int tg, const std::vector<std::string>& words)
{
	unsigned int ambeLength = 0U;
	for (std::vector<std::string>::const_iterator it = words.begin(); it != words.end(); ++it) {
		if (m_positions.count(*it) > 0U) {
			CPositions* position = m_positions.at(*it);
			ambeLength += position->m_length;
		} else {
			LogWarning("Unable to find character/phrase \"%s\" in the index", (*it).c_str());
		}
	}

	// Ensure that the AMBE is an integer number of NXDN frames
	if ((ambeLength % (2U * AMBE_LENGTH)) != 0U)
		ambeLength++;

	// Add space for silence before and after the voice
	ambeLength += SILENCE_LENGTH * AMBE_LENGTH;
	ambeLength += SILENCE_LENGTH * AMBE_LENGTH;

	unsigned char* ambeData = new unsigned char[ambeLength];

	// Fill the AMBE data with silence
	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < (ambeLength / AMBE_LENGTH); i++, offset += AMBE_LENGTH)
		::memcpy(ambeData + offset, SILENCE, AMBE_LENGTH);

	// Put offset in for silence at the beginning
	unsigned int pos = SILENCE_LENGTH * AMBE_LENGTH;
	for (std::vector<std::string>::const_iterator it = words.begin(); it != words.end(); ++it) {
		if (m_positions.count(*it) > 0U) {
			CPositions* position = m_positions.at(*it);
			unsigned int start  = position->m_start;
			unsigned int length = position->m_length;
			::memcpy(ambeData + pos, m_ambe + start, length);
			pos += length;
		}
	}

	m_voiceLength = 0U;

	createHeader(true, tg);

	unsigned char sacch[12U];
	::memset(sacch, 0x00U, 12U);
	sacch[0U] = 0x01U;
	sacch[2U] = 0x20U;

	sacch[3U] = (m_srcId >> 8) & 0xFFU;
	sacch[4U] = (m_srcId >> 0) & 0xFFU;

	sacch[5U] = (tg >> 8) & 0xFFU;
	sacch[6U] = (tg >> 0) & 0xFFU;

	unsigned int n = 0U;
	for (unsigned int i = 0U; i < ambeLength; i += (2U * AMBE_LENGTH)) {
		unsigned char* p = ambeData + i;

		unsigned char buffer[NXDN_FRAME_LENGTH];
		::memset(buffer, 0x00U, NXDN_FRAME_LENGTH);

		buffer[0U] = 0xAEU;

		switch (n) {
		case 0U: buffer[1U] = 0xC1U; break;
		case 1U: buffer[1U] = 0x81U; break;
		case 2U: buffer[1U] = 0x41U; break;
		default: buffer[1U] = 0x01U; break;
		}

		for (unsigned int j = 0U; j < 18U; j++) {
			bool b = READ_BIT1(sacch, j + n * 18U);
			WRITE_BIT1(buffer, j + 16U, b);
		}

		CNXDNCRC::encodeCRC6(buffer + 1U, 26U);

		::memcpy(buffer + 5U + 0U,  p + 0U,  AMBE_LENGTH);
		::memcpy(buffer + 5U + 14U, p + 13U, AMBE_LENGTH);

		n = (n + 1U) % 4U;

		::memcpy(m_voiceData + m_voiceLength, buffer, NXDN_FRAME_LENGTH);
		m_voiceLength += NXDN_FRAME_LENGTH;
	}

	createTrailer(true, tg);

	delete[] ambeData;
}

unsigned int CVoice::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_status != VS_SENDING)
		return 0U;

	unsigned int count = m_stopWatch.elapsed() / NXDN_FRAME_TIME;

	if (m_sent < count) {
		unsigned int offset = m_sent * NXDN_FRAME_LENGTH;
		::memcpy(data, m_voiceData + offset, NXDN_FRAME_LENGTH);

		offset += NXDN_FRAME_LENGTH;
		m_sent++;

		if (offset >= m_voiceLength) {
			m_timer.stop();
			m_voiceLength = 0U;
			m_status = VS_NONE;
		}

		return NXDN_FRAME_LENGTH;
	}

	return 0U;
}

void CVoice::eof()
{
	if (m_voiceLength == 0U)
		return;

	m_status = VS_WAITING;

	m_timer.start();
}

void CVoice::clock(unsigned int ms)
{
	m_timer.clock(ms);
	if (m_timer.isRunning() && m_timer.hasExpired()) {
		if (m_status == VS_WAITING) {
			m_stopWatch.start();
			m_status = VS_SENDING;
			m_sent = 0U;
		}
	}
}

void CVoice::createHeader(bool grp, unsigned int dstId)
{
	unsigned char buffer[NXDN_FRAME_LENGTH];

	::memset(buffer, 0x00U, NXDN_FRAME_LENGTH);
	::memcpy(buffer, HEADER, 8U);

	if (grp)
		buffer[7U] = 0x20U;

	buffer[8U] = (m_srcId >> 8) & 0xFFU;
	buffer[9U] = (m_srcId >> 0) & 0xFFU;

	buffer[10U] = (dstId >> 8) & 0xFFU;
	buffer[11U] = (dstId >> 0) & 0xFFU;

	CNXDNCRC::encodeCRC12(buffer + 5U, 80U);

	::memcpy(buffer + 19U, buffer + 5U, 12U);

	::memcpy(m_voiceData + m_voiceLength, buffer, NXDN_FRAME_LENGTH);
	m_voiceLength += NXDN_FRAME_LENGTH;
}

void CVoice::createTrailer(bool grp, unsigned int dstId)
{
	unsigned char buffer[NXDN_FRAME_LENGTH];

	::memset(buffer, 0x00U, NXDN_FRAME_LENGTH);
	::memcpy(buffer, TRAILER, 8U);

	if (grp)
		buffer[7U] = 0x20U;

	buffer[8U] = (m_srcId >> 8) & 0xFFU;
	buffer[9U] = (m_srcId >> 0) & 0xFFU;

	buffer[10U] = (dstId >> 8) & 0xFFU;
	buffer[11U] = (dstId >> 0) & 0xFFU;

	CNXDNCRC::encodeCRC12(buffer + 5U, 80U);

	::memcpy(buffer + 19U, buffer + 5U, 12U);

	::memcpy(m_voiceData + m_voiceLength, buffer, NXDN_FRAME_LENGTH);
	m_voiceLength += NXDN_FRAME_LENGTH;
}

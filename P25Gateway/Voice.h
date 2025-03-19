/*
*   Copyright (C) 2017,2018,2019,2024,2025 by Jonathan Naylor G4KLX
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

#if !defined(Voice_H)
#define	Voice_H

#include "StopWatch.h"
#include "Timer.h"

#include <string>
#include <vector>
#include <unordered_map>

enum class VOICE_STATUS {
	NONE,
	WAITING,
	SENDING
};

struct CPositions {
	unsigned int m_start;
	unsigned int m_length;
};

class CVoice {
public:
	CVoice(const std::string& directory, const std::string& language, unsigned int srcId);
	~CVoice();

	bool open();

	void linkedTo(unsigned int tg);
	void unlinked();

	unsigned int read(unsigned char* data);

	void eof();

	void clock(unsigned int ms);

	bool isBusy() const;

private:
	std::string                            m_language;
	std::string                            m_indxFile;
	std::string                            m_imbeFile;
	unsigned int                           m_srcId;
	VOICE_STATUS                           m_status;
	CTimer                                 m_timer;
	CStopWatch                             m_stopWatch;
	unsigned int                           m_sent;
	unsigned int                           m_n;
	unsigned int                           m_dstId;
	unsigned char*                         m_imbe;
	unsigned char*                         m_voiceData;
	unsigned int                           m_voiceLength;
	std::unordered_map<std::string, CPositions*> m_positions;

	void createVoice(unsigned int tg, const std::vector<std::string>& words);
};

#endif

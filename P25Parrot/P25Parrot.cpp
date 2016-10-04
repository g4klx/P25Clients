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

#include "StopWatch.h"
#include "P25Parrot.h"
#include "Parrot.h"
#include "Network.h"
#include "Version.h"
#include "Timer.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv)
{
	if (argc == 1) {
		::fprintf(stderr, "Usage: P25Parrot [-d|--debug] <port>\n");
		return 1;
	}

	unsigned int n = 1U;

	bool debug = false;
	if (::strcmp(argv[1], "-d") == 0 || ::strcmp(argv[1], "--debug") == 0) {
		debug = true;
		n = 2U;
	}

	unsigned int port = ::atoi(argv[n]);
	if (port == 0U) {
		::fprintf(stderr, "P25Parrot: invalid port number - %s\n", argv[n]);
		return 1;
	}

	CP25Parrot parrot(port, debug);
	parrot.run();

	return 0;
}

CP25Parrot::CP25Parrot(unsigned int port, bool debug) :
m_port(port),
m_debug(debug)
{
}

CP25Parrot::~CP25Parrot()
{
}

void CP25Parrot::run()
{
	bool ret = ::LogInitialise(".", "P25Parrot", m_debug ? 1U : 2U, m_debug ? 1U : 2U);
	if (!ret) {
		::fprintf(stderr, "P25Parrot: unable to open the log file\n");
		return;
	}

	CParrot parrot(180U);
	CNetwork network(m_port);

	ret = network.open();
	if (!ret) {
		::LogFinalise();
		return;
	}

	CStopWatch stopWatch;
	stopWatch.start();

	CTimer watchdogTimer(1000U, 0U, 1500U);
	CTimer turnaroundTimer(1000U, 2U);

	CStopWatch playoutTimer;
	unsigned int count = 0U;
	bool playing = false;

	LogInfo("Starting P25Parrot-%s", VERSION);

	for (;;) {
		unsigned char buffer[200U];

		unsigned int len = network.read(buffer);
		if (len > 0U) {
			parrot.write(buffer, len);
			watchdogTimer.start();

			if ((buffer[0U] == 0x6AU && buffer[15U] == 0x00U) || (buffer[0U] == 0x73U && buffer[15U] == 0x00U)) {
				LogDebug("Received end of transmission");
				turnaroundTimer.start();
				watchdogTimer.stop();
				parrot.end();
			}
		}

		if (turnaroundTimer.isRunning() && turnaroundTimer.hasExpired()) {
			if (!playing) {
				playoutTimer.start();
				playing = true;
				count = 0U;
			}

			// A frame every 20ms
			unsigned int wanted = playoutTimer.elapsed() / 20U;
			while (count < wanted) {
				len = parrot.read(buffer);
				if (len > 0U) {
					network.write(buffer, len);
					count++;
				} else {
					parrot.clear();
					network.end();
					turnaroundTimer.stop();
					playing = false;
					count = wanted;
				}
			}
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		network.clock(ms);
		watchdogTimer.clock(ms);
		turnaroundTimer.clock(ms);

		if (watchdogTimer.isRunning() && watchdogTimer.hasExpired()) {
			LogDebug("Network watchdog has expired");
			turnaroundTimer.start();
			watchdogTimer.stop();
			parrot.end();
		}

		if (ms < 5U) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
		}
	}

	network.close();

	::LogFinalise();
}

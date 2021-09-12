/*
*   Copyright (C) 2016,2018,2020,2021 by Jonathan Naylor G4KLX
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

#include "P25Reflector.h"
#include "StopWatch.h"
#include "DMRLookup.h"
#include "Network.h"
#include "Version.h"
#include "Thread.h"
#include "Utils.h"
#include "Log.h"

#if defined(_WIN32) || defined(_WIN64)
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "P25Reflector.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/P25Reflector.ini";
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <cstring>
#include <algorithm>

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "P25Reflector version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: P25Reflector [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	CP25Reflector* reflector = new CP25Reflector(std::string(iniFile));
	reflector->run();
	delete reflector;

	return 0;
}

CP25Reflector::CP25Reflector(const std::string& file) :
m_conf(file),
m_repeaters()
{
	CUDPSocket::startup();
}

CP25Reflector::~CP25Reflector()
{
	CUDPSocket::shutdown();
}

void CP25Reflector::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "P25Reflector: cannot read the .ini file\n");
		return;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1) {
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return;
		}

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return;
			}

			// Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return;
			}
		}
	}
#endif

#if !defined(_WIN32) && !defined(_WIN64)
        ret = ::LogInitialise(m_daemon, m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), m_conf.getLogDisplayLevel(), m_conf.getLogFileRotate());
#else
        ret = ::LogInitialise(false, m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), m_conf.getLogDisplayLevel(), m_conf.getLogFileRotate());
#endif
	if (!ret) {
		::fprintf(stderr, "P25Gateway: unable to open the log file\n");
		return;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif

	CNetwork network(m_conf.getNetworkPort(), m_conf.getNetworkDebug());

	ret = network.open();
	if (!ret) {
		::LogFinalise();
		return;
	}
	
	CDMRLookup* lookup = new CDMRLookup(m_conf.getLookupName(), m_conf.getLookupTime());
	lookup->read();

	CStopWatch stopWatch;
	stopWatch.start();

	CTimer dumpTimer(1000U, 120U);
	dumpTimer.start();

	LogMessage("Starting P25Reflector-%s", VERSION);

	CP25Repeater* current = NULL;
	bool displayed = false;
	bool seen64 = false;
	bool seen65 = false;

	CTimer watchdogTimer(1000U, 0U, 1500U);

	unsigned char lcf = 0U;
	unsigned int srcId = 0U;
	unsigned int dstId = 0U;

	for (;;) {
		unsigned char buffer[200U];
		sockaddr_storage addr;
		unsigned int addrLen;

		unsigned int len = network.readData(buffer, 200U, addr, addrLen);
		if (len > 0U) {
			CP25Repeater* rpt = findRepeater(addr);

			if (buffer[0U] == 0xF0U) {
				if (rpt == NULL) {
					rpt = new CP25Repeater;
					rpt->m_timer.start();
					::memcpy(&rpt->m_addr, &addr, sizeof(struct sockaddr_storage));
					rpt->m_addrLen  = addrLen;
					rpt->m_callsign = std::string((char*)(buffer + 1U), 10U);
					m_repeaters.push_back(rpt);

					char buff[80U];
					LogMessage("Adding %s (%s)", rpt->m_callsign.c_str(), CUDPSocket::display(addr, buff, 80U));
				} else {
					rpt->m_timer.start();
				}

				// Return the poll
				network.writeData(buffer, len, addr, addrLen);
			} else if (buffer[0U] == 0xF1U && rpt != NULL) {
				char buff[80U];
				LogMessage("Removing %s (%s) unlinked", rpt->m_callsign.c_str(), CUDPSocket::display(addr, buff, 80U));

				for (std::vector<CP25Repeater*>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
					if (CUDPSocket::match((*it)->m_addr, rpt->m_addr)) {
						delete *it;
						m_repeaters.erase(it);
						break;
					}
				}
			} else if (rpt != NULL) {
				rpt->m_timer.start();

				if (current == NULL) {
					current   = rpt;
					displayed = false;
					seen64    = false;
					seen65    = false;
					LogMessage("Transmission started from %s", current->m_callsign.c_str());
				}

				if (current == rpt) {
					watchdogTimer.start();

					if (buffer[0U] == 0x64U && !seen64) {
						lcf = buffer[1U];
						seen64 = true;
					}

					if (buffer[0U] == 0x65U && !seen65) {
						dstId = (buffer[1U] << 16) & 0xFF0000U;
						dstId |= (buffer[2U] << 8) & 0x00FF00U;
						dstId |= (buffer[3U] << 0) & 0x0000FFU;
						seen65 = true;
					}

					if (buffer[0U] == 0x66U && seen64 && seen65 && !displayed) {
						srcId = (buffer[1U] << 16) & 0xFF0000U;
						srcId |= (buffer[2U] << 8) & 0x00FF00U;
						srcId |= (buffer[3U] << 0) & 0x0000FFU;
						displayed = true;

						std::string callsign = lookup->find(srcId);
						LogMessage("Transmission from %s at %s to %s%u", callsign.c_str(), current->m_callsign.c_str(), lcf == 0x00U ? "TG " : "", dstId);
					}

					for (std::vector<CP25Repeater*>::const_iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
						if (!CUDPSocket::match(addr, (*it)->m_addr))
							network.writeData(buffer, len, (*it)->m_addr, (*it)->m_addrLen);
					}

					if (buffer[0U] == 0x80U) {
						LogMessage("Received end of transmission");
						watchdogTimer.stop();
						current = NULL;
					}
				}
			} else {
				LogMessage("Data received from an unknown source");
				CUtils::dump(2U, "Data", buffer, len);
			}
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		// Remove any repeaters that haven't reported for a while
		for (std::vector<CP25Repeater*>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
			(*it)->m_timer.clock(ms);

		for (std::vector<CP25Repeater*>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
			if ((*it)->m_timer.hasExpired()) {
				char buff[80U];
				LogMessage("Removing %s (%s) disappeared", (*it)->m_callsign.c_str(),
														   CUDPSocket::display((*it)->m_addr, buff, 80U));

				delete *it;
				m_repeaters.erase(it);
				break;
			}
		}

		watchdogTimer.clock(ms);
		if (watchdogTimer.isRunning() && watchdogTimer.hasExpired()) {
			LogMessage("Network watchdog has expired");
			watchdogTimer.stop();
			current = NULL;
		}

		dumpTimer.clock(ms);
		if (dumpTimer.hasExpired()) {
			dumpRepeaters();
			dumpTimer.start();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	network.close();

	lookup->stop();

	::LogFinalise();
}

CP25Repeater* CP25Reflector::findRepeater(const sockaddr_storage& addr) const
{
	for (std::vector<CP25Repeater*>::const_iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
		if (CUDPSocket::match(addr, (*it)->m_addr))
			return *it;
	}

	return NULL;
}

void CP25Reflector::dumpRepeaters() const
{
	if (m_repeaters.size() == 0U) {
		LogMessage("No repeaters linked");
		return;
	}

	LogMessage("Currently linked repeaters:");

	for (std::vector<CP25Repeater*>::const_iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
		char buffer[80U];
		LogMessage("    %s: %s %u/%u", (*it)->m_callsign.c_str(),
			   CUDPSocket::display((*it)->m_addr, buffer, 80U),
			   (*it)->m_timer.getTimer(),
			   (*it)->m_timer.getTimeout());
	}
}

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

#include "P25Gateway.h"
#include "Reflectors.h"
#include "StopWatch.h"
#include "DMRLookup.h"
#include "Network.h"
#include "Version.h"
#include "Log.h"

#if defined(_WIN32) || defined(_WIN64)
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
const char* DEFAULT_INI_FILE = "P25Gateway.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/P25Gateway.ini";
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <cstring>

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "P25Gateway version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: P25Gateway [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	CP25Gateway* gateway = new CP25Gateway(std::string(iniFile));
	gateway->run();
	delete gateway;

	return 0;
}

CP25Gateway::CP25Gateway(const std::string& file) :
m_conf(file)
{
}

CP25Gateway::~CP25Gateway()
{
}

void CP25Gateway::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "P25Gateway: cannot read the .ini file\n");
		return;
	}

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), 1U, 1U);
	if (!ret) {
		::fprintf(stderr, "P25Gateway: unable to open the log file\n");
		return;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::LogWarning("Couldn't fork() , exiting");
			return;
		}
		else if (pid != 0)
			exit(EXIT_SUCCESS);

		// Create new session and process group
		if (::setsid() == -1) {
			::LogWarning("Couldn't setsid(), exiting");
			return;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::LogWarning("Couldn't cd /, exiting");
			return;
		}

		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);

		//If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::LogError("Could not get the mmdvm user, exiting");
				return;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			//Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::LogWarning("Could not set mmdvm GID, exiting");
				return;
			}

			if (setuid(mmdvm_uid) != 0) {
				::LogWarning("Could not set mmdvm UID, exiting");
				return;
			}

			//Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::LogWarning("It's possible to regain root - something is wrong!, exiting");
				return;
			}
		}
	}
#endif

	in_addr rptAddr = CUDPSocket::lookup(m_conf.getRptAddress());
	unsigned int rptPort = m_conf.getRptPort();

	CNetwork localNetwork(m_conf.getMyPort(), m_conf.getCallsign(), false);

	ret = localNetwork.open();
	if (!ret) {
		::LogFinalise();
		return;
	}

	CNetwork* remoteNetwork = NULL;
	if (m_conf.getNetworkEnabled()) {
		remoteNetwork = new CNetwork(m_conf.getNetworkDataPort(), m_conf.getCallsign(), m_conf.getNetworkDebug());
		ret = remoteNetwork->open();
		if (!ret) {
			localNetwork.close();
			::LogFinalise();
			return;
		}
	}

	CReflectors reflectors(m_conf.getNetworkHosts(), m_conf.getNetworkReloadTime());
	if (m_conf.getNetworkParrotPort() > 0U)
		reflectors.setParrot(m_conf.getNetworkParrotAddress(), m_conf.getNetworkParrotPort());
	reflectors.load();

	CDMRLookup* lookup = new CDMRLookup(m_conf.getLookupName(), m_conf.getLookupTime());
	lookup->read();

	CTimer lostTimer(1000U, 120U);
	CTimer pollTimer(1000U, 20U);

	CStopWatch stopWatch;
	stopWatch.start();

	LogMessage("Starting P25Gateway-%s", VERSION);

	unsigned int srcId = 0U;
	unsigned int dstId = 0U;

	unsigned int currentId = 9999U;
	in_addr currentAddr;
	unsigned int currentPort = 0U;

	if (remoteNetwork != NULL) {
		unsigned int id = m_conf.getNetworkStartup();
		if (id != 9999U) {
			CP25Reflector* reflector = reflectors.find(id);
			if (reflector != NULL) {
				currentId   = id;
				currentAddr = reflector->m_address;
				currentPort = reflector->m_port;

				pollTimer.start();
				lostTimer.start();

				remoteNetwork->writePoll(currentAddr, currentPort);
				remoteNetwork->writePoll(currentAddr, currentPort);
				remoteNetwork->writePoll(currentAddr, currentPort);

				LogMessage("Linked at startup to reflector %u", currentId);
			}
		}
	}

	for (;;) {
		unsigned char buffer[200U];
		in_addr address;
		unsigned int port;

		// From the reflector to the MMDVM
		if (remoteNetwork != NULL) {
			unsigned int len = remoteNetwork->readData(buffer, 200U, address, port);
			if (len > 0U) {
				// If we're linked and it's from the right place, send it on
				if (currentId != 9999U && currentAddr.s_addr == address.s_addr && currentPort == port) {
					// Don't pass reflector control data through to the MMDVM
					if (buffer[0U] != 0xF0U && buffer[0U] != 0xF1U) {
						// Rewrite the LCF and the destination TG
						if (buffer[0U] == 0x64U) {
							buffer[1U] = 0x00U;			// LCF is for TGs
						} else if (buffer[0U] == 0x65U) {
							buffer[1U] = (currentId >> 16) & 0xFFU;
							buffer[2U] = (currentId >> 8) & 0xFFU;
							buffer[3U] = (currentId >> 0) & 0xFFU;
						}

						localNetwork.writeData(buffer, len, rptAddr, rptPort);
					}

					// Any network activity is proof that the reflector is alive
					lostTimer.start();
				}
			}
		}

		// From the MMDVM to the reflector or control data
		unsigned int len = localNetwork.readData(buffer, 200U, address, port);
		if (len > 0U) {
			if (buffer[0U] == 0x65U) {
				dstId  = (buffer[1U] << 16) & 0xFF0000U;
				dstId |= (buffer[2U] << 8)  & 0x00FF00U;
				dstId |= (buffer[3U] << 0)  & 0x0000FFU;
			} else if (buffer[0U] == 0x66U) {
				srcId  = (buffer[1U] << 16) & 0xFF0000U;
				srcId |= (buffer[2U] << 8)  & 0x00FF00U;
				srcId |= (buffer[3U] << 0)  & 0x0000FFU;

				if (dstId != currentId) {
					if (dstId == 9999U) {
						std::string callsign = lookup->find(srcId);
						LogMessage("Unlinked from reflector %u by %s", currentId, callsign.c_str());
						currentId = dstId;

						if (remoteNetwork != NULL) {
							remoteNetwork->writeUnlink(currentAddr, currentPort);
							remoteNetwork->writeUnlink(currentAddr, currentPort);
							remoteNetwork->writeUnlink(currentAddr, currentPort);
							pollTimer.stop();
							lostTimer.stop();
						}
					} else {
						CP25Reflector* reflector = reflectors.find(dstId);
						if (reflector != NULL) {
							currentId   = dstId;
							currentAddr = reflector->m_address;
							currentPort = reflector->m_port;

							std::string callsign = lookup->find(srcId);
							LogMessage("Linked to reflector %u by %s", currentId, callsign.c_str());

							if (remoteNetwork != NULL) {
								remoteNetwork->writePoll(currentAddr, currentPort);
								remoteNetwork->writePoll(currentAddr, currentPort);
								remoteNetwork->writePoll(currentAddr, currentPort);
								pollTimer.start();
								lostTimer.start();
							}
						}
					}
				}
			}

			// If we're linked and we have a network, send it on
			if (currentId != 9999U && remoteNetwork != NULL) {
				// Rewrite the LCF and the destination TG
				if (buffer[0U] == 0x64U) {
					buffer[1U] = 0x00U;			// LCF is for TGs
				} else if (buffer[0U] == 0x65U) {
					buffer[1U] = (currentId >> 16) & 0xFFU;
					buffer[2U] = (currentId >> 8)  & 0xFFU;
					buffer[3U] = (currentId >> 0)  & 0xFFU;
				}

				remoteNetwork->writeData(buffer, len, currentAddr, currentPort);
			}
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		reflectors.clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			if (currentId != 9999U && remoteNetwork != NULL)
				remoteNetwork->writePoll(currentAddr, currentPort);
			pollTimer.start();
		}

		lostTimer.clock(ms);
		if (lostTimer.isRunning() && lostTimer.hasExpired()) {
			if (currentId != 9999U) {
				LogWarning("No response from %u, unlinking", currentId);
				currentId = 9999U;
			}
			lostTimer.stop();
		}

		if (ms < 5U) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
		}
	}

	localNetwork.close();

	if (remoteNetwork != NULL) {
		remoteNetwork->close();
		delete remoteNetwork;
	}

	lookup->stop();

	::LogFinalise();
}

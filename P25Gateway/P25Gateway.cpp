/*
*   Copyright (C) 2016-2020 by Jonathan Naylor G4KLX
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
#include "Thread.h"
#include "Voice.h"
#include "Timer.h"
#include "Utils.h"
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

const unsigned P25_VOICE_ID = 10999U;

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

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), 1U, 1U);
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

	in_addr rptAddr = CUDPSocket::lookup(m_conf.getRptAddress());
	unsigned int rptPort = m_conf.getRptPort();

	CUDPSocket* remoteSocket = NULL;
	if (m_conf.getRemoteCommandsEnabled()) {
		remoteSocket = new CUDPSocket(m_conf.getRemoteCommandsPort());
		ret = remoteSocket->open();
		if (!ret) {
			delete remoteSocket;
			remoteSocket = NULL;
		}
	}

	CNetwork localNetwork(m_conf.getMyPort(), m_conf.getCallsign(), false);
	ret = localNetwork.open();
	if (!ret) {
		::LogFinalise();
		return;
	}

	CNetwork remoteNetwork(m_conf.getNetworkPort(), m_conf.getCallsign(), m_conf.getNetworkDebug());
	ret = remoteNetwork.open();
	if (!ret) {
		localNetwork.close();
		::LogFinalise();
		return;
	}

	CReflectors reflectors(m_conf.getNetworkHosts1(), m_conf.getNetworkHosts2(), m_conf.getNetworkReloadTime());
	if (m_conf.getNetworkParrotPort() > 0U)
		reflectors.setParrot(m_conf.getNetworkParrotAddress(), m_conf.getNetworkParrotPort());
	if (m_conf.getNetworkP252DMRPort() > 0U)
		reflectors.setP252DMR(m_conf.getNetworkP252DMRAddress(), m_conf.getNetworkP252DMRPort());
	if (m_conf.getNetworkP252PCMPort() > 0U)
		reflectors.setP252PCM(m_conf.getNetworkP252PCMAddress(), m_conf.getNetworkP252PCMPort());
	reflectors.load();

	CDMRLookup* lookup = new CDMRLookup(m_conf.getLookupName(), m_conf.getLookupTime());
	lookup->read();

	CTimer inactivityTimer(1000U, m_conf.getNetworkInactivityTimeout() * 60U);
	CTimer lostTimer(1000U, 120U);
	CTimer pollTimer(1000U, 5U);

	CStopWatch stopWatch;
	stopWatch.start();

	CVoice* voice = NULL;
	if (m_conf.getVoiceEnabled()) {
		voice = new CVoice(m_conf.getVoiceDirectory(), m_conf.getVoiceLanguage(), P25_VOICE_ID);
		bool ok = voice->open();
		if (!ok) {
			delete voice;
			voice = NULL;
		}
	}

	LogMessage("Starting P25Gateway-%s", VERSION);

	unsigned int srcId = 0U;
	unsigned int dstId = 0U;

	unsigned int currentId = 9999U;
	in_addr currentAddr;
	unsigned int currentPort = 0U;

	unsigned int startupId = m_conf.getNetworkStartup();
	bool p252dmr_enabled = (startupId == 20) ? true : false;
	bool p252pcm_enabled = (startupId == 30) ? true : false;
	
	if (startupId != 9999U) {
		CP25Reflector* reflector = reflectors.find(startupId);
		if (reflector != NULL) {
			currentId   = startupId;
			currentAddr = reflector->m_address;
			currentPort = reflector->m_port;

			inactivityTimer.start();
			pollTimer.start();
			lostTimer.start();

			remoteNetwork.writePoll(currentAddr, currentPort);
			remoteNetwork.writePoll(currentAddr, currentPort);
			remoteNetwork.writePoll(currentAddr, currentPort);

			LogMessage("Linked at startup to reflector %u", currentId);
		}
	}

	for (;;) {
		unsigned char buffer[200U];
		in_addr address;
		unsigned int port;

		// From the reflector to the MMDVM
		unsigned int len = remoteNetwork.readData(buffer, 200U, address, port);
		if (len > 0U) {
			// If we're linked and it's from the right place, send it on
			if (currentId != 9999U && currentAddr.s_addr == address.s_addr && currentPort == port) {
				// Don't pass reflector control data through to the MMDVM
				if (buffer[0U] != 0xF0U && buffer[0U] != 0xF1U) {
					// Rewrite the LCF and the destination TG
					if (buffer[0U] == 0x64U) {
						buffer[1U] = 0x00U;			// LCF is for TGs
					} else if ((buffer[0U] == 0x65U) && !p252pcm_enabled) {
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

		// From the MMDVM to the reflector or control data
		len = localNetwork.readData(buffer, 200U, address, port);
		if (len > 0U) {
			if (buffer[0U] == 0x65U) {
				dstId  = (buffer[1U] << 16) & 0xFF0000U;
				dstId |= (buffer[2U] << 8)  & 0x00FF00U;
				dstId |= (buffer[3U] << 0)  & 0x0000FFU;
			} else if (buffer[0U] == 0x66U) {
				srcId  = (buffer[1U] << 16) & 0xFF0000U;
				srcId |= (buffer[2U] << 8)  & 0x00FF00U;
				srcId |= (buffer[3U] << 0)  & 0x0000FFU;
				
				if(p252dmr_enabled || p252pcm_enabled){
					currentId = dstId;
				}
				else if (dstId != currentId) {
					CP25Reflector* reflector = NULL;
					if (dstId != 9999U)
						reflector = reflectors.find(dstId);

					// If we're unlinking or changing reflectors, unlink from the current one
					if (dstId == 9999U || reflector != NULL) {
						std::string callsign = lookup->find(srcId);

						if (currentId != 9999U) {
							LogMessage("Unlinked from reflector %u by %s", currentId, callsign.c_str());

							remoteNetwork.writeUnlink(currentAddr, currentPort);
							remoteNetwork.writeUnlink(currentAddr, currentPort);
							remoteNetwork.writeUnlink(currentAddr, currentPort);

							inactivityTimer.stop();
							pollTimer.stop();
							lostTimer.stop();
						}

						if (voice != NULL) {
							if (dstId == 9999U)
								voice->unlinked();
							else
								voice->linkedTo(dstId);
						}

						currentId = dstId;
					}

					// Link to the new reflector
					if (reflector != NULL) {
						currentId   = dstId;
						currentAddr = reflector->m_address;
						currentPort = reflector->m_port;

						std::string callsign = lookup->find(srcId);
						LogMessage("Linked to reflector %u by %s", currentId, callsign.c_str());

						remoteNetwork.writePoll(currentAddr, currentPort);
						remoteNetwork.writePoll(currentAddr, currentPort);
						remoteNetwork.writePoll(currentAddr, currentPort);

						inactivityTimer.start();
						pollTimer.start();
						lostTimer.start();
					}
				}
			} else if (buffer[0U] == 0x80U) {
				if (voice != NULL)
					voice->eof();
			}

			// If we're linked and we have a network, send it on
			if (currentId != 9999U) {
				// Rewrite the LCF and the destination TG
				if (buffer[0U] == 0x64U) {
					buffer[1U] = 0x00U;			// LCF is for TGs
				} else if (buffer[0U] == 0x65U) {
					buffer[1U] = (currentId >> 16) & 0xFFU;
					buffer[2U] = (currentId >> 8)  & 0xFFU;
					buffer[3U] = (currentId >> 0)  & 0xFFU;
				}

				remoteNetwork.writeData(buffer, len, currentAddr, currentPort);
				inactivityTimer.start();
			}
		}

		if (voice != NULL) {
			unsigned int length = voice->read(buffer);
			if (length > 0U)
				localNetwork.writeData(buffer, length, rptAddr, rptPort);
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		reflectors.clock(ms);

		if (remoteSocket != NULL) {
			int res = remoteSocket->read(buffer, 200U, address, port);
			if (res > 0) {
				buffer[res] = '\0';
				if (::memcmp(buffer + 0U, "TalkGroup", 9U) == 0) {
					unsigned int tg = (unsigned int)::atoi((char*)(buffer + 9U));

					CP25Reflector* reflector = NULL;
					if (tg != 9999U)
						reflector = reflectors.find(tg);

					if (reflector == NULL && currentId != 9999U) {
						LogMessage("Unlinked from reflector %u by remote command", currentId);

						if (voice != NULL)
							voice->unlinked();

						remoteNetwork.writeUnlink(currentAddr, currentPort);
						remoteNetwork.writeUnlink(currentAddr, currentPort);
						remoteNetwork.writeUnlink(currentAddr, currentPort);

						inactivityTimer.stop();
						pollTimer.stop();
						lostTimer.stop();

						currentId = 9999U;
					} else if (reflector != NULL && currentId == 9999U) {
						currentId = tg;
						currentAddr = reflector->m_address;
						currentPort = reflector->m_port;

						LogMessage("Linked to reflector %u by remote command", currentId);

						if (voice != NULL)
							voice->linkedTo(currentId);

						remoteNetwork.writePoll(currentAddr, currentPort);
						remoteNetwork.writePoll(currentAddr, currentPort);
						remoteNetwork.writePoll(currentAddr, currentPort);

						inactivityTimer.start();
						pollTimer.start();
						lostTimer.start();
					} else if (reflector != NULL && currentId != 9999U) {
						LogMessage("Unlinked from reflector %u by remote command", currentId);

						remoteNetwork.writeUnlink(currentAddr, currentPort);
						remoteNetwork.writeUnlink(currentAddr, currentPort);
						remoteNetwork.writeUnlink(currentAddr, currentPort);

						currentId = tg;
						currentAddr = reflector->m_address;
						currentPort = reflector->m_port;

						LogMessage("Linked to reflector %u by remote command", currentId);

						if (voice != NULL)
							voice->linkedTo(currentId);

						remoteNetwork.writePoll(currentAddr, currentPort);
						remoteNetwork.writePoll(currentAddr, currentPort);
						remoteNetwork.writePoll(currentAddr, currentPort);

						inactivityTimer.start();
						pollTimer.start();
						lostTimer.start();
					}
				} else {
					CUtils::dump("Invalid remote command received", buffer, res);
				}
			}
		}

		if (voice != NULL)
			voice->clock(ms);

		inactivityTimer.clock(ms);
		if (inactivityTimer.isRunning() && inactivityTimer.hasExpired()) {
			if (currentId != 9999U && startupId == 9999U) {
				LogMessage("Unlinking from %u due to inactivity", currentId);

				remoteNetwork.writeUnlink(currentAddr, currentPort);
				remoteNetwork.writeUnlink(currentAddr, currentPort);
				remoteNetwork.writeUnlink(currentAddr, currentPort);

				if (voice != NULL)
					voice->unlinked();
				currentId = 9999U;

				pollTimer.stop();
				lostTimer.stop();
				inactivityTimer.stop();
			} else if (currentId != startupId) {
				if (currentId != 9999U) {
					remoteNetwork.writeUnlink(currentAddr, currentPort);
					remoteNetwork.writeUnlink(currentAddr, currentPort);
					remoteNetwork.writeUnlink(currentAddr, currentPort);
				}

				CP25Reflector* reflector = reflectors.find(startupId);
				if (reflector != NULL) {
					currentId   = startupId;
					currentAddr = reflector->m_address;
					currentPort = reflector->m_port;

					inactivityTimer.start();
					pollTimer.start();
					lostTimer.start();

					LogMessage("Linked to reflector %u due to inactivity", currentId);

					if (voice != NULL)
						voice->linkedTo(currentId);

					remoteNetwork.writePoll(currentAddr, currentPort);
					remoteNetwork.writePoll(currentAddr, currentPort);
					remoteNetwork.writePoll(currentAddr, currentPort);
				} else {
					startupId = 9999U;
					inactivityTimer.stop();
					pollTimer.stop();
					lostTimer.stop();
				}
			}
		}

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			if (currentId != 9999U)
				remoteNetwork.writePoll(currentAddr, currentPort);
			pollTimer.start();
		}

		lostTimer.clock(ms);
		if (lostTimer.isRunning() && lostTimer.hasExpired()) {
			if (currentId != 9999U) {
				LogWarning("No response from %u, unlinking", currentId);
				currentId = 9999U;
			}

			inactivityTimer.stop();
			lostTimer.stop();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	delete voice;

	if (remoteSocket != NULL) {
		remoteSocket->close();
		delete remoteSocket;
	}

	localNetwork.close();

	remoteNetwork.close();

	lookup->stop();

	::LogFinalise();
}

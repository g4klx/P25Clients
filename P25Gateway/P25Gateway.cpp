/*
*   Copyright (C) 2016-2025 by Jonathan Naylor G4KLX
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
#include "RptNetwork.h"
#include "P25Network.h"
#include "Reflectors.h"
#include "StopWatch.h"
#include "DMRLookup.h"
#include "Version.h"
#include "Thread.h"
#include "Timer.h"
#include "Utils.h"
#include "Log.h"
#include "GitVersion.h"

#if defined(_WIN32) || defined(_WIN64)
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
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
const unsigned int P25_FRAME_TIME = 20U;

static bool m_killed = false;
static int  m_signal = 0;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler(int signum)
{
	m_killed = true;
	m_signal = signum;
}
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <cstring>

class CStaticTG {
public:
	unsigned int     m_tg;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
};

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "P25Gateway version %s git #%.7s\n", VERSION, gitversion);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: P25Gateway [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

#if !defined(_WIN32) && !defined(_WIN64)
	::signal(SIGINT,  sigHandler);
	::signal(SIGTERM, sigHandler);
	::signal(SIGHUP,  sigHandler);
#endif

	int ret = 0;

	do {
		m_signal = 0;
		m_killed = false;

		CP25Gateway* gateway = new CP25Gateway(std::string(iniFile));
		ret = gateway->run();

		delete gateway;

		switch (m_signal) {
			case 0:
				break;
			case 2:
				::LogInfo("P25Gateway-%s exited on receipt of SIGINT", VERSION);
				break;
			case 15:
				::LogInfo("P25Gateway-%s exited on receipt of SIGTERM", VERSION);
				break;
			case 1:
				::LogInfo("P25Gateway-%s is restarting on receipt of SIGHUP", VERSION);
				break;
			default:
				::LogInfo("P25Gateway-%s exited on receipt of an unknown signal", VERSION);
				break;
		}
	} while (m_signal == 1);

	::LogFinalise();

	return ret;
}

CP25Gateway::CP25Gateway(const std::string& file) :
m_conf(file),
m_voice(nullptr)
{
	CUDPSocket::startup();
}

CP25Gateway::~CP25Gateway()
{
	CUDPSocket::shutdown();
}

int CP25Gateway::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "P25Gateway: cannot read the .ini file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return 1;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1) {
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return 1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return 1;
		}

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == nullptr) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return 1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return 1;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return 1;
			}

			// Double check it worked (AKA Paranoia)
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return 1;
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
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif

	sockaddr_storage rptAddr;
	unsigned int rptAddrLen;
	if (CUDPSocket::lookup(m_conf.getRptAddress(), m_conf.getRptPort(), rptAddr, rptAddrLen) != 0) {
		LogError("Unable to resolve the address of the host");
		return 1;
	}

	CRptNetwork localNetwork(m_conf.getMyPort(), rptAddr, rptAddrLen, m_conf.getCallsign(), m_conf.getDebug());
	ret = localNetwork.open();
	if (!ret) {
		::LogFinalise();
		return 1;
	}

	CP25Network remoteNetwork(m_conf.getNetworkPort(), m_conf.getCallsign(), m_conf.getNetworkDebug());
	ret = remoteNetwork.open();
	if (!ret) {
		localNetwork.close();
		::LogFinalise();
		return 1;
	}

	CUDPSocket* remoteSocket = nullptr;
	if (m_conf.getRemoteCommandsEnabled()) {
		remoteSocket = new CUDPSocket(m_conf.getRemoteCommandsPort());
		ret = remoteSocket->open();
		if (!ret) {
			delete remoteSocket;
			remoteSocket = nullptr;
		}
	}

	CReflectors reflectors(m_conf.getNetworkHosts1(), m_conf.getNetworkHosts2(), m_conf.getNetworkReloadTime());
	if (m_conf.getNetworkParrotPort() > 0U)
		reflectors.setParrot(m_conf.getNetworkParrotAddress(), m_conf.getNetworkParrotPort());
	if (m_conf.getNetworkP252DMRPort() > 0U)
		reflectors.setP252DMR(m_conf.getNetworkP252DMRAddress(), m_conf.getNetworkP252DMRPort());
	reflectors.load();

	CDMRLookup* lookup = new CDMRLookup(m_conf.getLookupName(), m_conf.getLookupTime());
	lookup->read();

	unsigned int rfHangTime  = m_conf.getNetworkRFHangTime();
	unsigned int netHangTime = m_conf.getNetworkNetHangTime();

	CTimer hangTimer(1000U);

	CTimer pollTimer(1000U, 5U);
	pollTimer.start();

	CStopWatch stopWatch;
	stopWatch.start();

	if (m_conf.getVoiceEnabled()) {
		m_voice = new CVoice(m_conf.getVoiceDirectory(), m_conf.getVoiceLanguage(), P25_VOICE_ID);
		bool ok = m_voice->open();
		if (!ok) {
			delete m_voice;
			m_voice = nullptr;
		}
	}

	LogMessage("Starting P25Gateway-%s", VERSION);

	unsigned int srcId = 0U;
	unsigned int dstTG = 0U;

	bool currentIsStatic        = false;
	unsigned int currentTG      = 0U;
	unsigned int currentAddrLen = 0U;
	sockaddr_storage currentAddr;
	unsigned char talkgroupBuff[4U];

	std::vector<unsigned int> staticIds = m_conf.getNetworkStatic();

	std::vector<CStaticTG> staticTGs;
	for (std::vector<unsigned int>::const_iterator it = staticIds.cbegin(); it != staticIds.cend(); ++it) {
		CP25Reflector* reflector = reflectors.find(*it);
		if (reflector != nullptr) {
			CStaticTG staticTG;
			staticTG.m_tg      = *it;
			staticTG.m_addr    = reflector->m_addr;
			staticTG.m_addrLen = reflector->m_addrLen;
			staticTGs.push_back(staticTG);

			remoteNetwork.poll(staticTG.m_addr, staticTG.m_addrLen);
			remoteNetwork.poll(staticTG.m_addr, staticTG.m_addrLen);
			remoteNetwork.poll(staticTG.m_addr, staticTG.m_addrLen);

			LogMessage("Statically linked to reflector %u", *it);
		}
	}

	while (!m_killed) {
		unsigned char buffer[200U];
		sockaddr_storage addr;
		unsigned int addrLen;

		// From the reflector to the MMDVM
		unsigned int len = remoteNetwork.read(buffer, 200U, addr, addrLen);
		// Read all queued packets so static talkgroup poll acks do not 
		// cause a problem.
		while (len > 0U) {
			// If we're linked and it's from the right place, send it on
			if (currentAddrLen > 0U && CUDPSocket::match(currentAddr, addr)) {
				// Don't pass reflector control data through to the MMDVM
				if (buffer[0U] != 0xF0U && buffer[0U] != 0xF1U) {
					// Rewrite the LCF and the destination TG
					if (buffer[0U] == 0x64U) {
						buffer[1U] = 0x00U;			// LCF is for TGs
					} else if (buffer[0U] == 0x65U) {
						buffer[1U] = (currentTG >> 16) & 0xFFU;
						buffer[2U] = (currentTG >> 8)  & 0xFFU;
						buffer[3U] = (currentTG >> 0)  & 0xFFU;
					}

					if (!isVoiceBusy())
						localNetwork.write(buffer, len);

					hangTimer.start();
				}
			} else if (currentTG == 0U) {
				bool poll = false;
				unsigned int receivedTG      = 0U;
				unsigned char pollReply[11U] = { 0xF0U };
				std::string callsign = m_conf.getCallsign();

				callsign.resize(10U, ' ');

				// Build poll reply data
				for (unsigned int i = 0U; i < 10U; i++)
					pollReply[i + 1U] = callsign.at(i);

				// Don't pass reflector control data through to the MMDVM
				unsigned int pollLen = 11U;
				if (len < pollLen)
					pollLen = len;

				poll = (::memcmp(buffer, pollReply, pollLen) == 0);

				// Find the static TG that this audio data belongs to
				for (std::vector<CStaticTG>::const_iterator it = staticTGs.cbegin(); it != staticTGs.cend(); ++it) {
					if (CUDPSocket::match(addr, (*it).m_addr)) {
						receivedTG = (*it).m_tg;
						break;
					}
				}
				// Reference for control byte buffer[0u]
				// https://github.com/Wodie/p25link/blob/master/MMDVM.pm
				if (buffer[0U] == 0xF0U  && poll) {
					// Poll response message
					// LogMessage("Received network poll response for talkgroup %u ", receivedTG);
				} else if (buffer[0U] == 0xF1U) {
					// Server talkgroup disconnect
					// LogMessage("Disconnect talkgroup for talkgroup %u ", receivedTG);
				} else {
					if (receivedTG != 0U) {
						// Changed talkgroup.  Let the modem know.
						// It may be told it by the content of the message.
						// Just in case send it anyway!
						unsigned char talkgroupBuff[4U];
						talkgroupBuff[0U] = 0x65U;
						talkgroupBuff[1U] = (receivedTG >> 16) & 0xFFU;
						talkgroupBuff[2U] = (receivedTG >> 8)  & 0xFFU;
						talkgroupBuff[3U] = (receivedTG >> 0)  & 0xFFU;

						if (!isVoiceBusy())
							localNetwork.write(talkgroupBuff, 4U);
					}

					currentTG = receivedTG;
					if (currentTG > 0U) {
						currentAddr     = addr;
						currentAddrLen  = addrLen;
						currentIsStatic = true;

						// Rewrite the LCF and the destination TG
						if (buffer[0U] == 0x64U) {
							buffer[1U] = 0x00U;			// LCF is for TGs
						} else if (buffer[0U] == 0x65U) {
							buffer[1U] = (currentTG >> 16) & 0xFFU;
							buffer[2U] = (currentTG >> 8)  & 0xFFU;
							buffer[3U] = (currentTG >> 0)  & 0xFFU;
						}

						if (!poll) {
							if (!isVoiceBusy())
								localNetwork.write(buffer, len);
						}

						LogMessage("Switched to reflector %u due to network activity", currentTG);

						hangTimer.setTimeout(netHangTime);
						hangTimer.start();
					}
				}
			}

			len = remoteNetwork.read(buffer, 200U, addr, addrLen);
		}

		// From the MMDVM to the reflector or control data
		len = localNetwork.read(buffer, 200U);
		while (len > 0U) {
			if (buffer[0U] == 0x65U) {
				dstTG  = (buffer[1U] << 16) & 0xFF0000U;
				dstTG |= (buffer[2U] << 8)  & 0x00FF00U;
				dstTG |= (buffer[3U] << 0)  & 0x0000FFU;
				if (dstTG != currentTG) {
					if (currentAddrLen > 0U) {
						std::string callsign = lookup->find(srcId);
						LogMessage("Unlinking from reflector %u", currentTG);

						if (!currentIsStatic) {
							remoteNetwork.unlink(currentAddr, currentAddrLen);
							remoteNetwork.unlink(currentAddr, currentAddrLen);
							remoteNetwork.unlink(currentAddr, currentAddrLen);
						}

						hangTimer.stop();
					}

					const CStaticTG* found = nullptr;
					for (std::vector<CStaticTG>::const_iterator it = staticTGs.cbegin(); it != staticTGs.cend(); ++it) {
						if (dstTG == (*it).m_tg) {
							found = &(*it);
							break;
						}
					}

					if (found == nullptr) {
						CP25Reflector* refl = reflectors.find(dstTG);
						if (refl != nullptr) {
							currentTG       = dstTG;
							currentAddr     = refl->m_addr;
							currentAddrLen  = refl->m_addrLen;
							currentIsStatic = false;
						} else {
							currentTG       = dstTG;
							currentAddrLen  = 0U;
							currentIsStatic = false;
						}
					} else {
						currentTG       = found->m_tg;
						currentAddr     = found->m_addr;
						currentAddrLen  = found->m_addrLen;
						currentIsStatic = true;
					}

					// Link to the new reflector
					if (currentAddrLen > 0U) {
						std::string callsign = lookup->find(srcId);
						LogMessage("Switched to reflector %u due to RF activity", currentTG);

						if (!currentIsStatic) {
							remoteNetwork.poll(currentAddr, currentAddrLen);
							remoteNetwork.poll(currentAddr, currentAddrLen);
							remoteNetwork.poll(currentAddr, currentAddrLen);
						}

						hangTimer.setTimeout(rfHangTime);
						hangTimer.start();
					} else {
						hangTimer.stop();
					}

					if (m_voice != nullptr) {
						if (currentAddrLen == 0U)
							m_voice->unlinked();
						else
							m_voice->linkedTo(dstTG);
					}
				}
			} else if (buffer[0U] == 0x66U) {
				srcId  = (buffer[1U] << 16) & 0xFF0000U;
				srcId |= (buffer[2U] << 8)  & 0x00FF00U;
				srcId |= (buffer[3U] << 0)  & 0x0000FFU;

			}

			if (buffer[0U] == 0x80U) {
				if (m_voice != nullptr)
					m_voice->eof();
			}

			// If we're linked and we have a network, send it on
			if (currentAddrLen > 0U) {
				// Rewrite the LCF and the destination TG
				if (buffer[0U] == 0x64U) {
					buffer[1U] = 0x00U;			// LCF is for TGs
				} else if (buffer[0U] == 0x65U) {
					buffer[1U] = (currentTG >> 16) & 0xFFU;
					buffer[2U] = (currentTG >> 8)  & 0xFFU;
					buffer[3U] = (currentTG >> 0)  & 0xFFU;
				}

				remoteNetwork.write(buffer, len, currentAddr, currentAddrLen);
				hangTimer.start();
			}

			len = localNetwork.read(buffer, 200U);
		}

		if (m_voice != nullptr) {
			unsigned int length = m_voice->read(buffer);
			if (length > 0U)
				localNetwork.write(buffer, length);
		}

		if (remoteSocket != nullptr) {
			sockaddr_storage addr;
			unsigned int addrLen;
			int res = remoteSocket->read(buffer, 200U, addr, addrLen);
			while (res > 0) {
				buffer[res] = '\0';
				if (::memcmp(buffer + 0U, "TalkGroup", 9U) == 0) {
					unsigned int tg = ((strlen((char*)buffer + 0U) > 10) ? (unsigned int)::atoi((char*)(buffer + 10U)) : 9999);

					if (tg != currentTG) {
						if (currentAddrLen > 0U) {
							LogMessage("Unlinked from reflector %u by remote command", currentTG);

							if (!currentIsStatic) {
								remoteNetwork.unlink(currentAddr, currentAddrLen);
								remoteNetwork.unlink(currentAddr, currentAddrLen);
								remoteNetwork.unlink(currentAddr, currentAddrLen);
							}

							hangTimer.stop();
						}

						const CStaticTG* found = nullptr;
						for (std::vector<CStaticTG>::const_iterator it = staticTGs.cbegin(); it != staticTGs.cend(); ++it) {
							if (tg == (*it).m_tg) {
								found = &(*it);
								break;
							}
						}

						if (found == nullptr) {
							CP25Reflector* refl = reflectors.find(tg);
							if (refl != nullptr) {
								currentTG       = tg;
								currentAddr     = refl->m_addr;
								currentAddrLen  = refl->m_addrLen;
								currentIsStatic = false;
							} else {
								currentTG       = tg;
								currentAddrLen  = 0U;
								currentIsStatic = false;
							}
						} else {
							currentTG       = found->m_tg;
							currentAddr     = found->m_addr;
							currentAddrLen  = found->m_addrLen;
							currentIsStatic = true;
						}

						// Link to the new reflector
						if (currentAddrLen > 0U) {
							LogMessage("Switched to reflector %u by remote command", currentTG);

							if (!currentIsStatic) {
								remoteNetwork.poll(currentAddr, currentAddrLen);
								remoteNetwork.poll(currentAddr, currentAddrLen);
								remoteNetwork.poll(currentAddr, currentAddrLen);
							}

							hangTimer.setTimeout(rfHangTime);
							hangTimer.start();
						} else {
							hangTimer.stop();
						}

						if (m_voice != nullptr) {
							if (currentAddrLen == 0U)
								m_voice->unlinked();
							else
								m_voice->linkedTo(currentTG);
						}

						if (currentAddrLen == 0U) {
							for (std::vector<CStaticTG>::const_iterator it = staticTGs.cbegin(); it != staticTGs.cend(); ++it)
								LogMessage("Statically linked to reflector %u", *it);
						}
					}
				} else if (::memcmp(buffer + 0U, "status", 6U) == 0) {
					std::string state = std::string("p25:") + (((currentAddrLen > 0) || !staticTGs.empty()) ? "conn" : "disc");
					remoteSocket->write((unsigned char*)state.c_str(), (unsigned int)state.length(), addr, addrLen);
				} else if (::memcmp(buffer + 0U, "host", 4U) == 0) {
					std::string ref;
					char buffer[INET6_ADDRSTRLEN];

					// Concat Statics, if defined.
					for (std::vector<CStaticTG>::const_iterator it = staticTGs.cbegin(); it != staticTGs.cend(); ++it) {

						// CurrentAddr is not in Static list
						if ((currentAddrLen == 0U) || ((currentAddrLen > 0U) && !CUDPSocket::match(currentAddr, (*it).m_addr))) {
							if (ref.length() > 0)
								ref += ",";
							
							if (::getnameinfo((struct sockaddr*)&(*it).m_addr, (*it).m_addrLen, buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST | NI_NUMERICSERV) == 0)
								ref += std::string(buffer);
						}
					}

					if (currentAddrLen > 0U) {
						if (::getnameinfo((struct sockaddr*)&currentAddr, currentAddrLen, buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
							if (ref.length() > 0)
								ref += ",";

							ref += std::string(buffer);
						}
					}

					std::string host = std::string("p25:\"") + ((ref.length() == 0) ? "NONE" : ref) + "\"";
					remoteSocket->write((unsigned char*)host.c_str(), (unsigned int)host.length(), addr, addrLen);
				} else {
					CUtils::dump("Invalid remote command received", buffer, res);
				}

				res = remoteSocket->read(buffer, 200U, addr, addrLen);
			}
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		reflectors.clock(ms);

		if (m_voice != nullptr)
			m_voice->clock(ms);

		hangTimer.clock(ms);
		if (hangTimer.isRunning() && hangTimer.hasExpired()) {
			if (currentAddrLen > 0U) {
				LogMessage("Unlinking from %u due to inactivity", currentTG);

				if (!currentIsStatic) {
					remoteNetwork.unlink(currentAddr, currentAddrLen);
					remoteNetwork.unlink(currentAddr, currentAddrLen);
					remoteNetwork.unlink(currentAddr, currentAddrLen);
				}

				if (m_voice != nullptr)
					m_voice->unlinked();

			}

			currentTG        = 0U;
			currentAddrLen   = 0U;
			currentIsStatic  = false;

			// Let modem know disconnected
			talkgroupBuff[0U] = 0x65U;
			talkgroupBuff[1U] = 0U;
			talkgroupBuff[2U] = 0U;
			talkgroupBuff[3U] = 0U;
			localNetwork.write(talkgroupBuff, 4);

			hangTimer.stop();
		}

		localNetwork.clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			// Poll the static TGs
			for (std::vector<CStaticTG>::const_iterator it = staticTGs.cbegin(); it != staticTGs.cend(); ++it)
				remoteNetwork.poll((*it).m_addr, (*it).m_addrLen);

			// Poll the dynamic TG
			if (!currentIsStatic && currentAddrLen > 0U)
				remoteNetwork.poll(currentAddr, currentAddrLen);

			pollTimer.start();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	delete m_voice;

	localNetwork.close();

	if (remoteSocket != nullptr) {
		remoteSocket->close();
		delete remoteSocket;
	}

	remoteNetwork.close();

	lookup->stop();

	return 0;
}

bool CP25Gateway::isVoiceBusy() const
{
	if (m_voice == nullptr)
		return false;

	return m_voice->isBusy();
}

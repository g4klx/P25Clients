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

#include "MQTTConnection.h"
#include "P25Gateway.h"
#include "RptNetwork.h"
#include "StopWatch.h"
#include "DMRLookup.h"
#include "Version.h"
#include "Thread.h"
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

// In Log.cpp
extern CMQTTConnection* m_mqtt;

static CP25Gateway* gateway = nullptr;

static bool m_killed = false;
static int  m_signal = 0;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler(int signum)
{
	m_killed = true;
	m_signal = signum;
}
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

		gateway = new CP25Gateway(std::string(iniFile));
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
m_voice(nullptr),
m_remoteNetwork(nullptr),
m_staticTGs(),
m_currentTG(),
m_currentIsStatic(false),
m_hangTimer(1000U),
m_rfHangTime(0U),
m_reflectors(nullptr)
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
			return -1;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1) {
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return -1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return -1;
		}

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == nullptr) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return -1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return -1;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return -1;
			}

			// Double check it worked (AKA Paranoia)
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}
#endif

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif
	::LogInitialise(m_conf.getLogDisplayLevel(), m_conf.getLogMQTTLevel());

	std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>> subscriptions;
	if (m_conf.getRemoteCommandsEnabled())
		subscriptions.push_back(std::make_pair("command", CP25Gateway::onCommand));

	m_mqtt = new CMQTTConnection(m_conf.getMQTTAddress(), m_conf.getMQTTPort(), m_conf.getMQTTName(), m_conf.getMQTTAuthEnabled(), m_conf.getMQTTUsername(), m_conf.getMQTTPassword(), subscriptions, m_conf.getMQTTKeepalive());
	ret = m_mqtt->open();
	if (!ret)
		return 1;

	sockaddr_storage rptAddr;
	unsigned int rptAddrLen;
	if (CUDPSocket::lookup(m_conf.getRptAddress(), m_conf.getRptPort(), rptAddr, rptAddrLen) != 0) {
		LogError("Unable to resolve the address of the host");
		return 1;
	}

	CRptNetwork localNetwork(m_conf.getMyPort(), rptAddr, rptAddrLen, m_conf.getCallsign(), m_conf.getDebug());
	ret = localNetwork.open();
	if (!ret)
		return 1;

	m_remoteNetwork = new CP25Network(m_conf.getNetworkPort(), m_conf.getCallsign(), m_conf.getNetworkDebug());
	ret = m_remoteNetwork->open();
	if (!ret) {
		delete m_remoteNetwork;
		localNetwork.close();
		return 1;
	}

	m_reflectors = new CReflectors(m_conf.getNetworkHosts1(), m_conf.getNetworkHosts2(), m_conf.getNetworkReloadTime());
	if (m_conf.getNetworkParrotPort() > 0U)
		m_reflectors->setParrot(m_conf.getNetworkParrotAddress(), m_conf.getNetworkParrotPort());
	if (m_conf.getNetworkP252DMRPort() > 0U)
		m_reflectors->setP252DMR(m_conf.getNetworkP252DMRAddress(), m_conf.getNetworkP252DMRPort());
	m_reflectors->load();

	CDMRLookup* lookup = new CDMRLookup(m_conf.getLookupName(), m_conf.getLookupTime());
	lookup->read();

	m_rfHangTime = m_conf.getNetworkRFHangTime();
	unsigned int netHangTime = m_conf.getNetworkNetHangTime();

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

	LogInfo("P25Gateway-%s is starting", VERSION);
	LogInfo("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	writeJSONStatus("P25Gateway is starting");

	unsigned int srcId = 0U;
	unsigned int dstTG = 0U;

	unsigned char talkgroupBuff[4U];

	std::vector<unsigned int> staticIds = m_conf.getNetworkStatic();

	for (const auto& it : staticIds) {
		CP25Reflector* reflector = m_reflectors->find(it);
		if (reflector != nullptr) {
			m_staticTGs.push_back(*reflector);

			m_remoteNetwork->poll(*reflector);
			m_remoteNetwork->poll(*reflector);
			m_remoteNetwork->poll(*reflector);

			LogMessage("Statically linked to reflector %u", it);
			writeJSONLinking("startup", it);
		} else {
			LogWarning("Unable to find static reflector %u", it);
		}
	}

	while (!m_killed) {
		unsigned char buffer[200U];
		sockaddr_storage addr;
		unsigned int addrLen;

		// From the reflector to the MMDVM
		unsigned int len = m_remoteNetwork->read(buffer, 200U, addr, addrLen);
		// Read all queued packets so static talkgroup poll acks do not 
		// cause a problem.
		while (len > 0U) {
			// If we're linked and it's from the right place, send it on
			if (m_currentTG.isUsed() && CP25Network::match(addr, m_currentTG)) {
				// Don't pass reflector control data through to the MMDVM
				if ((buffer[0U] != 0xF0U) && (buffer[0U] != 0xF1U)) {
					// Rewrite the LCF and the destination TG
					if (buffer[0U] == 0x64U) {
						buffer[1U] = 0x00U;			// LCF is for TGs
					} else if (buffer[0U] == 0x65U) {
						buffer[1U] = (m_currentTG.m_id >> 16) & 0xFFU;
						buffer[2U] = (m_currentTG.m_id >> 8)  & 0xFFU;
						buffer[3U] = (m_currentTG.m_id >> 0)  & 0xFFU;
					}

					if (!isVoiceBusy())
						localNetwork.write(buffer, len);

					m_hangTimer.start();
				}
			} else if (m_currentTG.isEmpty()) {
				bool poll = false;
				CP25Reflector receivedTG;
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
				for (const auto& it : m_staticTGs) {
					if (CP25Network::match(addr, it)) {
						receivedTG = it;
						break;
					}
				}
				// Reference for control byte buffer[0u]
				// https://github.com/Wodie/p25link/blob/master/MMDVM.pm
				if ((buffer[0U] == 0xF0U) && poll) {
					// Poll response message
					// LogMessage("Received network poll response for talkgroup %u ", receivedTG.m_id);
				} else if (buffer[0U] == 0xF1U) {
					// Server talkgroup disconnect
					// LogMessage("Disconnect talkgroup for talkgroup %u ", receivedTG.m_id);
				} else {
					if (receivedTG.isUsed()) {
						// Changed talkgroup.  Let the modem know.
						// It may be told it by the content of the message.
						// Just in case send it anyway!
						unsigned char talkgroupBuff[4U];
						talkgroupBuff[0U] = 0x65U;
						talkgroupBuff[1U] = (receivedTG.m_id >> 16) & 0xFFU;
						talkgroupBuff[2U] = (receivedTG.m_id >> 8)  & 0xFFU;
						talkgroupBuff[3U] = (receivedTG.m_id >> 0)  & 0xFFU;

						localNetwork.write(talkgroupBuff, 4U);
					}

					m_currentTG = receivedTG;
					if (receivedTG.isUsed()) {
						m_currentIsStatic = true;

						// Rewrite the LCF and the destination TG
						if (buffer[0U] == 0x64U) {
							buffer[1U] = 0x00U;			// LCF is for TGs
						} else if (buffer[0U] == 0x65U) {
							buffer[1U] = (m_currentTG.m_id >> 16) & 0xFFU;
							buffer[2U] = (m_currentTG.m_id >> 8)  & 0xFFU;
							buffer[3U] = (m_currentTG.m_id >> 0)  & 0xFFU;
						}

						if (!poll) {
							if (!isVoiceBusy())
								localNetwork.write(buffer, len);
						}

						LogMessage("Switched to reflector %u due to network activity", m_currentTG.m_id);
						writeJSONLinking("network", m_currentTG.m_id);

						m_hangTimer.setTimeout(netHangTime);
						m_hangTimer.start();
					}
				}
			}

			len = m_remoteNetwork->read(buffer, 200U, addr, addrLen);
		}

		// From the MMDVM to the reflector or control data
		len = localNetwork.read(buffer, 200U);
		while (len > 0U) {
			if (buffer[0U] == 0x65U) {
				dstTG  = (buffer[1U] << 16) & 0xFF0000U;
				dstTG |= (buffer[2U] << 8)  & 0x00FF00U;
				dstTG |= (buffer[3U] << 0)  & 0x0000FFU;
				if (dstTG != m_currentTG.m_id) {
					if (m_currentTG.isUsed()) {
						std::string callsign = lookup->find(srcId);
						LogMessage("Unlinking from reflector %u by %s", m_currentTG.m_id, callsign.c_str());
						writeJSONUnlinked("user");

						if (!m_currentIsStatic) {
							m_remoteNetwork->unlink(m_currentTG);
							m_remoteNetwork->unlink(m_currentTG);
							m_remoteNetwork->unlink(m_currentTG);
						}

						m_hangTimer.stop();
					}

					CP25Reflector found;
					for (const auto& it : m_staticTGs) {
						if (dstTG == it.m_id) {
							found = it;
							break;
						}
					}

					if (found.isEmpty()) {
						CP25Reflector* refl = m_reflectors->find(dstTG);
						if (refl != nullptr) {
							m_currentTG       = *refl;
							m_currentIsStatic = false;
						} else {
							m_currentTG.reset();
							m_currentIsStatic = false;
						}
					} else {
						m_currentTG       = found;
						m_currentIsStatic = true;
					}

					// Link to the new reflector
					if (m_currentTG.isUsed()) {
						std::string callsign = lookup->find(srcId);
						LogMessage("Switched to reflector %u due to RF activity from %s", m_currentTG.m_id, callsign.c_str());
						writeJSONLinking("user", m_currentTG.m_id);

						if (!m_currentIsStatic) {
							m_remoteNetwork->poll(m_currentTG);
							m_remoteNetwork->poll(m_currentTG);
							m_remoteNetwork->poll(m_currentTG);
						}

						m_hangTimer.setTimeout(m_rfHangTime);
						m_hangTimer.start();
					} else {
						m_hangTimer.stop();
					}

					if (m_voice != nullptr) {
						if (m_currentTG.isEmpty())
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
			if (m_currentTG.isUsed()) {
				// Rewrite the LCF and the destination TG
				if (buffer[0U] == 0x64U) {
					buffer[1U] = 0x00U;			// LCF is for TGs
				} else if (buffer[0U] == 0x65U) {
					buffer[1U] = (m_currentTG.m_id >> 16) & 0xFFU;
					buffer[2U] = (m_currentTG.m_id >> 8)  & 0xFFU;
					buffer[3U] = (m_currentTG.m_id >> 0)  & 0xFFU;
				}

				m_remoteNetwork->write(buffer, len, m_currentTG);
				m_hangTimer.start();
			}

			len = localNetwork.read(buffer, 200U);
		}

		if (m_voice != nullptr) {
			unsigned int length = m_voice->read(buffer);
			if (length > 0U)
				localNetwork.write(buffer, length);
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_reflectors->clock(ms);

		if (m_voice != nullptr)
			m_voice->clock(ms);

		m_hangTimer.clock(ms);
		if (m_hangTimer.isRunning() && m_hangTimer.hasExpired()) {
			if (m_currentTG.isUsed()) {
				LogMessage("Unlinking from reflector %u due to inactivity", m_currentTG.m_id);
				writeJSONUnlinked("timer");

				if (!m_currentIsStatic) {
					m_remoteNetwork->unlink(m_currentTG);
					m_remoteNetwork->unlink(m_currentTG);
					m_remoteNetwork->unlink(m_currentTG);
				}

				if (m_voice != nullptr)
					m_voice->unlinked();
			}
			
			m_currentTG.reset();
			m_currentIsStatic = false;

			// Let modem know disconnected
			talkgroupBuff[0U] = 0x65U;
			talkgroupBuff[1U] = 0U;
			talkgroupBuff[2U] = 0U;
			talkgroupBuff[3U] = 0U;
			localNetwork.write(talkgroupBuff, 4U);

			m_hangTimer.stop();
		}

		localNetwork.clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			// Poll the static TGs
			for (const auto& it : m_staticTGs)
				m_remoteNetwork->poll(it);

			// Poll the dynamic TG
			if (!m_currentIsStatic && m_currentTG.isUsed())
					m_remoteNetwork->poll(m_currentTG);

			pollTimer.start();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	delete m_voice;

	localNetwork.close();

	m_remoteNetwork->close();
	delete m_remoteNetwork;

	lookup->stop();

	return 0;
}

void CP25Gateway::writeCommand(const std::string& command)
{
	if (command.substr(0, 9) == "TalkGroup") {
		unsigned int tg = 9999U;
		if (command.length() > 10)
			tg = (unsigned int)std::stoi(command.substr(10));

		if (tg != m_currentTG.m_id) {
			if (m_currentTG.isUsed()) {
				LogMessage("Unlinked from reflector %u by remote command", m_currentTG.m_id);
				writeJSONUnlinked("remote");

				if (!m_currentIsStatic) {
					m_remoteNetwork->unlink(m_currentTG);
					m_remoteNetwork->unlink(m_currentTG);
					m_remoteNetwork->unlink(m_currentTG);
				}

				m_hangTimer.stop();
			}

			CP25Reflector found;
			for (const auto& it : m_staticTGs) {
				if (tg == it.m_id) {
					found = it;
					break;
				}
			}

			if (found.isEmpty()) {
				CP25Reflector* refl = m_reflectors->find(tg);
				if (refl != nullptr) {
					m_currentTG = *refl;
					m_currentIsStatic = false;
				} else {
					m_currentTG.reset();
					m_currentIsStatic = false;
				}
			} else {
				m_currentTG = found;
				m_currentIsStatic = true;
			}

			// Link to the new reflector
			if (m_currentTG.isUsed()) {
				LogMessage("Switched to reflector %u by remote command", m_currentTG.m_id);
				writeJSONLinking("remote", m_currentTG.m_id);

				if (!m_currentIsStatic) {
					m_remoteNetwork->poll(m_currentTG);
					m_remoteNetwork->poll(m_currentTG);
					m_remoteNetwork->poll(m_currentTG);
				}

				m_hangTimer.setTimeout(m_rfHangTime);
				m_hangTimer.start();
			} else {
				m_hangTimer.stop();
			}

			if (m_voice != nullptr) {
				if (m_currentTG.isEmpty())
					m_voice->unlinked();
				else
					m_voice->linkedTo(m_currentTG.m_id);
			}
		}
	} else if (command.substr(0, 6) == "status") {
		std::string state = std::string("p25:") + (m_currentTG.isUsed() ? "conn" : "disc");
		m_mqtt->publish("response", state);
	} else if (command.substr(0, 4) == "host") {
		std::string host = "p25:\"NONE\"";

		if (m_currentTG.isUsed()) {
			if (m_remoteNetwork->hasIPv6() && m_currentTG.hasIPv6()) {
				char buffer[100U];
				if (::getnameinfo((struct sockaddr*)&m_currentTG.IPv6.m_addr, m_currentTG.IPv6.m_addrLen, buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST | NI_NUMERICSERV) == 0)
					host = "p25:\"" + std::string(buffer) + "\"";
			} else if (m_remoteNetwork->hasIPv4() && m_currentTG.hasIPv4()) {
				char buffer[100U];
				if (::getnameinfo((struct sockaddr*)&m_currentTG.IPv4.m_addr, m_currentTG.IPv4.m_addrLen, buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST | NI_NUMERICSERV) == 0)
					host = "p25:\"" + std::string(buffer) + "\"";
			}
		}

		m_mqtt->publish("response", host);
	} else {
		CUtils::dump("Invalid remote command received", (unsigned char*)command.c_str(), (unsigned int)command.length());
	}
}

void CP25Gateway::writeJSONStatus(const std::string& status)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["message"]   = status;

	WriteJSON("status", json, false);
}

void CP25Gateway::writeJSONLinking(const std::string& reason, unsigned int tg)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["action"]    = "linking";
	json["reason"]    = reason;
	json["talkgroup"] = int(tg);

	WriteJSON("link", json, true);
}

void CP25Gateway::writeJSONUnlinked(const std::string& reason)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["action"]    = "unlinked";
	json["reason"]    = reason;

	WriteJSON("link", json, true);
}

void CP25Gateway::writeJSONRelinking(unsigned int tg)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["action"]    = "relinking";
	json["talkgroup"] = int(tg);

	WriteJSON("link", json, true);
}

void CP25Gateway::onCommand(const unsigned char* command, unsigned int length)
{
	assert(gateway != nullptr);
	assert(command != nullptr);

	gateway->writeCommand(std::string((char*)command, length));
}

bool CP25Gateway::isVoiceBusy() const
{
	if (m_voice == nullptr)
		return false;

	return m_voice->isBusy();
}

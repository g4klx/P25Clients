/*
 *	 Copyright (C) 2015-2020,2023 by Jonathan Naylor G4KLX
 *
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the Free Software
 *	 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum SECTION {
	SECTION_NONE,
	SECTION_GENERAL,
	SECTION_ID_LOOKUP,
	SECTION_VOICE,
	SECTION_LOG,
	SECTION_MQTT,
	SECTION_NETWORK,
	SECTION_REMOTE_COMMANDS
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_rptAddress(),
m_rptPort(0U),
m_myPort(0U),
m_debug(false),
m_daemon(false),
m_lookupName(),
m_lookupTime(0U),
m_voiceEnabled(true),
m_voiceLanguage("en_GB"),
m_voiceDirectory(),
m_logDisplayLevel(0U),
m_logMQTTLevel(0U),
m_mqttAddress("127.0.0.1"),
m_mqttPort(1883U),
m_mqttKeepalive(60U),
m_mqttName("p25-gateway"),
m_networkPort(0U),
m_networkHosts1(),
m_networkHosts2(),
m_networkReloadTime(0U),
m_networkParrotAddress("127.0.0.1"),
m_networkParrotPort(0U),
m_networkStatic(),
m_networkRFHangTime(120U),
m_networkNetHangTime(60U),
m_networkDebug(false),
m_remoteCommandsEnabled(false)
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
	FILE* fp = ::fopen(m_file.c_str(), "rt");
	if (fp == NULL) {
		::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
		return false;
	}

	SECTION section = SECTION_NONE;

	char buffer[BUFFER_SIZE];
	while (::fgets(buffer, BUFFER_SIZE, fp) != NULL) {
		if (buffer[0U] == '#')
			continue;

		if (buffer[0U] == '[') {
			if (::strncmp(buffer, "[General]", 9U) == 0)
				section = SECTION_GENERAL;
			else if (::strncmp(buffer, "[Id Lookup]", 11U) == 0)
				section = SECTION_ID_LOOKUP;
			else if (::strncmp(buffer, "[Voice]", 7U) == 0)
				section = SECTION_VOICE;
			else if (::strncmp(buffer, "[Log]", 5U) == 0)
				section = SECTION_LOG;
			else if (::strncmp(buffer, "[MQTT]", 6U) == 0)
				section = SECTION_MQTT;
			else if (::strncmp(buffer, "[Network]", 9U) == 0)
				section = SECTION_NETWORK;
			else if (::strncmp(buffer, "[Remote Commands]", 17U) == 0)
				section = SECTION_REMOTE_COMMANDS;
			else
				section = SECTION_NONE;

			continue;
		}

		char* key = ::strtok(buffer, " \t=\r\n");
		if (key == NULL)
			continue;

		char* value = ::strtok(NULL, "\r\n");
		if (value == NULL)
			continue;

		// Remove quotes from the value
		size_t len = ::strlen(value);
		if (len > 1U && *value == '"' && value[len - 1U] == '"') {
			value[len - 1U] = '\0';
			value++;
		} else {
			char *p;

			// if value is not quoted, remove after # (to make comment)
			if ((p = strchr(value, '#')) != NULL)
				*p = '\0';

			// remove trailing tab/space
			for (p = value + strlen(value) - 1U; p >= value && (*p == '\t' || *p == ' '); p--)
				*p = '\0';
		}

		if (section == SECTION_GENERAL) {
			if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_callsign = value;
			} else if (::strcmp(key, "RptAddress") == 0)
				m_rptAddress = value;
			else if (::strcmp(key, "RptPort") == 0)
				m_rptPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalPort") == 0)
				m_myPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_debug = ::atoi(value) == 1;
			else if (::strcmp(key, "Daemon") == 0)
				m_daemon = ::atoi(value) == 1;
		} else if (section == SECTION_ID_LOOKUP) {
			if (::strcmp(key, "Name") == 0)
				m_lookupName = value;
			else if (::strcmp(key, "Time") == 0)
				m_lookupTime = (unsigned int)::atoi(value);
		} else if (section == SECTION_VOICE) {
			if (::strcmp(key, "Enabled") == 0)
				m_voiceEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Language") == 0)
				m_voiceLanguage = value;
			else if (::strcmp(key, "Directory") == 0)
				m_voiceDirectory = value;
		} else if (section == SECTION_LOG) {
			if (::strcmp(key, "MQTTLevel") == 0)
				m_logMQTTLevel = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayLevel") == 0)
				m_logDisplayLevel = (unsigned int)::atoi(value);
		} else if (section == SECTION_MQTT) {
			if (::strcmp(key, "Address") == 0)
				m_mqttAddress = value;
			else if (::strcmp(key, "Port") == 0)
				m_mqttPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "Keepalive") == 0)
				m_mqttKeepalive = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Name") == 0)
				m_mqttName = value;
		} else if (section == SECTION_NETWORK) {
			if (::strcmp(key, "Port") == 0)
				m_networkPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "HostsFile1") == 0)
				m_networkHosts1 = value;
			else if (::strcmp(key, "HostsFile2") == 0)
				m_networkHosts2 = value;
			else if (::strcmp(key, "ReloadTime") == 0)
				m_networkReloadTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ParrotAddress") == 0)
				m_networkParrotAddress = value;
			else if (::strcmp(key, "ParrotPort") == 0)
				m_networkParrotPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "P252DMRAddress") == 0)
				m_networkP252DMRAddress = value;
			else if (::strcmp(key, "P252DMRPort") == 0)
				m_networkP252DMRPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "Static") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != NULL) {
					unsigned int tg = (unsigned int)::atoi(p);
					m_networkStatic.push_back(tg);
					p = ::strtok(NULL, ",\r\n");
				}
			} else if (::strcmp(key, "RFHangTime") == 0)
				m_networkRFHangTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "NetHangTime") == 0)
				m_networkNetHangTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_networkDebug = ::atoi(value) == 1;
		} else if (section == SECTION_REMOTE_COMMANDS) {
			if (::strcmp(key, "Enable") == 0)
				m_remoteCommandsEnabled = ::atoi(value) == 1;
		}
	}

	::fclose(fp);

	return true;
}

std::string CConf::getCallsign() const
{
	return m_callsign;
}

std::string CConf::getRptAddress() const
{
	return m_rptAddress;
}

unsigned short CConf::getRptPort() const
{
	return m_rptPort;
}

unsigned short CConf::getMyPort() const
{
	return m_myPort;
}

bool CConf::getDebug() const
{
	return m_debug;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

std::string CConf::getLookupName() const
{
	return m_lookupName;
}

unsigned int CConf::getLookupTime() const
{
	return m_lookupTime;
}

bool CConf::getVoiceEnabled() const
{
	return m_voiceEnabled;
}

std::string CConf::getVoiceLanguage() const
{
	return m_voiceLanguage;
}

std::string CConf::getVoiceDirectory() const
{
	return m_voiceDirectory;
}

unsigned int CConf::getLogDisplayLevel() const
{
	return m_logDisplayLevel;
}

unsigned int CConf::getLogMQTTLevel() const
{
	return m_logMQTTLevel;
}

std::string CConf::getMQTTAddress() const
{
	return m_mqttAddress;
}

unsigned short CConf::getMQTTPort() const
{
	return m_mqttPort;
}

unsigned int CConf::getMQTTKeepalive() const
{
	return m_mqttKeepalive;
}

std::string CConf::getMQTTName() const
{
	return m_mqttName;
}

unsigned short CConf::getNetworkPort() const
{
	return m_networkPort;
}

std::string CConf::getNetworkHosts1() const
{
	return m_networkHosts1;
}

std::string CConf::getNetworkHosts2() const
{
	return m_networkHosts2;
}

unsigned int CConf::getNetworkReloadTime() const
{
	return m_networkReloadTime;
}

std::string CConf::getNetworkParrotAddress() const
{
	return m_networkParrotAddress;
}

unsigned short CConf::getNetworkParrotPort() const
{
	return m_networkParrotPort;
}

std::string CConf::getNetworkP252DMRAddress() const
{
	return m_networkP252DMRAddress;
}

unsigned short CConf::getNetworkP252DMRPort() const
{
	return m_networkP252DMRPort;
}

std::vector<unsigned int> CConf::getNetworkStatic() const
{
	return m_networkStatic;
}

unsigned int CConf::getNetworkRFHangTime() const
{
	return m_networkRFHangTime;
}

unsigned int CConf::getNetworkNetHangTime() const
{
	return m_networkNetHangTime;
}

bool CConf::getNetworkDebug() const
{
	return m_networkDebug;
}

bool CConf::getRemoteCommandsEnabled() const
{
	return m_remoteCommandsEnabled;
}


/*
 *   Copyright (C) 2015-2020,2025 by Jonathan Naylor G4KLX
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

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum class SECTION {
	NONE,
	GENERAL,
	ID_LOOKUP,
	VOICE,
	LOG,
	NETWORK,
	REMOTE_COMMANDS
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
m_logFileLevel(0U),
m_logFilePath(),
m_logFileRoot(),
m_logFileRotate(true),
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
m_remoteCommandsEnabled(false),
m_remoteCommandsPort(6074U)
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
  FILE* fp = ::fopen(m_file.c_str(), "rt");
  if (fp == nullptr) {
    ::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
    return false;
  }

  SECTION section = SECTION::NONE;

  char buffer[BUFFER_SIZE];
  while (::fgets(buffer, BUFFER_SIZE, fp) != nullptr) {
	  if (buffer[0U] == '#')
		  continue;

	  if (buffer[0U] == '[') {
		  if (::strncmp(buffer, "[General]", 9U) == 0)
			  section = SECTION::GENERAL;
		  else if (::strncmp(buffer, "[Id Lookup]", 11U) == 0)
			  section = SECTION::ID_LOOKUP;
		  else if (::strncmp(buffer, "[Voice]", 7U) == 0)
			  section = SECTION::VOICE;
		  else if (::strncmp(buffer, "[Log]", 5U) == 0)
			  section = SECTION::LOG;
		  else if (::strncmp(buffer, "[Network]", 9U) == 0)
			  section = SECTION::NETWORK;
		  else if (::strncmp(buffer, "[Remote Commands]", 17U) == 0)
			  section = SECTION::REMOTE_COMMANDS;
		  else
			  section = SECTION::NONE;

		  continue;
	  }

	  char* key = ::strtok(buffer, " \t=\r\n");
	  if (key == nullptr)
		  continue;

	  char* value = ::strtok(nullptr, "\r\n");
	  if (value == nullptr)
		  continue;

	  // Remove quotes from the value
	  size_t len = ::strlen(value);
	  if (len > 1U && *value == '"' && value[len - 1U] == '"') {
		  value[len - 1U] = '\0';
		  value++;
	  } else {
		  char *p;

		  // if value is not quoted, remove after # (to make comment)
		  if ((p = strchr(value, '#')) != nullptr)
			  *p = '\0';

		  // remove trailing tab/space
		  for (p = value + strlen(value) - 1U; p >= value && (*p == '\t' || *p == ' '); p--)
			  *p = '\0';
	  }

	  if (section == SECTION::GENERAL) {
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
	  } else if (section == SECTION::ID_LOOKUP) {
		  if (::strcmp(key, "Name") == 0)
			  m_lookupName = value;
		  else if (::strcmp(key, "Time") == 0)
			  m_lookupTime = (unsigned int)::atoi(value);
	  } else if (section == SECTION::VOICE) {
		  if (::strcmp(key, "Enabled") == 0)
			  m_voiceEnabled = ::atoi(value) == 1;
		  else if (::strcmp(key, "Language") == 0)
			  m_voiceLanguage = value;
		  else if (::strcmp(key, "Directory") == 0)
			  m_voiceDirectory = value;
	  } else if (section == SECTION::LOG) {
		  if (::strcmp(key, "FilePath") == 0)
			  m_logFilePath = value;
		  else if (::strcmp(key, "FileRoot") == 0)
			  m_logFileRoot = value;
		  else if (::strcmp(key, "FileLevel") == 0)
			  m_logFileLevel = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "DisplayLevel") == 0)
			  m_logDisplayLevel = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "FileRotate") == 0)
			  m_logFileRotate = ::atoi(value) == 1;
	  } else if (section == SECTION::NETWORK) {
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
			  while (p != nullptr) {
				  unsigned int tg = (unsigned int)::atoi(p);
				  m_networkStatic.push_back(tg);
				  p = ::strtok(nullptr, ",\r\n");
			  }
		  } else if (::strcmp(key, "RFHangTime") == 0)
			  m_networkRFHangTime = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "NetHangTime") == 0)
			  m_networkNetHangTime = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "Debug") == 0)
			  m_networkDebug = ::atoi(value) == 1;
	  } else if (section == SECTION::REMOTE_COMMANDS) {
		  if (::strcmp(key, "Enable") == 0)
			  m_remoteCommandsEnabled = ::atoi(value) == 1;
		  else if (::strcmp(key, "Port") == 0)
			  m_remoteCommandsPort = (unsigned short)::atoi(value);
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

unsigned int CConf::getLogFileLevel() const
{
	return m_logFileLevel;
}

std::string CConf::getLogFilePath() const
{
	return m_logFilePath;
}

std::string CConf::getLogFileRoot() const
{
	return m_logFileRoot;
}

bool CConf::getLogFileRotate() const
{
	return m_logFileRotate;
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

unsigned short CConf::getRemoteCommandsPort() const
{
	return m_remoteCommandsPort;
}

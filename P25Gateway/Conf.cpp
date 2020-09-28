/*
 *   Copyright (C) 2015-2020 by Jonathan Naylor G4KLX
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

enum SECTION {
  SECTION_NONE,
  SECTION_GENERAL,
  SECTION_ID_LOOKUP,
  SECTION_VOICE,
  SECTION_LOG,
  SECTION_NETWORK,
  SECTION_REMOTE_COMMANDS
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_rptAddress(),
m_rptPort(0U),
m_myPort(0U),
m_daemon(false),
m_lookupName(),
m_lookupTime(0U),
m_voiceEnabled(true),
m_voiceLanguage("en_GB"),
m_voiceDirectory(),
m_logFilePath(),
m_logFileRoot(),
m_networkPort(0U),
m_networkHosts1(),
m_networkHosts2(),
m_networkReloadTime(0U),
m_networkParrotAddress("127.0.0.1"),
m_networkParrotPort(0U),
m_networkP252DMRAddress("127.0.0.1"),
m_networkP252DMRPort(0U),
m_networkP252PCMAddress("127.0.0.1"),
m_networkP252PCMPort(0U),
m_networkStartup(9999U),
m_networkInactivityTimeout(0U),
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
	  if (section == SECTION_GENERAL) {
		  if (::strcmp(key, "Callsign") == 0) {
			  // Convert the callsign to upper case
			  for (unsigned int i = 0U; value[i] != 0; i++)
				  value[i] = ::toupper(value[i]);
			  m_callsign = value;
		  } else if (::strcmp(key, "RptAddress") == 0)
			  m_rptAddress = value;
		  else if (::strcmp(key, "RptPort") == 0)
			  m_rptPort = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "LocalPort") == 0)
			  m_myPort = (unsigned int)::atoi(value);
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
		  if (::strcmp(key, "FilePath") == 0)
			  m_logFilePath = value;
		  else if (::strcmp(key, "FileRoot") == 0)
			  m_logFileRoot = value;
	  } else if (section == SECTION_NETWORK) {
		  if (::strcmp(key, "Port") == 0)
			  m_networkPort = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "HostsFile1") == 0)
			  m_networkHosts1 = value;
		  else if (::strcmp(key, "HostsFile2") == 0)
			  m_networkHosts2 = value;
		  else if (::strcmp(key, "ReloadTime") == 0)
			  m_networkReloadTime = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "ParrotAddress") == 0)
			  m_networkParrotAddress = value;
		  else if (::strcmp(key, "ParrotPort") == 0)
			  m_networkParrotPort = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "P252DMRAddress") == 0)
			  m_networkP252DMRAddress = value;
		  else if (::strcmp(key, "P252DMRPort") == 0)
			  m_networkP252DMRPort = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "P252PCMAddress") == 0)
			  m_networkP252PCMAddress = value;
		  else if (::strcmp(key, "P252PCMPort") == 0)
			  m_networkP252PCMPort = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "Startup") == 0)
			  m_networkStartup = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "InactivityTimeout") == 0)
			  m_networkInactivityTimeout = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "Debug") == 0)
			  m_networkDebug = ::atoi(value) == 1;
	  } else if (section == SECTION_REMOTE_COMMANDS) {
		  if (::strcmp(key, "Enable") == 0)
			  m_remoteCommandsEnabled = ::atoi(value) == 1;
		  else if (::strcmp(key, "Port") == 0)
			  m_remoteCommandsPort = (unsigned int)::atoi(value);
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

unsigned int CConf::getRptPort() const
{
	return m_rptPort;
}

unsigned int CConf::getMyPort() const
{
	return m_myPort;
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

std::string CConf::getLogFilePath() const
{
  return m_logFilePath;
}

std::string CConf::getLogFileRoot() const
{
  return m_logFileRoot;
}

unsigned int CConf::getNetworkPort() const
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

unsigned int CConf::getNetworkParrotPort() const
{
	return m_networkParrotPort;
}

std::string CConf::getNetworkP252DMRAddress() const
{
	return m_networkP252DMRAddress;
}

unsigned int CConf::getNetworkP252DMRPort() const
{
	return m_networkP252DMRPort;
}

std::string CConf::getNetworkP252PCMAddress() const
{
	return m_networkP252PCMAddress;
}

unsigned int CConf::getNetworkP252PCMPort() const
{
	return m_networkP252PCMPort;
}

unsigned int CConf::getNetworkStartup() const
{
	return m_networkStartup;
}

unsigned int CConf::getNetworkInactivityTimeout() const
{
	return m_networkInactivityTimeout;
}

bool CConf::getNetworkDebug() const
{
	return m_networkDebug;
}

bool CConf::getRemoteCommandsEnabled() const
{
	return m_remoteCommandsEnabled;
}

unsigned int CConf::getRemoteCommandsPort() const
{
	return m_remoteCommandsPort;
}

/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(CONF_H)
#define	CONF_H

#include <string>
#include <vector>

class CConf
{
public:
  CConf(const std::string& file);
  ~CConf();

  bool read();

  // The General section
  std::string  getCallsign() const;
  std::string  getRptAddress() const;
  unsigned int getRptPort() const;
  unsigned int getMyPort() const;
  bool         getDaemon() const;

  // The Id Lookup section
  std::string  getLookupName() const;
  unsigned int getLookupTime() const;

  // The Log section
  unsigned int getLogDisplayLevel() const;
  unsigned int getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

  // The Network section
  bool         getNetworkEnabled() const;
  unsigned int getNetworkDataPort() const;
  std::string  getNetworkHosts() const;
  unsigned int getNetworkReloadTime() const;
  std::string  getNetworkParrotAddress() const;
  unsigned int getNetworkParrotPort() const;
  unsigned int getNetworkStartup() const;
  bool         getNetworkDebug() const;

private:
  std::string  m_file;
  std::string  m_callsign;
  std::string  m_rptAddress;
  unsigned int m_rptPort;
  unsigned int m_myPort;
  bool         m_daemon;

  std::string  m_lookupName;
  unsigned int m_lookupTime;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;

  bool         m_networkEnabled;
  unsigned int m_networkDataPort;
  std::string  m_networkHosts;
  unsigned int m_networkReloadTime;
  std::string  m_networkParrotAddress;
  unsigned int m_networkParrotPort;
  unsigned int m_networkStartup;
  bool         m_networkDebug;
};

#endif

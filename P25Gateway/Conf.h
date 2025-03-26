/*
 *	 Copyright (C) 2015-2020,2023,2025 by Jonathan Naylor G4KLX
 *
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the Free Software
 *	 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	std::string   getCallsign() const;
	std::string   getRptAddress() const;
	unsigned short getRptPort() const;
	unsigned short getMyPort() const;
	bool          getDebug() const;
	bool          getDaemon() const;

	// The Id Lookup section
	std::string  getLookupName() const;
	unsigned int getLookupTime() const;

	// The Voice section
	bool         getVoiceEnabled() const;
	std::string  getVoiceLanguage() const;
	std::string  getVoiceDirectory() const;

	// The Log section
	unsigned int getLogDisplayLevel() const;
	unsigned int getLogMQTTLevel() const;

	// The MQTT section
	std::string  getMQTTAddress() const;
	unsigned short getMQTTPort() const;
	unsigned int getMQTTKeepalive() const;
	std::string  getMQTTName() const;
	bool         getMQTTAuthEnabled() const;
	std::string  getMQTTUsername() const;
	std::string  getMQTTPassword() const;

	// The Network section
	unsigned short getNetworkPort() const;
	std::string  getNetworkHosts1() const;
	std::string  getNetworkHosts2() const;
	unsigned int getNetworkReloadTime() const;
	std::string  getNetworkParrotAddress() const;
	unsigned short getNetworkParrotPort() const;
	std::string  getNetworkP252DMRAddress() const;
	unsigned short getNetworkP252DMRPort() const;
	std::vector<unsigned int> getNetworkStatic() const;
	unsigned int getNetworkRFHangTime() const;
	unsigned int getNetworkNetHangTime() const;
	bool         getNetworkDebug() const;

	// The Remote Commands section
	bool         getRemoteCommandsEnabled() const;

private:
	std::string  m_file;
	std::string  m_callsign;
	std::string  m_rptAddress;
	unsigned short m_rptPort;
	unsigned short m_myPort;
	bool         m_debug;
	bool         m_daemon;

	std::string  m_lookupName;
	unsigned int m_lookupTime;

	bool         m_voiceEnabled;
	std::string  m_voiceLanguage;
	std::string  m_voiceDirectory;

	unsigned int m_logDisplayLevel;
	unsigned int m_logMQTTLevel;

	std::string  m_mqttAddress;
	unsigned short m_mqttPort;
	unsigned int m_mqttKeepalive;
	std::string  m_mqttName;
	bool         m_mqttAuthEnabled;
	std::string  m_mqttUsername;
	std::string  m_mqttPassword;

	unsigned short m_networkPort;
	std::string  m_networkHosts1;
	std::string  m_networkHosts2;
	unsigned int m_networkReloadTime;
	std::string  m_networkParrotAddress;
	unsigned short m_networkParrotPort;
	std::string  m_networkP252DMRAddress;
	unsigned short m_networkP252DMRPort;
	std::vector<unsigned int> m_networkStatic;;
	unsigned int m_networkRFHangTime;
	unsigned int m_networkNetHangTime;
	bool         m_networkDebug;

	bool         m_remoteCommandsEnabled;
};

#endif

/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/lib/zapitclient.cpp,v 1.80 2003/03/02 11:22:47 thegoodguy Exp $ *
 *
 * Zapit client interface - DBoxII-Project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de> & the DBoxII-Project
 *
 * License: GPL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>

/* libevent */
#include <eventserver.h>


#include <zapit/client/zapitclient.h>
#include <zapit/client/msgtypes.h>

std::string CZapitClient::Utf8_to_Latin1(const std::string s)
{
	std::string r;

	for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
	{
		if (((*it) & 0xf0) == 0xf0)      // skip (can't be encoded in Latin1)
		{
			it++;
			if (it == s.end())
				return r;
			it++;
			if (it == s.end())
				return r;
			it++;
			if (it == s.end())
				return r;
		}
		else if (((*it) & 0xe0) == 0xe0) // skip (can't be encoded in Latin1)
		{
			it++;
			if (it == s.end())
				return r;
			it++;
			if (it == s.end())
				return r;
		}
		else if (((*it) & 0xc0) == 0xc0)
		{
			char c = (((*it) & 3) << 6);
			it++;
			if (it == s.end())
				return r;
			r += (c | ((*it) & 0x3f));
		}
		else r += *it;
	}
	return r;
}

const unsigned char   CZapitClient::getVersion   () const
{
	return CZapitMessages::ACTVERSION;
}

const          char * CZapitClient::getSocketName() const
{
	return ZAPIT_UDS_NAME;
}

void CZapitClient::shutdown()
{
	send(CZapitMessages::CMD_SHUTDOWN);
	close_connection();
}

//***********************************************/
/*					     */
/* general functions for zapping	       */
/*					     */
/***********************************************/

/* zaps to channel of specified bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::zapTo(const unsigned int bouquet, const unsigned int channel)
{
	CZapitMessages::commandZapto msg;

	msg.bouquet = bouquet;
	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO, (char*)&msg, sizeof(msg));

	close_connection();
}

/* zaps to channel by nr */
void CZapitClient::zapTo(const unsigned int channel)
{
	CZapitMessages::commandZaptoChannelNr msg;

	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO_CHANNELNR, (char*)&msg, sizeof(msg));

	close_connection();
}

t_channel_id CZapitClient::getCurrentServiceID()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEID);

	CZapitMessages::responseGetCurrentServiceID response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.channel_id;
}

CZapitClient::CCurrentServiceInfo CZapitClient::getCurrentServiceInfo()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEINFO);

	CZapitClient::CCurrentServiceInfo response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response;
}

void CZapitClient::getLastChannel(unsigned int &channumber, char &mode)
{
	send(CZapitMessages::CMD_GET_LAST_CHANNEL);

	CZapitClient::responseGetLastChannel response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	channumber = response.channelNumber + 1;
	mode = response.mode;

	close_connection();
}

void CZapitClient::setAudioChannel(const unsigned int channel)
{
	CZapitMessages::commandSetAudioChannel msg;

	msg.channel = channel;

	send(CZapitMessages::CMD_SET_AUDIOCHAN, (char*)&msg, sizeof(msg));

	close_connection();
}

/* zaps to onid_sid, returns the "zap-status" */
unsigned int CZapitClient::zapTo_serviceID(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID, (char*)&msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

unsigned int CZapitClient::zapTo_subServiceID(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID, (char*)&msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

/* zaps to channel, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_serviceID_NOWAIT(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID_NOWAIT, (char*)&msg, sizeof(msg));

	close_connection();
}

/* zaps to subservice, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_subServiceID_NOWAIT(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT, (char*)&msg, sizeof(msg));

	close_connection();
}


void CZapitClient::setMode( channelsMode mode )
{
	CZapitMessages::commandSetMode msg;

	msg.mode = mode;

	send(CZapitMessages::CMD_SET_MODE, (char*)&msg, sizeof(msg));

	close_connection();
}

int CZapitClient::getMode()
{
	send(CZapitMessages::CMD_GET_MODE);

	CZapitMessages::responseGetMode response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.mode;
}

void CZapitClient::setSubServices( subServiceList& subServices )
{
	unsigned int i;

	send(CZapitMessages::CMD_SETSUBSERVICES);

	for (i = 0; i< subServices.size(); i++)
		send_data((char*)&subServices[i], sizeof(subServices[i]));

	close_connection();
}

void CZapitClient::getPIDS(responseGetPIDs& pids)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetAPIDs                       responseAPID;

	send(CZapitMessages::CMD_GETPIDS);

	CBasicClient::receive_data((char* )&(pids.PIDs), sizeof(pids.PIDs));

	pids.APIDs.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.APIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseAPID, sizeof(responseAPID));
			pids.APIDs.push_back(responseAPID);
		};
	}

	close_connection();
}

void CZapitClient::zaptoNvodSubService(int num)
{
	CZapitMessages::commandInt msg;

	msg.val = num;

	send(CZapitMessages::CMD_NVOD_SUBSERVICE_NUM, (char*)&msg, sizeof(msg));

	close_connection();
}

/* gets all bouquets */
/* bouquets are numbered starting at 0 */
void CZapitClient::getBouquets(BouquetList& bouquets, const bool emptyBouquetsToo, const bool utf_encoded)
{
	char buffer[30 + 1];

	CZapitMessages::commandGetBouquets msg;

	msg.emptyBouquetsToo = emptyBouquetsToo;

	send(CZapitMessages::CMD_GET_BOUQUETS, (char*)&msg, sizeof(msg));

	responseGetBouquets response;
	while (CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquets)))
	{
		if (response.bouquet_nr == 0xFFFF) /* <- end marker */
			break;

		if (!utf_encoded)
		{
			buffer[30] = (char) 0x00;
			strncpy(buffer, response.name, 30);
			strncpy(response.name, Utf8_to_Latin1(std::string(buffer)).c_str(), 30);
		}
		bouquets.push_back(response);
	}

	close_connection();
}


bool CZapitClient::receive_channel_list(BouquetChannelList& channels, const bool utf_encoded)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetBouquetChannels             response;
	char                                   buffer[30 + 1];
	
	channels.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		channels.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			if (!CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquetChannels)))
				return false;
			
			response.nr++;
			if (!utf_encoded)
			{
				buffer[30] = (char) 0x00;
				strncpy(buffer, response.name, 30);
				strncpy(response.name, Utf8_to_Latin1(std::string(buffer)).c_str(), 30);
			}
			channels.push_back(response);
		}
	}
	return true;
}


/* gets all channels that are in specified bouquet */
/* bouquets are numbered starting at 0 */
bool CZapitClient::getBouquetChannels(const unsigned int bouquet, BouquetChannelList& channels, channelsMode mode, const bool utf_encoded)
{
	bool                                      return_value;
	CZapitMessages::commandGetBouquetChannels msg;

	msg.bouquet = bouquet;
	msg.mode = mode;

	return_value = (send(CZapitMessages::CMD_GET_BOUQUET_CHANNELS, (char*)&msg, sizeof(msg))) ? receive_channel_list(channels, utf_encoded) : false;

	close_connection();
	return return_value;
}

/* gets all channels */
bool CZapitClient::getChannels( BouquetChannelList& channels, channelsMode mode, channelsOrder order, const bool utf_encoded)
{
	bool                               return_value;
	CZapitMessages::commandGetChannels msg;

	msg.mode = mode;
	msg.order = order;

	return_value = (send(CZapitMessages::CMD_GET_CHANNELS, (char*)&msg, sizeof(msg))) ? receive_channel_list(channels, utf_encoded) : false;

	close_connection();
	return return_value;
}

/* restore bouquets so as if they where just loaded*/
void CZapitClient::restoreBouquets()
{
	send(CZapitMessages::CMD_BQ_RESTORE);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
}

/* reloads channels and services*/
void CZapitClient::reinitChannels()
{
	send(CZapitMessages::CMD_REINIT_CHANNELS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
}


/* commit bouquet change */
void CZapitClient::commitBouquetChange()
{
	send(CZapitMessages::CMD_BQ_COMMIT_CHANGE);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
}

void CZapitClient::muteAudio(const bool mute)
{
	CZapitMessages::commandBoolean msg;

	msg.truefalse = mute;

	send(CZapitMessages::CMD_MUTE, (char*)&msg, sizeof(msg));

	close_connection();
}

void CZapitClient::setVolume(const unsigned int left, const unsigned int right)
{
	CZapitMessages::commandVolume msg;

	msg.left = left;
	msg.right = right;

	send(CZapitMessages::CMD_SET_VOLUME, (char*)&msg, sizeof(msg));

	close_connection();
}


delivery_system_t CZapitClient::getDeliverySystem(void)
{
	send(CZapitMessages::CMD_GET_DELIVERY_SYSTEM, 0, 0);

	CZapitMessages::responseDeliverySystem response;

	if (!CBasicClient::receive_data((char* )&response, sizeof(response)))
		response.system = DVB_S;  // return DVB_S if communication fails

	close_connection();

	return response.system;
}


/***********************************************/
/*					     */
/*  Scanning stuff			     */
/*					     */
/***********************************************/

/* start TS-Scan */
bool CZapitClient::startScan()
{
	bool reply = send(CZapitMessages::CMD_SCANSTART);
	
	close_connection();
	
	return reply;
}

/* query if ts-scan is ready - response gives status */
bool CZapitClient::isScanReady(unsigned int &satellite, unsigned int &transponder, unsigned int &services )
{
	send(CZapitMessages::CMD_SCANREADY);

	CZapitMessages::responseIsScanReady response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	satellite = response.satellite;
	transponder = response.transponder;
	services = response.services;

	close_connection();
	return response.scanReady;
}

/* query possible satellits*/
void CZapitClient::getScanSatelliteList( SatelliteList& satelliteList )
{
	send(CZapitMessages::CMD_SCANGETSATLIST);

	responseGetSatelliteList response;
	while ( CBasicClient::receive_data((char*)&response, sizeof(responseGetSatelliteList)))
		satelliteList.push_back(response);

	close_connection();

}

/* tell zapit which satellites to scan*/
void CZapitClient::setScanSatelliteList( ScanSatelliteList& satelliteList )
{
	send(CZapitMessages::CMD_SCANSETSCANSATLIST);

	for (uint i=0; i<satelliteList.size(); i++)
	{
		send_data((char*)&satelliteList[i], sizeof(satelliteList[i]));
	}
	close_connection();
}

/* set diseqcType*/
void CZapitClient::setDiseqcType( diseqc_t diseqc)
{
	send(CZapitMessages::CMD_SCANSETDISEQCTYPE, (char*)&diseqc, sizeof(diseqc));
	close_connection();
}

/* set diseqcRepeat*/
void CZapitClient::setDiseqcRepeat( uint32_t repeat)
{
	send(CZapitMessages::CMD_SCANSETDISEQCREPEAT, (char*)&repeat, sizeof(repeat));
	close_connection();
}

/* set diseqcRepeat*/
void CZapitClient::setScanBouquetMode( bouquetMode mode)
{
	send(CZapitMessages::CMD_SCANSETBOUQUETMODE, (char*)&mode, sizeof(mode));
	close_connection();
}


/***********************************************/
/*					     */
/* Bouquet editing functions		   */
/*					     */
/***********************************************/

/* adds bouquet at the end of the bouquetlist*/
void CZapitClient::addBouquet(const std::string name)
{
	CZapitMessages::commandAddBouquet msg;

	strncpy( msg.name, name.c_str(), 30);

	send(CZapitMessages::CMD_BQ_ADD_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* moves a bouquet from one position to another */
/* bouquets are numbered starting at 0 */
void CZapitClient::moveBouquet(const unsigned int bouquet, const unsigned int newPos)
{
	CZapitMessages::commandMoveBouquet msg;

	msg.bouquet = bouquet;
	msg.newPos = newPos;

	send(CZapitMessages::CMD_BQ_MOVE_BOUQUET, (char*)&msg, sizeof(msg));
	close_connection();
}

/* deletes a bouquet with all its channels*/
/* bouquets are numbered starting at 0 */
void CZapitClient::deleteBouquet(const unsigned int bouquet)
{
	CZapitMessages::commandDeleteBouquet msg;

	msg.bouquet = bouquet;

	send(CZapitMessages::CMD_BQ_DELETE_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* assigns new name to bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::renameBouquet(const unsigned int bouquet, const std::string newName)
{
	CZapitMessages::commandRenameBouquet msg;

	msg.bouquet = bouquet;
	strncpy( msg.name, newName.c_str(), 30);

	send(CZapitMessages::CMD_BQ_RENAME_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

// -- check if Bouquet-Name exists
// -- Return: Bouquet-ID  or  -1 == no Bouquet found
/* bouquets are numbered starting at 0 */
signed int CZapitClient::existsBouquet(const std::string name)
{
	CZapitMessages::commandExistsBouquet msg;
	CZapitMessages::responseGeneralInteger response;

	strncpy( msg.name, name.c_str(), 30);

	send(CZapitMessages::CMD_BQ_EXISTS_BOUQUET, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.number;
}

// -- check if Channel already is in Bouquet
// -- Return: true/false
/* bouquets are numbered starting at 0 */
bool CZapitClient::existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandExistsChannelInBouquet msg;
	CZapitMessages::responseGeneralTrueFalse response;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return (unsigned int) response.status;
}



/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
/* bouquets are numbered starting at 0 */
void CZapitClient::moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode)
{
	CZapitMessages::commandMoveChannel msg;

	msg.bouquet = bouquet;
	msg.oldPos  = oldPos - 1;
	msg.newPos  = newPos - 1;
	msg.mode    = mode;

	send(CZapitMessages::CMD_BQ_MOVE_CHANNEL, (char*)&msg, sizeof(msg));

	close_connection();
}

/* adds a channel at the end of then channel list to specified bouquet */
/* same channels can be in more than one bouquet */
/* bouquets can contain both tv and radio channels */
/* bouquets are numbered starting at 0 */
void CZapitClient::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandAddChannelToBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* removes a channel from specified bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandRemoveChannelFromBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* set a bouquet's lock-state*/
/* bouquets are numbered starting at 0 */
void CZapitClient::setBouquetLock(const unsigned int bouquet, const bool lock)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = lock;

	send(CZapitMessages::CMD_BQ_SET_LOCKSTATE, (char*)&msg, sizeof(msg));

	close_connection();
}

/* set a bouquet's hidden-state*/
/* bouquets are numbered starting at 0 */
void CZapitClient::setBouquetHidden(const unsigned int bouquet, const bool hidden)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = hidden;

	send(CZapitMessages::CMD_BQ_SET_HIDDENSTATE, (char*)&msg, sizeof(msg));
	close_connection();
}

/* renums the channellist, means gives the channels new numbers */
/* based on the bouquet order and their order within bouquets */
/* necessarily after bouquet editing operations*/
void CZapitClient::renumChannellist()
{
	send(CZapitMessages::CMD_BQ_RENUM_CHANNELLIST);
	close_connection();
}


/* saves current bouquet configuration to bouquets.xml*/
void CZapitClient::saveBouquets()
{
	send(CZapitMessages::CMD_BQ_SAVE_BOUQUETS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
}

void CZapitClient::setStandby(const bool enable)
{
	CZapitMessages::commandBoolean msg;
	msg.truefalse = enable;
	send(CZapitMessages::CMD_SET_STANDBY, (char*)&msg, sizeof(msg));
	close_connection();
}

void CZapitClient::startPlayBack()
{
	send(CZapitMessages::CMD_SB_START_PLAYBACK);
	close_connection();
}

void CZapitClient::stopPlayBack()
{
	send(CZapitMessages::CMD_SB_STOP_PLAYBACK);
	close_connection();
}

bool CZapitClient::isPlayBackActive()
{
	send(CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE);

	CZapitMessages::responseGetPlaybackState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.activated;
}

void CZapitClient::setDisplayFormat(const video_display_format_t format)
{
	CZapitMessages::commandInt msg;
	msg.val = format;
	send(CZapitMessages::CMD_SET_DISPLAY_FORMAT, (char*)&msg, sizeof(msg));
	close_connection();
}

void CZapitClient::setAudioMode(const int mode)
{
	CZapitMessages::commandInt msg;
	msg.val = mode;
	send(CZapitMessages::CMD_SET_AUDIO_MODE, (char*)&msg, sizeof(msg));
	close_connection();
}

void CZapitClient::setRecordMode(const bool activate)
{
	CZapitMessages::commandSetRecordMode msg;
	msg.activate = activate;
	send(CZapitMessages::CMD_SET_RECORD_MODE, (char*)&msg, sizeof(msg));
	close_connection();
}

bool CZapitClient::isRecordModeActive()
{
	send(CZapitMessages::CMD_GET_RECORD_MODE);

	CZapitMessages::responseGetRecordModeState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.activated;
}

void CZapitClient::registerEvent(const unsigned int eventID, const unsigned int clientID, const std::string udsName)
{
	CEventServer::commandRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	strcpy(msg.udsName, udsName.c_str());

	send(CZapitMessages::CMD_REGISTEREVENTS, (char*)&msg, sizeof(msg));

	close_connection();
}

void CZapitClient::unRegisterEvent(const unsigned int eventID, const unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	send(CZapitMessages::CMD_UNREGISTEREVENTS, (char*)&msg, sizeof(msg));

	close_connection();
}

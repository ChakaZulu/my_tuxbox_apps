/*
 * $Id: zapit.cpp,v 1.251 2002/10/03 19:05:12 thegoodguy Exp $
 *
 * zapit - d-box2 linux project
 *
 * (C) 2001, 2002 by Philipp Leusmann <faralla@berlios.de>
 *
 * based on code from older applications of the d-box2 linux project.
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

/* system headers */
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

/* d-box specific headers */
#ifdef DBOX2
#include <dbox/avia_gt_vbi.h>
#endif

/* tuxbox headers */
#include <configfile.h>
#include <lcddclient.h>

/* zapit library headers */
#include <zapci/cam.h>
#include <zapost/audio.h>
#include <zapost/dmx.h>
#include <zapost/frontend.h>
#include <zapost/video.h>
#include <zapsi/pat.h>
#include <zapsi/pmt.h>

/* zapit headers */
#include "settings.h"
#include "getservices.h"
#include "xmlinterface.h"
#include "zapit.h"
#include <msgtypes.h>

#define debug(fmt, args...) { if (debug) { printf(fmt, ## args); fflush(stdout); } }

/* the conditional access module */
CCam * cam = NULL;
/* the configuration file */
CConfigFile * config = NULL;
/* the event server */
CEventServer * eventServer = NULL;
/* the dvb audio device */
CAudio * audio = NULL;
/* the dvb frontend device */
CFrontend * frontend = NULL;
/* the dvb video device */
CVideo * video = NULL;
/* the current channel */
CZapitChannel * channel = NULL;
/* the transponder scan xml input */
XMLTreeParser * scanInputParser = NULL;
/* the bouquet manager */
CBouquetManager * bouquetManager = NULL;

/* the map which stores the wanted cable/satellites */
std::map <uint8_t, std::string> scanProviders;

/* current zapit mode */
enum
{
	TV_MODE = 0x01,
	RADIO_MODE = 0x02,
	RECORD_MODE = 0x04
};

int currentMode;

int connfd;

#ifdef DBOX2
CLcddClient lcdd;
#endif /* DBOX2 */

bool playbackStopForced = false;

bool debug = false;

/* near video on demand */
tallchans nvodchannels;         //  tallchans defined in "bouquets.h"
std::string nvodname;
bool current_is_nvod;

/* file descriptors */
int dmx_audio_fd = -1;
int dmx_general_fd = -1;
int dmx_pcr_fd = -1;
int dmx_video_fd = -1;
int vbi_fd = -1;

/* list of all channels (services) */
tallchans allchans;             //  tallchans defined in "bouquets.h"

/* transponder scan */
std::map <uint32_t, transponder>transponders;
pthread_t scan_thread;
extern int found_transponders;
extern int found_channels;
extern short curr_sat;
extern short scan_runs;
CZapitClient::bouquetMode bouquetMode = CZapitClient::BM_CREATEBOUQUETS;

void CZapitDestructor()
{
	save_settings(true);

	if (connfd != -1)
	{
		close(connfd);
	}

	stopPlayBack();

	if (dmx_video_fd != -1)
		close(dmx_video_fd);
	if (dmx_audio_fd != -1)
		close(dmx_audio_fd);
	if (dmx_pcr_fd != -1)
		close(dmx_pcr_fd);
	if (dmx_general_fd != -1)
		close(dmx_general_fd);

	delete cam;
	delete video;
	delete audio;
	delete frontend;
	delete config;

	// remove this in class
	exit(0);
}

void signal_handler (int signum)
{
	switch (signum)
	{
		case SIGUSR1:
			debug = (debug ? false : true);
			break;

		default:
			CZapitDestructor();
	}
}

#ifdef DBOX2
/*
 * return 0 on success or if nothing to do
 * return -1 otherwise
 */
int startVbi ()
{
	if ((channel->getTeletextPid() == NONE) || (channel->getTeletextPid() >= INVALID))
	{
		return -1;
	}

	if ((vbi_fd == -1) && ((vbi_fd = open(VBI_DEVICE, O_RDWR)) < 0))
	{
		perror ("[zapit] " VBI_DEVICE);
		return -1;
	}

	if (ioctl(vbi_fd, AVIA_VBI_START_VTXT, channel->getTeletextPid()) < 0)
	{
		perror("[zapit] VBI_START_VTXT");
		close(vbi_fd);
		vbi_fd = -1;
		return -1;
	}

	return 0;
}

/*
 * return 0 on success or if nothing to do
 * return -1 otherwise
 */
int stopVbi ()
{
	if (vbi_fd == -1)
	{
		return 0;
	}

	if (ioctl(vbi_fd, AVIA_VBI_STOP_VTXT, 0) < 0)
	{
		perror("[zapit] VBI_STOP_VTXT");
		close(vbi_fd);
		vbi_fd = -1;
		return -1;
	}

	close(vbi_fd);
	vbi_fd = -1;
	return 0;
}
#endif /* DBOX2 */

void save_settings (bool write)
{
	if (channel != NULL)
	{
		config->setInt32("lastChannelMode", (currentMode & RADIO_MODE) ? 1 : 0);

		// now save the lowest channel number with the current channel_id
		int c = ((currentMode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).getLowestChannelNumberWithChannelID(channel->getChannelID());
		if (c >= 0)
			config->setInt32((currentMode & RADIO_MODE) ? "lastChannelRadio" : "lastChannelTV", c);
	}

	if (write)
	{
		config->setInt32("diseqcRepeats", frontend->getDiseqcRepeats());
		config->setInt32("diseqcType", frontend->getDiseqcType());
		config->saveConfig(CONFIGFILE);
	}
}

CZapitClient::responseGetLastChannel load_settings()
{
	CZapitClient::responseGetLastChannel lastchannel;

	if (config->getInt32("lastChannelMode", 0))
		lastchannel.mode = 'r';
	else
		lastchannel.mode = 't';

	lastchannel.channelNumber = config->getInt32((currentMode & RADIO_MODE) ? "lastChannelRadio" : "lastChannelTV", 0);

	return lastchannel;
}

/*
 * - find transponder
 * - stop teletext, video, audio, pcr
 * - tune
 * - set up pids
 * - send channel name to lcdd
 * - start descrambler
 * - start pcr, audio, video, teletext
 *
 * return 0 on success
 * return -1 otherwise
 *
 */
int zapit(const t_channel_id channel_id, bool in_nvod)
{
	bool transponder_change;
	tallchans_iterator cit;

	if (in_nvod)
	{
		current_is_nvod = true;

		cit = nvodchannels.find(channel_id);
		if (cit == nvodchannels.end())
		{
			debug("[zapit] channel_id %08x not found\n", channel_id);
			return -1;
		}
	}
	else
	{
		current_is_nvod = false;

		cit = allchans.find(channel_id);

		if (currentMode & RADIO_MODE)
		{
			if ((cit == allchans.end()) || (cit->second.getServiceType() != DIGITAL_RADIO_SOUND_SERVICE))
			{
				debug("[zapit] channel_id %08x not found\n", channel_id);
				return -1;
			}
		}
		else
		{
			if (cit == allchans.end() || (cit->second.getServiceType() == DIGITAL_RADIO_SOUND_SERVICE))
			{
				debug("[zapit] channel_id %08x not found\n", channel_id);
				return -1;
			}
			nvodname = cit->second.getName();
		}
	}

	stopPlayBack();

	/* store the new channel */
	if ((channel == NULL) || (channel_id != channel->getChannelID()))
	{
		channel = &(cit->second);
	}

	/* if channel's transponder does not match frontend's tuned transponder ... */
	if (channel->getTsidOnid() != frontend->getTsidOnid())
	{
		/* ... tune to it if not in record mode ... */
		if (currentMode & RECORD_MODE)
		{
			return -1;
		}

		if (frontend->tuneChannel(channel) == false)
		{
#ifdef EXCESSIVE_TUNING_RETRIES
			unsigned char retries;

			for (retries = 0; retries < 5; retries++)
			{
				printf("[zapit] tune retry %d\n", retries);

				if (frontend->tuneChannel(channel) == true)
				{
					break;
				}
			}

			if (retries == 5)
			{
				return -1;
			}
#else
			return -1;
#endif
		}
		
		if (channel->getTsidOnid() != frontend->getTsidOnid())
		{
			printf("[zapit] zigzag tuning probably failed:\n");
			printf("[zapit] requested tsid/onid %08x but frontend is at tsid/onid %08x\n", channel->getTsidOnid(), frontend->getTsidOnid());
			return -1;
		}

		transponder_change = true;
	}
	else
	{
		transponder_change = false;
	}

	if (channel->getServiceType() == NVOD_REFERENCE_SERVICE)
	{
		current_is_nvod = true;
		save_settings(false);
		return 0;
	}

	/* search pids if they are unknown */
#ifdef USE_PID_CACHE
	if (channel->getPidsFlag() == false)
#endif

	{
		bool failed = false;
		int dmx_sct_fd;

		debug("[zapit] looking up pids for channel_id %08x\n", channel->getChannelID());

		/* open demux device */
		if ((dmx_sct_fd = open(DEMUX_DEVICE, O_RDWR)) < 0)
		{
			perror("[zapit] " DEMUX_DEVICE);
			return -1;
		}

		/* get program map table pid from program association table */
		if (channel->getPmtPid() == NONE)
		{
			if (parse_pat(dmx_sct_fd, channel) < 0)
			{
				debug("[zapit] pat parsing failed\n");
				failed = true;
			}
		}

		/* parse program map table and store pids */
		if ((!failed) && (parse_pmt(dmx_sct_fd, channel) < 0))
		{
			debug("[zapit] pmt parsing failed\n");
			failed = true;
		}

		if ((!failed) && (channel->getAudioPid() == NONE) && (channel->getVideoPid() == NONE))
		{
			debug("[zapit] neither audio nor video pid found.\n");
			failed = true;
		}

		close (dmx_sct_fd);

		if (failed)
		{
			channel->resetPids();
			return -1;
		}
	}

	if (transponder_change == true)
	{
		channel->getCaPmt()->ca_pmt_list_management = 0x03;
	}
	else
	{
		channel->getCaPmt()->ca_pmt_list_management = 0x04;
	}

#ifdef DBOX2
	if (in_nvod)
	{
		lcdd.setServiceName(nvodname);
	}
	else
	{
		lcdd.setServiceName(cit->second.getName());
	}
#endif /* DBOX2 */

	debug("[zapit] setting ca pmt\n");
	cam->setCaPmt(channel->getCaPmt());

	startPlayBack();

	save_settings(false);

	return 0;
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int changeapid (uint8_t index)
{
	/* stop demux filter */
	if (stopDmxFilter(dmx_audio_fd) < 0)
		return -1;

	/* stop audio playback */
	//audio->stop();

	/* update current channel */
	channel->setAudioChannel(index);

	/* set bypass mode */
	if (channel->getAudioChannel()->isAc3)
		audio->enableBypass();
	else
		audio->disableBypass();

	/* set demux filter */
	if (setDmxPesFilter(dmx_audio_fd, DMX_OUT_DECODER, DMX_PES_AUDIO, channel->getAudioPid()) < 0)
		return -1;

	/* start audio playback */
	//audio->start();

	/* start demux filter */
	if (startDmxFilter(dmx_audio_fd) < 0)
		return -1;

	return 0;
}

void setRadioMode ()
{
	currentMode |= RADIO_MODE;
	currentMode &= ~TV_MODE;
}

void setTVMode ()
{
	currentMode |= TV_MODE;
	currentMode &= ~RADIO_MODE;
}

void setRecordMode ()
{
	currentMode |= RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_ACTIVATED, CEventServer::INITID_ZAPIT );
}

void unsetRecordMode ()
{
	currentMode &= ~RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_DEACTIVATED, CEventServer::INITID_ZAPIT );
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int prepare_channels ()
{
	// for the case this function is NOT called for the first time (by main())
	// we clear all cannel lists, they are refilled
	// by LoadServices() and LoadBouquets()
	transponders.clear();
	bouquetManager->clearAll();
	allchans.clear();  // <- this invalidates all bouquets, too!

	if (LoadServices() < 0)
		return -1;

	printf("[zapit] LoadServices: success\n");

	bouquetManager->loadBouquets();

	return 0;
}


void parseScanInputXml()
{
	switch (frontend->getInfo()->type)
	{
	case FE_QPSK:
		scanInputParser = parseXmlFile(string(SATELLITES_XML));
		break;
		
	case FE_QAM:
		scanInputParser = parseXmlFile(string(CABLES_XML));
		break;
		
	default:
		printf("Unknown type %d\n", frontend->getInfo()->type);
		return;
	}
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int start_scan ()
{
	if (scanInputParser == NULL)
	{
		parseScanInputXml();
		if (scanInputParser == NULL)
		{
			printf("[zapit] scan not configured. won't scan.\n");
			return -1;
		}
	}

	transponders.clear();
	bouquetManager->clearAll();
	allchans.clear();  // <- this invalidates all bouquets, too!

	found_transponders = 0;
	found_channels = 0;

	stopPlayBack();

	scan_runs = 1;

	if (pthread_create(&scan_thread, 0, start_scanthread, NULL))
	{
		perror("[zapit] pthread_create: scan_thread");
		scan_runs = 0;
		return -1;
	}

	return 0;
}

void parse_command (CZapitMessages::commandHead &rmsg)
{
	debug("[zapit] cmd %d (version %d) received\n", rmsg.cmd, rmsg.version);

	if (rmsg.version == CZapitMessages::ACTVERSION)
	{
		switch (rmsg.cmd)
		{
			case CZapitMessages::CMD_ZAPTO:
			{
				CZapitMessages::commandZapto msgZapto;
				read(connfd, &msgZapto, sizeof(msgZapto)); // bouquet & channel number are already starting at 0!
				zapTo(msgZapto.bouquet, msgZapto.channel);
				break;
			}
			case CZapitMessages::CMD_ZAPTO_CHANNELNR:
			{
				CZapitMessages::commandZaptoChannelNr msgZaptoChannelNr;
				read(connfd, &msgZaptoChannelNr, sizeof(msgZaptoChannelNr)); // bouquet & channel number are already starting at 0!
				zapTo(msgZaptoChannelNr.channel);
				break;
			}
			case CZapitMessages::CMD_ZAPTO_SERVICEID:
			case CZapitMessages::CMD_ZAPTO_SUBSERVICEID:
			{
				CZapitMessages::commandZaptoServiceID msgZaptoServiceID;
				CZapitMessages::responseZapComplete msgResponseZapComplete;
				read(connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
				msgResponseZapComplete.zapStatus = zapTo_ChannelID(msgZaptoServiceID.channel_id, (rmsg.cmd == CZapitMessages::CMD_ZAPTO_SUBSERVICEID));
				send(connfd, &msgResponseZapComplete, sizeof(msgResponseZapComplete), 0);
				break;
			}
			case CZapitMessages::CMD_ZAPTO_SERVICEID_NOWAIT:
			case CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT:
			{
				CZapitMessages::commandZaptoServiceID msgZaptoServiceID;
				read(connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
				zapTo_ChannelID(msgZaptoServiceID.channel_id, (rmsg.cmd == CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT));
				break;
			}
			case CZapitMessages::CMD_GET_LAST_CHANNEL:
			{
				CZapitClient::responseGetLastChannel responseGetLastChannel;
				responseGetLastChannel = load_settings();
				send(connfd, &responseGetLastChannel, sizeof(responseGetLastChannel), 0); // bouquet & channel number are already starting at 0!
				break;
			}
			case CZapitMessages::CMD_SET_AUDIOCHAN:
			{
				CZapitMessages::commandSetAudioChannel msgSetAudioChannel;
				read(connfd, &msgSetAudioChannel, sizeof(msgSetAudioChannel));
				changeapid(msgSetAudioChannel.channel);
				break;
			}
			case CZapitMessages::CMD_SET_MODE:
			{
				CZapitMessages::commandSetMode msgSetMode;
				read(connfd, &msgSetMode, sizeof(msgSetMode));
				if (msgSetMode.mode == CZapitClient::MODE_TV)
				{
					setTVMode();
				}
				else if (msgSetMode.mode == CZapitClient::MODE_RADIO)
				{
					setRadioMode();
				}
				break;
			}
			case CZapitMessages::CMD_GET_CURRENT_SERVICEID:
			{
				CZapitMessages::responseGetCurrentServiceID msgCurrentSID;
				msgCurrentSID.channel_id = channel->getChannelID();
				send(connfd, &msgCurrentSID, sizeof(msgCurrentSID), 0);
				break;
			}
			case CZapitMessages::CMD_GET_CURRENT_SERVICEINFO:
			{
				CZapitClient::CCurrentServiceInfo msgCurrentServiceInfo;
				msgCurrentServiceInfo.onid = channel->getOriginalNetworkId();
				msgCurrentServiceInfo.sid = channel->getServiceId();
				msgCurrentServiceInfo.tsid = channel->getTransportStreamId();
				msgCurrentServiceInfo.vdid = channel->getVideoPid();
				msgCurrentServiceInfo.apid = channel->getAudioPid();
				msgCurrentServiceInfo.vtxtpid = channel->getTeletextPid();
				msgCurrentServiceInfo.pcrpid = channel->getPcrPid();
				msgCurrentServiceInfo.tsfrequency = frontend->getFrequency();
				if (frontend->getInfo()->type == FE_QPSK)
				{
					msgCurrentServiceInfo.polarisation = frontend->getPolarization();
				}
				else
				{
					msgCurrentServiceInfo.polarisation = 2;
				}
				msgCurrentServiceInfo.diseqc = channel->getDiSEqC();
				send(connfd, &msgCurrentServiceInfo, sizeof(msgCurrentServiceInfo), 0);
				break;
			}
			case CZapitMessages::CMD_GET_BOUQUETS:
			{
				CZapitMessages::commandGetBouquets msgGetBouquets;
				read(connfd, &msgGetBouquets, sizeof(msgGetBouquets));
				sendBouquets(msgGetBouquets.emptyBouquetsToo); // bouquet & channel number are already starting at 0!
				break;
			}
			case CZapitMessages::CMD_GET_BOUQUET_CHANNELS:
			{
				CZapitMessages::commandGetBouquetChannels msgGetBouquetChannels;
				read(connfd, &msgGetBouquetChannels, sizeof(msgGetBouquetChannels));
				sendBouquetChannels(msgGetBouquetChannels.bouquet, msgGetBouquetChannels.mode); // bouquet & channel number are already starting at 0!
				break;
			}
			case CZapitMessages::CMD_GET_CHANNELS:
			{
				CZapitMessages::commandGetChannels msgGetChannels;
				read(connfd, &msgGetChannels, sizeof(msgGetChannels));
				sendChannels(msgGetChannels.mode, msgGetChannels.order); // bouquet & channel number are already starting at 0!
				break;
			}
			case CZapitMessages::CMD_RESTORE_BOUQUETS:
			{
				CZapitMessages::responseCmd response;
				bouquetManager->restoreBouquets();
				response.cmd = CZapitMessages::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
				break;
			}
			case CZapitMessages::CMD_REINIT_CHANNELS:
			{
				CZapitMessages::responseCmd response;
				prepare_channels();
				response.cmd = CZapitMessages::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
				eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
				break;
			}
			case CZapitMessages::CMD_COMMIT_BOUQUET_CHANGE:
			{
				CZapitMessages::responseCmd response;
				bouquetManager->renumServices();
				response.cmd = CZapitMessages::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
				eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
				break;
			}
			case CZapitMessages::CMD_SCANSTART:
				start_scan();
				break;

			case CZapitMessages::CMD_SCANREADY:
			{
				CZapitMessages::responseIsScanReady msgResponseIsScanReady;
				msgResponseIsScanReady.satellite = curr_sat;
				msgResponseIsScanReady.transponder = found_transponders;
				msgResponseIsScanReady.services = found_channels;
				if (scan_runs > 0)
				{
					msgResponseIsScanReady.scanReady = false;
				}
				else
				{
					msgResponseIsScanReady.scanReady = true;
				}
				send(connfd, &msgResponseIsScanReady, sizeof(msgResponseIsScanReady), 0);
				break;
			}
			case CZapitMessages::CMD_SCANGETSATLIST:
			{
				if (scanInputParser == NULL)
				{
					parseScanInputXml();

					if (scanInputParser == NULL)
						break;
				}

				CZapitClient::responseGetSatelliteList msgResponseGetSatelliteList;
				XMLTreeNode *search = scanInputParser->RootNode()->GetChild();

				while (search != NULL)
				{
					strncpy(msgResponseGetSatelliteList.satName, search->GetAttributeValue("name"), sizeof(msgResponseGetSatelliteList.satName));
					send(connfd, &msgResponseGetSatelliteList, sizeof(msgResponseGetSatelliteList), 0);
					search = search->GetNext();
				}
				break;
			}
			case CZapitMessages::CMD_SCANSETSCANSATLIST:
			{
				CZapitClient::commandSetScanSatelliteList sat;
				scanProviders.clear();
				while (read(connfd, &sat, sizeof(sat)))
				{
					printf("[zapit] adding %s (diseqc %d)\n", sat.satName, sat.diseqc);
					scanProviders[sat.diseqc] = sat.satName;
				}
				break;
			}
			case CZapitMessages::CMD_SCANSETDISEQCTYPE:
			{
				diseqc_t diseqc;
				read(connfd, &diseqc, sizeof(diseqc));
				frontend->setDiseqcType(diseqc);
				printf("[zapit] set diseqc type %d\n", diseqc);
				break;
			}
			case CZapitMessages::CMD_SCANSETDISEQCREPEAT:
			{
				uint32_t repeats;
				read(connfd, &repeats, sizeof(repeats));
				frontend->setDiseqcRepeats(repeats);
				printf("[zapit] set diseqc repeats to %d\n", repeats);
				break;
			}
			case CZapitMessages::CMD_SCANSETBOUQUETMODE:
				read(connfd, &bouquetMode, sizeof(bouquetMode));
				break;

			case CZapitMessages::CMD_SET_RECORD_MODE:
			{
				CZapitMessages::commandSetRecordMode msgSetRecordMode;
				read(connfd, &msgSetRecordMode, sizeof(msgSetRecordMode));
				if (msgSetRecordMode.activate)
				{
					setRecordMode();
				}
				else
				{
					unsetRecordMode();
				}
				break;
			}
			case CZapitMessages::CMD_GET_RECORD_MODE:
			{
				CZapitMessages::responseGetRecordModeState msgGetRecordModeState;
				msgGetRecordModeState.activated = (currentMode & RECORD_MODE);
				send(connfd, &msgGetRecordModeState, sizeof(msgGetRecordModeState), 0);
				break;
			}
			case CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE:
			{
				CZapitMessages::responseGetPlaybackState msgGetPlaybackState;
				if (video->isPlaying())
				{
					msgGetPlaybackState.activated = 1;
				}
				else
				{
					msgGetPlaybackState.activated = 0;
				}
				send(connfd, &msgGetPlaybackState, sizeof(msgGetPlaybackState), 0);
				break;
			}
			case CZapitMessages::CMD_BQ_ADD_BOUQUET:
			{
				CZapitMessages::commandAddBouquet msgAddBouquet;
				read(connfd, &msgAddBouquet, sizeof(msgAddBouquet));
				bouquetManager->addBouquet(msgAddBouquet.name);
				break;
			}
			case CZapitMessages::CMD_BQ_DELETE_BOUQUET:
			{
				CZapitMessages::commandDeleteBouquet msgDeleteBouquet;
				read(connfd, &msgDeleteBouquet, sizeof(msgDeleteBouquet)); // bouquet & channel number are already starting at 0!
				bouquetManager->deleteBouquet(msgDeleteBouquet.bouquet);
				break;
			}
			case CZapitMessages::CMD_BQ_RENAME_BOUQUET:
			{
				CZapitMessages::commandRenameBouquet msgRenameBouquet;
				read(connfd, &msgRenameBouquet, sizeof(msgRenameBouquet)); // bouquet & channel number are already starting at 0!
				if (msgRenameBouquet.bouquet < bouquetManager->Bouquets.size())
					bouquetManager->Bouquets[msgRenameBouquet.bouquet]->Name = msgRenameBouquet.name;
				break;
			}
			case CZapitMessages::CMD_BQ_EXISTS_BOUQUET:
			{
				CZapitMessages::commandExistsBouquet msgExistsBouquet;
				CZapitMessages::responseGeneralInteger responseInteger;
				read(connfd, &msgExistsBouquet, sizeof(msgExistsBouquet));
				responseInteger.number = bouquetManager->existsBouquet(msgExistsBouquet.name);
				send(connfd, &responseInteger, sizeof(responseInteger), 0); // bouquet & channel number are already starting at 0!
				break;
			}
			case CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET:
			{
				CZapitMessages::commandExistsChannelInBouquet msgExistsChInBq;
				CZapitMessages::responseGeneralTrueFalse responseBool;
				read(connfd, &msgExistsChInBq, sizeof(msgExistsChInBq)); // bouquet & channel number are already starting at 0!
				responseBool.status = bouquetManager->existsChannelInBouquet(msgExistsChInBq.bouquet, msgExistsChInBq.channel_id);
				send(connfd, &responseBool, sizeof(responseBool), 0);
				break;
			}
			case CZapitMessages::CMD_BQ_MOVE_BOUQUET:
			{
				CZapitMessages::commandMoveBouquet msgMoveBouquet;
				read(connfd, &msgMoveBouquet, sizeof(msgMoveBouquet)); // bouquet & channel number are already starting at 0!
				bouquetManager->moveBouquet(msgMoveBouquet.bouquet, msgMoveBouquet.newPos);
				break;
			}
			case CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET:
			{
				CZapitMessages::commandAddChannelToBouquet msgAddChannelToBouquet;
				read(connfd, &msgAddChannelToBouquet, sizeof(msgAddChannelToBouquet)); // bouquet & channel number are already starting at 0!
				addChannelToBouquet(msgAddChannelToBouquet.bouquet, msgAddChannelToBouquet.channel_id);
				break;
			}
			case CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET:
			{
				CZapitMessages::commandRemoveChannelFromBouquet msgRemoveChannelFromBouquet;
				read(connfd, &msgRemoveChannelFromBouquet, sizeof(msgRemoveChannelFromBouquet)); // bouquet & channel number are already starting at 0!
				if (msgRemoveChannelFromBouquet.bouquet < bouquetManager->Bouquets.size())
					bouquetManager->Bouquets[msgRemoveChannelFromBouquet.bouquet]->removeService(msgRemoveChannelFromBouquet.channel_id);
				break;
			}
			case CZapitMessages::CMD_BQ_MOVE_CHANNEL:
			{
				CZapitMessages::commandMoveChannel msgMoveChannel;
				read(connfd, &msgMoveChannel, sizeof(msgMoveChannel)); // bouquet & channel number are already starting at 0!
				if (msgMoveChannel.bouquet < bouquetManager->Bouquets.size())
					bouquetManager->Bouquets[msgMoveChannel.bouquet]->moveService(msgMoveChannel.oldPos, msgMoveChannel.newPos, (((currentMode & RADIO_MODE) && msgMoveChannel.mode == CZapitClient::MODE_CURRENT) || (msgMoveChannel.mode==CZapitClient::MODE_RADIO)) ? 2 : 1);
				break;
			}
			case CZapitMessages::CMD_BQ_SET_LOCKSTATE:
			{
				CZapitMessages::commandBouquetState msgBouquetLockState;
				read(connfd, &msgBouquetLockState, sizeof(msgBouquetLockState)); // bouquet & channel number are already starting at 0!
				if (msgBouquetLockState.bouquet < bouquetManager->Bouquets.size())
					bouquetManager->Bouquets[msgBouquetLockState.bouquet]->bLocked = msgBouquetLockState.state;
				break;
			}
			case CZapitMessages::CMD_BQ_SET_HIDDENSTATE:
			{
				CZapitMessages::commandBouquetState msgBouquetHiddenState;
				read(connfd, &msgBouquetHiddenState, sizeof(msgBouquetHiddenState)); // bouquet & channel number are already starting at 0!
				if (msgBouquetHiddenState.bouquet < bouquetManager->Bouquets.size())
					bouquetManager->Bouquets[msgBouquetHiddenState.bouquet]->bHidden = msgBouquetHiddenState.state;
				break;
			}
			case CZapitMessages::CMD_BQ_RENUM_CHANNELLIST:
				bouquetManager->renumServices();
				break;

			case CZapitMessages::CMD_BQ_SAVE_BOUQUETS:
			{
				CZapitMessages::responseCmd response;
				bouquetManager->saveBouquets();
				response.cmd = CZapitMessages::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
				break;
			}
			case CZapitMessages::CMD_SB_START_PLAYBACK:
				playbackStopForced = false;
				startPlayBack();
				break;

			case CZapitMessages::CMD_SB_STOP_PLAYBACK:
				stopPlayBack();
				playbackStopForced = true;
				break;

			case CZapitMessages::CMD_SET_DISPLAY_FORMAT:
			{
				CZapitMessages::commandInt msg;
				read(connfd, &msg, sizeof(msg));
				video->setCroppingMode((videoDisplayFormat_t) msg.val);
				break;
			}

			case CZapitMessages::CMD_SET_AUDIO_MODE:
			{
				CZapitMessages::commandInt msg;
				read(connfd, &msg, sizeof(msg));
				audio->selectChannel((audioChannelSelect_t) msg.val);
				break;
			}

			case CZapitMessages::CMD_GETPIDS:
			{
				CZapitClient::responseGetOtherPIDs responseGetOtherPIDs;
				responseGetOtherPIDs.vpid = channel->getVideoPid();
				responseGetOtherPIDs.ecmpid = NONE; // TODO: remove
				responseGetOtherPIDs.vtxtpid = channel->getTeletextPid();
				responseGetOtherPIDs.pcrpid = channel->getPcrPid();
				responseGetOtherPIDs.selected_apid = channel->getAudioChannelIndex();
				send(connfd, &responseGetOtherPIDs, sizeof(responseGetOtherPIDs), 0);
				sendAPIDs();
				break;
			}
			case CZapitMessages::CMD_SETSUBSERVICES:
			{
				CZapitClient::commandAddSubServices msgAddSubService;

				while (read(connfd, &msgAddSubService, sizeof(msgAddSubService)))
				{
					t_original_network_id original_network_id = msgAddSubService.original_network_id;
					t_service_id          service_id          = msgAddSubService.service_id;
					nvodchannels.insert
					(
					    std::pair <t_channel_id, CZapitChannel>
					    (
						CREATE_CHANNEL_ID,
						CZapitChannel
						(
						    "NVOD",
						    service_id,
						    msgAddSubService.transport_stream_id,
						    original_network_id,
						    1,
						    channel->getDiSEqC()
						)
					    )
					);
				}

				current_is_nvod = true;
				break;
			}
			case CZapitMessages::CMD_REGISTEREVENTS:
				eventServer->registerEvent(connfd);
				break;

			case CZapitMessages::CMD_UNREGISTEREVENTS :
				eventServer->unRegisterEvent(connfd);
				break;

			case CZapitMessages::CMD_MUTE:
			{
				CZapitMessages::commandBoolean msgBoolean;
				read(connfd, &msgBoolean, sizeof(msgBoolean));
				if (msgBoolean.truefalse)
				{
					audio->mute();
				}
				else
				{
					audio->unmute();
				}
				break;
			}
			case CZapitMessages::CMD_SET_VOLUME:
			{
				CZapitMessages::commandVolume msgVolume;
				read(connfd, &msgVolume, sizeof(msgVolume));
				audio->setVolume(msgVolume.left, msgVolume.right);
				break;
			}
			default:
				printf("[zapit] unknown command %d (version %d)\n", rmsg.cmd, CZapitMessages::ACTVERSION);
				break;
		}
	}
	else
	{
		printf("[zapit] Command ignored: cmd version %d received - zapit cmd version is %d\n", rmsg.version, CZapitMessages::ACTVERSION);
		return;
	}
	debug("[zapit] cmd %d processed\n", rmsg.cmd);
}

int main (int argc, char **argv)
{
	int listenfd;
	struct sockaddr_un servaddr;
	int clilen;

	CZapitClient::responseGetLastChannel test_lastchannel;
	int i;

	printf("$Id: zapit.cpp,v 1.251 2002/10/03 19:05:12 thegoodguy Exp $\n\n");

	if (argc > 1)
	{
		for (i = 1; i < argc ; i++)
		{
			if (!strcmp(argv[i], "-d"))
			{
				debug = true;
			}
			else if (!strcmp(argv[i], "-q"))
			{
				/* don't say anything */
				int fd;

				close(STDOUT_FILENO);
				if ((fd = open("/dev/null", O_WRONLY)) != STDOUT_FILENO)
					close(fd);

				close(STDERR_FILENO);
				if ((fd = open("/dev/null", O_WRONLY)) != STDERR_FILENO)
					close(fd);
			}
			else
			{
				printf("Usage: zapit [-d] [-q]\n");
				exit(0);
			}
		}
	}

	scan_runs = 0;
	found_transponders = 0;
	found_channels = 0;
	curr_sat = -1;

	/* load configuration */
	config = new CConfigFile(',');

	if (!config->loadConfig(CONFIGFILE))
	{
		/* set defaults if no configuration file exists */
		printf("[zapit] %s not found\n", CONFIGFILE);
	}

	/* create bouquet manager */
	bouquetManager = new CBouquetManager();

	test_lastchannel = load_settings();

	if (test_lastchannel.mode == 'r')
		setRadioMode();
	else
		setTVMode();

	if (prepare_channels() < 0)
		printf("[zapit] error parsing services!\n");
	else
		printf("[zapit] channels have been loaded succesfully\n");

	/* initialize frontend */
	frontend = new CFrontend();

	if (!frontend->isInitialized())
	{
		printf("[zapit] unable to open frontend devices. bye.\n");
		CZapitDestructor();
	}
	else
	{
		char tmp[16];

		frontend->setDiseqcType((diseqc_t) config->getInt32("diseqcType", NO_DISEQC));
		frontend->setDiseqcRepeats(config->getInt32("diseqcRepeats", 0));

		for (i = 0; i < MAX_LNBS; i++)
		{
			/* low offset */
			sprintf(tmp, "lnb%d_OffsetLow", i);
			frontend->setLnbOffset(false, i, config->getInt32(tmp, 9750000));
			/* high offset */
			sprintf(tmp, "lnb%d_OffsetHigh", i);
			frontend->setLnbOffset(true, i, config->getInt32(tmp, 10600000));
		}
	}

	audio = new CAudio();

	if (!audio->isInitialized())
	{
		printf("[zapit] unable to initialize audio device\n");
		CZapitDestructor();
	}
	else
	{
		audio->setVolume(255, 255);
	}

	video = new CVideo();

	if (!video->isInitialized())
	{
		printf("[zapit] unable to initialize video device\n");
		CZapitDestructor();
	}
	else
	{
		video->setBlank(true);
	}

	/* initialize cam */
	cam = new CCam();

	/* network setup */
	std::string filename = ZAPIT_UDS_NAME;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(filename.c_str());

	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[zapit] socket");
		return -1;
	}
	if (bind(listenfd, (struct sockaddr*) &servaddr, clilen) < 0)
	{
		perror("[zapit] bind");
		return -1;
	}
	if (listen(listenfd, 5) != 0)
	{
		perror("[zapit] listen");
		return -1;
	}

	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGUSR1, signal_handler);

	if (debug == false)
	{
		switch (fork())
		{
			case -1: /* can't fork */
				perror("[zapit] fork");
				return -1;

			case 0: /* child, process becomes a daemon */
				if (setsid() == -1)
				{
					perror("[zapit] setsid");
					return -1;
				}
				break;

			default: /* parent returns to calling process */
				return 0;
		}
	}

	// create eventServer
	eventServer = new CEventServer;

	while (true)
	{
		CZapitMessages::commandHead rmsg;
		connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
		memset(&rmsg, 0, sizeof(rmsg));
		read(connfd, &rmsg, sizeof(rmsg));
		parse_command(rmsg);
		close(connfd);
		connfd = -1;
	}

	return 0;
}

/****************************************************************/
/*								*/
/*  functions for new command handling via CZapitClient		*/
/*								*/
/*  these functions should be encapsulated in a class CZapit	*/
/*								*/
/****************************************************************/

void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	printf("addChannelToBouquet(%d, %d)\n", bouquet, channel_id);
	CZapitChannel* chan = bouquetManager->findChannelByChannelID(channel_id);
	if (chan != NULL)
		if (bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[bouquet]->addService(chan);
		else
			printf("bouquet not found!\n");
	else
		printf("channel_id not found in channellist!\n");
}

void sendBouquets(bool emptyBouquetsToo)
{
	for (uint i=0; i<bouquetManager->Bouquets.size(); i++)
	{
		if (emptyBouquetsToo ||
			((currentMode & RADIO_MODE) && (bouquetManager->Bouquets[i]->radioChannels.size()> 0) && (!bouquetManager->Bouquets[i]->bHidden)) ||
			(currentMode & TV_MODE) && (bouquetManager->Bouquets[i]->tvChannels.size()> 0) && (!bouquetManager->Bouquets[i]->bHidden))
		{
// ATTENTION: in RECORD_MODE empty bouquets are not send!
			if ((!(currentMode & RECORD_MODE)) || ((currentMode & RECORD_MODE) &&
							       (((currentMode & RADIO_MODE) && (bouquetManager->Bouquets[i]->recModeRadioSize( frontend->getTsidOnid())) > 0 ) ||
								(currentMode & TV_MODE)    && (bouquetManager->Bouquets[i]->recModeTVSize( frontend->getTsidOnid())) > 0 )))
			{
				CZapitClient::responseGetBouquets msgBouquet;

				strncpy(msgBouquet.name, bouquetManager->Bouquets[i]->Name.c_str(), 30);
				msgBouquet.bouquet_nr = i;
				msgBouquet.locked     = bouquetManager->Bouquets[i]->bLocked;
				msgBouquet.hidden     = bouquetManager->Bouquets[i]->bHidden;
				if (send(connfd, &msgBouquet, sizeof(msgBouquet),0) == -1)
				{
					perror("[zapit] could not send any return\n");
					return;
				}
			}
		}
	}
}

void internalSendChannels(ChannelList* channels, const unsigned int first_channel_nr)
{
	for (uint32_t i = 0; i < channels->size();i++)
	{
		if ((currentMode & RECORD_MODE) && ((*channels)[i]->getTsidOnid() != frontend->getTsidOnid()))
			continue;

		CZapitClient::responseGetBouquetChannels response;
		strncpy(response.name, (*channels)[i]->getName().c_str(),30);
		response.channel_id = (*channels)[i]->getChannelID();
		response.nr = first_channel_nr + i;

		if (send(connfd, &response, sizeof(response),0) == -1)
		{
			perror("[zapit] could not send any return\n");
			return;
		}
	}
}

void sendAPIDs()
{
	for (uint32_t i = 0; i < channel->getAudioChannelCount(); i++)
	{
		CZapitClient::responseGetAPIDs response;
		response.pid = channel->getAudioPid(i);
		strncpy(response.desc, channel->getAudioChannel(i)->description.c_str(), 25);
		response.is_ac3 = channel->getAudioChannel(i)->isAc3;
		response.component_tag = channel->getAudioChannel(i)->componentTag;

		if (send(connfd, &response, sizeof(response),0) == -1)
		{
			perror("[zapit] could not send any return\n");
			return;
		}
	}
}


void sendBouquetChannels(const unsigned int bouquet, CZapitClient::channelsMode mode)
{
	if (bouquet >= bouquetManager->Bouquets.size())
	{
		printf("[zapit] invalid bouquet number: %d", bouquet);
		return;
	}

	if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode == CZapitClient::MODE_RADIO))
		internalSendChannels(&(bouquetManager->Bouquets[bouquet]->radioChannels), bouquetManager->radioChannelsBegin().getNrofFirstChannelofBouquet(bouquet));
	else
		internalSendChannels(&(bouquetManager->Bouquets[bouquet]->tvChannels), bouquetManager->tvChannelsBegin().getNrofFirstChannelofBouquet(bouquet));
}

void sendChannels( CZapitClient::channelsMode mode, CZapitClient::channelsOrder order)
{
	ChannelList channels;

	if (order == CZapitClient::SORT_BOUQUET)
	{
		CBouquetManager::ChannelIterator cit = (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO)) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin();
		for (; !(cit.EndOfChannels()); cit++)
			channels.push_back(*cit);
	}
	else if (order == CZapitClient::SORT_ALPHA)   // ATTENTION: in this case response.nr does not return the actual number of the channel for zapping!
	{
		if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO))
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() == DIGITAL_RADIO_SOUND_SERVICE) 
					channels.push_back(&(it->second));
		}
		else
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() != DIGITAL_RADIO_SOUND_SERVICE) 
					channels.push_back(&(it->second));
		}
		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendChannels(&channels, 0);
}

int startPlayBack()
{
	bool have_audio = false;
	bool have_pcr = false;
	bool have_video = false;

	if (playbackStopForced == true)
		return -1;

	if ((dmx_pcr_fd == -1) && (dmx_pcr_fd = open(DEMUX_DEVICE, O_RDWR)) < 0)
	{
		perror("[zapit] " DEMUX_DEVICE);
		return -1;
	}

	if ((dmx_audio_fd == -1) && (dmx_audio_fd = open(DEMUX_DEVICE, O_RDWR)) < 0)
	{
		perror("[zapit] " DEMUX_DEVICE);
		return -1;
	}

	if ((dmx_video_fd == -1) && (dmx_video_fd = open(DEMUX_DEVICE, O_RDWR)) < 0)
	{
		perror("[zapit] " DEMUX_DEVICE);
		return -1;
	}

	if (channel->getAudioPid() != 0)
		have_audio = true;

	if (channel->getPcrPid() != 0)
		have_pcr = true;

	if ((channel->getVideoPid() != 0) && (currentMode & TV_MODE))
		have_video = true;

	if ((!have_audio) && (!have_video))
		return -1;

	/* set demux filters */
	if (have_pcr)
		setDmxPesFilter(dmx_pcr_fd, DMX_OUT_DECODER, DMX_PES_PCR, channel->getPcrPid());

	if (have_audio)
		setDmxPesFilter(dmx_audio_fd, DMX_OUT_DECODER, DMX_PES_AUDIO, channel->getAudioPid());
#ifdef DBOX2
	/* FIXME: what happens on other devices, when setting an invalid pid? */
	else
		setDmxPesFilter(dmx_audio_fd, DMX_OUT_DECODER, DMX_PES_AUDIO, 0xFFFF);
#endif

	if (have_video)
		setDmxPesFilter(dmx_video_fd, DMX_OUT_DECODER, DMX_PES_VIDEO, channel->getVideoPid());
#ifdef DBOX2
	/* FIXME: what happens on other devices, when setting an invalid pid? */
	else
		setDmxPesFilter(dmx_video_fd, DMX_OUT_DECODER, DMX_PES_VIDEO, 0xFFFF);
#endif

	if (have_video) {
		/* start video */
		video->setSource(VIDEO_SOURCE_DEMUX);
		video->start();
	}

	if (have_audio) {
		/* select audio output */
		if (channel->getAudioChannel()->isAc3)
			audio->enableBypass();
		else
			audio->disableBypass();

		/* start audio */
		audio->setSource(AUDIO_SOURCE_DEMUX);
		audio->start();
	}

	/* start demux filters */
	if (have_pcr)
		startDmxFilter(dmx_pcr_fd);
	if (have_audio)
		startDmxFilter(dmx_audio_fd);
	if (have_video)
		startDmxFilter(dmx_video_fd);

#if 0
	if ((dmx_general_fd == -1) && (dmx_general_fd = open(DEMUX_DEVICE, O_RDWR)))
	{
		perror("[zapit] " DEMUX_DEVICE);
		return -1;
	}

	setPesFilter(dmx_general_fd, DMX_OUT_TAP, DMX_PES_OTHER, channel->getAudioPid());

	bool indicator = true;
	struct pollfd pfd[1];
	uint8_t i;
	uint8_t buf[16384];
	uint16_t j;
	uint8_t k = 8;
	ssize_t len;

	pfd[0].fd = dmx_general_fd;
	pfd[0].events = POLLIN;

	if ((dmx_general_fd != -1) && (poll(pfd, 1, 1000) > 0) && (pfd[0].revents & POLLIN))
	{
		for (i = 0; i < k; i++)
		{
			if ((len = read(dmx_general_fd, buf, sizeof(buf))) < 0)
			{
				perror("[zapit] read");
				break;
			}

			for (j = 0; j < len; j++)
			{
				if ((buf[j] == 0x00) && (buf[j+1] == 0x00) && (buf[j+2] == 0x01))
				{
					switch (buf[j+3])
					{
							case 0xC0 ... 0xDF:
							if (((buf[j+6] >> 6) & 0x03) == 2)
							{
								if (((buf[j+6] >> 4) & 0x03) == 0)
								{
									indicator = false;
								}
							}
							break;
					}
				}

				if (indicator == false)
				{
					break;
				}
			}

			if (indicator == false)
			{
				break;
			}
		}
	}

	unsetPesFilter(dmx_general_fd);

	printf("[zapit] indicator: %d\n", indicator);
#endif

#ifdef DBOX2
	startVbi();
#endif /* DBOX2 */

	return 0;
}

int stopPlayBack()
{
	if (playbackStopForced == true)
		return -1;

#ifdef DBOX2
	stopVbi();
#endif /* DBOX2 */

	audio->stop();
	video->stop();

	stopDmxFilter(dmx_video_fd);
	stopDmxFilter(dmx_audio_fd);
	stopDmxFilter(dmx_pcr_fd);

	return 0;
}

unsigned zapTo(const unsigned int bouquet, const unsigned int channel)
{
	if (bouquet >= bouquetManager->Bouquets.size())
	{
		printf("[zapit] Invalid bouquet %d\n", bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	ChannelList* channels;

	if (currentMode & RADIO_MODE)
		channels = &(bouquetManager->Bouquets[bouquet]->radioChannels);
	else
		channels = &(bouquetManager->Bouquets[bouquet]->tvChannels);

	if (channel >= channels->size())
	{
		printf("[zapit] Invalid channel %d in bouquet %d\n", channel, bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	return zapTo_ChannelID((*channels)[channel]->getChannelID(), false);
}

unsigned int zapTo_ChannelID(t_channel_id channel_id, bool isSubService)
{
	unsigned int result = 0;

	if (zapit(channel_id, isSubService) < 0)
	{
		eventServer->sendEvent((isSubService ? CZapitClient::EVT_ZAP_SUB_FAILED : CZapitClient::EVT_ZAP_FAILED), CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
		return result;
	}

	result |= CZapitClient::ZAP_OK;

	if (isSubService)
	{
		eventServer->sendEvent(CZapitClient::EVT_ZAP_SUB_COMPLETE, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
	}
	else if (current_is_nvod)
	{
		eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
		result |= CZapitClient::ZAP_IS_NVOD;
	}
	else
		eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));

	return result;
}

unsigned zapTo(const unsigned int channel)
{
	CBouquetManager::ChannelIterator cit = ((currentMode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).FindChannelNr(channel);
	if (!(cit.EndOfChannels()))
		return zapTo_ChannelID((*cit)->getChannelID(), false);
	else
		return 0;
}

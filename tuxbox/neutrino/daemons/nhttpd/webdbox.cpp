/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webdbox.cpp,v 1.46 2003/02/21 19:23:49 dirch Exp $

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#include <neutrinoMessages.h>

#include "webdbox.h"
#include "webserver.h"
#include "request.h"
#include "helper.h"
#include "debug.h"

#define SA struct sockaddr
#define SAI struct sockaddr_in


//-------------------------------------------------------------------------
void CWebDbox::UpdateBouquets(void)
{
	BouquetList.clear();
	Zapit->getBouquets(BouquetList,true); 

	for(unsigned int i = 1; i <= BouquetList.size();i++)
		UpdateBouquet(i);
	UpdateChannelList();
}
//-------------------------------------------------------------------------

void CWebDbox::ZapTo(string target)
{
	t_channel_id channel_id = atoi(target.c_str());
	if(channel_id == Zapit->getCurrentServiceID())
	{
//		printf("Kanal ist aktuell\n");
		return;
	}
	unsigned int status = Zapit->zapTo_serviceID(channel_id);
	if(status != CZapitClient::ZAP_INVALID_PARAM)
	{
		if(status == CZapitClient::ZAP_IS_NVOD)
		{
			Zapit->zaptoNvodSubService(1);
		}
		Sectionsd->setServiceChanged(channel_id,false);
	}
}
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// Konstruktor und destruktor
//-------------------------------------------------------------------------

CWebDbox::CWebDbox(CWebserver *server)
{
	Parent=server;
//	standby_mode=false;

	Controld = new CControldClient();
	Sectionsd = new CSectionsdClient();
	Zapit = new CZapitClient();
	Timerd = new CTimerdClient();

	ControlAPI = new CControlAPI(this);
	WebAPI = new CWebAPI(this);
	BouqueteditAPI = new CBouqueteditAPI(this);

	UpdateBouquets();

	Dbox_Hersteller[1] = "Nokia";
	Dbox_Hersteller[2] = "Sagem";
	Dbox_Hersteller[3] = "Philips";
	videooutput_names[0] = "Composite";
	videooutput_names[1] = "RGB";
	videooutput_names[2] = "S-Video";
	videoformat_names[0] = "automatic";
	videoformat_names[1] = "16:9";
	videoformat_names[2] = "4:3";
	audiotype_names[1] =  "single channel";
	audiotype_names[2] = "dual channel";
	audiotype_names[3] = "joint stereo";
	audiotype_names[4] = "stereo";

	EventServer = new CEventServer;
	EventServer->registerEvent2( NeutrinoMessages::SHUTDOWN, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");
	EventServer->registerEvent2( NeutrinoMessages::STANDBY_ON, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");
	EventServer->registerEvent2( NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");
	EventServer->registerEvent2( NeutrinoMessages::STANDBY_TOGGLE, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");
	EventServer->registerEvent2( NeutrinoMessages::EVT_POPUP, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");
	EventServer->registerEvent2( NeutrinoMessages::EVT_EXTMSG, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");
	EventServer->registerEvent2( NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, "/tmp/neutrino.sock");

}
//-------------------------------------------------------------------------

CWebDbox::~CWebDbox()
{
	if(BouqueteditAPI)
		delete BouqueteditAPI;
	if(WebAPI)
		delete WebAPI;
	if(ControlAPI)
		delete ControlAPI;

	if(Controld)
		delete Controld;
	Controld = NULL;
	if(Sectionsd)
		delete Sectionsd;
	Sectionsd = NULL;
	if(Zapit)
		delete Zapit;
	Zapit = NULL;
	if(Timerd)
		delete Timerd;
	Timerd = NULL;
	if(EventServer)
		delete EventServer;
	EventServer = NULL;
}

//-------------------------------------------------------------------------
// Get functions
//-------------------------------------------------------------------------

bool CWebDbox::GetStreamInfo(int bitInfo[10])
{
	char *key,*tmpptr,buf[100];
	int value, pos=0;

	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		aprintf("error while opening proc-bitstream\n" );
		return false;
	}

	fgets(buf,29,fd);//dummy
	while(!feof(fd))
	{
		if(fgets(buf,29,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			key=strsep(&tmpptr,":");
			for(;tmpptr[0]==' ';tmpptr++);
			value=atoi(tmpptr);
			bitInfo[pos]= value;
			pos++;
		}
	}
	fclose(fd);

	return true;
}

//-------------------------------------------------------------------------

void CWebDbox::GetChannelEvents()
{
	eList = Sectionsd->getChannelEvents();
	CChannelEventList::iterator eventIterator;

    for( eventIterator = eList.begin(); eventIterator != eList.end(); eventIterator++ )
		ChannelListEvents[(*eventIterator).serviceID()] = &(*eventIterator);
}
//-------------------------------------------------------------------------
string CWebDbox::GetServiceName(t_channel_id channel_id)
{
	for(unsigned int i = 0; i < TVChannelList.size();i++)
		if( TVChannelList[i].channel_id == channel_id)
			return TVChannelList[i].name;
	for(unsigned int i = 0; i < RadioChannelList.size();i++)
		if( RadioChannelList[i].channel_id == channel_id)
			return RadioChannelList[i].name;
	return "";
}


//-------------------------------------------------------------------------
CZapitClient::BouquetChannelList * CWebDbox::GetBouquet(unsigned int BouquetNr, int Mode)
{
int mode;
	if(Mode == CZapitClient::MODE_CURRENT )
		mode = Zapit->getMode();
	else
		mode = Mode;
	
	if(mode == CZapitClient::MODE_TV)
		return &TVBouquetsList[BouquetNr];
	else
		return &RadioBouquetsList[BouquetNr];
}
//-------------------------------------------------------------------------
CZapitClient::BouquetChannelList * CWebDbox::GetChannelList(int Mode)
{
int mode;
	if(Mode == CZapitClient::MODE_CURRENT )
		mode = Zapit->getMode();
	else
		mode = Mode;
	
	if(mode == CZapitClient::MODE_TV)
		return &TVChannelList;
	else
		return &RadioChannelList;
}
//-------------------------------------------------------------------------
void CWebDbox::UpdateBouquet(unsigned int BouquetNr)
{
	TVBouquetsList[BouquetNr].clear();
	RadioBouquetsList[BouquetNr].clear();
	Zapit->getBouquetChannels(BouquetNr - 1, TVBouquetsList[BouquetNr], CZapitClient::MODE_TV);
	Zapit->getBouquetChannels(BouquetNr - 1, RadioBouquetsList[BouquetNr], CZapitClient::MODE_RADIO);
}
//-------------------------------------------------------------------------

void CWebDbox::UpdateChannelList(void)
{
	TVChannelList.clear();
	RadioChannelList.clear();
	Zapit->getChannels(RadioChannelList, CZapitClient::MODE_RADIO);
	Zapit->getChannels(TVChannelList, CZapitClient::MODE_TV);
}
//-------------------------------------------------------------------------

void CWebDbox::timerEventType2Str(CTimerd::CTimerEventTypes type, char *str,int len)
{
   switch(type)
   {
      case CTimerd::TIMER_SHUTDOWN : strncpy(str, "Shutdown",len);
         break;
      case CTimerd::TIMER_NEXTPROGRAM : strncpy(str, "N�chstes Programm", len);
         break;
      case CTimerd::TIMER_ZAPTO : strncpy(str, "Umschalten", len);
         break;
      case CTimerd::TIMER_STANDBY : strncpy(str, "Standby", len);
         break;
      case CTimerd::TIMER_RECORD : strncpy(str, "Aufnahme", len);
         break;
      case CTimerd::TIMER_REMIND : strncpy(str, "Erinnerung", len);
         break;
      case CTimerd::TIMER_SLEEPTIMER: strncpy(str, "Sleeptimer", len);
         break;
      default: strncpy(str, "Unbekannt", len);
   }
   str[len]=0;
}
//-------------------------------------------------------------------------
void CWebDbox::timerEventRepeat2Str(CTimerd::CTimerEventRepeat rep, char *str,int len)
{
   switch(rep)
   {
      case CTimerd::TIMERREPEAT_ONCE : strncpy(str, "einmal",len);
         break;
      case CTimerd::TIMERREPEAT_DAILY : strncpy(str, "t�glich",len);
         break;
      case CTimerd::TIMERREPEAT_WEEKLY : strncpy(str, "w�chentlich",len);
         break;
      case CTimerd::TIMERREPEAT_BIWEEKLY : strncpy(str, "2-w�chentlich",len);
         break;
      case CTimerd::TIMERREPEAT_FOURWEEKLY : strncpy(str, "4-w�chentlich",len);
         break;
      case CTimerd::TIMERREPEAT_MONTHLY : strncpy(str, "monatlich",len);
         break;
      case CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION : strncpy(str, "siehe event",len);
         break;
      case CTimerd::TIMERREPEAT_WEEKDAYS : strncpy(str, "wochentage",len);
         break;
		default: 
			if(rep > CTimerd::TIMERREPEAT_WEEKDAYS)
			{
				str[0]=0;
				if(rep & 0x200)
					strcat(str,"Mo ");
				if(rep & 0x400)
					strcat(str,"Di ");
				if(rep & 0x800)
					strcat(str,"Mi ");
				if(rep & 0x1000)
					strcat(str,"Do ");
				if(rep & 0x2000)
					strcat(str,"Fr ");
				if(rep & 0x4000)
					strcat(str,"Sa ");
				if(rep & 0x8000)
					strcat(str,"So ");
			}
			else
				strncpy(str, "Unbekannt", len);
   }
   str[len]=0;
}
//-------------------------------------------------------------------------

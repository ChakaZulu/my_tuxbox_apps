/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webapi.cpp,v 1.23 2002/12/09 17:59:27 dirch Exp $

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

#include <neutrinoMessages.h>


#include "webapi.h"
#include "debug.h"
#include "algorithm"


//-------------------------------------------------------------------------
bool CWebAPI::Execute(CWebserverRequest* request)
{
	int operation = 0;

	const char *operations[] = {
		"test.dbox2", "timer.dbox2","info.dbox2","dbox.dbox2","bouquetlist.dbox2",
		"channellist.dbox2","controlpanel.dbox2",
		"actualepg.dbox2","epg.dbox2","switch.dbox2",NULL};

	dprintf("Executing %s\n",request->Filename.c_str());

	while (operations[operation]) {
		if (request->Filename.compare(operations[operation]) == 0) {
			break;
		}
		operation++;
	}

	if (operations[operation] == NULL) {
		request->Send404Error();
		return false;
	}

	if (request->Method == M_HEAD) {
		request->SendPlainHeader("text/html");
		return true;
	}
	switch(operation)
	{
		case 0:	return Test(request);
			break;
		case 1:	return Timer(request);
			break;
		case 2:	return ShowCurrentStreamInfo(request);
			break;
		case 3:	return Dbox(request);
			break;
		case 4:	return ShowBouquets(request);
			break;
		case 5:	return Channellist(request);
			break;
		case 6:	return Controlpanel(request);
			break;
		case 7:	return ActualEPG(request);
			break;
		case 8:	return EPG(request);
			break;
		case 9:	return Switch(request);
			break;
		default:
			request->Send404Error();
			return false;
	}
}
//-------------------------------------------------------------------------
bool CWebAPI::Test(CWebserverRequest* request)
// testing stuff
{
	request->SendPlainHeader("text/html");		
	return true;
}

void CWebAPI::loadTimerMain(CWebserverRequest* request)
{
//	request->SocketWrite("<HTML><script language=\"JavaScript\">location.href=\"/fb/timer.dbox2\"</script></HTML>\n");
	request->Send302("/fb/timer.dbox2");
}

//-------------------------------------------------------------------------
bool CWebAPI::Timer(CWebserverRequest* request)
// timer functions
{

	if(Parent->Timerd->isTimerdAvailable())
	{
		if(request->ParameterList.size() > 0)
		{
			if(!request->Authenticate())
				return false;
			if(request->ParameterList["action"] == "remove")
			{
				unsigned removeId = atoi(request->ParameterList["id"].c_str());
				Parent->Timerd->removeTimerEvent(removeId);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "modify-form")
			{
				request->SendPlainHeader("text/html");
				unsigned modyId = atoi(request->ParameterList["id"].c_str());
				modifyTimerForm(request, modyId);
			}
			else if(request->ParameterList["action"] == "modify")
			{
				doModifyTimer(request);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "new-form")
			{
				request->SendPlainHeader("text/html");
				newTimerForm(request);
			}
			else if(request->ParameterList["action"] == "new")
			{
				doNewTimer(request);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "none")
			{
				request->SendPlainHeader("text/html");
				ShowTimerList(request);
			}
			else
			{
				request->SendPlainHeader("text/html");
				request->SendHTMLHeader("UNKNOWN ACTION");
				aprintf("Unknown action : %s\n",request->ParameterList["action"].c_str());
				request->SendHTMLFooter ();
			}
		}
		else
		{
			request->SendPlainHeader("text/html");
			ShowTimerList(request);
		}
	}
	else
	{
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader ("Error");
		aprintf("<h1>Error: Timerd not available</h1>\n");
		request->SendHTMLFooter ();
	}
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::Dbox(CWebserverRequest* request)
// shows "menu" page
{
	request->SendPlainHeader("text/html");		
	ShowDboxMenu(request);
	return true;
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowBouquets(CWebserverRequest* request)
// show the bouquet list
{
string classname;
	request->SendPlainHeader("text/html");
//	ShowBouquets(request,(request->ParameterList["bouquet"] != "")?atoi(request->ParameterList["bouquet"].c_str()):0);
//	return true;
	int BouquetNr = (request->ParameterList["bouquet"] != "")?atoi(request->ParameterList["bouquet"].c_str()):0;
	bool javascript = (request->ParameterList["js"].compare("1") == 0);
	request->SocketWriteLn("<HTML>\n<HEAD><title>DBOX2-Neutrino Bouquetliste</title><link rel=\"stylesheet\" TYPE=\"text/css\" HREF=\"../global.css\">");
	request->SocketWrite("<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n function go_to(url1, url2)\n{\n top.content.location.href = url1;\n top.bouquets.location.href = url2;\n }\n//-->\n </SCRIPT>\n</HEAD><BODY>");

	request->SocketWriteLn("<TABLE cellspacing=0 cellpadding=0 border=0 width=\"100%\">");
	request->SocketWriteLn("<TR><TD><A CLASS=\"blist\" HREF=\"/bouquetedit/main\" TARGET=\"content\">Bouqueteditor</A></TD></TR>\n<TR><TD><HR></TD></TR>");
	classname = (BouquetNr == 0)?" CLASS=\"bouquet\"":"";
	if(javascript)
		request->SocketWrite("<TR height=20"+ classname + "><TD><a CLASS=\"blist\" HREF=\"javascript:go_to('/fb/channellist.dbox2#akt','/fb/bouquetlist.dbox2?bouquet=0')\">Alle Kan�le</a></TD></TR>\n");
	else
		request->SocketWrite("<TR height=20"+ classname + "><TD><a CLASS=\"blist\" HREF=\"/fb/channellist.dbox2#akt\" TARGET=\"content\">Alle Kan�le</a></TD></TR>\n");
	request->SocketWrite("<TR><TD><HR></TD></TR>\n");
	CZapitClient::BouquetList::iterator bouquet = Parent->BouquetList.begin();
	for(; bouquet != Parent->BouquetList.end();bouquet++)
	{
		if(!bouquet->hidden)
		{
			classname = ((bouquet->bouquet_nr + 1) == (uint) BouquetNr)?" CLASS=\"bouquet\"":"";
			if(javascript)
				request->printf("<tr height=\"20\"%s><TD><NOBR><a CLASS=\"blist\" HREF=\"javascript:go_to('/fb/channellist.dbox2?bouquet=%d#akt','/fb/bouquetlist.dbox2?js=1&bouquet=%d');\">%s</a></NOBR></TD></TR>\n",classname.c_str(),(bouquet->bouquet_nr + 1),(bouquet->bouquet_nr + 1),bouquet->name);
			else
				request->printf("<tr height=\"20\"%s><TD><NOBR><a CLASS=\"blist\" HREF=\"/fb/channellist.dbox2?bouquet=%d#akt\" TARGET=\"content\">%s</a></NOBR></TD></TR>\n",classname.c_str(),(bouquet->bouquet_nr + 1),bouquet->name);
		}
	}
	request->SocketWrite("</TABLE>\n");
	request->SendHTMLFooter();
	return true;

}
//-------------------------------------------------------------------------

bool CWebAPI::Channellist(CWebserverRequest* request)
// show the channel (bouquet) list
{
	request->SendPlainHeader("text/html");
	if( (request->ParameterList.size() == 1) && ( request->ParameterList["bouquet"] != "") )
	{
		ShowBouquet(request,atoi(request->ParameterList["bouquet"].c_str()));
	}
	else
		ShowBouquet(request);
	return true;
}
//-------------------------------------------------------------------------

bool CWebAPI::Controlpanel(CWebserverRequest* request)
// show controlpanel
{	
int mode;

	if (request->ParameterList.size() > 0)							// parse the parameters first
	{
		if(!request->Authenticate())
				return false;

		if( request->ParameterList["1"] == "volumemute")
		{
			bool mute = Parent->Controld->getMute();
			Parent->Controld->setMute( !mute );
			dprintf("mute\n");
		}
		else if( request->ParameterList["1"] == "volumeplus")
		{
			char vol = Parent->Controld->getVolume();
			vol+=5;
			if (vol>100)
				vol=100;
			Parent->Controld->setVolume(vol);
			dprintf("Volume plus: %d\n",vol);
		}
		else if( request->ParameterList["1"] == "volumeminus")
		{
			char vol = Parent->Controld->getVolume();
			if (vol>0)
				vol-=5;
			Parent->Controld->setVolume(vol);
			dprintf("Volume minus: %d\n",vol);
		}
		else if( request->ParameterList["1"] == "standby")
		{
			Parent->EventServer->sendEvent(NeutrinoMessages::STANDBY_TOGGLE, CEventServer::INITID_HTTPD);
		}
		else if(request->ParameterList["1"] == "tvmode")				// switch to tv mode
		{
			mode = NeutrinoMessages::mode_tv;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
			request->Send302("channellist.dbox2#akt");
			return true;
		}
		else if(request->ParameterList["1"] == "radiomode")				// switch to radio mode
		{
			if(!request->Authenticate())
				return false;
			mode = NeutrinoMessages::mode_radio;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
			request->Send302("channellist.dbox2#akt");
			return true;
		}
	}

	ShowControlpanel(request);									// show the controlpanel
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ActualEPG(CWebserverRequest* request)
// show epg info about the actual tuned program
{
	ShowActualEpg(request);
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::EPG(CWebserverRequest* request)
// show epg for eventid or epgid and startzeit
{

	if(request->ParameterList.size() > 0)
	{											

		if(request->ParameterList["eventlist"] != "")				// what the hell has this to to here ?
		{															// TBD: move it here
			request->SendPlainHeader("text/html");
			unsigned id = atol( request->ParameterList["eventlist"].c_str() );
			ShowEventList( request, id );
			return true;
		}
		if(request->ParameterList["1"] == "eventlist")				// s.a.
		{
			request->SendPlainHeader("text/html");
			dprintf("service id: %u\n",Parent->Zapit->getCurrentServiceID());
			ShowEventList( request, Parent->Zapit->getCurrentServiceID() );
			return true;
		}

		if(request->ParameterList["eventid"] != "")
		{
			ShowEpg(request,request->ParameterList["eventid"]);
			return true;
		}
		else if(request->ParameterList["epgid"] != "")
		{
			ShowEpg(request,request->ParameterList["epgid"],request->ParameterList["startzeit"]);
			return true;
		}
	}
	dperror("[HTTPD] Get epgid error\n");
	return false;
}

//-------------------------------------------------------------------------
bool CWebAPI::Switch(CWebserverRequest* request)
// switch something
{
	if(request->ParameterList.size() > 0)
	{

		if(request->ParameterList["zapto"] != "")					// zap to channel and redirect to channel/bouquet list
		{
			if(!request->Authenticate())
				return false;

			Parent->ZapTo(request->ParameterList["zapto"]);
			request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");

			if(request->ParameterList["bouquet"] != "")
				request->SocketWriteLn("Location: channellist.dbox2?bouquet="+request->ParameterList["bouquet"]+"#akt");
			else
				request->SocketWriteLn("Location: channellist.dbox2#akt");
			return true;
		}


		if(request->ParameterList["1"] == "shutdown")				// turn box off
		{
			if(!request->Authenticate())
				return false;
			request->SendPlainHeader("text/html");
			request->SendFile("/","shutdown.html");	// send shutdown page
			request->EndRequest();
			sleep(1);															// wait 
			Parent->EventServer->sendEvent(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_HTTPD);
			return true;
		}

	}
	dprintf("Keine Parameter gefunden\n");
	request->Send404Error();
	return false;
}

//-------------------------------------------------------------------------
// Show funtions (Execute)
//-------------------------------------------------------------------------

bool CWebAPI::ShowDboxMenu(CWebserverRequest* request)
{
	CStringList params;
	params["BoxType"] = Parent->Dbox_Hersteller[Parent->Controld->getBoxType()];
	request->ParseFile("dbox.html",params);
	return true;
}


//-------------------------------------------------------------------------
bool CWebAPI::ShowCurrentStreamInfo(CWebserverRequest* request)
{
	int bitInfo[10];
	char buf[100];
	CStringList params;
	CZapitClient::CCurrentServiceInfo serviceinfo;

	request->SendPlainHeader("text/html");		

	serviceinfo = Parent->Zapit->getCurrentServiceInfo();
	params["onid"] = itoh(serviceinfo.onid);
	params["sid"] = itoh(serviceinfo.sid);
	params["tsid"] = itoh(serviceinfo.tsid);
	params["vpid"] = itoh(serviceinfo.vdid);
	params["apid"] = itoh(serviceinfo.apid);
	params["vtxtpid"] = (serviceinfo.vtxtpid != 0)?itoh(serviceinfo.vtxtpid):"nicht verf�gbar";
	params["tsfrequency"] = itoa(serviceinfo.tsfrequency);
	params["polarisation"] = serviceinfo.polarisation==1?"v":"h";
	params["ServiceName"] = Parent->GetServiceName(Parent->Zapit->getCurrentServiceID());
	Parent->GetStreamInfo(bitInfo);
	
	sprintf((char*) buf, "%d x %d", bitInfo[0], bitInfo[1] );
	params["VideoFormat"] = buf; //Resolution x y
	sprintf((char*) buf, "%d\n", bitInfo[4]*50);
	params["BitRate"] = buf; //Bitrate bit/sec
	
	switch ( bitInfo[2] ) //format
	{
		case 2: params["AspectRatio"] = "4:3"; break;
		case 3: params["AspectRatio"] = "16:9"; break;
		case 4: params["AspectRatio"] = "2.21:1"; break;
		default: params["AspectRatio"] = "unknown"; break;
	}

	switch ( bitInfo[3] ) //fps
	{
		case 3: params["FPS"] = "25"; break;
		case 6: params["FPS"] = "50"; break;
		default: params["FPS"] = "unknown";
	}

	switch ( bitInfo[6] )
	{
		case 1: params["AudioType"] = "single channel"; break;
		case 2: params["AudioType"] = "dual channel"; break;
		case 3: params["AudioType"] = "joint stereo"; break;
		case 4: params["AudioType"] = "stereo"; break;
		default: params["AudioType"] = "unknown";
	}
	request->ParseFile("settings.html",params);
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEventList(CWebserverRequest *request,t_channel_id channel_id)
{
char classname;
int pos = 0;
char mode;

	mode = (Parent->Zapit->getMode() == CZapitClient::MODE_RADIO)?'R':'T';
	Parent->eList = Parent->Sectionsd->getEventsServiceKey(channel_id);
	CChannelEventList::iterator eventIterator;
	request->SendHTMLHeader("DBOX2-Neutrino Channellist");


	request->SocketWriteLn("<CENTER><H3 CLASS=\"epg\">Programmvorschau: " + Parent->GetServiceName(channel_id) + "</H3></CENTER>");

	request->SocketWrite("<CENTER><TABLE WIDTH=\"95%\" CELLSPACING=\"0\">\n");

	for( eventIterator = Parent->eList.begin(); eventIterator != Parent->eList.end(); eventIterator++, pos++ )
	{
		classname = (pos&1)?'a':'b';
		char zbuffer[25] = {0};
		struct tm *mtime = localtime(&eventIterator->startTime); //(const time_t*)eventIterator->startTime);
		strftime(zbuffer,20,"%d.%m. %H:%M",mtime);
		request->printf("<TR VALIGN=\"middle\" HEIGHT=\"%d\" CLASS=\"%c\">\n",(eventIterator->duration > 20 * 60)?(eventIterator->duration / 60):20 , classname);
		request->printf("<TD><NOBR>");
		request->printf("<A HREF=\"/fb/timer.dbox2?action=new&type=%d&alarm=%u&stop=%u&channel_id=%c%u\">&nbsp;<IMG SRC=\"/images/record.gif\" WIDTH=\"16\" HEIGHT=\"16\" ALT=\"Sendung aufnehmen\"></A>&nbsp;\n",CTimerd::TIMER_RECORD,(uint) eventIterator->startTime,(uint) eventIterator->startTime + eventIterator->duration,mode,channel_id); 
		request->printf("<A HREF=\"/fb/timer.dbox2?action=new&type=%d&alarm=%u&channel_id=%c%u\">&nbsp;<IMG SRC=\"/images/timer.gif\" WIDTH=\"21\" HEIGHT=\"21\" ALT=\"Timer setzen\"></A>&nbsp;\n",CTimerd::TIMER_ZAPTO,(uint) eventIterator->startTime,mode,channel_id); 
		request->printf("</NOBR></TD><TD><NOBR>%s&nbsp;<font size=\"-2\">(%d min)</font>&nbsp;</NOBR></TD>\n", zbuffer, eventIterator->duration / 60);
		request->printf("<TD><A CLASS=\"elist\" HREF=epg.dbox2?eventid=%llx>%s</A></TD>\n</TR>\n", eventIterator->eventID, eventIterator->description.c_str());
		if(eventIterator->text.length() > 0)
			request->printf("<TR VALIGN=\"middle\" CLASS=\"%c\"><TD COLSPAN=2></TD><TD>%s</TD></TR>\n",classname,eventIterator->text.c_str());

	}

	request->SocketWriteLn("</TABLE></CENTER>");
	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool CWebAPI::ShowBouquet(CWebserverRequest* request, int BouquetNr)
{
	CZapitClient::BouquetChannelList *channellist;
	if(BouquetNr > 0)
		channellist = Parent->GetBouquet(BouquetNr,CZapitClient::MODE_CURRENT);
	else
		channellist = Parent->GetChannelList(CZapitClient::MODE_CURRENT);

	Parent->GetChannelEvents();

	request->SendHTMLHeader("DBOX2-Neutrino Kanalliste");

	request->SocketWriteLn("<TABLE cellspacing=0 border=0 WIDTH=\"90%\">");

	int i = 1;
	char classname;
	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	int prozent;

	CZapitClient::BouquetChannelList::iterator channel = channellist->begin();
	for(; channel != channellist->end();channel++)
	{
		classname = (i++&1)?'a':'b';
		if(channel->channel_id == current_channel)
			classname = 'c';


		string bouquetstr = (BouquetNr >=0)?"&bouquet="+itoa(BouquetNr):"";
		
		request->printf("<TR><TD colspan=2 CLASS=\"%c\">",classname);
		request->printf("%s<A CLASS=\"clist\" HREF=\"switch.dbox2?zapto=%d%s\">%d. %s</A>&nbsp;<A HREF=\"epg.dbox2?eventlist=%u\">%s</A>",((channel->channel_id == current_channel)?"<A NAME=akt></a>":" "),channel->channel_id,bouquetstr.c_str(),channel->nr,channel->name,channel->channel_id,((Parent->ChannelListEvents[channel->channel_id])?"<img src=\"../images/elist.gif\" ALT=\"Programmvorschau\">":""));

		if(channel->channel_id == current_channel)
			request->printf("&nbsp;&nbsp;<A HREF=\"/fb/info.dbox2\"><IMG SRC=\"/images/streaminfo.gif\" BORDER=0 ALT=\"Streaminfo\"></A>");
		request->printf("</TD></TR>");
		CChannelEvent *event = Parent->ChannelListEvents[channel->channel_id];
		if(event)
		{
			prozent = 100 * (time(NULL) - event->startTime) / event->duration;
			request->printf("<TR><TD align=left width=31 CLASS=\"%cepg\">",classname);
			request->printf("<TABLE border=1 rules=none bordercolor=#000000 heigth=10 width=30 cellspacing=0 cellpadding=0><TR><TD bgcolor=\"#2211FF\" height=10 width=%d></TD><TD bgcolor=\"#EAEBFF\" heigth=10 width=%d></TD></TR></TABLE></TD>",(prozent / 10) * 3,(10 - (prozent / 10))*3);
			request->printf("<TD CLASS=\"%cepg\">",classname);
			request->printf("<A CLASS=\"clistsmall\" HREF=epg.dbox2?epgid=%llx&startzeit=%lx>",event->eventID,event->startTime);
			request->printf("%s&nbsp;",event->description.c_str()); 
			request->printf("<font size=-3><NOBR>(%ld von %d min, %d%%)</NOBR></font></a>&nbsp;</TD></TR>\n",(time(NULL) - event->startTime)/60,event->duration / 60,prozent  ); 
		}
		request->printf("<tr height=2><TD colspan=2></TD></TR>\n");

	}

	request->printf("</TABLE>\n");

	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------

bool CWebAPI::ShowControlpanel(CWebserverRequest* request)
{
CStringList params;
char volbuf[5];

	int vol = Parent->Controld->getVolume();
	sprintf(volbuf,"%d",vol);
	params["VOL1"] = volbuf;
	sprintf(volbuf,"%d",100 - vol);
	params["VOL2"] = volbuf;

	if(Parent->Parent->NewGui)
	{
		request->SendPlainHeader("text/html");

		if(	Parent->Controld->getMute())
		{
			params["MUTE0"] = "01";
			params["MUTE1"] = "00";
			params["MUTE_ICON0"] = "<img src=\"/images/if_lsmuted.jpg\" height=\"27\" width=\"100\"><br> <!--\n";
			params["MUTE_ICON1"] = "-->";
		}
		else
		{
			params["MUTE0"] = "00";
			params["MUTE1"] = "01";
			params["MUTE_ICON0"] = " ";
			params["MUTE_ICON1"] = " ";
		}
		
		request->ParseFile("controlpanel.html",params);
	}
	else
	{
		request->SendPlainHeader("text/html");
		if(	Parent->Controld->getMute())
			params["MUTE"] = "mute";
		else
			params["MUTE"] = "muted";
		request->ParseFile("controlpanel_old.html", params);
	}
	return true;

}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEPG(CWebserverRequest *request,string Title, string Info1, string Info2)
{
	CStringList params;
	params["Title"] = (Title != "")?Title:"Kein EPG vorhanden";
	params["Info1"] = (Info1 != "")?Info1:"keine ausf�hrlichen Informationen verf�gbar";
	params["Info2"] = (Info2 != "")?Info2:" ";
	request->ParseFile("epg.html",params);
	return true;
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowActualEpg(CWebserverRequest *request)
{
		CEPGData *epg = new CEPGData;
		if(Parent->Sectionsd->getActualEPGServiceKey(Parent->Zapit->getCurrentServiceID(),epg))
			ShowEPG(request,epg->title,epg->info1,epg->info2);			// epg available do show epg 
		else
			ShowEPG(request,epg->title,epg->info1,epg->info2);			// no epg available, TBD: show noepg page
		delete epg;
		return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEpg(CWebserverRequest *request,string EpgID,string Startzeit)
{
	unsigned long long epgid;
	uint startzeit;

	const char * idstr = EpgID.c_str();
	sscanf(idstr, "%llx", &epgid);

	if(Startzeit.length() > 0)
	{
		CEPGData *epg = new CEPGData;
		const char * timestr = Startzeit.c_str();
		sscanf(timestr, "%x", &startzeit);

		if(Parent->Sectionsd->getEPGid(epgid,startzeit,epg))			// starttime available then get all infos
			ShowEPG(request,epg->title,epg->info1,epg->info2);
		delete epg;
	}
	else
	{
		CShortEPGData *epg = new CShortEPGData;
		if(Parent->Sectionsd->getEPGidShort(epgid,(CShortEPGData *)epg))	// no starttime, short infos
			ShowEPG(request,epg->title,epg->info1,epg->info2);
		delete epg;
	}
	return true;
}

//-------------------------------------------------------------------------
int minmax(int value,int min, int max)
{
	if(value < min)	return min;
	if(value > max)	return max;
	return value;
}

void CWebAPI::correctTime(struct tm *zt)
{

	zt->tm_year = minmax(zt->tm_year,0,129);
	zt->tm_mon = minmax(zt->tm_mon,0,11);
	zt->tm_mday = minmax(zt->tm_mday,1,31); //-> eine etwas laxe pruefung, aber mktime biegt das wieder grade
	zt->tm_hour = minmax(zt->tm_hour,0,23);
	zt->tm_min = minmax(zt->tm_min,0,59);
	zt->tm_sec = minmax(zt->tm_sec,0,59);
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowTimerList(CWebserverRequest* request)
{

	CTimerd::TimerList timerlist;				// List of bouquets

	timerlist.clear();
	Parent->Timerd->getTimerList(timerlist);
	sort(timerlist.begin(), timerlist.end());

	CZapitClient::BouquetChannelList channellist_tv;     
	CZapitClient::BouquetChannelList channellist_radio;     
	channellist_tv.clear();
	channellist_radio.clear();

	request->SendHTMLHeader("TIMERLISTE");
	request->SocketWrite("<center>\n<TABLE CLASS=\"timer\" border=0>\n<TR>\n");
	request->SocketWrite("<TD CLASS=\"ctimer\" align=\"center\"><b>Alarm-Zeit</TD>\n");
	request->SocketWrite("<TD CLASS=\"ctimer\" align=\"center\"><b>Stop-Zeit</TD>\n");
	request->SocketWrite("<TD CLASS=\"ctimer\" align=\"center\"><b>Wiederholung</TD>\n");
	request->SocketWrite("<TD CLASS=\"ctimer\" align=\"center\"><b>Typ</TD>\n");
	request->SocketWrite("<TD CLASS=\"ctimer\" align=\"center\"><b>Beschreibung</TD>\n");
	request->SocketWrite("<TD CLASS=\"ctimer\"><TD CLASS=\"ctimer\"></TR>\n");

	int i = 1;
	char classname= 'a';
	CTimerd::TimerList::iterator timer = timerlist.begin();
	for(; timer != timerlist.end();timer++)
	{
		classname = (i++&1)?'a':'b';

		char zAlarmTime[25] = {0};
		struct tm *alarmTime = localtime(&(timer->alarmTime));
		strftime(zAlarmTime,20,"%d.%m. %H:%M",alarmTime);

		char zAnnounceTime[25] = {0};
		struct tm *announceTime = localtime(&(timer->announceTime));
		strftime(zAnnounceTime,20,"%d.%m. %H:%M",announceTime);

		char zStopTime[25] = {0};
		if(timer->stopTime > 0)
		{
			struct tm *stopTime = localtime(&(timer->stopTime));
			strftime(zStopTime,20,"%d.%m. %H:%M",stopTime);     
		}

		request->printf("<TR><TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zAlarmTime);
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zStopTime);
		char zRep[20+1];
		Parent->timerEventRepeat2Str(timer->eventRepeat,zRep,sizeof(zRep)-1);
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zRep);
		char zType[20+1];
		Parent->timerEventType2Str(timer->eventType,zType,sizeof(zType)-1);
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s", classname, zType);

		// Add Data
		char zAddData[20+1]={0};
		switch(timer->eventType)
		{
			case CTimerd::TIMER_NEXTPROGRAM :
			case CTimerd::TIMER_ZAPTO :
			case CTimerd::TIMER_RECORD :
			{
				if(timer->mode == CTimerd::MODE_RADIO)
				{ // Radiokanal
					if(channellist_radio.size()==0)
					{
						Parent->Zapit->getChannels(channellist_radio,CZapitClient::MODE_RADIO);
					}
					CZapitClient::BouquetChannelList::iterator channel = channellist_radio.begin();
					for(; channel != channellist_radio.end();channel++)
					{
						if (channel->channel_id == timer->channel_id)
						{
							strncpy(zAddData, channel->name, 20);
							zAddData[20]=0;
							break;
						}
					}
					if(channel == channellist_radio.end())
						strcpy(zAddData,"Unbekannter Radiokanal");
				}
				else
				{ //TV Kanal
					if(channellist_tv.size()==0)
					{
						Parent->Zapit->getChannels(channellist_tv, CZapitClient::MODE_TV);
					}
					CZapitClient::BouquetChannelList::iterator channel = channellist_tv.begin();
					for(; channel != channellist_tv.end();channel++)
					{
						if (channel->channel_id == timer->channel_id)
						{
							strncpy(zAddData, channel->name, 20);
							zAddData[20]=0;
							break;
						}
					}
					if(channel == channellist_tv.end())
						strcpy(zAddData,"Unbekannter TV-Kanal");
				}

			}
			break;
			case CTimerd::TIMER_STANDBY :
			{
				sprintf(zAddData,"Standby: %s",(timer->standby_on ? "An" : "Aus"));
			}
			break;
			case CTimerd::TIMER_REMIND :
			{
				strncpy(zAddData, timer->message, 20);
				zAddData[20]=0;
			}
			break;

			default:{}
		}
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s\n",
			classname, zAddData);
		request->printf("<TD CLASS=\"%ctimer\" align=center><a HREF=\"/fb/timer.dbox2?action=remove&id=%d\">\n",
			classname, timer->eventID);
		request->SocketWrite("<img src=\"../images/remove.gif\" alt=\"Timer l�schen\"></a></TD>\n");
		request->printf("<TD CLASS=\"%ctimer\" align=center><a HREF=\"/fb/timer.dbox2?action=modify-form&id=%d\">", 
			classname, timer->eventID);
		request->printf("<img src=\"../images/modify.gif\" alt=\"Timer �ndern\"></a><NOBR></TD></TR>\n");
	}
	classname = (i++&1)?'a':'b';
	request->printf("<TR><TD CLASS=\"%ctimer\" colspan=5></TD>\n<TD CLASS=\"%ctimer\" align=\"center\">\n",classname,classname);
	request->SocketWrite("<a HREF=\"javascript:location.reload()\">\n");
	request->SocketWrite("<img src=\"../images/reload.gif\" alt=\"Aktualisieren\"></a></TD>\n");   
	request->printf("<TD CLASS=\"%ctimer\" align=\"center\">\n",classname);
	request->SocketWrite("<a HREF=\"/fb/timer.dbox2?action=new-form\">\n");
	request->SocketWrite("<img src=\"../images/new.gif\" alt=\"neuer Timer\"></a></TD></TR>\n");
	request->SocketWrite("</TABLE>\n");
	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------
void CWebAPI::modifyTimerForm(CWebserverRequest *request, unsigned timerId)
{
	CTimerd::responseGetTimer timer;             // Timer

	Parent->Timerd->getTimer(timer, timerId);

	char zType[20+1];
	Parent->timerEventType2Str(timer.eventType,zType,20);

	request->SendHTMLHeader("TIMER BEARBEITEN");
	// Javascript
	request->SocketWrite("<script language =\"javascript\">\n");
	request->SocketWrite("function my_show(id) {document.getElementById(id).style.visibility=\"visible\";}\n");
	request->SocketWrite("function my_hide(id) {document.getElementById(id).style.visibility=\"hidden\";}\n");
	request->SocketWrite("function focusNMark() { document.modify.wd.select();\n");
	request->SocketWrite("                        document.modify.wd.focus();}\n");
	request->SocketWrite("function onEventChange() { tType=document.modify.rep.value;\n");
	request->printf("  if (tType == \"%d\") my_show(\"WeekdaysRow\"); else my_hide(\"WeekdaysRow\");\n",
		  (int)CTimerd::TIMERREPEAT_WEEKDAYS);
	request->SocketWrite("  focusNMark();}\n");
	request->SocketWrite("</script>\n");
	
	request->SocketWrite("<center>");
	request->SocketWrite("<TABLE border=2 ><tr CLASS=\"a\"><TD>\n");
	request->SocketWrite("<form method=\"GET\" name=\"modify\" action=\"/fb/timer.dbox2\">\n");
	request->SocketWrite("<INPUT TYPE=\"hidden\" name=\"action\" value=\"modify\">\n");
	request->printf("<INPUT name=\"id\" TYPE=\"hidden\" value=\"%d\">\n",timerId);
	request->SocketWrite("<TABLE border=0 >\n");
	request->printf("<tr CLASS=\"c\"><TD colspan=\"2\" align=\"center\">TIMER %d BEARBEITEN (%s)</TD></TR>\n",
		  timerId,zType);

	struct tm *alarmTime = localtime(&(timer.alarmTime));
	request->printf("<TR><TD align=\"center\"><NOBR>Alarm-Datum <INPUT TYPE=\"text\" name=\"ad\" value=\"%02d\" size=2 maxlength=2>. ",
		  alarmTime->tm_mday );
	request->printf("<INPUT TYPE=\"text\" name=\"amo\" value=\"%02d\" size=2 maxlength=2>.&nbsp",
		  alarmTime->tm_mon +1);
	request->printf("<INPUT TYPE=\"text\" name=\"ay\" value=\"%04d\" size=4 maxlength=4></TD>\n",
		  alarmTime->tm_year + 1900);
	request->printf("<TD align=\"center\"><NOBR>Zeit&nbsp;<INPUT TYPE=\"text\" name=\"ah\" value=\"%02d\" size=2 maxlength=2>&nbsp;:&nbsp;",
		  alarmTime->tm_hour );
	request->printf("<INPUT TYPE=\"text\" name=\"ami\" value=\"%02d\" size=2 maxlength=2></TD></TR>\n",
		  alarmTime->tm_min);
	if(timer.stopTime > 0)
	{
		struct tm *stopTime = localtime(&(timer.stopTime));
		request->printf("<TR><NOBR><TD align=\"center\"><NOBR>Stop-Datum&nbsp;<INPUT TYPE=\"text\" name=\"sd\" value=\"%02d\" size=2 maxlength=2>.&nbsp;",
			 stopTime->tm_mday );
		request->printf("<INPUT TYPE=\"text\" name=\"smo\" value=\"%02d\" size=2 maxlength=2>.&nbsp;",
			 stopTime->tm_mon +1);
		request->printf("<INPUT TYPE=\"text\" name=\"sy\" value=\"%04d\" size=4 maxlength=4></TD>\n",
			 stopTime->tm_year + 1900);
		request->printf("<TD align=\"center\"><NOBR>Zeit&nbsp;<INPUT TYPE=\"text\" name=\"sh\" value=\"%02d\" size=2 maxlength=2>&nbsp;:&nbsp;",
			 stopTime->tm_hour );
		request->printf("<INPUT TYPE=\"text\" name=\"smi\" value=\"%02d\" size=2 maxlength=2></TD></TR>\n",
			 stopTime->tm_min);
	}
	request->SocketWrite("<TR><TD align=\"center\">Wiederholung\n");
	request->SocketWrite("<select name=\"rep\" onchange=\"onEventChange();\">\n");
	char zRep[21];
	string visibility;
	for(int i=0; i<=6;i++)
	{
		if(i!=(int)CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION)
		{
			Parent->timerEventRepeat2Str((CTimerd::CTimerEventRepeat) i, zRep, sizeof(zRep)-1);
			request->printf("<option value=\"%d\"",i);
			if(((int)timer.eventRepeat) == i)
				request->SocketWrite(" selected");
			request->printf(">%s\n",zRep);
		}
	}
	request->printf("<option value=\"%d\"",(int)CTimerd::TIMERREPEAT_WEEKDAYS);
	Parent->timerEventRepeat2Str(CTimerd::TIMERREPEAT_WEEKDAYS, zRep, sizeof(zRep)-1);
	if(timer.eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
	{
		request->SocketWrite(" selected");
		visibility="visible";
	}
	else
		visibility="hidden";
	request->printf(">%s\n",zRep);
	request->SocketWrite("</select></TD></TR>\n");
	// Weekdays
	char weekdays[8];
	Parent->Timerd->setWeekdaysToStr(timer.eventRepeat, weekdays);
	request->printf("<tr id=\"WeekdaysRow\" style=\"visibility:%s\"><TD align=\"center\">\n",
								visibility.c_str());
	request->printf("Wochentage <INPUT TYPE=\"text\" name=\"wd\" value=\"%s\" size=7 maxlength=7> (Mo-So, X=Timer)</TD></TR>\n",
		 weekdays);

	request->SocketWrite("<TR><TD colspan=2 height=10></TR>\n");
	request->SocketWrite("<TR><TD align=\"center\"><INPUT TYPE=\"submit\" value=\"OK\"></form></TD>\n");
	request->SocketWrite("<TD align=\"center\"><form method=\"GET\" action=\"/fb/timer.dbox2\">\n");
	request->SocketWrite("<INPUT type=\"hidden\" name=\"action\" value=\"none\">\n");
	request->SocketWrite("<INPUT TYPE=\"submit\" value=\"CANCEL\"></form></TD>\n");
	request->SocketWrite("</TR></TABLE></TABLE>");
	request->SendHTMLFooter();
}

//-------------------------------------------------------------------------
void CWebAPI::doModifyTimer(CWebserverRequest *request)
{
	unsigned modyId = atoi(request->ParameterList["id"].c_str());
	CTimerd::responseGetTimer timer;
	Parent->Timerd->getTimer(timer, modyId);

	struct tm *alarmTime = localtime(&(timer.alarmTime));
	if(request->ParameterList["ad"] != "")
	{
		alarmTime->tm_mday = atoi(request->ParameterList["ad"].c_str());
	}
	if(request->ParameterList["amo"] != "")
	{
		alarmTime->tm_mon = atoi(request->ParameterList["amo"].c_str())-1;
	}
	if(request->ParameterList["ay"] != "")
	{
		alarmTime->tm_year = atoi(request->ParameterList["ay"].c_str())-1900;
	}
	if(request->ParameterList["ah"] != "")
	{
		alarmTime->tm_hour = atoi(request->ParameterList["ah"].c_str());
	}
	if(request->ParameterList["ami"] != "")
	{
		alarmTime->tm_min = atoi(request->ParameterList["ami"].c_str());
	}
	correctTime(alarmTime);
	time_t alarmTimeT = mktime(alarmTime);

	struct tm *stopTime = localtime(&(timer.stopTime));
	if(request->ParameterList["sd"] != "")
	{
		stopTime->tm_mday = atoi(request->ParameterList["sd"].c_str());
	}
	if(request->ParameterList["smo"] != "")
	{
		stopTime->tm_mon = atoi(request->ParameterList["smo"].c_str())-1;
	}
	if(request->ParameterList["sy"] != "")
	{
		stopTime->tm_year = atoi(request->ParameterList["sy"].c_str())-1900;
	}
	if(request->ParameterList["sh"] != "")
	{
		stopTime->tm_hour = atoi(request->ParameterList["sh"].c_str());
	}
	if(request->ParameterList["smi"] != "")
	{
		stopTime->tm_min = atoi(request->ParameterList["smi"].c_str());
	}
	correctTime(stopTime);
	time_t stopTimeT = mktime(stopTime);
	time_t announceTimeT = alarmTimeT-60;

	CTimerd::CTimerEventRepeat rep = 
	(CTimerd::CTimerEventRepeat) atoi(request->ParameterList["rep"].c_str());
	if(((int)rep) >= ((int)CTimerd::TIMERREPEAT_WEEKDAYS) && request->ParameterList["wd"] != "")
		Parent->Timerd->getWeekdaysFromStr((int*)&rep, request->ParameterList["wd"].c_str());
	Parent->Timerd->modifyTimerEvent(modyId, announceTimeT, alarmTimeT, stopTimeT, rep);
}

//-------------------------------------------------------------------------
void CWebAPI::newTimerForm(CWebserverRequest *request)
{
	request->SendHTMLHeader("NEUER TIMER");
	// Javascript
	request->SocketWrite("<script language =\"javascript\">\n");
	request->SocketWrite("function my_show(id) {document.getElementById(id).style.visibility=\"visible\";}\n");
	request->SocketWrite("function my_hide(id) {document.getElementById(id).style.visibility=\"hidden\";}\n");
	request->SocketWrite("function focusNMark() { document.NewTimerForm.ad.select();\n");
	request->SocketWrite("                        document.NewTimerForm.ad.focus();}\n");
	request->SocketWrite("function onEventChange() { tType=document.NewTimerForm.type.value;\n");
	request->printf("  if (tType == \"%d\") my_show(\"StopDateRow\"); else my_hide(\"StopDateRow\");\n",
		  (int)CTimerd::TIMER_RECORD);
	request->printf("  if (tType == \"%d\") my_show(\"StandbyRow\"); else my_hide(\"StandbyRow\");\n",
		  (int)CTimerd::TIMER_STANDBY);
	request->printf("  if (tType == \"%d\" || tType==\"%d\" || tType==\"%d\")\n",
		  (int)CTimerd::TIMER_RECORD, (int)CTimerd::TIMER_NEXTPROGRAM,
		  (int)CTimerd::TIMER_ZAPTO);
	request->SocketWrite("     my_show(\"ProgramRow\"); else my_hide(\"ProgramRow\");\n");
	request->printf("  if (tType == \"%d\") my_show(\"MessageRow\"); else my_hide(\"MessageRow\");\n",
		  (int)CTimerd::TIMER_REMIND);
	request->SocketWrite("  focusNMark();}\n");
	request->SocketWrite("function onEventChange2() { tType=document.NewTimerForm.rep.value;\n");
	request->printf("  if (tType == \"%d\") my_show(\"WeekdaysRow\"); else my_hide(\"WeekdaysRow\");\n",
		  (int)CTimerd::TIMERREPEAT_WEEKDAYS);
	request->SocketWrite("}\n");
	request->SocketWrite("</script>\n");
	// head of TABLE
	request->SocketWrite("<center><TABLE border=2 width=\"70%\"><tr CLASS=\"a\"><TD>\n");
	// Form
	request->SocketWrite("<form method=\"GET\" action=\"/fb/timer.dbox2\" name=\"NewTimerForm\">\n");
	request->SocketWrite("<INPUT TYPE=\"hidden\" name=\"action\" value=\"new\">\n");
	request->SocketWrite("<TABLE border=0 width=\"100%%\">\n");
	request->SocketWrite("<tr CLASS=\"c\"><TD colspan=\"2\" align=\"center\">NEUER TIMER</TD></TR>\n");
	// Timer type
	request->SocketWrite("<TR><TD align=\"center\">Timer-Typ\n");
	request->SocketWrite("<select name=\"type\" onchange=\"onEventChange();\">\n");
	for(int i=1; i<=7;i++)
	{
		if(i!=(int)CTimerd::TIMER_NEXTPROGRAM)
		{
			char zType[21];
			Parent->timerEventType2Str((CTimerd::CTimerEventTypes) i, zType, sizeof(zType)-1);
			request->printf("<option value=\"%d\">%s\n",i,zType);
		}
	}
	request->SocketWrite("</select>\n");
	// timer repeat
	request->SocketWrite("<TD align=\"center\">Wiederholung\n");
	request->SocketWrite("<select name=\"rep\" onchange=\"onEventChange2();\">\n");
	char zRep[21];
	for(int i=0; i<=6;i++)
	{
		if(i!=(int)CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION)
		{
			Parent->timerEventRepeat2Str((CTimerd::CTimerEventRepeat) i, zRep, sizeof(zRep)-1);
			request->printf("<option value=\"%d\">%s\n",i,zRep);
		}
	}
	Parent->timerEventRepeat2Str(CTimerd::TIMERREPEAT_WEEKDAYS, zRep, sizeof(zRep)-1);
	request->printf("<option value=\"%d\">%s\n",(int)CTimerd::TIMERREPEAT_WEEKDAYS, zRep);
	request->SocketWrite("</select>\n");
	
	time_t now_t = time(NULL);
	struct tm *now=localtime(&now_t);
	// alarm day
	request->SocketWrite("<TR><TD align=\"center\">\n<NOBR>");
	request->printf("Alarm-Datum <INPUT TYPE=\"text\" name=\"ad\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mday);
	// alarm month
	request->printf("<INPUT TYPE=\"text\" name=\"amo\" value=\"%02d\" size=2 maxlength=2>. \n",
		now->tm_mon+1);
	// alarm year
	request->printf("<INPUT TYPE=\"text\" name=\"ay\" value=\"%04d\" size=4 maxlength=4>\n",
		  now->tm_year+1900);
	// alarm time
	request->SocketWrite("</NOBR></TD><TD align=\"center\"><NOBR>\n");
	request->printf("Zeit <INPUT TYPE=\"text\" name=\"ah\" value=\"%02d\" size=2 maxlength=2> : \n",
		  now->tm_hour);
	request->printf("<INPUT TYPE=\"text\" name=\"ami\" value=\"%02d\" size=2 maxlength=2></NOBR></TD>\n",
		  now->tm_min);
	// stop day
	request->printf("</TR><tr id=\"StopDateRow\" style=\"visibility:hidden\"><TD align=\"center\"><NOBR>\n");
	request->printf("Stop-Datum <INPUT TYPE=\"text\" name=\"sd\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mday);
	// stop month
	request->printf("<INPUT TYPE=\"text\" name=\"smo\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mon+1);
	// stop year
	request->printf("<INPUT TYPE=\"text\" name=\"sy\" value=\"%04d\" size=4 maxlength=4>\n",
		  now->tm_year+1900);
	request->SocketWrite("</NOBR></TD><TD align=\"center\"><NOBR>\n");
	// stop time
	request->printf("Zeit <INPUT TYPE=\"text\" name=\"sh\" value=\"%02d\" size=2 maxlength=2> : \n",
		  now->tm_hour);
	request->printf("<INPUT TYPE=\"text\" name=\"smi\" value=\"%02d\" size=2 maxlength=2></NOBR></TD></TR>\n",
		  now->tm_min);
	// ONID-SID
	request->SocketWrite("<tr id=\"ProgramRow\" style=\"visibility:hidden\"><TD colspan=2>\n");
	request->SocketWrite("<select name=\"channel_id\">\n");
	CZapitClient::BouquetChannelList channellist;     
	channellist.clear();
	Parent->Zapit->getChannels(channellist,CZapitClient::MODE_TV);
	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
	for(; channel != channellist.end();channel++)
	{
		request->printf("<option value=\"T%u\"",channel->channel_id);
		if(channel->channel_id == current_channel)
			request->SocketWrite(" selected");
		request->printf(">%s\n",channel->name);
	}
	channellist.clear();
	Parent->Zapit->getChannels(channellist,CZapitClient::MODE_RADIO);
	channel = channellist.begin();
	for(; channel != channellist.end();channel++)
	{
		request->printf("<option value=\"R%u\"",channel->channel_id);
		if(channel->channel_id == current_channel)
			request->SocketWrite(" selected");
		request->printf(">%s\n",channel->name);
	}
	request->SocketWrite("</selected></TR>\n");
	//standby
	request->SocketWrite("<tr><TD id=\"StandbyRow\" style=\"visibility:hidden\">\n");
	request->SocketWrite("Standby <INPUT TYPE=\"radio\" name=\"sbon\" value=\"1\">An\n");
	request->SocketWrite("<INPUT TYPE=\"radio\" name=\"sbon\" value=\"0\" checked>Aus</TD>\n");
	// weekdays
	request->SocketWrite("<TD id=\"WeekdaysRow\" style=\"visibility:hidden\" align=\"center\">\n");
	request->SocketWrite("Wochentage <INPUT TYPE=\"text\" name=\"wd\" value=\"-------\" size=7 maxlength=7> (Mo-So,X=Ja)</TD></TR>\n");
	//message
	request->SocketWrite("<tr id=\"MessageRow\" style=\"visibility:hidden\"><TD colspan=2>\n");
	request->printf("Nachricht <INPUT TYPE=\"text\" name=\"msg\" value=\"\" size=20 maxlength=%d> ('/'=NL)\n",REMINDER_MESSAGE_MAXLEN-1);
	request->SocketWrite("</TD></TR>\n");
	// Buttons
	request->SocketWrite("<TD align=\"center\"><INPUT TYPE=\"submit\" value=\"OK\"></form>\n");
	request->SocketWrite("<TD align=\"center\"><form method=\"GET\" action=\"/fb/timer.dbox2\">\n");
	request->SocketWrite("<INPUT TYPE=\"hidden\" NAME=\"action\" VALUE=\"none\">\n");
	request->SocketWrite("<INPUT TYPE=\"submit\" value=\"CANCEL\"></form></TD>\n");
	request->SocketWrite("</TABLE></TABLE>\n");
	request->SendHTMLFooter();
}

//-------------------------------------------------------------------------
void CWebAPI::doNewTimer(CWebserverRequest *request)
{
time_t	announceTimeT = 0,
		stopTimeT = 0,
		alarmTimeT = 0;

	if(request->ParameterList["alarm"] != "")		// wenn alarm angegeben dann parameter im time_t format
	{
		alarmTimeT = atoi(request->ParameterList["alarm"].c_str());
		if(request->ParameterList["stop"] != "")
			stopTimeT = atoi(request->ParameterList["stop"].c_str());
		if(request->ParameterList["announce"] != "")
			announceTimeT = atoi(request->ParameterList["announce"].c_str());
	}
	else			// sonst formular-parameter parsen
	{
		time_t now = time(NULL);
		struct tm *alarmTime=localtime(&now);
		if(request->ParameterList["ad"] != "")
		{
			alarmTime->tm_mday = atoi(request->ParameterList["ad"].c_str());
		}
		if(request->ParameterList["amo"] != "")
		{
			alarmTime->tm_mon = atoi(request->ParameterList["amo"].c_str())-1;
		}
		if(request->ParameterList["ay"] != "")
		{
			alarmTime->tm_year = atoi(request->ParameterList["ay"].c_str())-1900;
		}
		if(request->ParameterList["ah"] != "")
		{
			alarmTime->tm_hour = atoi(request->ParameterList["ah"].c_str());
		}
		if(request->ParameterList["ami"] != "")
		{
			alarmTime->tm_min = atoi(request->ParameterList["ami"].c_str());
		}
		correctTime(alarmTime);
		alarmTimeT = mktime(alarmTime);

		struct tm *stopTime = alarmTime;
		if(request->ParameterList["sd"] != "")
		{
		  stopTime->tm_mday = atoi(request->ParameterList["sd"].c_str());
		}
		if(request->ParameterList["smo"] != "")
		{
		  stopTime->tm_mon = atoi(request->ParameterList["smo"].c_str())-1;
		}
		if(request->ParameterList["sy"] != "")
		{
		  stopTime->tm_year = atoi(request->ParameterList["sy"].c_str())-1900;
		}
		if(request->ParameterList["sh"] != "")
		{
		  stopTime->tm_hour = atoi(request->ParameterList["sh"].c_str());
		}
		if(request->ParameterList["smi"] != "")
		{
		  stopTime->tm_min = atoi(request->ParameterList["smi"].c_str());
		}
		correctTime(alarmTime);
		stopTimeT = mktime(stopTime);
	}
		
	announceTimeT = alarmTimeT-60;
	CTimerd::CTimerEventTypes type  = 
	(CTimerd::CTimerEventTypes) atoi(request->ParameterList["type"].c_str());
	CTimerd::CTimerEventRepeat rep = 
	(CTimerd::CTimerEventRepeat) atoi(request->ParameterList["rep"].c_str());
	if(((int)rep) >= ((int)CTimerd::TIMERREPEAT_WEEKDAYS) && request->ParameterList["wd"] != "")
		Parent->Timerd->getWeekdaysFromStr((int*)&rep, request->ParameterList["wd"].c_str());
	bool standby_on = (request->ParameterList["sbon"]=="1");
	CTimerd::EventInfo eventinfo;
	eventinfo.epgID      = 0;
	if(request->ParameterList["channel_id"].substr(0,1)=="R")
		eventinfo.mode = CTimerd::MODE_RADIO;
	else
		eventinfo.mode = CTimerd::MODE_TV;
	sscanf(request->ParameterList["channel_id"].substr(1).c_str(),"%u",&eventinfo.channel_id);
	void *data=NULL;
	if(type == CTimerd::TIMER_STANDBY)
		data=&standby_on;
	else if(type==CTimerd::TIMER_NEXTPROGRAM || type==CTimerd::TIMER_ZAPTO ||
			type==CTimerd::TIMER_RECORD)
		data= &eventinfo;
	else if(type==CTimerd::TIMER_REMIND)
	{
		char msg[REMINDER_MESSAGE_MAXLEN];
		memset(msg, 0, sizeof(msg));
		strncpy(msg, request->ParameterList["msg"].c_str(),REMINDER_MESSAGE_MAXLEN-1);
		data=msg;
	}
	Parent->Timerd->addTimerEvent(type,data,announceTimeT,alarmTimeT,stopTimeT,rep);
}


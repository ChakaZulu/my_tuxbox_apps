/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#include <system/setting_helpers.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "libnet.h"

#include <libucodes.h>

#include <config.h>

#include <global.h>
#include <neutrino.h>

#include <gui/streaminfo.h>

#include <gui/widget/messagebox.h>

#include <gui/gamelist.h>
#include <daemonc/remotecontrol.h>
extern CPlugins       * g_PluginList;    /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

extern "C" int pinghost( const char *hostname );

bool CSatDiseqcNotifier::changeNotify(const std::string & OptionName, void* Data)
{
	if (*((int*) Data) == NO_DISEQC)
	{
		satMenu->setActive(true);
		extMenu->setActive(false);
		extMotorMenu->setActive(false);
		repeatMenu->setActive(false);
	}
	else
	if (*((int*) Data) == DISEQC_1_2)
	{
		satMenu->setActive(true);
		extMenu->setActive(true);
		extMotorMenu->setActive(true);
		repeatMenu->setActive(true);
	}
	else
	{
		satMenu->setActive(false);
		extMenu->setActive(true);
		extMotorMenu->setActive(false);
		repeatMenu->setActive((*((int*) Data) != DISEQC_1_0));
	}
	return true;
}

CDHCPNotifier::CDHCPNotifier( CMenuForwarder* a1, CMenuForwarder* a2, CMenuForwarder* a3, CMenuForwarder* a4, CMenuForwarder* a5)
{
	toDisable[0] = a1;
	toDisable[1] = a2;
	toDisable[2] = a3;
	toDisable[3] = a4;
	toDisable[4] = a5;
}


bool CDHCPNotifier::changeNotify(const std::string & OptionName, void* data)
{
	CNeutrinoApp::getInstance()->networkConfig.inet_static = ((*(int*)(data)) == 0);
	for(int x=0;x<5;x++)
		toDisable[x]->setActive(CNeutrinoApp::getInstance()->networkConfig.inet_static);
	return true;
}

CStreamingNotifier::CStreamingNotifier( CMenuItem* i1, CMenuItem* i2, CMenuItem* i3, CMenuItem* i4, CMenuItem* i5, CMenuItem* i6, CMenuItem* i7, CMenuItem* i8, CMenuItem* i9, CMenuItem* i10)
{
   toDisable[0]=i1;
   toDisable[1]=i2;
   toDisable[2]=i3;
   toDisable[3]=i4;
   toDisable[4]=i5;
   toDisable[5]=i6;
   toDisable[6]=i7;
   toDisable[7]=i8;
   toDisable[8]=i9;
   toDisable[9]=i10;

}

bool CStreamingNotifier::changeNotify(const std::string & OptionName, void*)
{
   if(g_settings.streaming_type==0)
   {
      for (int i=0; i<=9; i++)
        toDisable[i]->setActive(false);
   }
   else if(g_settings.streaming_type==1)
   {
      for (int i=0; i<=9; i++)
        toDisable[i]->setActive(true);
   }
   return true;
}

CRecordingNotifier::CRecordingNotifier( CMenuItem* i1, CMenuItem* i2, CMenuItem* i3,
                                        CMenuItem* i4, CMenuItem* i5, CMenuItem* i6,
                                        CMenuItem* i7)
{
   toDisable[0]=i1;
   toDisable[1]=i2;
   toDisable[2]=i3;
   toDisable[3]=i4;
   toDisable[4]=i5;
   toDisable[5]=i6;
   toDisable[6]=i7;
}
bool CRecordingNotifier::changeNotify(const std::string & OptionName, void*)
{
   if(g_settings.recording_type==0)
   {
      for(int i=0;i<7;i++)
         toDisable[i]->setActive(false);
   }
   else if(g_settings.recording_type==1)
   {
      toDisable[0]->setActive(true);
      toDisable[1]->setActive(true);
      toDisable[2]->setActive(true);
      toDisable[3]->setActive(g_settings.recording_server_wakeup==1);
      toDisable[4]->setActive(true);
      toDisable[5]->setActive(true);
      toDisable[6]->setActive(false);
   }
   else if(g_settings.recording_type==2)
   {
      toDisable[0]->setActive(false);
      toDisable[1]->setActive(false);
      toDisable[2]->setActive(false);
      toDisable[3]->setActive(false);
      toDisable[4]->setActive(false);
      toDisable[5]->setActive(false);
      toDisable[6]->setActive(true);
   }
   return true;
}

CRecordingNotifier2::CRecordingNotifier2( CMenuItem* i1)
{
   toDisable[0]=i1;
}
bool CRecordingNotifier2::changeNotify(const std::string & OptionName, void*)
{
   toDisable[0]->setActive(g_settings.recording_server_wakeup==1);
   return true;
}

CMiscNotifier::CMiscNotifier( CMenuItem* i1)
{
   toDisable[0]=i1;
}
bool CMiscNotifier::changeNotify( const std::string & OptionName, void* )
{
   toDisable[0]->setActive(!g_settings.shutdown_real);
   return true;
}

bool CConsoleDestChangeNotifier::changeNotify(const std::string & OptionName, void *Data)
{
	g_settings.uboot_console = *(int *)Data;

	return true;
}

CLcdNotifier::CLcdNotifier(int *lcdPowerSetting, int *lcdInverseSetting, int *lcdAutoDimmSetting)
{
	LcdPowerSetting    = lcdPowerSetting;
	LcdInverseSetting  = lcdInverseSetting;
	LcdAutoDimmSetting = lcdAutoDimmSetting;
}

bool CLcdNotifier::changeNotify(const std::string & OptionName, void *Data)
{
	CLCD::getInstance()->setPower(*LcdPowerSetting == 1);
	CLCD::getInstance()->setInverse(*LcdInverseSetting == 1);
	CLCD::getInstance()->setAutoDimm(*LcdAutoDimmSetting == 1);
	//CLCD::getInstance()->update();
	return true;
}

/*
bool CCableSpectalInversionNotifier::changeNotify(const std::string & OptionName, void* Data)
{
	static bool messageShowed = false;
	
	if (!messageShowed)
	{
		ShowMsgUTF("messagebox.info", g_Locale->getText("cablesetup.spectralInversionWarning"), CMessageBox::mbrYes, CMessageBox::mbYes, "info.raw"); // UTF-8
		messageShowed = true;
	}

	if( *((int*) Data)!=0)
	{	//file anlegen (direktstart)
		FILE* fd = fopen("/var/etc/.specinv", "w");
		if(fd)
		{
			fclose(fd);
		}
		else
			return false;
	}
	else
	{
		remove("/var/etc/.specinv");
	}
	return true;
}
*/
bool CPauseSectionsdNotifier::changeNotify(const std::string & OptionName, void* Data)
{
	g_Sectionsd->setPauseScanning((*((int *)Data)) == 0);

	return true;
}

bool CShowBootInfoNotifier::changeNotify(const std::string & OptionName, void* Data)
{
	if( *((int*) Data)!=0)
	{	//file anlegen (direktstart)
		FILE* fd = fopen("/var/etc/.cdkVcInfo", "w");
		if(fd)
		{
			fclose(fd);
		}
		else
			return false;
	}
	else
	{
		remove("/var/etc/.cdkVcInfo");
	}
	return true;
}

bool CBHDriverNotifier::changeNotify(const std::string & OptionName, void* Data)
{
	if( *((int*) Data)!=0)
	{	//file anlegen (BH Treiber laden)
		FILE* fd = fopen("/var/etc/.bh", "w");
		if(fd)
		{
			fclose(fd);
		}
		else
			return false;
	}
	else
	{
		remove("/var/etc/.bh");
	}
	return true;
}

bool CIPChangeNotifier::changeNotify(const std::string & OptionName, void* Data)
{
	char ip[16];
	unsigned char _ip[4];
	sscanf((char*) Data, "%hhu.%hhu.%hhu.%hhu", &_ip[0], &_ip[1], &_ip[2], &_ip[3]);

	sprintf(ip, "%hhu.%hhu.%hhu.255", _ip[0], _ip[1], _ip[2]);
	CNeutrinoApp::getInstance()->networkConfig.broadcast = ip;

	CNeutrinoApp::getInstance()->networkConfig.netmask = (_ip[0] == 10) ? "255.0.0.0" : "255.255.255.0";

	return true;
}

bool CColorSetupNotifier::changeNotify(const std::string & OptionName, void*)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
//	unsigned char r,g,b;
	//setting colors-..
	frameBuffer->paletteGenFade(COL_MENUHEAD,
	                              convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue),
	                              convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue),
	                              8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ) );

	frameBuffer->paletteGenFade(COL_MENUCONTENT,
	                              convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );


	frameBuffer->paletteGenFade(COL_MENUCONTENTDARK,
	                              convertSetupColor2RGB(int(g_settings.menu_Content_red*0.6), int(g_settings.menu_Content_green*0.6), int(g_settings.menu_Content_blue*0.6)),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	frameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED,
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );

	frameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE,
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );

	frameBuffer->paletteGenFade(COL_INFOBAR,
	                              convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

/*	frameBuffer->paletteSetColor( COL_INFOBAR_SHADOW,
	                                convertSetupColor2RGB(
	                                    int(g_settings.infobar_red*0.4),
	                                    int(g_settings.infobar_green*0.4),
	                                    int(g_settings.infobar_blue*0.4)),
	                                g_settings.infobar_alpha);
*/
	frameBuffer->paletteGenFade(COL_INFOBAR_SHADOW,
	                              convertSetupColor2RGB(int(g_settings.infobar_red*0.4), int(g_settings.infobar_green*0.4), int(g_settings.infobar_blue*0.4)),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );


	frameBuffer->paletteSet();
	return false;
}

bool CAudioSetupNotifier::changeNotify(const std::string & OptionName, void*)
{
	//printf("notify: %s\n", OptionName.c_str() );

	if(OptionName=="audiomenu.PCMOffset")
	{
		if (g_settings.audio_avs_Control == 2)   //lirc
			g_Controld->setVolume(100 - atoi(g_settings.audio_PCMOffset), 0);

		return true;
	}

	if(OptionName=="audiomenu.analogout")
	{
		g_Zapit->setAudioMode(g_settings.audio_AnalogMode);
		return true;
	}
}

CAudioSetupNotifier2::CAudioSetupNotifier2( CMenuItem* i1)
{
   toDisable[0]=i1;
}

bool CAudioSetupNotifier2::changeNotify(const std::string & OptionName, void*)
{
	toDisable[0]->setActive(g_settings.audio_avs_Control == 2);

	if (g_settings.audio_avs_Control == 2)   
		g_Controld->setVolume(100 - atoi(g_settings.audio_PCMOffset), 0);
	else
		g_Controld->setVolume(100, g_settings.audio_avs_Control);
 
	return true;

}

bool CVideoSetupNotifier::changeNotify(const std::string & OptionName, void*)
{
	if(OptionName=="videomenu.videosignal")
	{
		g_Controld->setVideoOutput( g_settings.video_Signal );
	}
	else if(OptionName=="videomenu.videoformat")
	{
		g_Controld->setVideoFormat( g_settings.video_Format );
	}

	printf("video notify: %s\n", OptionName.c_str() );
	return false;
}

bool CLanguageSetupNotifier::changeNotify(const std::string & OptionName, void*)
{
	//	printf("language notify: %s - %s\n", OptionName.c_str(), g_settings.language );
	g_Locale->loadLocale(g_settings.language);
	return true;
}

bool CKeySetupNotifier::changeNotify(const std::string & OptionName, void*)
{
	//    printf("CKeySetupNotifier notify: %s\n", OptionName.c_str() );
	g_RCInput->repeat_block = atoi(g_settings.repeat_blocker)* 1000;
	g_RCInput->repeat_block_generic = atoi(g_settings.repeat_genericblocker)* 1000;
	return false;
}

int CAPIDChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//    printf("CAPIDChangeExec exec: %s\n", actionKey.c_str());
	unsigned int sel= atoi(actionKey.c_str());
	if (g_RemoteControl->current_PIDs.PIDs.selected_apid!= sel )
	{
		g_RemoteControl->setAPID(sel);
	}
	return menu_return::RETURN_EXIT;
}


int CNVODChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//    printf("CNVODChangeExec exec: %s\n", actionKey.c_str());
	unsigned sel= atoi(actionKey.c_str());
	g_RemoteControl->setSubChannel(sel);

	parent->hide();
	g_InfoViewer->showSubchan();
	return menu_return::RETURN_EXIT;
}

int CStreamFeaturesChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//printf("CStreamFeaturesChangeExec exec: %s\n", actionKey.c_str());
	int sel= atoi(actionKey.c_str());

	parent->hide();
	if (sel==-1)
	{
		CStreamInfo StreamInfo;
		StreamInfo.exec(NULL, "");
	}
	else if (sel>=0)
	{
		g_PluginList->setvtxtpid( g_RemoteControl->current_PIDs.PIDs.vtxtpid );
		g_PluginList->startPlugin( sel );
	}

	return menu_return::RETURN_EXIT;
}

int CUCodeCheckExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	std::string text;
	char res[60];

	text = g_Locale->getText("ucodecheck.avia500");
	text += ": ";
	checkFile(UCODEDIR "/avia500.ux", (char*) &res);
	text += res;
	text += '\n';
	text += g_Locale->getText("ucodecheck.avia600");
	text += ": ";
	checkFile(UCODEDIR "/avia600.ux", (char*) &res);
	text += res;
	text += '\n';
	text += g_Locale->getText("ucodecheck.ucode");
	text += ": ";
	checkFile(UCODEDIR "/ucode.bin", (char*) &res);
	if (strcmp("not found", res) == 0)
		text += "ucode_0014 (built-in)";
	else
		text += res;
	text += '\n';
	text += g_Locale->getText("ucodecheck.cam-alpha");
	text += ": ";
	checkFile(UCODEDIR "/cam-alpha.bin", (char*) &res);
	text += res;

	ShowMsgUTF("ucodecheck.head", text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
	return 1;
}

const char * mypinghost(const char * const host)
{
	int retvalue = pinghost(host);
	switch (retvalue)
	{
		case 1: return (g_Locale->getText("ping.ok"));
		case 0: return (g_Locale->getText("ping.unreachable"));
		case -1: return (g_Locale->getText("ping.protocol"));
		case -2: return (g_Locale->getText("ping.socket"));
	}
	return "";
}

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool dhcp)
{
	char our_ip[16];
	char our_mask[16];
	char our_broadcast[16];
	char our_gateway[16];
	char our_nameserver[16];
	std::string text;

	if (!dhcp) {
		strcpy(our_ip,ip);
		strcpy(our_mask,netmask);
		strcpy(our_broadcast,broadcast);
		strcpy(our_gateway,gateway);
		strcpy(our_nameserver,nameserver);
	}
	else {
		netGetIP("eth0",our_ip,our_mask,our_broadcast);
		netGetDefaultRoute(our_gateway);
		netGetNameserver(our_nameserver);
	}
		
	printf("testNw IP       : %s\n", our_ip);
	printf("testNw Netmask  : %s\n", our_mask);
	printf("testNw Broadcast: %s\n", our_broadcast);
	printf("testNw Gateway: %s\n", our_gateway);
	printf("testNw Nameserver: %s\n", our_nameserver);

	text = our_ip;
	text += ": ";
	text += mypinghost(our_ip);
	text += '\n';
	text += g_Locale->getText("networkmenu.gateway");
	text += ": ";
	text += our_gateway;
	text += ' ';
	text += mypinghost(our_gateway);
	text += '\n';
	text += g_Locale->getText("networkmenu.nameserver");
	text += ": ";
	text += our_nameserver;
	text += ' ';
	text += mypinghost(our_nameserver);
	text += "\ndboxupdate.berlios.de: ";
	text += mypinghost("195.37.77.138");

	ShowMsgUTF("networkmenu.test", text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

void showCurrentNetworkSettings()
{
	char ip[16];
	char mask[16];
	char broadcast[16];
	char router[16];
	char nameserver[16];
	std::string text;

	netGetIP("eth0",ip,mask,broadcast);
	if (ip[0] == 0) {
		text = "Network inactive\n";
	}
	else {
		netGetNameserver(nameserver);
		netGetDefaultRoute(router);
		text  = g_Locale->getText("networkmenu.ipaddress");
		text += ": ";
		text += ip;
		text += '\n';
		text += g_Locale->getText("networkmenu.netmask");
		text += ": ";
		text += mask;
		text += '\n';
		text += g_Locale->getText("networkmenu.broadcast");
		text += ": ";
		text += broadcast;
		text += '\n';
		text += g_Locale->getText("networkmenu.nameserver");
		text += ": ";
		text += nameserver;
		text += '\n';
		text += g_Locale->getText("networkmenu.gateway");
		text += ": ";
		text += router;
	}
	ShowMsgUTF("networkmenu.show", text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

unsigned long long getcurrenttime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
}

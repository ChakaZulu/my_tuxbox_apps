/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
							 and some other guys
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#include <config.h>

#define NEUTRINO_CPP

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>


#include "global.h"
#include "neutrino.h"

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/vcrcontrol.h>
#include <driver/irsend.h>

#include "gui/widget/colorchooser.h"
#include "gui/widget/menue.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/icons.h"
#include "gui/widget/lcdcontroler.h"
#include "gui/widget/keychooser.h"
#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"

#include "gui/color.h"

#include "gui/bedit/bouqueteditor_bouquets.h"
#include <gui/bouquetlist.h>
#include "gui/eventlist.h"
#include "gui/channellist.h"
#include "gui/screensetup.h"
#include "gui/gamelist.h"
#include "gui/infoviewer.h"
#include "gui/epgview.h"
#include "gui/update.h"
#include "gui/scan.h"
#include "gui/favorites.h"
#include "gui/sleeptimer.h"
#include "gui/dboxinfo.h"
#include "gui/timerlist.h"
#include "gui/alphasetup.h"
#include "gui/mp3player.h"

#if HAVE_DVB_API_VERSION >= 3
#include "gui/movieplayer.h"
#endif

#include "gui/nfs.h"
#include "gui/pictureviewer.h"
#include "gui/motorcontrol.h"

#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>

#include <string.h>


/* the following generic menu items are integrated into multiple menus at the same time */
CMenuSeparator GenericMenuSeparator;
CMenuSeparator GenericMenuSeparatorLine(CMenuSeparator::LINE);
CMenuForwarder GenericMenuBack("menu.back");


CBouquetList   * bouquetList;
CPlugins       * g_PluginList;
CRemoteControl * g_RemoteControl;

// Globale Variablen - to use import global.h

// I don't like globals, I would have hidden them in classes,
// but if you wanna do it so... ;)

CZapitClient::SatelliteList satList;
CZapitClient::SatelliteList::iterator satList_it;

static void initGlobals(void)
{
	g_fontRenderer  = NULL;
	g_Fonts         = NULL;

	g_RCInput       = NULL;
	g_Controld      = new CControldClient;
	g_Timerd        = NULL;
	g_Zapit         = new CZapitClient;
	g_RemoteControl = NULL;

	g_EpgData       = NULL;
	g_InfoViewer    = NULL;
	g_EventList     = NULL;

	g_Locale        = NULL;
	g_PluginList    = NULL;
}



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                                                     +
+          CNeutrinoApp - Constructor, initialize g_fontRenderer                      +
+                                                                                     +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
CNeutrinoApp::CNeutrinoApp()
: configfile('\t')
{
	frameBuffer = CFrameBuffer::getInstance();
	frameBuffer->setIconBasePath(DATADIR "/neutrino/icons/");

	g_fontRenderer = new FBFontRenderClass();
	SetupFrameBuffer();

	settingsFile = CONFIGDIR "/neutrino.conf";
	scanSettingsFile = CONFIGDIR "/scan.conf";

	mode = mode_unknown;
	channelList = NULL;
	bouquetList = NULL;
	skipShutdownTimer=false;
}

/*-------------------------------------------------------------------------------------
-                                                                                     -
-           CNeutrinoApp - Destructor                                                 -
-                                                                                     -
-------------------------------------------------------------------------------------*/
CNeutrinoApp::~CNeutrinoApp()
{
	if(channelList)
		delete channelList;
}

CNeutrinoApp* CNeutrinoApp::getInstance()
{
	static CNeutrinoApp* neutrinoApp = NULL;

	if(!neutrinoApp)
	{
		neutrinoApp = new CNeutrinoApp();
		dprintf(DEBUG_DEBUG, "NeutrinoApp Instance created\n");
	}
	return neutrinoApp;
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (Neutrino)                               *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_neutrino()
{
	g_settings.menu_Head_alpha = 0x00;
	g_settings.menu_Head_red   = 0x00;
	g_settings.menu_Head_green = 0x0A;
	g_settings.menu_Head_blue  = 0x19;

	g_settings.menu_Head_Text_alpha = 0x00;
	g_settings.menu_Head_Text_red   = 0x5f;
	g_settings.menu_Head_Text_green = 0x46;
	g_settings.menu_Head_Text_blue  = 0x00;

	g_settings.menu_Content_alpha = 0x14;
	g_settings.menu_Content_red   = 0x00;
	g_settings.menu_Content_green = 0x0f;
	g_settings.menu_Content_blue  = 0x23;

	g_settings.menu_Content_Text_alpha = 0x00;
	g_settings.menu_Content_Text_red   = 0x64;
	g_settings.menu_Content_Text_green = 0x64;
	g_settings.menu_Content_Text_blue  = 0x64;

	g_settings.menu_Content_Selected_alpha = 0x14;
	g_settings.menu_Content_Selected_red   = 0x19;
	g_settings.menu_Content_Selected_green = 0x37;
	g_settings.menu_Content_Selected_blue  = 0x64;

	g_settings.menu_Content_Selected_Text_alpha  = 0x00;
	g_settings.menu_Content_Selected_Text_red    = 0x00;
	g_settings.menu_Content_Selected_Text_green  = 0x00;
	g_settings.menu_Content_Selected_Text_blue   = 0x00;

	g_settings.menu_Content_inactive_alpha = 0x14;
	g_settings.menu_Content_inactive_red   = 0x00;
	g_settings.menu_Content_inactive_green = 0x0f;
	g_settings.menu_Content_inactive_blue  = 0x23;

	g_settings.menu_Content_inactive_Text_alpha  = 0x00;
	g_settings.menu_Content_inactive_Text_red    = 55;
	g_settings.menu_Content_inactive_Text_green  = 70;
	g_settings.menu_Content_inactive_Text_blue   = 85;

	g_settings.infobar_alpha = 0x14;
	g_settings.infobar_red   = 0x00;
	g_settings.infobar_green = 0x0e;
	g_settings.infobar_blue  = 0x23;

	g_settings.infobar_Text_alpha = 0x00;
	g_settings.infobar_Text_red   = 0x64;
	g_settings.infobar_Text_green = 0x64;
	g_settings.infobar_Text_blue  = 0x64;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (classic)                                *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_classic()
{
	g_settings.menu_Head_alpha = 20;
	g_settings.menu_Head_red   =  5;
	g_settings.menu_Head_green = 10;
	g_settings.menu_Head_blue  = 60;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 100;

	g_settings.menu_Content_alpha = 20;
	g_settings.menu_Content_red   = 50;
	g_settings.menu_Content_green = 50;
	g_settings.menu_Content_blue  = 50;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 20;
	g_settings.menu_Content_Selected_red   = 5;
	g_settings.menu_Content_Selected_green = 10;
	g_settings.menu_Content_Selected_blue  = 60;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 100;
	g_settings.menu_Content_Selected_Text_green  = 100;
	g_settings.menu_Content_Selected_Text_blue   = 100;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 50;
	g_settings.menu_Content_inactive_green = 50;
	g_settings.menu_Content_inactive_blue  = 50;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 80;
	g_settings.menu_Content_inactive_Text_green  = 80;
	g_settings.menu_Content_inactive_Text_blue   = 80;

	g_settings.infobar_alpha = 20;
	g_settings.infobar_red   = 5;
	g_settings.infobar_green = 10;
	g_settings.infobar_blue  = 60;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::loadSetup()
{
	int erg = 0;

	//settings laden - und dabei Defaults setzen!
	if(!configfile.loadConfig(settingsFile))
	{
		//file existiert nicht
		erg = 1;
	}

	//video
	g_settings.video_Signal = configfile.getInt32( "video_Signal", 1 ); //RGB + CVBS
	g_settings.video_Format = configfile.getInt32( "video_Format", 2 ); //4:3

	//fb-alphawerte f�r gtx
	g_settings.gtx_alpha1 = configfile.getInt32( "gtx_alpha1", 0);
	g_settings.gtx_alpha2 = configfile.getInt32( "gtx_alpha2", 1);

	//misc
	g_settings.shutdown_real = configfile.getInt32( "shutdown_real", true );
	g_settings.shutdown_real_rcdelay = configfile.getInt32( "shutdown_real_rcdelay", true );
	g_settings.shutdown_showclock = configfile.getInt32( "shutdown_showclock", 1 );
	g_settings.show_camwarning = configfile.getInt32( "show_camwarning", 1 );
	strcpy(g_settings.record_safety_time_before, configfile.getString( "record_safety_time_before", "00").c_str());
	strcpy(g_settings.record_safety_time_after, configfile.getString( "record_safety_time_after", "00").c_str());
	g_settings.infobar_sat_display = configfile.getInt32( "infobar_sat_display", true );

	//audio
	g_settings.audio_AnalogMode = configfile.getInt32( "audio_AnalogMode", 0 );
	g_settings.audio_DolbyDigital = configfile.getInt32( "audio_DolbyDigital", 0 );
	g_settings.audio_avs_Control = configfile.getInt32( "audio_avs_Control", true );


	//vcr
	g_settings.vcr_AutoSwitch = configfile.getInt32( "vcr_AutoSwitch", 1 );

	//language
	strcpy( g_settings.language, configfile.getString( "language", "deutsch" ).c_str() );

	//widget settings
	g_settings.widget_fade = configfile.getInt32( "widget_fade", 1 );

	//colors (neutrino defaultcolors)
	g_settings.menu_Head_alpha = configfile.getInt32( "menu_Head_alpha", 0x00 );
	g_settings.menu_Head_red = configfile.getInt32( "menu_Head_red", 0x00 );
	g_settings.menu_Head_green = configfile.getInt32( "menu_Head_green", 0x0A );
	g_settings.menu_Head_blue = configfile.getInt32( "menu_Head_blue", 0x19 );

	g_settings.menu_Head_Text_alpha = configfile.getInt32( "menu_Head_Text_alpha", 0x00 );
	g_settings.menu_Head_Text_red = configfile.getInt32( "menu_Head_Text_red", 0x5f );
	g_settings.menu_Head_Text_green = configfile.getInt32( "menu_Head_Text_green", 0x46 );
	g_settings.menu_Head_Text_blue = configfile.getInt32( "menu_Head_Text_blue", 0x00 );

	g_settings.menu_Content_alpha = configfile.getInt32( "menu_Content_alpha", 0x14 );
	g_settings.menu_Content_red = configfile.getInt32( "menu_Content_red", 0x00 );
	g_settings.menu_Content_green = configfile.getInt32( "menu_Content_green", 0x0f );
	g_settings.menu_Content_blue = configfile.getInt32( "menu_Content_blue", 0x23 );

	g_settings.menu_Content_Text_alpha = configfile.getInt32( "menu_Content_Text_alpha", 0x00 );
	g_settings.menu_Content_Text_red = configfile.getInt32( "menu_Content_Text_red", 0x64 );
	g_settings.menu_Content_Text_green = configfile.getInt32( "menu_Content_Text_green", 0x64 );
	g_settings.menu_Content_Text_blue = configfile.getInt32( "menu_Content_Text_blue", 0x64 );

	g_settings.menu_Content_Selected_alpha = configfile.getInt32( "menu_Content_Selected_alpha", 0x14 );
	g_settings.menu_Content_Selected_red = configfile.getInt32( "menu_Content_Selected_red", 0x19 );
	g_settings.menu_Content_Selected_green = configfile.getInt32( "menu_Content_Selected_green", 0x37 );
	g_settings.menu_Content_Selected_blue = configfile.getInt32( "menu_Content_Selected_blue", 0x64 );

	g_settings.menu_Content_Selected_Text_alpha = configfile.getInt32( "menu_Content_Selected_Text_alpha", 0x00 );
	g_settings.menu_Content_Selected_Text_red = configfile.getInt32( "menu_Content_Selected_Text_red", 0x00 );
	g_settings.menu_Content_Selected_Text_green = configfile.getInt32( "menu_Content_Selected_Text_green", 0x00 );
	g_settings.menu_Content_Selected_Text_blue = configfile.getInt32( "menu_Content_Selected_Text_blue", 0x00 );

	g_settings.menu_Content_inactive_alpha = configfile.getInt32( "menu_Content_inactive_alpha", 0x14 );
	g_settings.menu_Content_inactive_red = configfile.getInt32( "menu_Content_inactive_red", 0x00 );
	g_settings.menu_Content_inactive_green = configfile.getInt32( "menu_Content_inactive_green", 0x0f );
	g_settings.menu_Content_inactive_blue = configfile.getInt32( "menu_Content_inactive_blue", 0x23 );

	g_settings.menu_Content_inactive_Text_alpha = configfile.getInt32( "menu_Content_inactive_Text_alpha", 0x00 );
	g_settings.menu_Content_inactive_Text_red = configfile.getInt32( "menu_Content_inactive_Text_red", 55 );
	g_settings.menu_Content_inactive_Text_green = configfile.getInt32( "menu_Content_inactive_Text_green", 70 );
	g_settings.menu_Content_inactive_Text_blue = configfile.getInt32( "menu_Content_inactive_Text_blue", 85 );

	g_settings.infobar_alpha = configfile.getInt32( "infobar_alpha", 0x14 );
	g_settings.infobar_red = configfile.getInt32( "infobar_red", 0x00 );
	g_settings.infobar_green = configfile.getInt32( "infobar_green", 0x0e );
	g_settings.infobar_blue = configfile.getInt32( "infobar_blue", 0x23 );

	g_settings.infobar_Text_alpha = configfile.getInt32( "infobar_Text_alpha", 0x00 );
	g_settings.infobar_Text_red = configfile.getInt32( "infobar_Text_red", 0x64 );
	g_settings.infobar_Text_green = configfile.getInt32( "infobar_Text_green", 0x64 );
	g_settings.infobar_Text_blue = configfile.getInt32( "infobar_Text_blue", 0x64 );

	//network
	g_settings.network_nfs_ip[0] = configfile.getString("network_nfs_ip_1", "");
	g_settings.network_nfs_ip[1] = configfile.getString("network_nfs_ip_2", "");
	g_settings.network_nfs_ip[2] = configfile.getString("network_nfs_ip_3", "");
	g_settings.network_nfs_ip[3] = configfile.getString("network_nfs_ip_4", "");
	strcpy( g_settings.network_nfs_dir[0], configfile.getString( "network_nfs_dir_1", "" ).c_str() );
	strcpy( g_settings.network_nfs_dir[1], configfile.getString( "network_nfs_dir_2", "" ).c_str() );
	strcpy( g_settings.network_nfs_dir[2], configfile.getString( "network_nfs_dir_3", "" ).c_str() );
	strcpy( g_settings.network_nfs_dir[3], configfile.getString( "network_nfs_dir_4", "" ).c_str() );
	strcpy( g_settings.network_nfs_local_dir[0], configfile.getString( "network_nfs_local_dir_1", "" ).c_str() );
	strcpy( g_settings.network_nfs_local_dir[1], configfile.getString( "network_nfs_local_dir_2", "" ).c_str() );
	strcpy( g_settings.network_nfs_local_dir[2], configfile.getString( "network_nfs_local_dir_3", "" ).c_str() );
	strcpy( g_settings.network_nfs_local_dir[3], configfile.getString( "network_nfs_local_dir_4", "" ).c_str() );
	g_settings.network_nfs_automount[0] = configfile.getInt32( "network_nfs_automount_1", 0);
	g_settings.network_nfs_automount[1] = configfile.getInt32( "network_nfs_automount_2", 0);
	g_settings.network_nfs_automount[2] = configfile.getInt32( "network_nfs_automount_3", 0);
	g_settings.network_nfs_automount[3] = configfile.getInt32( "network_nfs_automount_4", 0);
	g_settings.network_nfs_type[0] = configfile.getInt32( "network_nfs_type_1", 0);
	g_settings.network_nfs_type[1] = configfile.getInt32( "network_nfs_type_2", 0);
	g_settings.network_nfs_type[2] = configfile.getInt32( "network_nfs_type_3", 0);
	g_settings.network_nfs_type[3] = configfile.getInt32( "network_nfs_type_4", 0);
	strcpy( g_settings.network_nfs_username[0], configfile.getString( "network_nfs_username_1", "" ).c_str() );
	strcpy( g_settings.network_nfs_username[1], configfile.getString( "network_nfs_username_2", "" ).c_str() );
	strcpy( g_settings.network_nfs_username[2], configfile.getString( "network_nfs_username_3", "" ).c_str() );
	strcpy( g_settings.network_nfs_username[3], configfile.getString( "network_nfs_username_4", "" ).c_str() );
	strcpy( g_settings.network_nfs_password[0], configfile.getString( "network_nfs_password_1", "" ).c_str() );
	strcpy( g_settings.network_nfs_password[1], configfile.getString( "network_nfs_password_2", "" ).c_str() );
	strcpy( g_settings.network_nfs_password[2], configfile.getString( "network_nfs_password_3", "" ).c_str() );
	strcpy( g_settings.network_nfs_password[3], configfile.getString( "network_nfs_password_4", "" ).c_str() );
	strcpy( g_settings.network_nfs_mount_options[0], configfile.getString( "network_nfs_mount_options_1", "ro,soft,udp" ).c_str() );
	strcpy( g_settings.network_nfs_mount_options[1], configfile.getString( "network_nfs_mount_options_2", "nolock,rsize=8192,wsize=8192" ).c_str() );
	strcpy( g_settings.network_nfs_mp3dir, configfile.getString( "network_nfs_mp3dir", "" ).c_str() );
	strcpy( g_settings.network_nfs_picturedir, configfile.getString( "network_nfs_picturedir", "" ).c_str() );
	strcpy( g_settings.network_nfs_moviedir, configfile.getString( "network_nfs_moviedir", "" ).c_str() );

	//recording (server + vcr)
	g_settings.recording_type = configfile.getInt32( "recording_type", 0 );
	g_settings.recording_stopplayback = configfile.getInt32( "recording_stopplayback", 0 );
	g_settings.recording_stopsectionsd = configfile.getInt32( "recording_stopsectionsd", 1 );
	g_settings.recording_server_ip = configfile.getString("recording_server_ip", "10.10.10.10");
	strcpy( g_settings.recording_server_port, configfile.getString( "recording_server_port", "4000").c_str() );
	g_settings.recording_server_wakeup = configfile.getInt32( "recording_server_wakeup", 0 );
	strcpy( g_settings.recording_server_mac, configfile.getString( "recording_server_mac", "11:22:33:44:55:66").c_str() );
	g_settings.recording_vcr_no_scart = configfile.getInt32( "recording_vcr_no_scart", false);

	//streaming (server)
	g_settings.streaming_type = configfile.getInt32( "streaming_type", 0 );
        g_settings.streaming_server_ip = configfile.getString("streaming_server_ip", "10.10.10.10");
	strcpy( g_settings.streaming_server_port, configfile.getString( "streaming_server_port", "8080").c_str() );
	strcpy( g_settings.streaming_server_cddrive, configfile.getString("streaming_server_cddrive", "D:").c_str() );
	strcpy( g_settings.streaming_videorate,  configfile.getString("streaming_videorate", "1000").c_str() );
	strcpy( g_settings.streaming_audiorate, configfile.getString("streaming_audiorate", "192").c_str() );
	strcpy( g_settings.streaming_server_startdir, configfile.getString("streaming_server_startdir", "C:/Movies").c_str() );
	g_settings.streaming_type = configfile.getInt32( "streaming_ac3_enabled", 0 );
	
	//rc-key configuration
	g_settings.key_tvradio_mode = configfile.getInt32( "key_tvradio_mode", CRCInput::RC_nokey );

	g_settings.key_channelList_pageup = configfile.getInt32( "key_channelList_pageup",  CRCInput::RC_minus );
	g_settings.key_channelList_pagedown = configfile.getInt32( "key_channelList_pagedown", CRCInput::RC_plus );
	g_settings.key_channelList_cancel = configfile.getInt32( "key_channelList_cancel",  CRCInput::RC_home );
	g_settings.key_channelList_sort = configfile.getInt32( "key_channelList_sort",  CRCInput::RC_blue );
	g_settings.key_channelList_addrecord = configfile.getInt32( "key_channelList_addrecord",  CRCInput::RC_nokey );
	g_settings.key_channelList_addremind = configfile.getInt32( "key_channelList_addremind",  CRCInput::RC_nokey );

	g_settings.key_quickzap_up = configfile.getInt32( "key_quickzap_up",  CRCInput::RC_up );
	g_settings.key_quickzap_down = configfile.getInt32( "key_quickzap_down",  CRCInput::RC_down );
	g_settings.key_bouquet_up = configfile.getInt32( "key_bouquet_up",  CRCInput::RC_right );
	g_settings.key_bouquet_down = configfile.getInt32( "key_bouquet_down",  CRCInput::RC_left );
	g_settings.key_subchannel_up = configfile.getInt32( "key_subchannel_up",  CRCInput::RC_right );
	g_settings.key_subchannel_down = configfile.getInt32( "key_subchannel_down",  CRCInput::RC_left );

	strcpy(g_settings.repeat_blocker, configfile.getString("repeat_blocker", g_info.box_Type == CControldClient::TUXBOX_MAKER_PHILIPS ? "150" : "25").c_str());
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", g_info.box_Type == CControldClient::TUXBOX_MAKER_PHILIPS ? "25" : "0").c_str());

	//screen configuration
	g_settings.screen_StartX = configfile.getInt32( "screen_StartX", 37 );
	g_settings.screen_StartY = configfile.getInt32( "screen_StartY", 23 );
	g_settings.screen_EndX = configfile.getInt32( "screen_EndX", 668 );
	g_settings.screen_EndY = configfile.getInt32( "screen_EndY", 555 );

	//font configuration
	strcpy( g_settings.fontsize_menu		, configfile.getString( "fontsize_menu", DEFAULT_FONTSIZE_MENU).c_str() );
	strcpy( g_settings.fontsize_menu_title		, configfile.getString( "fontsize_menu_title", DEFAULT_FONTSIZE_MENU_TITLE).c_str() );
	strcpy( g_settings.fontsize_menu_info		, configfile.getString( "fontsize_menu_info", DEFAULT_FONTSIZE_MENU_INFO).c_str() );
	strcpy( g_settings.fontsize_epg_title		, configfile.getString( "fontsize_epg_title", DEFAULT_FONTSIZE_EPG_TITLE).c_str() );
	strcpy( g_settings.fontsize_epg_info1		, configfile.getString( "fontsize_epg_info1", DEFAULT_FONTSIZE_EPG_INFO1).c_str() );
	strcpy( g_settings.fontsize_epg_info2		, configfile.getString( "fontsize_epg_info2", DEFAULT_FONTSIZE_EPG_INFO2).c_str() );
	strcpy( g_settings.fontsize_epg_date		, configfile.getString( "fontsize_epg_date", DEFAULT_FONTSIZE_EPG_DATE).c_str() );
	strcpy( g_settings.fontsize_alert		, configfile.getString( "fontsize_alert", DEFAULT_FONTSIZE_ALERT).c_str() );
	strcpy( g_settings.fontsize_eventlist_title	, configfile.getString( "fontsize_eventlist_title", DEFAULT_FONTSIZE_EVENTLIST_TITLE).c_str() );
	strcpy( g_settings.fontsize_eventlist_itemlarge	, configfile.getString( "fontsize_eventlist_itemlarge", DEFAULT_FONTSIZE_EVENTLIST_ITEMLARGE).c_str() );
	strcpy( g_settings.fontsize_eventlist_itemsmall	, configfile.getString( "fontsize_eventlist_itemsmall", DEFAULT_FONTSIZE_EVENTLIST_ITEMSMALL).c_str() );
	strcpy( g_settings.fontsize_eventlist_datetime	, configfile.getString( "fontsize_eventlist_datetime", DEFAULT_FONTSIZE_EVENTLIST_DATETIME).c_str() );

	strcpy( g_settings.fontsize_gamelist_itemlarge	, configfile.getString( "fontsize_gamelist_itemlarge", DEFAULT_FONTSIZE_GAMELIST_ITEMLARGE).c_str() );
	strcpy( g_settings.fontsize_gamelist_itemsmall	, configfile.getString( "fontsize_gamelist_itemsmall", DEFAULT_FONTSIZE_GAMELIST_ITEMSMALL).c_str() );

	strcpy( g_settings.fontsize_channellist		, configfile.getString( "fontsize_channellist", DEFAULT_FONTSIZE_CHANNELLIST).c_str() );
	strcpy( g_settings.fontsize_channellist_descr	, configfile.getString( "fontsize_channellist_descr", DEFAULT_FONTSIZE_CHANNELLIST_DESCR).c_str() );
	strcpy( g_settings.fontsize_channellist_number	, configfile.getString( "fontsize_channellist_number", DEFAULT_FONTSIZE_CHANNELLIST_NUMBER).c_str() );
	strcpy( g_settings.fontsize_channel_num_zap	, configfile.getString( "fontsize_channel_num_zap", DEFAULT_FONTSIZE_CHANNEL_NUM_ZAP).c_str() );

	strcpy( g_settings.fontsize_infobar_number	, configfile.getString( "fontsize_infobar_number", DEFAULT_FONTSIZE_INFOBAR_NUMBER).c_str() );
	strcpy( g_settings.fontsize_infobar_channame	, configfile.getString( "fontsize_infobar_channame", DEFAULT_FONTSIZE_INFOBAR_CHANNAME).c_str() );
	strcpy( g_settings.fontsize_infobar_info	, configfile.getString( "fontsize_infobar_info", DEFAULT_FONTSIZE_INFOBAR_INFO).c_str() );
	strcpy( g_settings.fontsize_infobar_small	, configfile.getString( "fontsize_infobar_small", DEFAULT_FONTSIZE_INFOBAR_SMALL).c_str() );

	strcpy( g_settings.fontsize_filebrowser_item	, configfile.getString( "fontsize_filebrowser_item", DEFAULT_FONTSIZE_FILEBROWSER_ITEM).c_str() );

	//Software-update
	g_settings.softupdate_mode = configfile.getInt32( "softupdate_mode", 1 );
	strcpy( g_settings.softupdate_currentversion, configfile.getString( "softupdate_currentversion", "" ).c_str() );
	strcpy( g_settings.softupdate_proxyserver, configfile.getString( "softupdate_proxyserver", "" ).c_str() );
	strcpy( g_settings.softupdate_proxyusername, configfile.getString( "softupdate_proxyusername", "" ).c_str() );
	strcpy( g_settings.softupdate_proxypassword, configfile.getString( "softupdate_proxypassword", "" ).c_str() );

	//BouquetHandling
	g_settings.bouquetlist_mode = configfile.getInt32( "bouquetlist_mode", 0 );

	// parentallock
	g_settings.parentallock_prompt = configfile.getInt32( "parentallock_prompt", 0 );
	g_settings.parentallock_lockage = configfile.getInt32( "parentallock_lockage", 12 );
	strcpy( g_settings.parentallock_pincode, configfile.getString( "parentallock_pincode", "0000" ).c_str() );

	//timing  (Einheit= 1 sec )
	g_settings.timing_menu = configfile.getInt32( "timing_menu", DEFAULT_TIMING_MENU );
	g_settings.timing_chanlist = configfile.getInt32( "timing_chanlist", DEFAULT_TIMING_CHANLIST );
	g_settings.timing_epg = configfile.getInt32( "timing_epg", 2* DEFAULT_TIMING_EPG );
	g_settings.timing_infobar = configfile.getInt32( "timing_infobar", DEFAULT_TIMING_INFOBAR );
	g_settings.timing_filebrowser = configfile.getInt32( "timing_filebrowser", DEFAULT_TIMING_FILEBROWSER );

	//lcdd
	g_settings.lcd_brightness =  configfile.getInt32("lcd_brightness", DEFAULT_LCD_BRIGHTNESS);
	g_settings.lcd_standbybrightness = configfile.getInt32("lcd_standbybrightness", DEFAULT_LCD_STANDBYBRIGHTNESS);
	g_settings.lcd_contrast = configfile.getInt32("lcd_contrast", DEFAULT_LCD_CONTRAST);
	g_settings.lcd_power = configfile.getInt32("lcd_power", DEFAULT_LCD_POWER);
	g_settings.lcd_inverse = configfile.getInt32("lcd_inverse", DEFAULT_LCD_INVERSE);
	g_settings.lcd_show_volume = configfile.getInt32("lcd_show_volume", DEFAULT_LCD_SHOW_VOLUME);
	g_settings.lcd_autodimm = configfile.getInt32("lcd_autodimm", DEFAULT_LCD_AUTODIMM);

	strcpy( g_settings.picviewer_slide_time, configfile.getString( "picviewer_slide_time", "10" ).c_str() );
	g_settings.picviewer_scaling = configfile.getInt32("picviewer_scaling", 1 /*(int)CPictureViewer::SIMPLE*/);

	g_settings.mp3player_display = configfile.getInt32("mp3player_display",(int)CMP3PlayerGui::ARTIST_TITLE);
	g_settings.mp3player_follow  = configfile.getInt32("mp3player_follow",0);
	strcpy( g_settings.mp3player_screensaver, configfile.getString( "mp3player_screensaver", "0" ).c_str() );

	if(configfile.getUnknownKeyQueryedFlag() && (erg==0))
	{
		erg = 2;
	}

	if (!scanSettings.loadSettings(scanSettingsFile, (g_info.delivery_system = g_Zapit->getDeliverySystem())))
	{
		dprintf(DEBUG_NORMAL, "Loading of scan settings failed. Using defaults.\n");
	}

	// uboot config file
	if(fromflash)
	{
		g_settings.uboot_console	= 0;
		g_settings.uboot_lcd_inverse	= -1;
		g_settings.uboot_lcd_contrast	= -1;
	
		FILE* fd = fopen("/var/tuxbox/boot/ppcboot.conf", "r");
		if(fd)
		{
			char buffer[100];
	
			while(fgets(buffer, 99, fd) != NULL)
			{
				if(strncmp(buffer,"console=",8) == 0)
				{
					if(strncmp(&buffer[8], "null", 4)==0)
						g_settings.uboot_console = 0;
					else if(strncmp(&buffer[8], "ttyS0", 5)==0)
						g_settings.uboot_console = 1;
					else if(strncmp(&buffer[8], "tty", 3)==0)
						g_settings.uboot_console = 2;
				}
				else if(strncmp(buffer,"lcd_inverse=", 12) == 0)
				{
					g_settings.uboot_lcd_inverse = atoi(&buffer[12]);
				}
				else if(strncmp(buffer,"lcd_contrast=", 13) == 0)
				{
					g_settings.uboot_lcd_contrast = atoi(&buffer[13]);
				}
				else
					printf("unknown entry found in ppcboot.conf\n");
			}
	
			fclose(fd);
		}
		g_settings.uboot_console_bak = g_settings.uboot_console;
	}

	return erg;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  saveSetup, save the application-settings                   *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::saveSetup()
{
	//uboot; write config only on changes
	if( fromflash &&
		((g_settings.uboot_console_bak != g_settings.uboot_console) ||
		(g_settings.uboot_lcd_inverse  != g_settings.lcd_inverse) ||
		(g_settings.uboot_lcd_contrast != g_settings.lcd_contrast)) )
	{
		FILE* fd = fopen("/var/tuxbox/boot/ppcboot.conf", "w");

		if(fd != NULL)
		{
			char buffer[10];
			g_settings.uboot_console_bak    = g_settings.uboot_console;
			g_settings.uboot_lcd_inverse	= g_settings.lcd_inverse;
			g_settings.uboot_lcd_contrast	= g_settings.lcd_contrast;

			switch(g_settings.uboot_console)
			{
			case 1:
				strcpy( buffer, "ttyS0" );
				break;
			case 2:
				strcpy( buffer, "tty" );
				break;
			default:
				strcpy( buffer, "null" );
				break;
			}
			fprintf( fd, "console=%s\n", buffer );
			fprintf( fd, "lcd_inverse=%d\n", g_settings.uboot_lcd_inverse );
			fprintf( fd, "lcd_contrast=%d\n", g_settings.uboot_lcd_contrast );

			fclose(fd);
		}
		else
		{
			dprintf(DEBUG_NORMAL, "unable to write file /var/tuxbox/boot/ppcboot.conf\n");
		}
	}

	//scan settings
	if(!scanSettings.saveSettings(scanSettingsFile))
	{
		dprintf(DEBUG_NORMAL, "error while saving scan-settings!\n");
	}

	//video
	configfile.setInt32( "video_Signal", g_settings.video_Signal );
	configfile.setInt32( "video_Format", g_settings.video_Format );

	//fb-alphawerte f�r gtx
	configfile.setInt32( "gtx_alpha1", g_settings.gtx_alpha1 );
	configfile.setInt32( "gtx_alpha2", g_settings.gtx_alpha2 );

	//misc
	configfile.setInt32( "shutdown_real", g_settings.shutdown_real );
	configfile.setInt32( "shutdown_real_rcdelay", g_settings.shutdown_real_rcdelay );
	configfile.setInt32( "shutdown_showclock", g_settings.shutdown_showclock);
	configfile.setInt32( "show_camwarning", g_settings.show_camwarning );
	configfile.setString( "record_safety_time_before", g_settings.record_safety_time_before );
	configfile.setString( "record_safety_time_after", g_settings.record_safety_time_after );
	configfile.setInt32( "infobar_sat_display", g_settings.infobar_sat_display );

	//audio
	configfile.setInt32( "audio_AnalogMode", g_settings.audio_AnalogMode );
	configfile.setInt32( "audio_DolbyDigital", g_settings.audio_DolbyDigital );
	configfile.setInt32( "audio_avs_Control", g_settings.audio_avs_Control );

	//vcr
	configfile.setInt32( "vcr_AutoSwitch", g_settings.vcr_AutoSwitch );

	//language
	configfile.setString( "language", g_settings.language );

	//widget settings
	configfile.setInt32( "widget_fade", g_settings.widget_fade );

	//colors
	configfile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	configfile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	configfile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	configfile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );

	configfile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	configfile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	configfile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	configfile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	configfile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	configfile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	configfile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	configfile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );

	configfile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	configfile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	configfile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	configfile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	configfile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	configfile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	configfile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	configfile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );

	configfile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	configfile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	configfile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	configfile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	configfile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	configfile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	configfile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	configfile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );

	configfile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	configfile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	configfile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	configfile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	configfile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	configfile.setInt32( "infobar_red", g_settings.infobar_red );
	configfile.setInt32( "infobar_green", g_settings.infobar_green );
	configfile.setInt32( "infobar_blue", g_settings.infobar_blue );

	configfile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	configfile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	configfile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	configfile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );

	//network
	configfile.setString( "network_nfs_ip_1", g_settings.network_nfs_ip[0] );
	configfile.setString( "network_nfs_ip_2", g_settings.network_nfs_ip[1] );
	configfile.setString( "network_nfs_ip_3", g_settings.network_nfs_ip[2] );
	configfile.setString( "network_nfs_ip_4", g_settings.network_nfs_ip[3] );
	configfile.setString( "network_nfs_dir_1", g_settings.network_nfs_dir[0] );
	configfile.setString( "network_nfs_dir_2", g_settings.network_nfs_dir[1] );
	configfile.setString( "network_nfs_dir_3", g_settings.network_nfs_dir[2] );
	configfile.setString( "network_nfs_dir_4", g_settings.network_nfs_dir[3] );
	configfile.setString( "network_nfs_local_dir_1", g_settings.network_nfs_local_dir[0] );
	configfile.setString( "network_nfs_local_dir_2", g_settings.network_nfs_local_dir[1] );
	configfile.setString( "network_nfs_local_dir_3", g_settings.network_nfs_local_dir[2] );
	configfile.setString( "network_nfs_local_dir_4", g_settings.network_nfs_local_dir[3] );
	configfile.setInt32( "network_nfs_automount_1", g_settings.network_nfs_automount[0]);
	configfile.setInt32( "network_nfs_automount_2", g_settings.network_nfs_automount[1]);
	configfile.setInt32( "network_nfs_automount_3", g_settings.network_nfs_automount[2]);
	configfile.setInt32( "network_nfs_automount_4", g_settings.network_nfs_automount[3]);
	configfile.setInt32( "network_nfs_type_1", g_settings.network_nfs_type[0]);
	configfile.setInt32( "network_nfs_type_2", g_settings.network_nfs_type[1]);
	configfile.setInt32( "network_nfs_type_3", g_settings.network_nfs_type[2]);
	configfile.setInt32( "network_nfs_type_4", g_settings.network_nfs_type[3]);
	configfile.setString( "network_nfs_username_1", g_settings.network_nfs_username[0] );
	configfile.setString( "network_nfs_username_2", g_settings.network_nfs_username[1] );
	configfile.setString( "network_nfs_username_3", g_settings.network_nfs_username[2] );
	configfile.setString( "network_nfs_username_4", g_settings.network_nfs_username[3] );
	configfile.setString( "network_nfs_password_1", g_settings.network_nfs_password[0] );
	configfile.setString( "network_nfs_password_2", g_settings.network_nfs_password[1] );
	configfile.setString( "network_nfs_password_3", g_settings.network_nfs_password[2] );
	configfile.setString( "network_nfs_password_4", g_settings.network_nfs_password[3] );
	configfile.setString( "network_nfs_mount_options_1", g_settings.network_nfs_mount_options[0]);
	configfile.setString( "network_nfs_mount_options_2", g_settings.network_nfs_mount_options[1]);
	configfile.setString( "network_nfs_mp3dir", g_settings.network_nfs_mp3dir);
	configfile.setString( "network_nfs_picturedir", g_settings.network_nfs_picturedir);
	configfile.setString( "network_nfs_moviedir", g_settings.network_nfs_moviedir);

	//recording (server + vcr)
	configfile.setInt32 ( "recording_type", g_settings.recording_type );
	configfile.setInt32 ( "recording_stopplayback", g_settings.recording_stopplayback);
	configfile.setInt32 ( "recording_stopsectionsd", g_settings.recording_stopsectionsd );
	configfile.setString( "recording_server_ip", g_settings.recording_server_ip );
	configfile.setString( "recording_server_port", g_settings.recording_server_port );
	configfile.setInt32 ( "recording_server_wakeup", g_settings.recording_server_wakeup );
	configfile.setString( "recording_server_mac", g_settings.recording_server_mac );
	configfile.setInt32 ( "recording_vcr_no_scart", g_settings.recording_vcr_no_scart );

	//streaming
	configfile.setInt32 ( "streaming_type", g_settings.streaming_type );
	configfile.setString( "streaming_server_ip", g_settings.streaming_server_ip );
	configfile.setString( "streaming_server_port", g_settings.streaming_server_port );
	configfile.setString( "streaming_server_cddrive", g_settings.streaming_server_cddrive );
	configfile.setString ( "streaming_videorate", g_settings.streaming_videorate );
	configfile.setString ( "streaming_audiorate", g_settings.streaming_audiorate );
	configfile.setString( "streaming_server_startdir", g_settings.streaming_server_startdir );
	configfile.setInt32 ( "streaming_ac3_enabled", g_settings.streaming_ac3_enabled );
	
	//rc-key configuration
	configfile.setInt32( "key_tvradio_mode", g_settings.key_tvradio_mode );

	configfile.setInt32( "key_channelList_pageup", g_settings.key_channelList_pageup );
	configfile.setInt32( "key_channelList_pagedown", g_settings.key_channelList_pagedown );
	configfile.setInt32( "key_channelList_cancel", g_settings.key_channelList_cancel );
	configfile.setInt32( "key_channelList_sort", g_settings.key_channelList_sort );
	configfile.setInt32( "key_channelList_addrecord", g_settings.key_channelList_addrecord );
	configfile.setInt32( "key_channelList_addremind", g_settings.key_channelList_addremind );

	configfile.setInt32( "key_quickzap_up", g_settings.key_quickzap_up );
	configfile.setInt32( "key_quickzap_down", g_settings.key_quickzap_down );
	configfile.setInt32( "key_bouquet_up", g_settings.key_bouquet_up );
	configfile.setInt32( "key_bouquet_down", g_settings.key_bouquet_down );
	configfile.setInt32( "key_subchannel_up", g_settings.key_subchannel_up );
	configfile.setInt32( "key_subchannel_down", g_settings.key_subchannel_down );

	configfile.setString( "repeat_blocker", g_settings.repeat_blocker );
	configfile.setString( "repeat_genericblocker", g_settings.repeat_genericblocker );

	//screen configuration
	configfile.setInt32( "screen_StartX", g_settings.screen_StartX );
	configfile.setInt32( "screen_StartY", g_settings.screen_StartY );
	configfile.setInt32( "screen_EndX", g_settings.screen_EndX );
	configfile.setInt32( "screen_EndY", g_settings.screen_EndY );

	//font configuration
	configfile.setString( "fontsize_menu", g_settings.fontsize_menu );
	configfile.setString( "fontsize_menu_title", g_settings.fontsize_menu_title );
	configfile.setString( "fontsize_menu_info", g_settings.fontsize_menu_info );
	configfile.setString( "fontsize_epg_title", g_settings.fontsize_epg_title );
	configfile.setString( "fontsize_epg_info1", g_settings.fontsize_epg_info1 );
	configfile.setString( "fontsize_epg_info2", g_settings.fontsize_epg_info2 );
	configfile.setString( "fontsize_epg_date", g_settings.fontsize_epg_date );
	configfile.setString( "fontsize_alert", g_settings.fontsize_alert );
	configfile.setString( "fontsize_eventlist_title", g_settings.fontsize_eventlist_title );
	configfile.setString( "fontsize_eventlist_itemlarge", g_settings.fontsize_eventlist_itemlarge );
	configfile.setString( "fontsize_eventlist_itemsmall", g_settings.fontsize_eventlist_itemsmall );
	configfile.setString( "fontsize_eventlist_datetime", g_settings.fontsize_eventlist_datetime );

	configfile.setString( "fontsize_gamelist_itemlarge", g_settings.fontsize_gamelist_itemlarge );
	configfile.setString( "fontsize_gamelist_itemsmall", g_settings.fontsize_gamelist_itemsmall );

	configfile.setString( "fontsize_channellist", g_settings.fontsize_channellist );
	configfile.setString( "fontsize_channellist_descr", g_settings.fontsize_channellist_descr );
	configfile.setString( "fontsize_channellist_number", g_settings.fontsize_channellist_number );
	configfile.setString( "fontsize_channel_num_zap", g_settings.fontsize_channel_num_zap );

	configfile.setString( "fontsize_infobar_number", g_settings.fontsize_infobar_number );
	configfile.setString( "fontsize_infobar_channame", g_settings.fontsize_infobar_channame );
	configfile.setString( "fontsize_infobar_info", g_settings.fontsize_infobar_info );
	configfile.setString( "fontsize_infobar_small", g_settings.fontsize_infobar_small );

	configfile.setString( "fontsize_filebrowser_item", g_settings.fontsize_filebrowser_item );

	//Software-update
	configfile.setInt32( "softupdate_mode", g_settings.softupdate_mode );
	configfile.setString( "softupdate_currentversion", g_settings.softupdate_currentversion );
	configfile.setString( "softupdate_proxyserver", g_settings.softupdate_proxyserver );
	configfile.setString( "softupdate_proxyusername", g_settings.softupdate_proxyusername );
	configfile.setString( "softupdate_proxypassword", g_settings.softupdate_proxypassword );

	//BouquetHandling
	configfile.setInt32( "bouquetlist_mode", g_settings.bouquetlist_mode );

	//parentallock
	configfile.setInt32( "parentallock_prompt", g_settings.parentallock_prompt );
	configfile.setInt32( "parentallock_lockage", g_settings.parentallock_lockage );
	configfile.setString( "parentallock_pincode", g_settings.parentallock_pincode );

	//timing
	configfile.setInt32( "timing_menu", g_settings.timing_menu );
	configfile.setInt32( "timing_chanlist", g_settings.timing_chanlist );
	configfile.setInt32( "timing_epg", g_settings.timing_epg );
	configfile.setInt32( "timing_infobar", g_settings.timing_infobar );
	configfile.setInt32( "timing_filebrowser", g_settings.timing_filebrowser );

	//lcdd
	configfile.setInt32( "lcd_brightness", g_settings.lcd_brightness );
	configfile.setInt32( "lcd_standbybrightness", g_settings.lcd_standbybrightness );
	configfile.setInt32( "lcd_contrast", g_settings.lcd_contrast );
	configfile.setInt32( "lcd_power", g_settings.lcd_power );
	configfile.setInt32( "lcd_inverse", g_settings.lcd_inverse );
	configfile.setInt32( "lcd_show_volume", g_settings.lcd_show_volume );
	configfile.setInt32( "lcd_autodimm", g_settings.lcd_autodimm );

	configfile.setString( "picviewer_slide_time", g_settings.picviewer_slide_time );
	configfile.setInt32( "picviewer_scaling", g_settings.picviewer_scaling );

	configfile.setInt32( "mp3player_display", g_settings.mp3player_display );
	configfile.setInt32( "mp3player_follow", g_settings.mp3player_follow );
	configfile.setString( "mp3player_screensaver", g_settings.mp3player_screensaver );

   if(configfile.getModifiedFlag())
	{
		dprintf(DEBUG_INFO, "saving neutrino txt-config\n");
		configfile.saveConfig(CONFIGDIR "/neutrino.conf");
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  firstChannel, get the initial channel                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::firstChannel()
{
	g_Zapit->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  doChecks, check if card fits cam		              *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::doChecks()
{
	FILE* fd;
	fd = fopen(UCODEDIR "/avia500.ux", "r");
	if(fd)
		fclose(fd);
	bool ucodes_ok= (fd);
	fd = fopen(UCODEDIR "/avia600.ux", "r");
	if(fd)
		fclose(fd);
	ucodes_ok= ucodes_ok||(fd);
/*	ucode.bin no longer needed, since built-in *
	fd = fopen(UCODEDIR "/ucode.bin", "r");
	if(fd)
		fclose(fd);
	ucodes_ok= ucodes_ok&&(fd);*/
	fd = fopen(UCODEDIR "/cam-alpha.bin", "r");
	if(fd)
		fclose(fd);
	ucodes_ok= ucodes_ok&&(fd);
	if (!ucodes_ok)
		DisplayErrorMessage(g_Locale->getText("ucodes.failure")); // UTF-8
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit()
{
	dprintf(DEBUG_DEBUG, "doing channelsInit\n");
	//deleting old channelList for mode-switching.

	delete channelList;
	channelList = new CChannelList( "channellist.head" );
	CZapitClient::BouquetChannelList zapitChannels;
	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_CURRENT, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelList->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}
	dprintf(DEBUG_DEBUG, "got channels\n");

	delete bouquetList;
	bouquetList = new CBouquetList();
	bouquetList->orgChannelList = channelList;
	CZapitClient::BouquetList zapitBouquets;
	g_Zapit->getBouquets(zapitBouquets, false, true); // UTF-8
	for(uint i=0; i<zapitBouquets.size(); i++)
	{
		bouquetList->addBouquet( zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
	}
	dprintf(DEBUG_DEBUG, "got bouquets\n");

	for( uint i=0; i< bouquetList->Bouquets.size(); i++ )
	{
		CZapitClient::BouquetChannelList zapitChannels;
		g_Zapit->getBouquetChannels(bouquetList->Bouquets[i]->unique_key, zapitChannels, CZapitClient::MODE_CURRENT, true); // UTF-8
		for(uint j=0; j<zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = channelList->getChannel(zapitChannels[j].nr);

			bouquetList->Bouquets[i]->channelList->addChannel(channel);
			if( bouquetList->Bouquets[i]->bLocked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}
	dprintf(DEBUG_DEBUG, "\nAll bouquets-channels received\n");
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  run, the main runloop                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::CmdParser(int argc, char **argv)
{
	softupdate = false;
	fromflash = false;

#define FONTNAME "Micron"
#define FONTFILE "micron"

	fontName = FONTNAME;
	fontFile = FONTDIR "/" FONTFILE;
	fontsSizeOffset = 0;

	for(int x=1; x<argc; x++)
	{
		if( !strcmp(argv[x], "-su"))
		{
			dprintf(DEBUG_NORMAL, "Software update enabled\n");
			softupdate = true;
		}
		else if( !strcmp(argv[x], "-flash"))
		{
			dprintf(DEBUG_NORMAL, "enable flash\n");
			fromflash = true;
		}
		else if( !strcmp(argv[x], "-font"))
		{
			if( ( x + 3 ) < argc )
			{
				fontFile = argv[x+ 1];
				fontName = argv[x+ 2];
				fontsSizeOffset = atoi(argv[x+ 3]);
			}
			x+=3;
		}
		else if( !strcmp(argv[x], "-debuglevel"))
		{
			int dl = atoi(argv[x+ 1]);
			dprintf(DEBUG_NORMAL, "set debuglevel: %d\n", dl);
			setDebugLevel(dl);
			x++;
		}
		else
		{
			dprintf(DEBUG_NORMAL, "Usage: neutrino [-su] [-debuglevel 0..3] [-flash] [-font /fontdir/fontfile fontname fontsize]\n");
			exit(0);
		}
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup the framebuffer                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupFrameBuffer()
{
	frameBuffer->init();
	if(frameBuffer->setMode(720, 576, 8))
	{
		dprintf(DEBUG_NORMAL, "Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transparent for dummy painting
	for(int count =0;count<8;count++)
		frameBuffer->paletteSetColor(count, 0x000000, 0xffff);
	frameBuffer->paletteSet();
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup fonts                                                *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupFonts()
{
	g_fontRenderer->AddFont((fontFile + ".ttf").c_str());

	char * bold_style_name;
	if (g_fontRenderer->AddFont((fontFile + "_bold.ttf").c_str()) == 0)
		bold_style_name = "Bold";
	else
		bold_style_name = "Bold Regular";

	if (g_fontRenderer->AddFont((fontFile + "_italic.ttf").c_str()) != 0)
		g_fontRenderer->AddFont((fontFile + ".ttf").c_str(), true);  // make italics

	g_Fonts->menu                = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_menu               )                      );
	g_Fonts->menu_title          = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_menu_title         )                      );
	g_Fonts->menu_info           = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_menu_info          )                      );

	g_Fonts->epg_title           = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_epg_title          ) +     fontsSizeOffset);
	g_Fonts->epg_info1           = g_fontRenderer->getFont(fontName.c_str(), "Italic"       , atoi(g_settings.fontsize_epg_info1          ) + 2 * fontsSizeOffset);
	g_Fonts->epg_info2           = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_epg_info2          ) + 2 * fontsSizeOffset);
	g_Fonts->epg_date            = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_epg_date           ) + 2 * fontsSizeOffset);

	g_Fonts->alert               = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_alert              )                      );

	g_Fonts->eventlist_title     = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_eventlist_title    )                      );
	g_Fonts->eventlist_itemLarge = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_eventlist_itemlarge) +     fontsSizeOffset);
	g_Fonts->eventlist_itemSmall = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_eventlist_itemsmall) +     fontsSizeOffset);
	g_Fonts->eventlist_datetime  = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_eventlist_datetime ) +     fontsSizeOffset);

	g_Fonts->gamelist_itemLarge  = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_gamelist_itemlarge ) +     fontsSizeOffset);
	g_Fonts->gamelist_itemSmall  = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_gamelist_itemsmall ) +     fontsSizeOffset);

	g_Fonts->channellist         = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_channellist        ) +     fontsSizeOffset);
	g_Fonts->channellist_descr   = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_channellist_descr  ) +     fontsSizeOffset);
	g_Fonts->channellist_number  = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_channellist_number ) + 2 * fontsSizeOffset);
	g_Fonts->channel_num_zap     = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_channel_num_zap    )                      );

	g_Fonts->infobar_number      = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_infobar_number     )                      );
	g_Fonts->infobar_channame    = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_infobar_channame   )                      );
	g_Fonts->infobar_info        = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_infobar_info       ) +     fontsSizeOffset);
	g_Fonts->infobar_small       = g_fontRenderer->getFont(fontName.c_str(), "Regular"      , atoi(g_settings.fontsize_infobar_small      ) +     fontsSizeOffset);

	g_Fonts->filebrowser_item    = g_fontRenderer->getFont(fontName.c_str(), bold_style_name, atoi(g_settings.fontsize_filebrowser_item   ) +     fontsSizeOffset);
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup the menu timouts                                     *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupTiming()
{
	sprintf(g_settings.timing_menu_string,"%d",g_settings.timing_menu);
	sprintf(g_settings.timing_chanlist_string,"%d",g_settings.timing_chanlist);
	sprintf(g_settings.timing_epg_string,"%d",g_settings.timing_epg);
	sprintf(g_settings.timing_infobar_string,"%d",g_settings.timing_infobar);
	sprintf(g_settings.timing_filebrowser_string,"%d",g_settings.timing_filebrowser);
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  init main menu                                             *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::InitMainMenu(CMenuWidget &mainMenu, CMenuWidget &mainSettings,  CMenuWidget &audioSettings, CMenuWidget &parentallockSettings,
				CMenuWidget &networkSettings, CMenuWidget &recordingSettings, CMenuWidget &colorSettings, CMenuWidget &lcdSettings,
				CMenuWidget &keySettings, CMenuWidget &videoSettings, CMenuWidget &languageSettings, CMenuWidget &miscSettings,
				CMenuWidget &service, CMenuWidget &fontSettings, CMenuWidget &mp3picSettings, CMenuWidget &streamingSettings)
{
	dprintf(DEBUG_DEBUG, "init mainmenue\n");
	mainMenu.addItem(&GenericMenuSeparator);
	mainMenu.addItem(new CMenuForwarder("mainmenu.tvmode", true, NULL, this, "tv", true, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED), true );
	mainMenu.addItem(new CMenuForwarder("mainmenu.radiomode", true, NULL, this, "radio", true, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN) );
	mainMenu.addItem(new CMenuForwarder("mainmenu.scartmode", true, NULL, this, "scart", true, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW) );
	mainMenu.addItem(new CMenuForwarder("mainmenu.games", true, NULL, new CGameList("mainmenu.games"), "", true, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE) );
	mainMenu.addItem(&GenericMenuSeparatorLine);
	mainMenu.addItem(new CMenuForwarder("mainmenu.mp3player", true, NULL, new CMP3PlayerGui()));

#if HAVE_DVB_API_VERSION >= 3
	mainMenu.addItem(new CMenuForwarder("mainmenu.movieplayer", true, NULL, new CMoviePlayerGui()));
#endif

	mainMenu.addItem(new CMenuForwarder("mainmenu.pictureviewer", true, NULL, new CPictureViewerGui()));
	mainMenu.addItem(&GenericMenuSeparatorLine);
	
	mainMenu.addItem(new CMenuForwarder("mainmenu.settings", true, NULL, &mainSettings));
	mainMenu.addItem(new CLockedMenuForwarder("mainmenu.service", g_settings.parentallock_pincode, false, true, NULL, &service) );
	mainMenu.addItem(&GenericMenuSeparatorLine);
	
	mainMenu.addItem(new CMenuForwarder("mainmenu.sleeptimer", true, NULL, new CSleepTimerWidget));
	mainMenu.addItem(new CMenuForwarder("mainmenu.shutdown", true, NULL, this, "shutdown", true, CRCInput::RC_standby, "power.raw") );
//	mainMenu.addItem(&GenericMenuSeparatorLine);
//	mainMenu.addItem( new CMenuForwarder("mainmenu.info", true, "", new CDBoxInfoWidget, "",true) );


	mainSettings.addItem(&GenericMenuSeparator);
	mainSettings.addItem(&GenericMenuBack);
	mainSettings.addItem(&GenericMenuSeparatorLine);
	mainSettings.addItem(new CMenuForwarder("mainsettings.savesettingsnow", true, NULL, this, "savesettings"));
	mainSettings.addItem(&GenericMenuSeparatorLine);
	mainSettings.addItem(new CMenuForwarder("mainsettings.video", true, NULL, &videoSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.audio", true, NULL, &audioSettings));
	mainSettings.addItem(new CLockedMenuForwarder("parentallock.parentallock", g_settings.parentallock_pincode, true, true, NULL, &parentallockSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.network", true, NULL, &networkSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.recording", true, NULL, &recordingSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.streaming", true, NULL, &streamingSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.language", true, NULL, &languageSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.colors", true, NULL, &colorSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.lcd", true, NULL, &lcdSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.keybinding", true, NULL, &keySettings));
	mainSettings.addItem(new CMenuForwarder("mp3picsettings.general", true, NULL, &mp3picSettings));
	mainSettings.addItem(new CMenuForwarder("mainsettings.misc", true, NULL, &miscSettings));
}


void CNeutrinoApp::InitScanSettings(CMenuWidget &settings)
{
	dprintf(DEBUG_DEBUG, "init scansettings\n");
	CMenuOptionChooser* ojBouquets = new CMenuOptionChooser("scants.bouquet", (int *)&scanSettings.bouquetMode, true );
	ojBouquets->addOption(CZapitClient::BM_DELETEBOUQUETS, "scants.bouquet_erase");
	ojBouquets->addOption(CZapitClient::BM_CREATEBOUQUETS, "scants.bouquet_create");
	ojBouquets->addOption(CZapitClient::BM_DONTTOUCHBOUQUETS, "scants.bouquet_leave");
	ojBouquets->addOption(CZapitClient::BM_UPDATEBOUQUETS, "scants.bouquet_update");
	ojBouquets->addOption(CZapitClient::BM_CREATESATELLITEBOUQUET, "scants.bouquet_satellite");

	//sat-lnb-settings
	if(g_info.delivery_system == DVB_S)
	{
		settings.addItem(&GenericMenuSeparator);
		settings.addItem(&GenericMenuBack);
		settings.addItem(&GenericMenuSeparatorLine);

		satList.clear();
		g_Zapit->getScanSatelliteList(satList);

		t_satellite_position currentSatellitePosition = g_Zapit->getCurrentSatellitePosition();

		if (scanSettings.diseqcMode == DISEQC_1_2)
		{
			//printf("[neutrino] received %d sats\n", satList.size());
			for (uint i = 0; i < satList.size(); i++)
			{
				//printf("[neutrino] received %d: %s, %d\n", i, satList[i].satName, satList[i].satPosition);
				scanSettings.satPosition[i] = satList[i].satPosition;
				scanSettings.satMotorPos[i] = satList[i].motorPosition;
				strcpy(scanSettings.satName[i], satList[i].satName);
				//scanSettings.satDiseqc[i] = satList[i].satDiseqc;
				if (satList[i].satPosition == currentSatellitePosition)
					strcpy(scanSettings.satNameNoDiseqc, satList[i].satName);
			}
			for (uint i = satList.size(); i < MAX_SATELLITES; i++)
			{
				scanSettings.satName[i][0] = 0;
				scanSettings.satPosition[i] = 0;
				scanSettings.satDiseqc[i] = -1;
			}
		}

		CMenuOptionStringChooser* ojSat = new CMenuOptionStringChooser("satsetup.satellite", scanSettings.satNameNoDiseqc, (scanSettings.diseqcMode == NO_DISEQC)/*, new CSatelliteNotifier*/, NULL, false);
		for (uint i=0; i < satList.size(); i++)
		{
			ojSat->addOption(satList[i].satName);
			dprintf(DEBUG_DEBUG, "got scanprovider (sat): %s\n", satList[i].satName );
		}

		CMenuOptionChooser* ojDiseqcRepeats = new CMenuOptionChooser("satsetup.diseqcrepeat", (int *)&scanSettings.diseqcRepeat, (scanSettings.diseqcMode != NO_DISEQC) && (scanSettings.diseqcMode != DISEQC_1_0)/*, new CSatelliteNotifier*/, NULL, false);
		ojDiseqcRepeats->addOption(0, "0");
		ojDiseqcRepeats->addOption(1, "1");
		ojDiseqcRepeats->addOption(2, "2");

		CMenuWidget* extSatSettings = new CMenuWidget("satsetup.extended", NEUTRINO_ICON_SETTINGS);
		extSatSettings->addItem(&GenericMenuSeparator);
		extSatSettings->addItem(&GenericMenuBack);
		extSatSettings->addItem(&GenericMenuSeparatorLine);

		CMenuForwarder* ojExtSatSettings = new CMenuForwarder("satsetup.extended", (scanSettings.diseqcMode != NO_DISEQC), NULL, extSatSettings);
		for( uint i=0; i < satList.size(); i++)
		{
			CMenuOptionChooser* oj = new CMenuOptionChooser( satList[i].satName, scanSettings.diseqscOfSat( satList[i].satName), true/*, new CSatelliteNotifier*/);
			oj->addOption( -1, "options.off");
			for(uint j=0; j < satList.size(); j++)
			{
				char jj[2 + 1];
				sprintf( jj, "%d", j + 1);
				oj->addOption( j, jj);
			}
			extSatSettings->addItem( oj);
		}

		CMenuWidget* extMotorSettings = new CMenuWidget("satsetup.extended_motor", NEUTRINO_ICON_SETTINGS);
		extMotorSettings->addItem(&GenericMenuSeparator);
		extMotorSettings->addItem(&GenericMenuBack);
		extMotorSettings->addItem( new CMenuForwarder("satsetup.savesettingsnow", true, NULL, this, "savesettings") );
		extMotorSettings->addItem( new CMenuForwarder("satsetup.motorcontrol", true, NULL, new CMotorControl()) );
		extMotorSettings->addItem(&GenericMenuSeparatorLine);

		CMenuForwarder* ojExtMotorSettings = new CMenuForwarder("satsetup.extended_motor", (scanSettings.diseqcMode == DISEQC_1_2), NULL, extMotorSettings);

		for( uint i=0; i < satList.size(); i++)
		{
			CMenuOptionChooser* oj = new CMenuOptionChooser( satList[i].satName, scanSettings.motorPosOfSat( satList[i].satName), true/*, new CSatelliteNotifier*/);
			oj->addOption(0, "options.off");
			for(uint j=1; j<=satList.size(); j++)
			{
				char jj[2 + 1];
				sprintf(jj, "%d", j);
				oj->addOption(j, jj);
			}
			extMotorSettings->addItem(oj);
		}

		CMenuOptionChooser* ojDiseqc = new CMenuOptionChooser("satsetup.diseqc", (int *)&scanSettings.diseqcMode, true, new CSatDiseqcNotifier( ojSat, ojExtSatSettings, ojExtMotorSettings, ojDiseqcRepeats));
		ojDiseqc->addOption( NO_DISEQC,   "satsetup.nodiseqc");
		ojDiseqc->addOption( MINI_DISEQC, "satsetup.minidiseqc");
		ojDiseqc->addOption( DISEQC_1_0,  "satsetup.diseqc10");
		ojDiseqc->addOption( DISEQC_1_1,  "satsetup.diseqc11");
		ojDiseqc->addOption( DISEQC_1_2,  "satsetup.diseqc12");
		ojDiseqc->addOption( SMATV_REMOTE_TUNING,  "satsetup.smatvremote");

		settings.addItem( ojDiseqc );
		settings.addItem( ojBouquets );
		settings.addItem( ojSat );
		settings.addItem( ojDiseqcRepeats );
		settings.addItem( ojExtSatSettings );
		settings.addItem( ojExtMotorSettings );
		settings.addItem(&GenericMenuSeparatorLine);
	}
	else
	{//kabel
		settings.addItem(&GenericMenuSeparator);
		settings.addItem(&GenericMenuBack);
		settings.addItem(&GenericMenuSeparatorLine);

		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);

		CMenuOptionStringChooser* oj = new CMenuOptionStringChooser("cablesetup.provider", (char*)&scanSettings.satNameNoDiseqc, true/*, new CCableProviderNotifier*/);

		for( uint i=0; i< providerList.size(); i++)
		{
			oj->addOption( providerList[i].satName);
			dprintf(DEBUG_DEBUG, "got scanprovider (cable): %s\n", providerList[i].satName );
		}
		settings.addItem( ojBouquets);
		settings.addItem( oj);
	}

	settings.addItem(new CMenuForwarder("scants.startnow", true, NULL, new CScanTs()));
}


void CNeutrinoApp::InitServiceSettings(CMenuWidget &service, CMenuWidget &scanSettings)
{
	dprintf(DEBUG_DEBUG, "init serviceSettings\n");
	service.addItem(&GenericMenuSeparator);
	service.addItem(&GenericMenuBack);
	service.addItem(&GenericMenuSeparatorLine);
	service.addItem( new CMenuForwarder("bouqueteditor.name", true, NULL, new CBEBouquetWidget()));
	service.addItem( new CMenuForwarder("servicemenu.scants", true, NULL, &scanSettings ) );
	service.addItem( new CMenuForwarder("servicemenu.reload", true, NULL, this, "reloadchannels" ) );
	service.addItem( new CMenuForwarder("servicemenu.ucodecheck", true, NULL, UCodeChecker ) );

	//softupdate
	if(softupdate)
	{
		dprintf(DEBUG_DEBUG, "init soft-update-stuff\n");
		CMenuWidget* updateSettings = new CMenuWidget("servicemenu.update", "softupdate.raw", 450);
		updateSettings->addItem(&GenericMenuSeparator);
		updateSettings->addItem(&GenericMenuBack);
		updateSettings->addItem(&GenericMenuSeparatorLine);


		//experten-funktionen f�r mtd lesen/schreiben
		CMenuWidget* mtdexpert = new CMenuWidget("flashupdate.expertfunctions", "softupdate.raw");
		mtdexpert->addItem(&GenericMenuSeparator);
		mtdexpert->addItem(&GenericMenuBack);
		mtdexpert->addItem(&GenericMenuSeparatorLine);
		CFlashExpert* fe = new CFlashExpert();
		mtdexpert->addItem( new CMenuForwarder("flashupdate.readflash", true, NULL, fe, "readflash") );
		mtdexpert->addItem( new CMenuForwarder("flashupdate.writeflash", true, NULL, fe, "writeflash") );
		mtdexpert->addItem(&GenericMenuSeparatorLine);
		mtdexpert->addItem( new CMenuForwarder("flashupdate.readflashmtd", true, NULL, fe, "readflashmtd") );
		mtdexpert->addItem( new CMenuForwarder("flashupdate.writeflashmtd", true, NULL, fe, "writeflashmtd") );
		updateSettings->addItem( new CMenuForwarder("flashupdate.expertfunctions", true, NULL, mtdexpert ) );

		updateSettings->addItem(&GenericMenuSeparatorLine);
		CMenuOptionChooser *oj = new CMenuOptionChooser("flashupdate.updatemode", &g_settings.softupdate_mode,true);
		oj->addOption(0, "flashupdate.updatemode_manual");
		oj->addOption(1, "flashupdate.updatemode_internet");
		updateSettings->addItem( oj );


		//get current flash-version SBBBYYYYMMTTHHMM -- formatsting
		CConfigFile configfile('\t');
		if(!configfile.loadConfig("/.version"))
		{
			//error default
			strcpy(g_settings.softupdate_currentversion, "0105200205212015");
		}
		else
		{
			strcpy(g_settings.softupdate_currentversion, configfile.getString( "version", "0105200205212015").c_str());
		}
		dprintf(DEBUG_INFO, "current flash-version: %s\n", g_settings.softupdate_currentversion);

		//aktuelle Version anzeigen
		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "flashupdate.currentversion_sep") );

		//updateSettings->addItem( new CMenuForwarder("flashupdate.currentversion", false, (char*) &g_settings.softupdate_currentversion, NULL ));
		CFlashVersionInfo versionInfo(g_settings.softupdate_currentversion);
		static char date[50];
		strcpy(date, versionInfo.getDate());
		updateSettings->addItem( new CMenuForwarder("flashupdate.currentversiondate", false, (char*) &date, NULL ));
		static char time[50];
		strcpy(time, versionInfo.getTime());
		updateSettings->addItem( new CMenuForwarder("flashupdate.currentversiontime", false, (char*) &time, NULL ));
		static char baseimage[50];
		strcpy(baseimage, versionInfo.getBaseImageVersion());
		updateSettings->addItem( new CMenuForwarder("flashupdate.currentversionbaseversion", false, (char*) &baseimage, NULL ));
		static char releasetype[50];
		strcpy(releasetype, versionInfo.getType());
		updateSettings->addItem( new CMenuForwarder("flashupdate.currentversionsnapshot", false, (char*) &releasetype, NULL ));

		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "flashupdate.proxyserver_sep") );

		CStringInputSMS * updateSettings_proxy = new CStringInputSMS("flashupdate.proxyserver", g_settings.softupdate_proxyserver, 23, "flashupdate.proxyserver_hint1", "flashupdate.proxyserver_hint2", "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
		updateSettings->addItem( new CMenuForwarder("flashupdate.proxyserver", true, g_settings.softupdate_proxyserver, updateSettings_proxy ) );

		CStringInputSMS * updateSettings_proxyuser = new CStringInputSMS("flashupdate.proxyusername", g_settings.softupdate_proxyusername, 23, "flashupdate.proxyusername_hint1", "flashupdate.proxyusername_hint2", "abcdefghijklmnopqrstuvwxyz0123456789!""�$%&/()=?-. ");
		updateSettings->addItem( new CMenuForwarder("flashupdate.proxyusername", true, g_settings.softupdate_proxyusername, updateSettings_proxyuser ) );

		CStringInputSMS * updateSettings_proxypass = new CStringInputSMS("flashupdate.proxypassword", g_settings.softupdate_proxypassword, 20, "flashupdate.proxypassword_hint1", "flashupdate.proxypassword_hint2", "abcdefghijklmnopqrstuvwxyz0123456789!""�$%&/()=?-. ");
		updateSettings->addItem( new CMenuForwarder("flashupdate.proxypassword", true, g_settings.softupdate_proxypassword, updateSettings_proxypass ) );

		updateSettings->addItem(&GenericMenuSeparatorLine);
		updateSettings->addItem(new CMenuForwarder("flashupdate.checkupdate", true, NULL, new CFlashUpdate()));

		service.addItem(new CMenuForwarder("servicemenu.update", true, NULL, updateSettings));
	}
}

void CNeutrinoApp::InitMp3PicSettings(CMenuWidget &mp3PicSettings)
{
	dprintf(DEBUG_DEBUG, "init mp3_pic_settings\n");

	mp3PicSettings.addItem(&GenericMenuSeparator);
	mp3PicSettings.addItem(&GenericMenuBack);
	
	CMenuOptionChooser *oj = new CMenuOptionChooser("pictureviewer.scaling", &g_settings.picviewer_scaling, true );
	oj->addOption((int)CPictureViewer::SIMPLE, "Simple");
	oj->addOption((int)CPictureViewer::COLOR, "Color Average");
	oj->addOption((int)CPictureViewer::NONE, "None");
	CStringInput * pic_timeout= new CStringInput("pictureviewer.slide_time", g_settings.picviewer_slide_time, 2, NULL, NULL, "0123456789 ");
	mp3PicSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "pictureviewer.head") );
	mp3PicSettings.addItem( oj );
	mp3PicSettings.addItem( new CMenuForwarder("pictureviewer.slide_time", true, g_settings.picviewer_slide_time, pic_timeout ));
	mp3PicSettings.addItem( new CMenuForwarder("pictureviewer.defdir", true, g_settings.network_nfs_picturedir, 
															 this, "picturedir"));

	oj = new CMenuOptionChooser("mp3player.display_order", &g_settings.mp3player_display, true );
	oj->addOption((int)CMP3PlayerGui::ARTIST_TITLE, "mp3player.artist_title");
	oj->addOption((int)CMP3PlayerGui::TITLE_ARTIST, "mp3player.title_artist");
	mp3PicSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "mp3player.name") );
	mp3PicSettings.addItem( oj );
  	oj = new CMenuOptionChooser("mp3player.follow", &g_settings.mp3player_follow, true );
	oj->addOption(0, "messagebox.no");
	oj->addOption(1, "messagebox.yes");
	mp3PicSettings.addItem( oj );
	CStringInput * mp3_screensaver= new CStringInput("mp3player.screensaver_timeout", g_settings.mp3player_screensaver, 2, NULL, NULL, "0123456789 ");
	mp3PicSettings.addItem( new CMenuForwarder("mp3player.screensaver_timeout", true, g_settings.mp3player_screensaver, mp3_screensaver ));
	mp3PicSettings.addItem( new CMenuForwarder("mp3player.defdir", true, g_settings.network_nfs_mp3dir, this, "mp3dir"));
}
void CNeutrinoApp::InitMiscSettings(CMenuWidget &miscSettings)
{
	dprintf(DEBUG_DEBUG, "init miscsettings\n");
	miscSettings.addItem(&GenericMenuSeparator);
	miscSettings.addItem(&GenericMenuBack);
	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "miscsettings.general" ) );

	CMenuOptionChooser *m1 = new CMenuOptionChooser("miscsettings.shutdown_real_rcdelay", &g_settings.shutdown_real_rcdelay, !g_settings.shutdown_real );
	m1->addOption(0, "options.off");
	m1->addOption(1, "options.on");

	CMiscNotifier* miscNotifier = new CMiscNotifier( m1 );

	CMenuOptionChooser *oj = new CMenuOptionChooser("miscsettings.shutdown_real", &g_settings.shutdown_real, true, miscNotifier );
	oj->addOption(1, "options.off");
	oj->addOption(0, "options.on");
	miscSettings.addItem( oj );

	if(fromflash)
	{
		static int dummy = 0;
		FILE* fd = fopen("/var/etc/.neutrino", "r");
		if(fd)
		{
			dummy=1;
			fclose(fd);
		}
		oj = new CMenuOptionChooser("miscsettings.startneutrinodirect", &dummy, true, new CStartNeutrinoDirectNotifier );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		miscSettings.addItem( oj );

#if HAVE_DVB_API_VERSION == 1
		static int dummy2 = 0;
		fd = fopen("/var/etc/.bh", "r");
		if(fd)
		{
			dummy2=1;
			fclose(fd);
		}
		oj = new CMenuOptionChooser("miscsettings.startbhdriver", &dummy2, true, new CBHDriverNotifier );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		miscSettings.addItem( oj );
#endif

		oj = new CMenuOptionChooser("miscsettings.fb_destination", &g_settings.uboot_console, true, ConsoleDestinationChanger );
		oj->addOption(0, "options.null");
		oj->addOption(1, "options.serial");
		oj->addOption(2, "options.fb");
		miscSettings.addItem( oj );
	}

	CMenuOptionChooser *oj2 = new CMenuOptionChooser("miscsettings.infobar_sat_display", &g_settings.infobar_sat_display, true );
	oj2->addOption(1, "options.on");
	oj2->addOption(0, "options.off");
	miscSettings.addItem( oj2 );

	keySetupNotifier = new CKeySetupNotifier;
	CStringInput * keySettings_repeat_genericblocker = new CStringInput("keybindingmenu.repeatblockgeneric", g_settings.repeat_blocker, 3, "repeatblocker.hint_1", "repeatblocker.hint_2", "0123456789 ", keySetupNotifier);
	CStringInput * keySettings_repeatBlocker = new CStringInput("keybindingmenu.repeatblock", g_settings.repeat_genericblocker, 3, "repeatblocker.hint_1", "repeatblocker.hint_2", "0123456789 ", keySetupNotifier);
	keySetupNotifier->changeNotify("initial", NULL);

	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.RC") );
	miscSettings.addItem( new CMenuForwarder("keybindingmenu.repeatblock", true, g_settings.repeat_genericblocker, keySettings_repeatBlocker ));
 	miscSettings.addItem( new CMenuForwarder("keybindingmenu.repeatblockgeneric", true, g_settings.repeat_blocker, keySettings_repeat_genericblocker ));
	miscSettings.addItem( m1 );
}


void CNeutrinoApp::InitLanguageSettings(CMenuWidget &languageSettings)
{
	languageSettings.addItem(&GenericMenuSeparator);
	languageSettings.addItem(&GenericMenuBack);
	languageSettings.addItem(&GenericMenuSeparatorLine);

	languageSetupNotifier = new CLanguageSetupNotifier;
	CMenuOptionStringChooser* oj = new CMenuOptionStringChooser("languagesetup.select", (char*)&g_settings.language, true, languageSetupNotifier, false);
	//search available languages....

	struct dirent **namelist;
	int n;
	//		printf("scanning locale dir now....(perhaps)\n");

	char *pfad[] = {DATADIR "/neutrino/locale","/var/tuxbox/config/locale"};
	std::string filen, locale;
	int pos;

	for(int p = 0;p < 2;p++)
	{
		n = scandir(pfad[p], &namelist, 0, alphasort);
		if(n < 0)
		{
			perror("loading locales: scandir");
		}
		else
		{
			for(int count=0;count<n;count++)
			{
				filen = namelist[count]->d_name;
				pos = filen.find(".locale");
				if(pos!=-1)
				{
					locale = filen.substr(0,pos);
					oj->addOption( locale );
				}
				free(namelist[count]);
			}
			free(namelist);
		}
	}
	languageSettings.addItem( oj );
}

void CNeutrinoApp::InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier)
{
	dprintf(DEBUG_DEBUG, "init audiosettings\n");
	audioSettings.addItem(&GenericMenuSeparator);
	audioSettings.addItem(&GenericMenuBack);
	audioSettings.addItem(&GenericMenuSeparatorLine);

	CMenuOptionChooser* oj = new CMenuOptionChooser("audiomenu.analogout", &g_settings.audio_AnalogMode, true, audioSetupNotifier);
	oj->addOption(0, "audiomenu.stereo");
	oj->addOption(1, "audiomenu.monoleft");
	oj->addOption(2, "audiomenu.monoright");

	audioSettings.addItem( oj );
	oj = new CMenuOptionChooser("audiomenu.dolbydigital", &g_settings.audio_DolbyDigital, true, audioSetupNotifier);
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	audioSettings.addItem( oj );

	oj = new CMenuOptionChooser("audiomenu.avs_control", &g_settings.audio_avs_Control, true, audioSetupNotifier);
	oj->addOption(0, "audiomenu.ost");
	oj->addOption(1, "audiomenu.avs");
	oj->addOption(2, "audiomenu.lirc");
	audioSettings.addItem( oj );

}

void CNeutrinoApp::InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier* videoSetupNotifier)
{
	dprintf(DEBUG_DEBUG, "init videosettings\n");
	videoSettings.addItem(&GenericMenuSeparator);
	videoSettings.addItem(&GenericMenuBack);
	videoSettings.addItem(&GenericMenuSeparatorLine);

	CMenuOptionChooser* oj = new CMenuOptionChooser("videomenu.videosignal", &g_settings.video_Signal, true, videoSetupNotifier);
	oj->addOption(1, "videomenu.videosignal_rgb");
	oj->addOption(2, "videomenu.videosignal_svideo");
	oj->addOption(3, "videomenu.videosignal_yuv_v");
	oj->addOption(4, "videomenu.videosignal_yuv_c");
	oj->addOption(0, "videomenu.videosignal_composite");

	videoSettings.addItem( oj );

	oj = new CMenuOptionChooser("videomenu.videoformat", &g_settings.video_Format, true, videoSetupNotifier);
	oj->addOption(2, "videomenu.videoformat_43");
	oj->addOption(3, "videomenu.videoformat_431");
	oj->addOption(1, "videomenu.videoformat_169");
	oj->addOption(0, "videomenu.videoformat_autodetect");

	if(g_settings.video_Format==0) // autodetect has to be initialized
	{
		videoSetupNotifier->changeNotify("videomenu.videoformat", NULL);
	}

	videoSettings.addItem( oj );

	oj = new CMenuOptionChooser("videomenu.vcrswitch", &g_settings.vcr_AutoSwitch, true, NULL);
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	videoSettings.addItem( oj );

	videoSettings.addItem(&GenericMenuSeparatorLine);
	videoSettings.addItem( new CMenuForwarder("videomenu.screensetup", true, NULL, new CScreenSetup()));
}

void CNeutrinoApp::InitParentalLockSettings(CMenuWidget &parentallockSettings)
{
	dprintf(DEBUG_DEBUG, "init parentallocksettings\n");
	parentallockSettings.addItem(&GenericMenuSeparator);
	parentallockSettings.addItem(&GenericMenuBack);
	parentallockSettings.addItem(&GenericMenuSeparatorLine);

	CMenuOptionChooser* oj = new CMenuOptionChooser("parentallock.prompt", &g_settings.parentallock_prompt, true);
	oj->addOption(PARENTALLOCK_PROMPT_NEVER         , "parentallock.never");
//	oj->addOption(PARENTALLOCK_PROMPT_ONSTART       , "parentallock.onstart");
	oj->addOption(PARENTALLOCK_PROMPT_CHANGETOLOCKED, "parentallock.changetolocked");
	oj->addOption(PARENTALLOCK_PROMPT_ONSIGNAL      , "parentallock.onsignal");
	parentallockSettings.addItem( oj );

	oj = new CMenuOptionChooser("parentallock.lockage", &g_settings.parentallock_lockage, true);
	oj->addOption(12, "parentallock.lockage12");
	oj->addOption(16, "parentallock.lockage16");
	oj->addOption(18, "parentallock.lockage18");
	parentallockSettings.addItem( oj );

	CPINChangeWidget * pinChangeWidget = new CPINChangeWidget("parentallock.changepin", g_settings.parentallock_pincode, 4, "parentallock.changepin_hint1", NULL);
	parentallockSettings.addItem( new CMenuForwarder("parentallock.changepin", true, g_settings.parentallock_pincode, pinChangeWidget));
}

void CNeutrinoApp::InitNetworkSettings(CMenuWidget &networkSettings)
{
	dprintf(DEBUG_DEBUG, "init networksettings\n");
	networkSettings.addItem(&GenericMenuSeparator);
	networkSettings.addItem(&GenericMenuBack);
	networkSettings.addItem(&GenericMenuSeparatorLine);

	network_automatic_start = networkConfig.automatic_start ? 1 : 0;
	CMenuOptionChooser* oj = new CMenuOptionChooser("networkmenu.setuponstartup", &network_automatic_start, true);
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");

	networkSettings.addItem( oj );
	networkSettings.addItem( new CMenuForwarder("networkmenu.test", true, NULL, this, "networktest"));
	networkSettings.addItem( new CMenuForwarder("networkmenu.show", true, NULL, this, "networkshow"));
	CMenuForwarder *m0 = new CMenuForwarder("networkmenu.setupnow", true, NULL, this, "network");
	networkSettings.addItem( m0 );

	networkSettings.addItem(&GenericMenuSeparatorLine);

	CIPInput * networkSettings_NetworkIP  = new CIPInput("networkmenu.ipaddress" , networkConfig.address   , "ipsetup.hint_1", "ipsetup.hint_2", MyIPChanger);
	CIPInput * networkSettings_NetMask    = new CIPInput("networkmenu.netmask"   , networkConfig.netmask   , "ipsetup.hint_1", "ipsetup.hint_2");
	CIPInput * networkSettings_Broadcast  = new CIPInput("networkmenu.broadcast" , networkConfig.broadcast , "ipsetup.hint_1", "ipsetup.hint_2");
	CIPInput * networkSettings_Gateway    = new CIPInput("networkmenu.gateway"   , networkConfig.gateway   , "ipsetup.hint_1", "ipsetup.hint_2");
	CIPInput * networkSettings_NameServer = new CIPInput("networkmenu.nameserver", networkConfig.nameserver, "ipsetup.hint_1", "ipsetup.hint_2");

	CMenuForwarder *m1 = new CMenuForwarder("networkmenu.ipaddress", networkConfig.inet_static, networkConfig.address, networkSettings_NetworkIP );
	CMenuForwarder *m2 = new CMenuForwarder("networkmenu.netmask", networkConfig.inet_static, networkConfig.netmask, networkSettings_NetMask);
	CMenuForwarder *m3 = new CMenuForwarder("networkmenu.broadcast", networkConfig.inet_static, networkConfig.broadcast, networkSettings_Broadcast );
	CMenuForwarder *m4 = new CMenuForwarder("networkmenu.gateway", networkConfig.inet_static, networkConfig.gateway, networkSettings_Gateway );
	CMenuForwarder *m5 = new CMenuForwarder("networkmenu.nameserver", networkConfig.inet_static, networkConfig.nameserver, networkSettings_NameServer );

	CDHCPNotifier* dhcpNotifier = new CDHCPNotifier(m1,m2,m3,m4,m5);

	network_dhcp = networkConfig.inet_static ? 0 : 1;
	oj = new CMenuOptionChooser("dhcp", &network_dhcp, true, dhcpNotifier);
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	networkSettings.addItem( oj );
	networkSettings.addItem(&GenericMenuSeparatorLine);

	networkSettings.addItem( m1);
	networkSettings.addItem( m2);
	networkSettings.addItem( m3);

	networkSettings.addItem(&GenericMenuSeparatorLine);
	networkSettings.addItem( m4);
	networkSettings.addItem( m5);
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "NFS/CIFS") );
	networkSettings.addItem( new CMenuForwarder("nfs.mount", true, NULL, new CNFSMountGui()));
	networkSettings.addItem( new CMenuForwarder("nfs.umount", true, NULL, new CNFSUmountGui()));
}

void CNeutrinoApp::InitRecordingSettings(CMenuWidget &recordingSettings)
{
	CIPInput * recordingSettings_server_ip = new CIPInput("recordingmenu.server_ip", g_settings.recording_server_ip, "ipsetup.hint_1", "ipsetup.hint_2");
	CStringInput * recordingSettings_server_port = new CStringInput("recordingmenu.server_port", g_settings.recording_server_port, 6, "ipsetup.hint_1", "ipsetup.hint_2", "0123456789 ");

	CMenuForwarder* mf1 = new CMenuForwarder("recordingmenu.server_ip", (g_settings.recording_type==1), g_settings.recording_server_ip,recordingSettings_server_ip);
	CMenuForwarder* mf2 = new CMenuForwarder("recordingmenu.server_port", (g_settings.recording_type==1), g_settings.recording_server_port,recordingSettings_server_port);

	CMACInput *recordingSettings_server_mac= new CMACInput("recordingmenu.server_mac",  g_settings.recording_server_mac, "ipsetup.hint_1", "ipsetup.hint_2");
	CMenuForwarder *mf3 = new CMenuForwarder("recordingmenu.server_mac", (g_settings.recording_type==1 && g_settings.recording_server_wakeup==1),   g_settings.recording_server_mac,
                                            recordingSettings_server_mac);
	CRecordingNotifier2 *RecordingNotifier2 =
      new CRecordingNotifier2(mf3);
	CMenuOptionChooser*	oj2 = new CMenuOptionChooser("recordingmenu.server_wakeup", &g_settings.recording_server_wakeup,
																	  (g_settings.recording_type==1), RecordingNotifier2);
	oj2->addOption(0, "options.off");
	oj2->addOption(1, "options.on");


	CMenuOptionChooser* oj3 = new CMenuOptionChooser("recordingmenu.stopplayback", &g_settings.recording_stopplayback, (g_settings.recording_type==1));
	oj3->addOption(0, "options.off");
	oj3->addOption(1, "options.on");

	CMenuOptionChooser* oj4 = new CMenuOptionChooser("recordingmenu.stopsectionsd", &g_settings.recording_stopsectionsd, (g_settings.recording_type==1));
	oj4->addOption(0, "options.off");
	oj4->addOption(1, "options.on");

	CMenuOptionChooser* oj5 = new CMenuOptionChooser("recordingmenu.no_scart", &g_settings.recording_vcr_no_scart, (g_settings.recording_type==2));
	oj5->addOption(0, "options.off");
	oj5->addOption(1, "options.on");

	CStringInput * timerSettings_record_safety_time_before = new CStringInput("timersettings.record_safety_time_before", g_settings.record_safety_time_before, 2, "timersettings.record_safety_time_before.hint_1", "timersettings.record_safety_time_before.hint_2","0123456789 ");
	CMenuForwarder *mf5 = new CMenuForwarder("timersettings.record_safety_time_before", true, g_settings.record_safety_time_before, timerSettings_record_safety_time_before );

	CStringInput * timerSettings_record_safety_time_after = new CStringInput("timersettings.record_safety_time_after", g_settings.record_safety_time_after, 2, "timersettings.record_safety_time_after.hint_1", "timersettings.record_safety_time_after.hint_2","0123456789 ");
	CMenuForwarder *mf6 = new CMenuForwarder("timersettings.record_safety_time_after", true, g_settings.record_safety_time_after, timerSettings_record_safety_time_after );

	CRecordingNotifier *RecordingNotifier =
		new CRecordingNotifier(mf1,mf2,oj2,mf3,oj3,oj4,oj5);

    CMenuOptionChooser* oj1 = new CMenuOptionChooser("recordingmenu.recording_type", &g_settings.recording_type,
                                                    true, RecordingNotifier);
	oj1->addOption(0, "recordingmenu.off");
	oj1->addOption(1, "recordingmenu.server");
	oj1->addOption(2, "recordingmenu.vcr");

	recordingSettings.addItem(&GenericMenuSeparator);
	recordingSettings.addItem(&GenericMenuBack);
	recordingSettings.addItem( new CMenuForwarder("recordingmenu.setupnow", true, NULL, this, "recording"));
	recordingSettings.addItem(&GenericMenuSeparatorLine);
	recordingSettings.addItem( oj1);
	recordingSettings.addItem(&GenericMenuSeparatorLine);
	recordingSettings.addItem( mf1);
	recordingSettings.addItem( mf2);
	recordingSettings.addItem( oj2);
	recordingSettings.addItem( mf3);
	recordingSettings.addItem( oj3 );
	recordingSettings.addItem( oj4);
	recordingSettings.addItem(&GenericMenuSeparatorLine);
	recordingSettings.addItem( oj5);
	recordingSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "timersettings.separator") );
	recordingSettings.addItem( mf5);
	recordingSettings.addItem( mf6);

	recordingstatus = 0;
}

void CNeutrinoApp::InitStreamingSettings(CMenuWidget &streamingSettings)
{
	CIPInput * streamingSettings_server_ip = new CIPInput("streamingmenu.server_ip",  g_settings.streaming_server_ip, "ipsetup.hint_1", "ipsetup.hint_2");
	CStringInput * streamingSettings_server_port = new CStringInput("streamingmenu.server_port", g_settings.streaming_server_port, 6, "ipsetup.hint_1", "ipsetup.hint_2","0123456789 ");
 	CStringInputSMS * cddriveInput = new CStringInputSMS("streamingmenu.streaming_server_cddrive", g_settings.streaming_server_cddrive, 20, NULL, NULL, "abcdefghijklmnopqrstuvwxyz0123456789!""�$%&/()=?-:\\ ");
	CStringInput * streamingSettings_videorate = new CStringInput("streamingmenu.streaming_videorate", g_settings.streaming_videorate, 5, "ipsetup.hint_1", "ipsetup.hint_2","0123456789 ");
	CStringInput * streamingSettings_audiorate = new CStringInput("streamingmenu.streaming_audiorate", g_settings.streaming_audiorate, 5, "ipsetup.hint_1", "ipsetup.hint_2","0123456789 ");
	CStringInputSMS * startdirInput = new CStringInputSMS("streamingmenu.streaming_server_startdir", g_settings.streaming_server_startdir, 30, NULL, NULL,"abcdefghijklmnopqrstuvwxyz0123456789!""�$%&/()=?-:\\ ");

	CMenuForwarder* mf1 = new CMenuForwarder("streamingmenu.server_ip", (g_settings.streaming_type==1), g_settings.streaming_server_ip,streamingSettings_server_ip);
	CMenuForwarder* mf2 = new CMenuForwarder("streamingmenu.server_port", (g_settings.streaming_type==1), g_settings.streaming_server_port,streamingSettings_server_port);
	CMenuForwarder* mf3 = new CMenuForwarder("streamingmenu.streaming_server_cddrive", (g_settings.streaming_type==1), g_settings.streaming_server_cddrive,cddriveInput);
	CMenuForwarder* mf4 = new CMenuForwarder("streamingmenu.streaming_videorate", (g_settings.streaming_type==1), g_settings.streaming_videorate,streamingSettings_videorate);
	CMenuForwarder* mf5 = new CMenuForwarder("streamingmenu.streaming_audiorate", (g_settings.streaming_type==1), g_settings.streaming_audiorate,streamingSettings_audiorate);
	CMenuForwarder* mf6 = new CMenuForwarder("streamingmenu.streaming_server_startdir", (g_settings.streaming_type==1), g_settings.streaming_server_startdir,startdirInput);

	CMenuForwarder* mf7 = new CMenuForwarder("movieplayer.defdir", true, g_settings.network_nfs_moviedir,this,"moviedir");
	
	CMenuOptionChooser* oj0 = new CMenuOptionChooser("streamingmenu.streaming_ac3_enabled", &g_settings.streaming_ac3_enabled,
                                                    true, NULL);
	oj0->addOption(0, "streamingmenu.off");
	oj0->addOption(1, "streamingmenu.on");
                                              
	CStreamingNotifier *StreamingNotifier =
		new CStreamingNotifier(mf1,mf2,mf3,mf4,mf5,mf6,oj0);

        CMenuOptionChooser* oj1 = new CMenuOptionChooser("streamingmenu.streaming_type", &g_settings.streaming_type,
                                                    true, StreamingNotifier);
	oj1->addOption(0, "streamingmenu.off");
	oj1->addOption(1, "streamingmenu.on");

      


	streamingSettings.addItem(&GenericMenuSeparator);
	streamingSettings.addItem(&GenericMenuBack);
	streamingSettings.addItem(&GenericMenuSeparatorLine);
	streamingSettings.addItem( oj1);
	streamingSettings.addItem(&GenericMenuSeparatorLine);
	streamingSettings.addItem( mf1);
	streamingSettings.addItem( mf2);
	streamingSettings.addItem( mf3);
	streamingSettings.addItem( mf4);
	streamingSettings.addItem( mf5);
	streamingSettings.addItem( mf6);
	streamingSettings.addItem( oj0);
	streamingSettings.addItem(&GenericMenuSeparatorLine);
	streamingSettings.addItem( mf7);
	//streamingSettings.addItem( new CMenuForwarder("movieplayer.defdir", true, g_settings.network_nfs_moviedir, 
	//														 this, "moviedir"));

}

void CNeutrinoApp::AddFontSettingItem(CMenuWidget &fontSettings, const char * const menuname, char *value)
{
	CStringInput * fontSize = new CStringInput(menuname, value, 3, "ipsetup.hint_1", "ipsetup.hint_2", "0123456789 ", this);
	fontSettings.addItem( new CMenuForwarder(menuname, true, value ,fontSize));
}

void CNeutrinoApp::InitFontSettings(CMenuWidget &fontSettings,CMenuWidget &fontSettings_Channellist , CMenuWidget &fontSettings_Eventlist , CMenuWidget &fontSettings_Infobar ,CMenuWidget &fontSettings_Epg, CMenuWidget &fontSettings_Gamelist )
{
	fontSettings_Epg.addItem(&GenericMenuSeparator);
	fontSettings_Epg.addItem(&GenericMenuBack);
	fontSettings_Epg.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings_Epg, "fontsize.epg_title", g_settings.fontsize_epg_title);
	AddFontSettingItem(fontSettings_Epg, "fontsize.epg_info1", g_settings.fontsize_epg_info1);
	AddFontSettingItem(fontSettings_Epg, "fontsize.epg_info2", g_settings.fontsize_epg_info2);
	AddFontSettingItem(fontSettings_Epg, "fontsize.epg_date", g_settings.fontsize_epg_date);
	fontSettings_Epg.addItem(&GenericMenuSeparatorLine);
	fontSettings_Epg.addItem( new CMenuForwarder("options.default", true, NULL, this, "fontsize.def1", this) );

	fontSettings_Eventlist.addItem(&GenericMenuSeparator);
	fontSettings_Eventlist.addItem(&GenericMenuBack);
	fontSettings_Eventlist.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings_Eventlist, "fontsize.eventlist_title", g_settings.fontsize_eventlist_title);
	AddFontSettingItem(fontSettings_Eventlist, "fontsize.eventlist_itemlarge", g_settings.fontsize_eventlist_itemlarge);
	AddFontSettingItem(fontSettings_Eventlist, "fontsize.eventlist_itemsmall", g_settings.fontsize_eventlist_itemsmall);
	AddFontSettingItem(fontSettings_Eventlist, "fontsize.eventlist_datetime", g_settings.fontsize_eventlist_datetime);
	fontSettings_Eventlist.addItem(&GenericMenuSeparatorLine);
	fontSettings_Eventlist.addItem( new CMenuForwarder("options.default", true, NULL, this, "fontsize.def2", this) );

	fontSettings_Gamelist.addItem(&GenericMenuSeparator);
	fontSettings_Gamelist.addItem(&GenericMenuBack);
	fontSettings_Gamelist.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings_Gamelist, "fontsize.gamelist_itemLarge", g_settings.fontsize_gamelist_itemlarge);
	AddFontSettingItem(fontSettings_Gamelist, "fontsize.gamelist_itemSmall", g_settings.fontsize_gamelist_itemsmall);
	fontSettings_Gamelist.addItem(&GenericMenuSeparatorLine);
	fontSettings_Gamelist.addItem( new CMenuForwarder("options.default", true, NULL, this, "fontsize.def3", this) );

	fontSettings_Channellist.addItem(&GenericMenuSeparator);
	fontSettings_Channellist.addItem(&GenericMenuBack);
	fontSettings_Channellist.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings_Channellist, "fontsize.channellist", g_settings.fontsize_channellist);
	AddFontSettingItem(fontSettings_Channellist, "fontsize.channellist_descr", g_settings.fontsize_channellist_descr);
	AddFontSettingItem(fontSettings_Channellist, "fontsize.channellist_number", g_settings.fontsize_channellist_number);
	AddFontSettingItem(fontSettings_Channellist, "fontsize.channel_num_zap", g_settings.fontsize_channel_num_zap);
	fontSettings_Channellist.addItem(&GenericMenuSeparatorLine);
	fontSettings_Channellist.addItem( new CMenuForwarder("options.default", true, NULL, this, "fontsize.def4", this) );

	fontSettings_Infobar.addItem(&GenericMenuSeparator);
	fontSettings_Infobar.addItem(&GenericMenuBack);
	fontSettings_Infobar.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings_Infobar, "fontsize.infobar_number", g_settings.fontsize_infobar_number);
	AddFontSettingItem(fontSettings_Infobar, "fontsize.infobar_channame", g_settings.fontsize_infobar_channame);
	AddFontSettingItem(fontSettings_Infobar, "fontsize.infobar_info", g_settings.fontsize_infobar_info);
	AddFontSettingItem(fontSettings_Infobar, "fontsize.infobar_small", g_settings.fontsize_infobar_small);
	fontSettings_Infobar.addItem(&GenericMenuSeparatorLine);
	fontSettings_Infobar.addItem( new CMenuForwarder("options.default", true, NULL, this, "fontsize.def5", this) );

	fontSettings.addItem(&GenericMenuSeparator);
	fontSettings.addItem(&GenericMenuBack);
	fontSettings.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings, "fontsize.alert", g_settings.fontsize_alert);
	fontSettings.addItem(&GenericMenuSeparatorLine);
	AddFontSettingItem(fontSettings, "fontsize.menu", g_settings.fontsize_menu);
	AddFontSettingItem(fontSettings, "fontsize.menu_title", g_settings.fontsize_menu_title);
	AddFontSettingItem(fontSettings, "fontsize.menu_info", g_settings.fontsize_menu_info);
	fontSettings.addItem(&GenericMenuSeparatorLine);
	fontSettings.addItem( new CMenuForwarder("fontmenu.channellist", true, NULL, &fontSettings_Channellist) );
	fontSettings.addItem( new CMenuForwarder("fontmenu.eventlist", true, NULL, &fontSettings_Eventlist) );
	fontSettings.addItem( new CMenuForwarder("fontmenu.epg", true, NULL, &fontSettings_Epg) );
	fontSettings.addItem( new CMenuForwarder("fontmenu.infobar", true, NULL, &fontSettings_Infobar) );
	fontSettings.addItem( new CMenuForwarder("fontmenu.gamelist", true, NULL, &fontSettings_Gamelist) );
	AddFontSettingItem(fontSettings, "fontsize.filebrowser_item", g_settings.fontsize_filebrowser_item);
	fontSettings.addItem(&GenericMenuSeparatorLine);
	fontSettings.addItem( new CMenuForwarder("options.default", true, NULL, this, "fontsize.def6", this) );
}

void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings, CMenuWidget &fontSettings )
{
	dprintf(DEBUG_DEBUG, "init colorsettings\n");
	colorSettings.addItem(&GenericMenuSeparator);
	colorSettings.addItem(&GenericMenuBack);
	colorSettings.addItem(&GenericMenuSeparatorLine);

	CMenuWidget *colorSettings_Themes = new CMenuWidget("colorthememenu.head", NEUTRINO_ICON_SETTINGS);
	InitColorThemesSettings(*colorSettings_Themes);

	colorSettings.addItem( new CMenuForwarder("colormenu.themeselect", true, NULL, colorSettings_Themes) );
	CMenuWidget *colorSettings_menuColors = new CMenuWidget("colormenusetup.head", NEUTRINO_ICON_SETTINGS, 400, 400);
	InitColorSettingsMenuColors(*colorSettings_menuColors);
	colorSettings.addItem( new CMenuForwarder("colormenu.menucolors", true, NULL, colorSettings_menuColors) );

	CMenuWidget *colorSettings_statusbarColors = new CMenuWidget("colormenu.statusbar", NEUTRINO_ICON_SETTINGS);
	InitColorSettingsStatusBarColors(*colorSettings_statusbarColors);
	colorSettings.addItem( new CMenuForwarder("colorstatusbar.head", true, NULL, colorSettings_statusbarColors) );

	colorSettings.addItem(&GenericMenuSeparatorLine);
	colorSettings.addItem( new CMenuForwarder("colormenu.font", true, NULL, &fontSettings) );
	CMenuWidget *colorSettings_timing = new CMenuWidget("colormenu.timing", NEUTRINO_ICON_SETTINGS);
	InitColorSettingsTiming(*colorSettings_timing);
	colorSettings.addItem( new CMenuForwarder("timing.head", true, NULL, colorSettings_timing));

	colorSettings.addItem(&GenericMenuSeparatorLine);
	if(g_info.box_Type != CControldClient::TUXBOX_MAKER_NOKIA)
	{
		//menuefaden nur bei enx-chips!
		CMenuOptionChooser* oj = new CMenuOptionChooser("colormenu.fade", &g_settings.widget_fade, true );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		colorSettings.addItem( oj );
	} else {
		//alpha-werte nur einstellen wenn gtx-chip
		CAlphaSetup* chAlphaSetup = new CAlphaSetup("colormenu.gtx_alpha", &g_settings.gtx_alpha1, &g_settings.gtx_alpha2, NULL);
		colorSettings.addItem( new CMenuForwarder("colormenu.gtx_alpha", true, NULL, chAlphaSetup));
	}
}


void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &colorSettings_Themes)
{
	dprintf(DEBUG_DEBUG, "init themesettings\n");
	colorSettings_Themes.addItem(&GenericMenuSeparator);
	colorSettings_Themes.addItem(&GenericMenuBack);
	colorSettings_Themes.addItem(&GenericMenuSeparatorLine);
	colorSettings_Themes.addItem( new CMenuForwarder("colorthememenu.neutrino_theme", true, NULL, this, "theme_neutrino") );
	colorSettings_Themes.addItem( new CMenuForwarder("colorthememenu.classic_theme", true, NULL, this, "theme_classic") );
}

void CNeutrinoApp::InitColorSettingsMenuColors(CMenuWidget &colorSettings_menuColors)
{
	dprintf(DEBUG_DEBUG, "init colormenuesettings\n");
	colorSettings_menuColors.addItem(&GenericMenuSeparator);
	colorSettings_menuColors.addItem(&GenericMenuBack);

	CColorChooser* chHeadcolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue,
																  &g_settings.menu_Head_alpha, colorSetupNotifier);
	CColorChooser* chHeadTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue,
																		NULL, colorSetupNotifier);
	CColorChooser* chContentcolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,
																	  &g_settings.menu_Content_alpha, colorSetupNotifier);
	CColorChooser* chContentTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue,
																			NULL, colorSetupNotifier);
	CColorChooser* chContentSelectedcolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,
																				 &g_settings.menu_Content_Selected_alpha, colorSetupNotifier);
	CColorChooser* chContentSelectedTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue,
																					  NULL, colorSetupNotifier);
	CColorChooser* chContentInactivecolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue,
																				 &g_settings.menu_Content_inactive_alpha, colorSetupNotifier);
	CColorChooser* chContentInactiveTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue,
																					  NULL, colorSetupNotifier);
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menuhead") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, NULL, chHeadcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, NULL, chHeadTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menucontent") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, NULL, chContentcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, NULL, chContentTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menucontent_inactive") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, NULL, chContentInactivecolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, NULL, chContentInactiveTextcolor));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menucontent_selected") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, NULL, chContentSelectedcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, NULL, chContentSelectedTextcolor ));
}

void CNeutrinoApp::InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_statusbarColors)
{
	dprintf(DEBUG_DEBUG, "init colorstatusbarsettings\n");
	colorSettings_statusbarColors.addItem(&GenericMenuSeparator);

	colorSettings_statusbarColors.addItem(&GenericMenuBack);

	CColorChooser* chInfobarcolor = new CColorChooser("colormenu.background_head", &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,
																	  &g_settings.infobar_alpha, colorSetupNotifier);
	CColorChooser* chInfobarTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue,
																			NULL, colorSetupNotifier);

	colorSettings_statusbarColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colorstatusbar.text") );
	colorSettings_statusbarColors.addItem( new CMenuForwarder("colormenu.background", true, NULL, chInfobarcolor ));
	colorSettings_statusbarColors.addItem( new CMenuForwarder("colormenu.textcolor", true, NULL, chInfobarTextcolor ));
}

void CNeutrinoApp::InitColorSettingsTiming(CMenuWidget &colorSettings_timing)
{
	dprintf(DEBUG_DEBUG, "init colorsettingstiming\n");
	sprintf(g_settings.timing_menu_string,"%d",g_settings.timing_menu);
	sprintf(g_settings.timing_chanlist_string,"%d",g_settings.timing_chanlist);
	sprintf(g_settings.timing_menu_string,"%d",g_settings.timing_menu);
	sprintf(g_settings.timing_menu_string,"%d",g_settings.timing_menu);
	sprintf(g_settings.timing_filebrowser_string,"%d",g_settings.timing_filebrowser);

	colorSettings_timing.addItem(&GenericMenuSeparator);
	colorSettings_timing.addItem(&GenericMenuBack);
	colorSettings_timing.addItem(&GenericMenuSeparatorLine);

	CStringInput * colorSettings_timing_menu = new CStringInput("timing.menu", g_settings.timing_menu_string, 3, "timing.hint_1", "timing.hint_2", "0123456789 ", this);
	colorSettings_timing.addItem( new CMenuForwarder("timing.menu", true, g_settings.timing_menu_string, colorSettings_timing_menu ) );

	CStringInput * colorSettings_timing_chanlist = new CStringInput("timing.chanlist", g_settings.timing_chanlist_string, 3, "timing.hint_1", "timing.hint_2", "0123456789 ", this);
	colorSettings_timing.addItem( new CMenuForwarder("timing.chanlist", true, g_settings.timing_chanlist_string, colorSettings_timing_chanlist ) );

	CStringInput * colorSettings_timing_epg = new CStringInput("timing.epg", g_settings.timing_epg_string, 3, "timing.hint_1", "timing.hint_2", "0123456789 ", this);
	colorSettings_timing.addItem( new CMenuForwarder("timing.epg", true, g_settings.timing_epg_string, colorSettings_timing_epg ) );

	CStringInput * colorSettings_timing_infobar = new CStringInput("timing.infobar", g_settings.timing_infobar_string, 3, "timing.hint_1", "timing.hint_2", "0123456789 ", this);
	colorSettings_timing.addItem( new CMenuForwarder("timing.infobar", true,  g_settings.timing_infobar_string, colorSettings_timing_infobar ) );

	CStringInput * colorSettings_timing_filebrowser = new CStringInput("timing.filebrowser", g_settings.timing_filebrowser_string, 3, "timing.hint_1", "timing.hint_2", "0123456789 ", this);
	colorSettings_timing.addItem( new CMenuForwarder("timing.filebrowser", true,  g_settings.timing_filebrowser_string, colorSettings_timing_filebrowser ) );

	colorSettings_timing.addItem(&GenericMenuSeparatorLine);
	colorSettings_timing.addItem( new CMenuForwarder("options.default", true, NULL, this, "osd.def"));
}

void CNeutrinoApp::InitLcdSettings(CMenuWidget &lcdSettings)
{
	lcdpower = CLCD::getInstance()->getPower()?1:0;
	static int lcdinverse = CLCD::getInstance()->getInverse()?1:0;
	static int lcdautodimm = CLCD::getInstance()->getAutoDimm()?1:0;

	dprintf(DEBUG_DEBUG, "init lcdsettings\n");
	lcdSettings.addItem(&GenericMenuSeparator);

	lcdSettings.addItem(&GenericMenuBack);
	lcdSettings.addItem(&GenericMenuSeparatorLine);

	CLcdControler* lcdsliders = new CLcdControler("lcdmenu.head", NULL);

	CLcdNotifier* lcdnotifier = new CLcdNotifier(&lcdpower, &lcdinverse, &lcdautodimm);

	CMenuOptionChooser* oj = new CMenuOptionChooser("lcdmenu.inverse", &lcdinverse, true, lcdnotifier );
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	lcdSettings.addItem( oj );

	oj = new CMenuOptionChooser("lcdmenu.power", &lcdpower, true, lcdnotifier );
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	lcdSettings.addItem( oj );

	if(g_info.box_Type != CControldClient::TUXBOX_MAKER_NOKIA)
	{
		// Autodimm only on Sagem/Philips available
		oj = new CMenuOptionChooser("lcdmenu.autodimm", &lcdautodimm, true, lcdnotifier );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		lcdSettings.addItem( oj );
	}

	lcdSettings.addItem(&GenericMenuSeparatorLine);
	lcdSettings.addItem( new CMenuForwarder("lcdmenu.lcdcontroler", true, NULL, lcdsliders));

	lcdSettings.addItem(&GenericMenuSeparatorLine);
	oj = new CMenuOptionChooser("lcdmenu.statusline", &g_settings.lcd_show_volume, true );
	oj->addOption(0, "lcdmenu.statusline.playtime");
	oj->addOption(1, "lcdmenu.statusline.volume");
	oj->addOption(2, "lcdmenu.statusline.both");
	lcdSettings.addItem( oj );
}

void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	dprintf(DEBUG_DEBUG, "init keysettings\n");
	keySettings.addItem(&GenericMenuSeparator);
	keySettings.addItem(&GenericMenuBack);

	CKeyChooser*   keySettings_tvradio_mode = new CKeyChooser(&g_settings.key_tvradio_mode, "keybindingmenu.tvradiomode_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_channelList_pageup = new CKeyChooser(&g_settings.key_channelList_pageup, "keybindingmenu.pageup_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_channelList_pagedown = new CKeyChooser(&g_settings.key_channelList_pagedown, "keybindingmenu.pagedown_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_channelList_cancel = new CKeyChooser(&g_settings.key_channelList_cancel, "keybindingmenu.cancel_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_channelList_sort = new CKeyChooser(&g_settings.key_channelList_sort, "keybindingmenu.sort_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_channelList_addrecord = new CKeyChooser(&g_settings.key_channelList_addrecord, "keybindingmenu.addrecord_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_channelList_addremind = new CKeyChooser(&g_settings.key_channelList_addremind, "keybindingmenu.addremind_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_quickzap_up = new CKeyChooser(&g_settings.key_quickzap_up, "keybindingmenu.channelup_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_quickzap_down = new CKeyChooser(&g_settings.key_quickzap_down, "keybindingmenu.channeldown_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_bouquet_up = new CKeyChooser(&g_settings.key_bouquet_up, "keybindingmenu.bouquetup_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_bouquet_down = new CKeyChooser(&g_settings.key_bouquet_down, "keybindingmenu.bouquetdown_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_subchannel_up = new CKeyChooser(&g_settings.key_subchannel_up, "keybindingmenu.subchannelup_head", NEUTRINO_ICON_SETTINGS);
	CKeyChooser*   keySettings_subchannel_down = new CKeyChooser(&g_settings.key_subchannel_down, "keybindingmenu.subchanneldown_head", NEUTRINO_ICON_SETTINGS);

	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.modechange") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.tvradiomode", true, NULL, keySettings_tvradio_mode));

	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.channellist") );
	CMenuOptionChooser *oj = new CMenuOptionChooser("keybindingmenu.bouquethandling" , &g_settings.bouquetlist_mode, true );
	oj->addOption(0, "keybindingmenu.bouquetchannels_on_ok");
	oj->addOption(1, "keybindingmenu.bouquetlist_on_ok");
	oj->addOption(2, "keybindingmenu.allchannels_on_ok");
	keySettings.addItem( oj );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.pageup", true, NULL, keySettings_channelList_pageup ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.pagedown", true, NULL, keySettings_channelList_pagedown ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.cancel", true, NULL, keySettings_channelList_cancel ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.sort", true, NULL, keySettings_channelList_sort ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.addrecord", true, NULL, keySettings_channelList_addrecord ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.addremind", true, NULL, keySettings_channelList_addremind ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.quickzap") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.channelup", true, NULL, keySettings_quickzap_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.channeldown", true, NULL, keySettings_quickzap_down ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.bouquetup", true, NULL, keySettings_bouquet_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.bouquetdown", true, NULL, keySettings_bouquet_down ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.subchannelup", true, NULL, keySettings_subchannel_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.subchanneldown", true, NULL, keySettings_subchannel_down ));
}

void CNeutrinoApp::SelectNVOD()
{
	if( g_RemoteControl->subChannels.size()> 0 )
	{
		// NVOD/SubService- Kanal!
		CMenuWidget NVODSelector( g_RemoteControl->are_subchannels?"nvodselector.subservice":"nvodselector.head", "video.raw", 350);

		NVODSelector.addItem(&GenericMenuSeparator);

		int count = 0;
		char nvod_id[5];

		for( CSubServiceListSorted::iterator e=g_RemoteControl->subChannels.begin(); e!=g_RemoteControl->subChannels.end(); ++e)
		{
			sprintf(nvod_id, "%d", count);

			if( !g_RemoteControl->are_subchannels )
			{
				char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
				char nvod_s[100];
				struct  tm *tmZeit;

				tmZeit= localtime(&e->startzeit );
				sprintf(nvod_time_a, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

				time_t endtime = e->startzeit+ e->dauer;
				tmZeit= localtime(&endtime);
				sprintf(nvod_time_e, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

				time_t jetzt=time(NULL);
				if(e->startzeit > jetzt)
				{
					int mins=(e->startzeit- jetzt)/ 60;
					sprintf(nvod_time_x, g_Locale->getText("nvod.starting"), mins);
				}
				else
					if( (e->startzeit<= jetzt) && (jetzt < endtime) )
				{
					int proz=(jetzt- e->startzeit)*100/ e->dauer;
					sprintf(nvod_time_x, g_Locale->getText("nvod.proz"), proz);
				}
				else
					nvod_time_x[0]= 0;

				sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
				NVODSelector.addItem(new CMenuForwarder(nvod_s, true, NULL, NVODChanger, nvod_id, false), (count == g_RemoteControl->selected_subchannel));
			}
			else
			{
				NVODSelector.addItem(new CMenuForwarder((Latin1_to_UTF8(e->subservice_name)).c_str(), true, NULL, NVODChanger, nvod_id, false, (uint)(CRCInput::convertDigitToKey(count))), (count == g_RemoteControl->selected_subchannel));
			}

			count++;
		}

		if( g_RemoteControl->are_subchannels )
		{
			NVODSelector.addItem(&GenericMenuSeparatorLine);
			CMenuOptionChooser* oj = new CMenuOptionChooser("nvodselector.directormode", &g_RemoteControl->director_mode, true, NULL, true, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
			oj->addOption(0, "options.off");
			oj->addOption(1, "options.on");
			NVODSelector.addItem( oj );
		}

		NVODSelector.exec(NULL, "");
	}
}

void CNeutrinoApp::SelectAPID()
{
	if( g_RemoteControl->current_PIDs.APIDs.size()> 1 )
	{
		// wir haben APIDs f�r diesen Kanal!

		CMenuWidget APIDSelector("apidselector.head", "audio.raw", 300);
		APIDSelector.addItem(&GenericMenuSeparator);

		for( unsigned int count=0; count<g_RemoteControl->current_PIDs.APIDs.size(); count++ )
		{
			char apid[5];
			sprintf(apid, "%d", count);
			APIDSelector.addItem(new CMenuForwarder(g_RemoteControl->current_PIDs.APIDs[count].desc, true, NULL, APIDChanger, apid, false, (uint)(CRCInput::convertDigitToKey(count + 1))), (count == g_RemoteControl->current_PIDs.PIDs.selected_apid));
		}
		APIDSelector.exec(NULL, "");
	}
}

void CNeutrinoApp::ShowStreamFeatures()
{
	CMenuWidget StreamFeatureSelector("streamfeatures.head", "features.raw", 350);
	StreamFeatureSelector.addItem(&GenericMenuSeparator);

	char id[5];
	int cnt = 0;
	int enabled_count = 0;

	for(unsigned int count=0;count < (unsigned int) g_PluginList->getNumberOfPlugins();count++)
	{
		if (g_PluginList->getType(count)== PLUGIN_TYPE_TOOL)
		{
			// zB vtxt-plugins

			sprintf(id, "%d", count);

			enabled_count++;

#warning TODO: make sure g_PluginList->getName(count) is UTF-8 encoded
			StreamFeatureSelector.addItem( new CMenuForwarder((g_PluginList->getName(count)).c_str(), true, NULL, StreamFeaturesChanger, id, false, (cnt== 0) ? CRCInput::RC_blue : CRCInput::RC_nokey, (cnt== 0)?NEUTRINO_ICON_BUTTON_BLUE:""), (cnt == 0) );
			cnt++;
		}
	}

	if(cnt>0)
	{
		StreamFeatureSelector.addItem(&GenericMenuSeparatorLine);
	}

	sprintf(id, "%d", -1);

	// -- Add Channel to favorites
	StreamFeatureSelector.addItem(new CMenuForwarder("favorites.menueadd", true, NULL, new CFavorites, id, true, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);

	// start/stop recording
	if(g_settings.recording_type > 0)
	{
		CMenuOptionChooser* oj = new CMenuOptionChooser("mainmenu.recording", &recordingstatus, true, this, true, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);
		oj->addOption(0, "mainmenu.recording_start");
		oj->addOption(1, "mainmenu.recording_stop");
		StreamFeatureSelector.addItem( oj );
		StreamFeatureSelector.addItem(&GenericMenuSeparatorLine);
	}
	// -- Timer Liste
	StreamFeatureSelector.addItem(new CMenuForwarder("timerlist.name", true, NULL, new CTimerList(), id, true, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

	// -- Sectionsd pause
	int dummy = g_Sectionsd->getIsScanningActive();
	CMenuOptionChooser* oj = new CMenuOptionChooser("mainmenu.pausesectionsd", &dummy, true, new CPauseSectionsdNotifier );
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	StreamFeatureSelector.addItem( oj );


	// -- Stream Info
	StreamFeatureSelector.addItem(new CMenuForwarder("streamfeatures.info", true, NULL, StreamFeaturesChanger, id, true, CRCInput::RC_help, "help_small.raw"), false);

	StreamFeatureSelector.exec(NULL, "");
}


void CNeutrinoApp::InitZapper()
{

	g_InfoViewer->start();
	g_EpgData->start();

	firstChannel();
	if(firstchannel.mode == 't')
	{
		tvMode();
	}
	else
	{
		radioMode();
	}
}

void CNeutrinoApp::setupRecordingDevice(void)
{
	if(g_settings.recording_type == 1)
	{
		CVCRControl::CServerDeviceInfo * info = new CVCRControl::CServerDeviceInfo;
		int port;
		sscanf(g_settings.recording_server_port, "%d", &port);
		info->ServerAddress = g_settings.recording_server_ip;
		info->ServerPort = port;
		info->StopPlayBack = (g_settings.recording_stopplayback == 1);
		info->StopSectionsd = (g_settings.recording_stopsectionsd == 1);
		info->Name = "ngrab";
		CVCRControl::getInstance()->registerDevice(CVCRControl::DEVICE_SERVER,info);
		delete info;
	}
	else if(g_settings.recording_type == 2)
	{
		CVCRControl::CVCRDeviceInfo * info = new CVCRControl::CVCRDeviceInfo;
		info->Name = "vcr";
		info->SwitchToScart = (g_settings.recording_vcr_no_scart==0);
		CVCRControl::getInstance()->registerDevice(CVCRControl::DEVICE_VCR,info);
      delete info;
	}
   else
   {
      if(CVCRControl::getInstance()->isDeviceRegistered())
         CVCRControl::getInstance()->unregisterDevice();
   }
}

bool CNeutrinoApp::getEnvironment(const char* name, int* value)
{
	char* content = getenv(name);
	if (content == NULL)
	{
		dprintf( DEBUG_NORMAL, "[neutrino] Fatal Error: Environment variable %s not set. Bye.\n\n", name);
		return false;
	}
	sscanf(content, "%x", value);
	return true;
}

int CNeutrinoApp::run(int argc, char **argv)
{
	CmdParser(argc, argv);

	CLCD::getInstance()->init((fontFile + ".ttf").c_str(), fontName.c_str(), false);

	g_info.box_Type = g_Controld->getBoxType();

	dprintf( DEBUG_DEBUG, "[neutrino] box_Type: %d\n", g_info.box_Type);




	int loadSettingsErg = loadSetup();

	//lcd aktualisieren
	CLCD::getInstance()->setlcdparameter();
	CLCD::getInstance()->showVolume(g_Controld->getVolume(g_settings.audio_avs_Control==1));
	CLCD::getInstance()->setMuted(g_Controld->getMute(g_settings.audio_avs_Control==1));

	SetupTiming();

	g_Fonts = new FontsDef;
	SetupFonts();

	frameBuffer->ClearFrameBuffer();

	g_Locale = new CLocaleManager;
	g_RCInput = new CRCInput;

	g_Sectionsd = new CSectionsdClient;
	g_Timerd = new CTimerdClient;

	g_RemoteControl = new CRemoteControl;
	g_EpgData = new CEpgData;
	g_InfoViewer = new CInfoViewer;
	g_EventList = new EventList;

	g_PluginList = new CPlugins;
	g_PluginList->setPluginDir(PLUGINDIR);

#ifdef USEACTIONLOG
	g_ActionLog = new CActionLog;
	g_ActionLog->println("neutrino startup");
#endif


	//printf("\nCNeutrinoApp::run - objects initialized...\n\n");
	g_Locale->loadLocale(g_settings.language);

	colorSetupNotifier   = new CColorSetupNotifier;
	audioSetupNotifier   = new CAudioSetupNotifier;
	videoSetupNotifier   = new CVideoSetupNotifier;
	APIDChanger       = new CAPIDChangeExec;
	UCodeChecker      = new CUCodeCheckExec;
	NVODChanger       = new CNVODChangeExec;
	StreamFeaturesChanger = new CStreamFeaturesChangeExec;
	MyIPChanger       = new CIPChangeNotifier;
	ConsoleDestinationChanger = new CConsoleDestChangeNotifier;

	colorSetupNotifier->changeNotify("initial", NULL);

	CNFSMountGui::automount();

	// setup recording device
	if(g_settings.recording_type > 0)
		setupRecordingDevice();
	channelList = new CChannelList( "channellist.head" );

	dprintf( DEBUG_NORMAL, "menue setup\n");
	//Main settings
	CMenuWidget mainMenu("mainmenu.head", "mainmenue.raw");
	CMenuWidget mainSettings("mainsettings.head", NEUTRINO_ICON_SETTINGS);
	CMenuWidget languageSettings("languagesetup.head", "language.raw");
	CMenuWidget videoSettings("videomenu.head", "video.raw");
	CMenuWidget audioSettings("audiomenu.head", "audio.raw");
	CMenuWidget parentallockSettings("parentallock.parentallock", "lock.raw", 500);
	CMenuWidget networkSettings("networkmenu.head", "network.raw");
	CMenuWidget recordingSettings("recordingmenu.head", "recording.raw");
	CMenuWidget streamingSettings("streamingmenu.head", "streaming.raw");
	CMenuWidget colorSettings("colormenu.head", "colors.raw");
	CMenuWidget fontSettings("fontmenu.head", "colors.raw");
	CMenuWidget lcdSettings("lcdmenu.head", "lcd.raw");
	CMenuWidget keySettings("keybindingmenu.head", "keybinding.raw", 400);
	CMenuWidget miscSettings("miscsettings.head", NEUTRINO_ICON_SETTINGS);
	CMenuWidget mp3picSettings("mp3picsettings.general", NEUTRINO_ICON_SETTINGS);
	CMenuWidget scanSettings("servicemenu.scants", NEUTRINO_ICON_SETTINGS);
	CMenuWidget service("servicemenu.head", NEUTRINO_ICON_SETTINGS);

	CMenuWidget fontSettings_Channellist("fontmenu.channellist", "colors.raw");
	CMenuWidget fontSettings_Eventlist("fontmenu.eventlist", "colors.raw");
	CMenuWidget fontSettings_Infobar("fontmenu.infobar", "colors.raw");
	CMenuWidget fontSettings_Epg("fontmenu.epg", "colors.raw");
	CMenuWidget fontSettings_Gamelist("fontmenu.gamelist", "colors.raw");


	InitMainMenu(mainMenu, mainSettings, audioSettings, parentallockSettings, networkSettings, recordingSettings,
					 colorSettings, lcdSettings, keySettings, videoSettings, languageSettings, miscSettings,
					 service, fontSettings, mp3picSettings, streamingSettings);

	//service
	InitServiceSettings(service, scanSettings);

	//language Setup
	InitLanguageSettings(languageSettings);

	//mp3/picviewer Setup
	InitMp3PicSettings(mp3picSettings);

   	//misc Setup
	InitMiscSettings(miscSettings);
	miscSettings.setOnPaintNotifier(this);

	//audio Setup
	InitAudioSettings(audioSettings, audioSetupNotifier);

	//video Setup
	InitVideoSettings(videoSettings, videoSetupNotifier);
	videoSettings.setOnPaintNotifier(this);

	// Parentallock settings
	InitParentalLockSettings( parentallockSettings);

	// ScanSettings
	InitScanSettings(scanSettings);

	dprintf( DEBUG_NORMAL, "control event register\n");
	g_Controld->registerEvent(CControldClient::EVT_MUTECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VOLUMECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_MODECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VCRCHANGED, 222, NEUTRINO_UDS_NAME);

	dprintf( DEBUG_NORMAL, "sectionsd event register\n");
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_TIMESET, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_GOT_CN_EPG, 222, NEUTRINO_UDS_NAME);

	dprintf( DEBUG_NORMAL, "zapit event register\n");
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_COMPLETE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_FAILED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_SUB_COMPLETE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_SUB_FAILED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_MOTOR, 222, NEUTRINO_UDS_NAME);
#ifndef SKIP_CA_STATUS
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_CA_CLEAR, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_CA_LOCK, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_CA_FTA, 222, NEUTRINO_UDS_NAME);
#endif
	g_Zapit->registerEvent(CZapitClient::EVT_RECORDMODE_ACTIVATED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_RECORDMODE_DEACTIVATED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_COMPLETE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_FAILED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCY, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCYP, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_SATELLITE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_NUM_CHANNELS, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_PROVIDER, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_BOUQUETS_CHANGED, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_SERVICENAME, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_FOUND_A_CHAN, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_FOUND_TV_CHAN, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN, 222, NEUTRINO_UDS_NAME);
 	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_FOUND_DATA_CHAN, 222, NEUTRINO_UDS_NAME);

	dprintf( DEBUG_NORMAL, "timerd event register\n");
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_ON, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_OFF, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_RECORD, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_START, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_STOP, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_REMIND, 222, NEUTRINO_UDS_NAME);

	//ucodes testen
	doChecks();
	//settins
	if(loadSettingsErg==1)
	{
		dprintf(DEBUG_INFO, "config file missing\n");
		saveSetup();
		ShowHintUTF("messagebox.info", g_Locale->getText("settings.noconffile")); // UTF-8
	}
	else if(loadSettingsErg==2)
	{
		dprintf(DEBUG_INFO, "parts of configfile missing\n");
		saveSetup();
		ShowHintUTF("messagebox.info", g_Locale->getText("settings.missingoptionsconffile")); // UTF-8
	}

	CLCD::getInstance()->showServicename("Waiting...");
	//init programm
	InitZapper();

	//network Setup
	InitNetworkSettings(networkSettings);

	//Recording Setup
	InitRecordingSettings(recordingSettings);

	//Recording Setup
	InitStreamingSettings(streamingSettings);

	//font Setup
	InitFontSettings(fontSettings, fontSettings_Channellist, fontSettings_Eventlist, fontSettings_Infobar, fontSettings_Epg, fontSettings_Gamelist);

	//color Setup
	InitColorSettings(colorSettings, fontSettings);

	//LCD Setup
	InitLcdSettings(lcdSettings);

	//keySettings
	InitKeySettings(keySettings);

	current_volume= g_Controld->getVolume(g_settings.audio_avs_Control);
	AudioMute( g_Controld->getMute(g_settings.audio_avs_Control), true );

	//load Pluginlist
	g_PluginList->loadPlugins();

	RealRun(mainMenu);

	ExitRun();

	return 0;
}


void CNeutrinoApp::RealRun(CMenuWidget &mainMenu)
{
	dprintf(DEBUG_NORMAL, "initialized everything\n");
	while( true )
	{
		uint msg; uint data;
		g_RCInput->getMsg(&msg, &data, 100 );	// 10 secs..

		if( ( mode == mode_tv ) || ( ( mode == mode_radio ) ) )
		{
			if( msg == NeutrinoMessages::SHOW_EPG )
			{
				// show EPG

				g_EpgData->show( channelList->getActiveChannel_ChannelID() );

			}
			else if( msg == (uint) g_settings.key_tvradio_mode )
			{
				if( mode == mode_tv )
				{
					radioMode();
				}
				else if( mode == mode_radio )
				{
					tvMode();
				}
			}
			else if( msg == CRCInput::RC_setup )
			{
				mainMenu.exec(NULL, "");
			}
			if( msg == CRCInput::RC_ok)
			{
				int bouqMode = g_settings.bouquetlist_mode;//bsmChannels;

				if((bouquetList!=NULL) && (bouquetList->Bouquets.size() == 0 ))
				{
					dprintf(DEBUG_DEBUG, "bouquets are empty\n");
					bouqMode = bsmAllChannels;
				}
				if((bouquetList!=NULL) && (bouqMode == 1/*bsmBouquets*/))
				{
					bouquetList->exec(true);
				}
				else if((bouquetList!=NULL) && (bouqMode == 0/*bsmChannels*/))
				{
					int nNewChannel = bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->show();
					if(nNewChannel>-1)
					{
						channelList->zapTo(bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->getKey(nNewChannel)-1);
					}
				}
				else
				{
					dprintf(DEBUG_DEBUG, "using all channels\n");
					channelList->exec();
				}
			}
			else if( msg == CRCInput::RC_red )
			{	// eventlist
				g_EventList->exec(channelList->getActiveChannel_ChannelID(), channelList->getActiveChannelName()); // UTF-8
			}
			else if( msg == CRCInput::RC_blue )
			{	// streaminfo
				ShowStreamFeatures();
			}
			else if( msg == CRCInput::RC_green )
			{	// APID
				SelectAPID();
			}
			else if( msg == CRCInput::RC_yellow )
			{	// NVODs
				SelectNVOD();
			}
			else if( ( msg == (uint) g_settings.key_quickzap_up ) || ( msg == (uint) g_settings.key_quickzap_down ) )
			{
				//quickzap
				channelList->quickZap( msg );
			}
			else if( ( msg == CRCInput::RC_help ) ||
						( msg == NeutrinoMessages::SHOW_INFOBAR ) )
			{
				// show Infoviewer
				g_InfoViewer->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID()); // UTF-8
			}
			else if (CRCInput::isNumeric(msg))
			{ //numeric zap
				if( g_RemoteControl->director_mode )
				{
					g_RemoteControl->setSubChannel(msg);
					g_InfoViewer->showSubchan();
				}
				else
					channelList->numericZap( msg );
			}
			else if( msg == (uint) g_settings.key_subchannel_up )
			{
				g_RemoteControl->subChannelUp();
				g_InfoViewer->showSubchan();
			}
			else if( msg == (uint) g_settings.key_subchannel_down )
			{
				g_RemoteControl->subChannelDown();
				g_InfoViewer->showSubchan();
			}
			else
			{
				handleMsg( msg, data );
			}

		}
		else
		{
			// mode == mode_scart
			if( msg == CRCInput::RC_home )
			{
				if( mode == mode_scart )
				{
					//wenn Aufnahme dann stoppen
					if(CVCRControl::getInstance()->isDeviceRegistered())
					{
						if(CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD || CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE)
						{
							CVCRControl::getInstance()->Stop();
							recordingstatus=0;
							startNextRecording();
						}
					}
					// Scart-Mode verlassen
					scartMode( false );
				}
			}
			else
			{
				handleMsg( msg, data );
			}
		}
	}
}

void CNeutrinoApp::showProfiling(std::string text)
{
	struct timeval tv;

	gettimeofday(&tv, NULL );
	long long now = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);


	//printf("--> '%s' %f\n", text.c_str(), (now- last_profile_call)/ 1000.);
	last_profile_call = now;
}

int CNeutrinoApp::handleMsg(uint msg, uint data)
{
	int res = 0;

	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
	res = res | channelList->handleMsg(msg, data);

	if( res != messages_return::unhandled )
	{
		if( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
			delete (unsigned char*) data;
		return( res & ( 0xFFFFFFFF - messages_return::unhandled ) );
	}

	if( msg == NeutrinoMessages::EVT_VCRCHANGED )
	{
		if( g_settings.vcr_AutoSwitch == 1 )
		{
			if( data != VCR_STATUS_OFF )
				g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
			else
				g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
		}
		return messages_return::handled | messages_return::cancel_info;
	}
	else if( msg == CRCInput::RC_standby )
	{
		if (data == 0)
		{
			// trigger StandBy
			struct timeval tv;
			gettimeofday(&tv, NULL );
			standby_pressed_at = (tv.tv_sec*1000000) + tv.tv_usec;

			if( mode == mode_standby )
			{
				g_RCInput->postMsg( NeutrinoMessages::STANDBY_OFF, 0 );
			}
			else if( !g_settings.shutdown_real && !g_settings.shutdown_real_rcdelay )
			{
				g_RCInput->postMsg( NeutrinoMessages::STANDBY_ON, 0 );
			}
			else if( !g_settings.shutdown_real )
			{
				int timeout = 5;
				int timeout1 = 5;

				sscanf(g_settings.repeat_blocker, "%d", &timeout);
				timeout = int(timeout/100.0) + 5;
				sscanf(g_settings.repeat_genericblocker, "%d", &timeout1);
				timeout1 = int(timeout1/100.0) + 5;
				if(timeout1>timeout)
					timeout=timeout1;

				uint msg; uint data;
				int diff = 0;
				long long endtime;

				do
				{
					g_RCInput->getMsg(&msg, &data, timeout );

					if( msg != CRCInput::RC_timeout )
					{
						gettimeofday(&tv, NULL );
						endtime = (tv.tv_sec*1000000) + tv.tv_usec;
						diff = int((endtime - standby_pressed_at)/100000. );
					}

				} while( ( msg != CRCInput::RC_timeout ) && ( diff < 10 ) );

				g_RCInput->postMsg( ( diff >= 10 ) ? NeutrinoMessages::SHUTDOWN : NeutrinoMessages::STANDBY_ON, 0 );
			}
			else
			{
				g_RCInput->postMsg( NeutrinoMessages::SHUTDOWN, 0 );
			}
			return messages_return::cancel_all | messages_return::handled;
		}
		else /* data == 1: Standby button released */
		{
			struct timeval tv;
			gettimeofday(&tv, NULL );
			long long endtime = (tv.tv_sec*1000000) + tv.tv_usec;
			int diff = int((endtime - standby_pressed_at)/100000. );
			if( diff >= 10 )
			{
				g_RCInput->postMsg( NeutrinoMessages::SHUTDOWN, 0 );
				return messages_return::cancel_all | messages_return::handled;
			}
		}
	}
	else if( msg == CRCInput::RC_plus ||
		 msg == CRCInput::RC_minus )
	{
		//volume
		setVolume( msg, ( mode != mode_scart ) );
		return messages_return::handled;
	}
	else if( msg == CRCInput::RC_spkr )
	{
		if( mode == mode_standby )
		{
			//switch lcd off/on
			CLCD::getInstance()->setPower( !CLCD::getInstance()->getPower() );
		}
		else
		{
			//mute
			AudioMute( !current_muted );
		}
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_VOLCHANGED )
	{
		current_volume = data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_MUTECHANGED )
	{
		AudioMute( (bool)data, true );
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_RECORDMODE )
	{
		dprintf(DEBUG_DEBUG, "neutino - recordmode %s\n", ( data ) ? "on":"off" );

		if( ( !g_InfoViewer->is_visible ) && data )
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

		t_channel_id old_id = channelList->getActiveChannel_ChannelID();

		channelsInit();

		if((old_id == 0) || (!(channelList->adjustToChannelID(old_id))))
			channelList->zapTo(0);
	}
	else if( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED )			// EVT_BOUQUETSCHANGED: initiated by zapit ; EVT_SERVICESCHANGED: no longer used
	{
		t_channel_id old_id = channelList->getActiveChannel_ChannelID();

		channelsInit();

		if((old_id == 0) || (!(channelList->adjustToChannelID(old_id))))
			channelList->zapTo(0);

		return messages_return::handled;
	}
	else if(msg == NeutrinoMessages::RECORD_START)
	{
		if(recordingstatus == 0)
		{
			if(CVCRControl::getInstance()->isDeviceRegistered())
			{
				recording_id = ((CTimerd::RecordingInfo *) data)->eventID;
				if(CVCRControl::getInstance()->Record((CTimerd::RecordingInfo *) data))
					recordingstatus = 1;
				else
					recordingstatus = 0;
			}
			else
				printf("Keine vcr Devices registriert\n");
			delete (unsigned char*) data;
		}
		else
		{
			// Es l�uft bereits eine Aufnahme
			if(nextRecordingInfo!=NULL)
			{
				delete nextRecordingInfo;
				nextRecordingInfo=NULL;
			}
			nextRecordingInfo=((CTimerd::RecordingInfo*)data);
		}
		return messages_return::handled | messages_return::cancel_all;
	}
	else if( msg == NeutrinoMessages::RECORD_STOP)
	{
		if(((CTimerd::RecordingStopInfo*)data)->eventID==recording_id)
		{ // passendes stop zur Aufnahme
			if(CVCRControl::getInstance()->isDeviceRegistered())
			{
				if(CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD || CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE)
				{
					CVCRControl::getInstance()->Stop();
					recordingstatus=0;
				}
				else
					printf("falscher state\n");
			}
			else
				printf("Keine vcr Devices registriert\n");
			startNextRecording();
		}
		else if(nextRecordingInfo!=NULL)
		{
			if(((CTimerd::RecordingStopInfo*)data)->eventID == nextRecordingInfo->eventID)
			{
				delete nextRecordingInfo;
				nextRecordingInfo=NULL;
			}
		}
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ZAPTO)
	{
		CTimerd::EventInfo * eventinfo;
		eventinfo = (CTimerd::EventInfo *) data;
		if(recordingstatus==0)
		{
			if(eventinfo->mode==CTimerd::MODE_RADIO && mode!=mode_radio)
			{
				radioMode(false);
				channelsInit();
			}
			else if(eventinfo->mode==CTimerd::MODE_TV && mode!=mode_tv)
			{
				tvMode(false);
				channelsInit();
			}
			channelList->zapTo_ChannelID(eventinfo->channel_id);
		}
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_ZAPTO)
	{
		if( mode == mode_standby )
		{
			// WAKEUP
			standbyMode( false );
		}
		if( mode != mode_scart )
			ShowHintUTF("messagebox.info", g_Locale->getText("zaptotimer.announce")); // UTF-8
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_RECORD)
	{
		if( g_settings.recording_server_wakeup )
		{
			std::string command = "etherwake ";
			command += g_settings.recording_server_mac;
			if(system(command.c_str()) != 0)
				perror("etherwake failed");
		}
		if( mode != mode_scart )
			ShowHintUTF("messagebox.info", g_Locale->getText("recordtimer.announce")); // UTF-8
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SLEEPTIMER)
	{
		if( mode != mode_scart )
			ShowHintUTF("messagebox.info", g_Locale->getText("sleeptimerbox.announce")); // UTF-8
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::SLEEPTIMER)
	{
		CIRSend irs("sleep");
		irs.Send();

		if(g_settings.shutdown_real)
			ExitRun();
		else
			standbyMode( true );
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_TOGGLE )
	{
		standbyMode( !(mode & mode_standby) );
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_ON )
	{
		if( mode != mode_standby )
		{
			// noch nicht im Standby-Mode...
			standbyMode( true );
		}
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_OFF )
	{
		if( mode == mode_standby )
		{
			// WAKEUP
			standbyMode( false );
		}
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SHUTDOWN)
	{
		if( mode != mode_scart )
			skipShutdownTimer = (ShowMsgUTF("messagebox.info", g_Locale->getText("shutdowntimer.announce"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 5) == CMessageBox::mbrYes); // UTF-8
	}
	else if( msg == NeutrinoMessages::SHUTDOWN )
	{
		// AUSSCHALTEN...
		if(!skipShutdownTimer)
		{
			ExitRun();
		}
		else
		{
			skipShutdownTimer=false;
		}
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_POPUP)
	{
		if (mode != mode_scart)
			ShowHintUTF("messagebox.info", (const char *) data); // UTF-8
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_EXTMSG)
	{
		if (mode != mode_scart)
			ShowMsgUTF("messagebox.info", std::string((char *) data), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::REMIND)
	{
		std::string text = (char*)data;
		std::string::size_type pos;
		while((pos=text.find("/",0))!= std::string::npos)
		{
			text.replace(pos,1,"\n");
		}
		if( mode != mode_scart )
			ShowMsgUTF("timerlist.type.remind", text, CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::CHANGEMODE )
	{
		if((data & mode_mask)== mode_radio)
		{
			if( mode != mode_radio )
				if((data & norezap)==norezap)
					radioMode(false);
				else
					radioMode(true);
		}
		if((data & mode_mask)== mode_tv)
		{
			if( mode != mode_tv )
				if((data & norezap)==norezap)
					tvMode(false);
				else
					tvMode(true);
		}
		if((data &mode_mask)== mode_standby)
		{
			if(mode != mode_standby)
				standbyMode( true );
		}
		if((data &mode_mask)== mode_mp3)
		{
			lastMode=mode;
			mode=mode_mp3;
		}
		if((data &mode_mask)== mode_pic)
		{
			lastMode=mode;
			mode=mode_pic;
		}
		if((data &mode_mask)== mode_ts)
		{
			lastMode=mode;
			mode=mode_ts;
		}
	}
	else if( msg == NeutrinoMessages::VCR_ON )
	{
		if( mode != mode_scart )
		{
			// noch nicht im Scart-Mode...
			scartMode( true );
		}
		else // sonst nur lcd akt.
			CLCD::getInstance()->setMode(CLCD::MODE_SCART);
	}

	else if( msg == NeutrinoMessages::VCR_OFF )
	{
		if( mode == mode_scart )
		{
			// noch nicht im Scart-Mode...
			scartMode( false );
		}
	}
	else if( msg == NeutrinoMessages::EVT_START_PLUGIN )
	{
		string plugname = std::string((char *) data);
				g_PluginList->setvtxtpid( g_RemoteControl->current_PIDs.PIDs.vtxtpid );
		g_PluginList->startPlugin( plugname );
		delete (unsigned char*) data;
	}

	if( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
		delete (unsigned char*) data;

	return messages_return::unhandled;
}



void CNeutrinoApp::ExitRun()
{
#ifdef USEACTIONLOG
	g_ActionLog->println("neutrino shutdown");
#endif
	CLCD::getInstance()->setMode(CLCD::MODE_SHUTDOWN);

	dprintf(DEBUG_INFO, "exit\n");
	for(int x=0;x<256;x++)
		frameBuffer->paletteSetColor(x, 0x000000, 0xffff);
	frameBuffer->paletteSet();

	if(frameBuffer->getActive())
		frameBuffer->loadPicture2FrameBuffer("shutdown.raw");
	frameBuffer->loadPal("shutdown.pal");

	networkConfig.automatic_start = (network_automatic_start == 1);
	networkConfig.commitConfig();
	saveSetup();
	g_Controld->shutdown();

	if (g_RCInput != NULL)
		delete g_RCInput;

	if (frameBuffer != NULL)
		delete frameBuffer;

	exit(0);
}

bool CNeutrinoApp::onPaintNotify(std::string MenuName)
{
	if(MenuName == "videomenu.head")
	{//aktuelle werte vom controld holen...
		g_settings.video_Signal = g_Controld->getVideoOutput();
		g_settings.video_Format = g_Controld->getVideoFormat();
	}

	return false;
}

void CNeutrinoApp::AudioMute( bool newValue, bool isEvent )
{
   if(g_settings.audio_avs_Control==2) //lirc
   {
      CIRSend irs("mute");
      irs.Send();
   }
   else
   {
      int dx = 40;
      int dy = 40;
      int x = g_settings.screen_EndX-dx;
      int y = g_settings.screen_StartY;

      CLCD::getInstance()->setMuted(newValue);
      if( newValue != current_muted )
      {
         current_muted = newValue;

         if( !isEvent )
         {
            if( current_muted )
               g_Controld->Mute((g_settings.audio_avs_Control));
            else
               g_Controld->UnMute((g_settings.audio_avs_Control));
         }
      }

      if( isEvent && ( mode != mode_scart ) && ( mode != mode_mp3) && ( mode != mode_pic))
      {
         // anzeigen NUR, wenn es vom Event kommt
         if( current_muted )
         {
            frameBuffer->paintBoxRel(x, y, dx, dy, COL_INFOBAR);
            frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, x+5, y+5);
         }
         else
            frameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
      }
   }
}

void CNeutrinoApp::setVolume(int key, bool bDoPaint)
{
	uint msg = key;
	if(g_settings.audio_avs_Control==2) //lirc
	{
		if(msg==CRCInput::RC_plus)
		{
			CIRSend irs("volplus");
			irs.Send();
		}
		else if(msg==CRCInput::RC_minus)
		{
			CIRSend irs("volminus");
			irs.Send();
		}
	}
	else
	{
		int dx = 256;
		int dy = 40;
		int x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
		int y = g_settings.screen_EndY- 100;

		unsigned char* pixbuf = NULL;

		if(bDoPaint)
	  	{
			pixbuf= new unsigned char[ dx * dy ];
			if(pixbuf!= NULL)
				frameBuffer->SaveScreen(x, y, dx, dy, pixbuf);
			frameBuffer->paintIcon("volume.raw",x,y, COL_INFOBAR);
		}

		uint data;

		unsigned long long timeoutEnd;

		do
	  	{
			if( msg <= CRCInput::RC_MaxRC )
				timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_infobar/ 2 );

			if(msg==CRCInput::RC_plus)
			{
				if (current_volume < 100)
				{
					if (current_volume < 100 - 5)
						current_volume += 5;
					else
						current_volume = 100;
				}
				g_Controld->setVolume(current_volume,(g_settings.audio_avs_Control));
			}
			else if(msg==CRCInput::RC_minus)
		  	{
				if (current_volume > 0)
				{
					if (current_volume > 5)
						current_volume -= 5;
					else
						current_volume = 0;
				}
				g_Controld->setVolume(current_volume,(g_settings.audio_avs_Control));
			}
			else
		  	{
				if( (msg!=CRCInput::RC_ok) || (msg!=CRCInput::RC_home) )
				{
					if( handleMsg( msg, data ) & messages_return::unhandled )
					{
						g_RCInput->postMsg( msg, data );
						msg= CRCInput::RC_timeout;
					}
				}
			}

			if(bDoPaint)
		  	{
				int vol = current_volume<<1;
				frameBuffer->paintBoxRel(x+40, y+12, 200, 15, COL_INFOBAR+1);
				frameBuffer->paintBoxRel(x+40, y+12, vol, 15, COL_INFOBAR+3);
			}

			CLCD::getInstance()->showVolume(current_volume);
			if( msg != CRCInput::RC_timeout )
			{
				g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
			}

		}
		while( msg != CRCInput::RC_timeout );

		if( (bDoPaint) && (pixbuf!= NULL) )
			frameBuffer->RestoreScreen(x, y, dx, dy, pixbuf);
	}
}

void CNeutrinoApp::tvMode( bool rezap )
{
	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	if( mode == mode_tv )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}

	mode = mode_tv;
#ifdef USEACTIONLOG
	g_ActionLog->println("mode: tv");
#endif


	//printf( "tv-mode\n" );

	frameBuffer->useBackground(false);
	frameBuffer->paintBackground();

	g_RemoteControl->tvMode();
	if( rezap )
	{
		firstChannel();
		channelsInit();
		channelList->zapTo( firstchannel.channelNumber -1 );
	}
}

void CNeutrinoApp::scartMode( bool bOnOff )
{
#ifdef USEACTIONLOG
	g_ActionLog->println( ( bOnOff ) ? "mode: scart on" : "mode: scart off" );
#endif


	//printf( ( bOnOff ) ? "mode: scart on\n" : "mode: scart off\n" );

	if( bOnOff )
	{
		// SCART AN
		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();

		g_Controld->setScartMode( 1 );
		CLCD::getInstance()->setMode(CLCD::MODE_SCART);
		lastMode = mode;
		mode = mode_scart;
	}
	else
	{
		// SCART AUS
		g_Controld->setScartMode( 0 );

		mode = mode_unknown;

		//re-set mode
		if( lastMode == mode_radio )
		{
			radioMode( false );
		}
		else if( lastMode == mode_tv )
		{
			tvMode( false );
		}
		else if( lastMode == mode_standby )
		{
			standbyMode( true );
		}
	}
}

void CNeutrinoApp::standbyMode( bool bOnOff )
{
#ifdef USEACTIONLOG
	g_ActionLog->println( ( bOnOff ) ? "mode: standby on" : "mode: standby off" );
#endif


	//printf( ( bOnOff ) ? "mode: standby on\n" : "mode: standby off\n" );

	if( bOnOff )
	{
		// STANDBY AN
		lcdpower = CLCD::getInstance()->getPower()?1:0;

		if( mode == mode_scart )
		{
			g_Controld->setScartMode( 0 );
		}

		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();

		CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
		g_Controld->videoPowerDown(true);

		lastMode = mode;
		mode = mode_standby;

		//Send ir
		CIRSend irs("sbon");
		irs.Send();
	}
	else
	{
		// STANDBY AUS

		CLCD::getInstance()->setPower(lcdpower);
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);

		//Send ir
		CIRSend irs("sboff");
		irs.Send();

		mode = mode_unknown;

		//re-set mode
		if( lastMode == mode_radio )
		{
			radioMode( false );
		}
		else
		{
			tvMode( false );
		}
	}
}

void CNeutrinoApp::radioMode( bool rezap)
{
	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	if( mode==mode_radio )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}

	mode = mode_radio;
#ifdef USEACTIONLOG
	g_ActionLog->println("mode: radio");
#endif

	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("radiomode.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	g_RemoteControl->radioMode();
	if( rezap )
	{
		firstChannel();
		channelsInit();
		channelList->zapTo( firstchannel.channelNumber -1 );
	}
}

void CNeutrinoApp::startNextRecording()
{
	if(recordingstatus==0 && nextRecordingInfo!=NULL)
	{
		if(CVCRControl::getInstance()->isDeviceRegistered())
		{
			recording_id = nextRecordingInfo->eventID;
			if(CVCRControl::getInstance()->Record(nextRecordingInfo))
				recordingstatus = 1;
			else
				recordingstatus = 0;
		}
		delete nextRecordingInfo;
		nextRecordingInfo=NULL;
	}
}
/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  exec, menuitem callback (shutdown)                         *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::exec(CMenuTarget* parent, std::string actionKey)
{
	//	printf("ac: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey=="theme_neutrino")
	{
		setupColors_neutrino();
		colorSetupNotifier->changeNotify("initial", NULL);
	}
	else if(actionKey=="theme_classic")
	{
		setupColors_classic();
		colorSetupNotifier->changeNotify("initial", NULL);
	}
	else if(actionKey=="shutdown")
	{
		ExitRun();
	}
	else if(actionKey=="tv")
	{
		tvMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="radio")
	{
		radioMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="scart")
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="network")
	{
		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.commitConfig();
		networkConfig.stopNetwork();
		networkConfig.startNetwork();
	}
	else if(actionKey=="networktest")
	{
		dprintf(DEBUG_INFO, "doing network test...\n");
		testNetworkSettings(networkConfig.address.c_str(), networkConfig.netmask.c_str(), networkConfig.broadcast.c_str(), networkConfig.gateway.c_str(), networkConfig.nameserver.c_str(), networkConfig.inet_static);
	}
	else if(actionKey=="networkshow")
	{
		dprintf(DEBUG_INFO, "showing current network settings...\n");
		showCurrentNetworkSettings();
	}
	else if(actionKey=="savesettings")
	{
		CHintBox * hintBox = new CHintBox("messagebox.info", g_Locale->getText("mainsettings.savesettingsnow_hint")); // UTF-8
		hintBox->paint();

		g_Controld->saveSettings();
		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.commitConfig();
		saveSetup();

		/* send motor position list to zapit */
		if (scanSettings.diseqcMode == DISEQC_1_2)
		{
			printf("[neutrino] sending motor positions list to zapit...\n");
			CZapitClient::ScanMotorPosList motorPosList;
			CNeutrinoApp::getInstance()->getScanSettings().toMotorPosList(motorPosList);
			g_Zapit->setScanMotorPosList(motorPosList);
		}

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="recording")
	{
		setupRecordingDevice();
	}
	else if(actionKey=="reloadchannels")
	{
	 	CHintBox * hintBox = new CHintBox("messagebox.info", g_Locale->getText("servicemenu.reload_hint")); // UTF-8
		hintBox->paint();

		g_Zapit->reinitChannels();

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="fontsize.def1")
	{
		strcpy( g_settings.fontsize_epg_title	, DEFAULT_FONTSIZE_EPG_TITLE );
		strcpy( g_settings.fontsize_epg_info1	, DEFAULT_FONTSIZE_EPG_INFO1 );
		strcpy( g_settings.fontsize_epg_info2	, DEFAULT_FONTSIZE_EPG_INFO2 );
		strcpy( g_settings.fontsize_epg_date	, DEFAULT_FONTSIZE_EPG_DATE );
		changeNotify("fontsize.", NULL);
	}
	else if(actionKey=="fontsize.def2")
	{
		strcpy( g_settings.fontsize_eventlist_title	, DEFAULT_FONTSIZE_EVENTLIST_TITLE );
		strcpy( g_settings.fontsize_eventlist_itemlarge	, DEFAULT_FONTSIZE_EVENTLIST_ITEMLARGE );
		strcpy( g_settings.fontsize_eventlist_itemsmall	, DEFAULT_FONTSIZE_EVENTLIST_ITEMSMALL );
		strcpy( g_settings.fontsize_eventlist_datetime	, DEFAULT_FONTSIZE_EVENTLIST_DATETIME );
		changeNotify("fontsize.", NULL);
	}
	else if(actionKey=="fontsize.def3")
	{
		strcpy( g_settings.fontsize_gamelist_itemlarge	, DEFAULT_FONTSIZE_GAMELIST_ITEMLARGE );
		strcpy( g_settings.fontsize_gamelist_itemsmall	, DEFAULT_FONTSIZE_GAMELIST_ITEMSMALL );
		changeNotify("fontsize.", NULL);
	}
	else if(actionKey=="fontsize.def4")
	{
		strcpy( g_settings.fontsize_channellist		, DEFAULT_FONTSIZE_CHANNELLIST );
		strcpy( g_settings.fontsize_channellist_descr	, DEFAULT_FONTSIZE_CHANNELLIST_DESCR );
		strcpy( g_settings.fontsize_channellist_number	, DEFAULT_FONTSIZE_CHANNELLIST_NUMBER );
		strcpy( g_settings.fontsize_channel_num_zap	, DEFAULT_FONTSIZE_CHANNEL_NUM_ZAP );
		changeNotify("fontsize.", NULL);
	}
	else if(actionKey=="fontsize.def5")
	{
		strcpy( g_settings.fontsize_infobar_number	, DEFAULT_FONTSIZE_INFOBAR_NUMBER );
		strcpy( g_settings.fontsize_infobar_channame	, DEFAULT_FONTSIZE_INFOBAR_CHANNAME );
		strcpy( g_settings.fontsize_infobar_info	, DEFAULT_FONTSIZE_INFOBAR_INFO );
		strcpy( g_settings.fontsize_infobar_small	, DEFAULT_FONTSIZE_INFOBAR_SMALL );
		changeNotify("fontsize.", NULL);
	}
	else if(actionKey=="fontsize.def6")
	{
		strcpy( g_settings.fontsize_alert		, DEFAULT_FONTSIZE_ALERT );
		strcpy( g_settings.fontsize_menu		, DEFAULT_FONTSIZE_MENU );
		strcpy( g_settings.fontsize_menu_title		, DEFAULT_FONTSIZE_MENU_TITLE );
		strcpy( g_settings.fontsize_menu_info		, DEFAULT_FONTSIZE_MENU_INFO );
		strcpy( g_settings.fontsize_filebrowser_item	, DEFAULT_FONTSIZE_FILEBROWSER_ITEM );
		changeNotify("fontsize.", NULL);
	}
	else if(actionKey=="osd.def")
	{
		sprintf(g_settings.timing_menu_string,		"%d", DEFAULT_TIMING_MENU);
		sprintf(g_settings.timing_chanlist_string,	"%d", DEFAULT_TIMING_CHANLIST);
		sprintf(g_settings.timing_epg_string,		"%d", DEFAULT_TIMING_EPG);
		sprintf(g_settings.timing_infobar_string,	"%d", DEFAULT_TIMING_INFOBAR);
		sprintf(g_settings.timing_filebrowser_string,	"%d", DEFAULT_TIMING_FILEBROWSER);
		changeNotify("timing.", NULL);
	}
	else if(actionKey == "mp3dir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		string startdir=g_settings.network_nfs_mp3dir;
		if (b.exec(startdir))
			strncpy(g_settings.network_nfs_mp3dir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_mp3dir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "picturedir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		string startdir=g_settings.network_nfs_picturedir;
		if (b.exec(startdir))
			strncpy(g_settings.network_nfs_picturedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_picturedir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "moviedir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		string startdir=g_settings.network_nfs_moviedir;
		if (b.exec(startdir))
			strncpy(g_settings.network_nfs_moviedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_moviedir)-1);
		return menu_return::RETURN_REPAINT;
	}

	return returnval;
}

/**************************************************************************************
*                                                                                     *
*          changeNotify - features menu recording start / stop                        *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::changeNotify(std::string OptionName, void *Data)
{
//	printf("OptionName: %s\n",OptionName.c_str());
	if(OptionName.substr(0,9).compare("fontsize.") == 0)
	{
		CHintBox * hintBox = new CHintBox("messagebox.info", g_Locale->getText("fontsize.hint")); // UTF-8
		hintBox->paint();

		SetupFonts();

		hintBox->hide();
		delete hintBox;
	}
	else if(OptionName.substr(0,7).compare("timing.") == 0)
	{
		g_settings.timing_menu = atoi(g_settings.timing_menu_string);
		g_settings.timing_chanlist = atoi(g_settings.timing_chanlist_string);
		g_settings.timing_epg = atoi(g_settings.timing_epg_string);
		g_settings.timing_infobar = atoi(g_settings.timing_infobar_string);
		g_settings.timing_filebrowser = atoi(g_settings.timing_filebrowser_string);

	}

	else if(OptionName.substr(0,3).compare("mp3.") == 0)
	{
// this code will never be reached
		CMP3Player::getInstance()->init();
	}

	else if(OptionName.substr(0,18).compare("mainmenu.recording") == 0)
	{
		CTimerd::RecordingInfo eventinfo;

		if(CVCRControl::getInstance()->isDeviceRegistered())
		{
			if(recordingstatus == 1)
			{
				recording_id=0;
				eventinfo.channel_id = g_RemoteControl->current_channel_id;
				eventinfo.epgID = g_RemoteControl->current_EPGid;
				eventinfo.epg_starttime = 0;
				strcpy(eventinfo.apids, "");
				if(getMode()==mode_radio)
					eventinfo.mode = CTimerd::MODE_RADIO;
				else
					eventinfo.mode = CTimerd::MODE_TV;

				if(CVCRControl::getInstance()->Record(&eventinfo)==false)
				{
					recordingstatus=0;
					return false;
				}
			}
			else
			{
				CVCRControl::getInstance()->Stop();
				startNextRecording();
			}
			return true;
		}
		else
			printf("Keine Streamingdevices registriert\n");
	}
	return false;
}



/**************************************************************************************
*                                                                                     *
*          Main programm - no function here                                           *
*                                                                                     *
**************************************************************************************/
int main(int argc, char **argv)
{
	setDebugLevel(DEBUG_NORMAL);

	tzset();
	initGlobals();
	return CNeutrinoApp::getInstance()->run(argc, argv);
}

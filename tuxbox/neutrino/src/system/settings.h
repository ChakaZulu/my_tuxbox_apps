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

#ifndef __settings__
#define __settings__

#include <configfile.h>
#include <zapit/client/zapitclient.h>

#include <string>

struct SNeutrinoSettings
{
	//video
	int video_Signal;
	int video_Format;

	//misc
	int shutdown_real;
	int shutdown_showclock;
	int show_camwarning;
	char record_safety_time_before[3];
	char record_safety_time_after[3];

	//audio
	int audio_AnalogMode;
	int audio_DolbyDigital;
	int audio_avs_Control;

	//vcr
	int	vcr_AutoSwitch;

	//language
	char language[25];

	//timing
	int timing_menu;
	int timing_chanlist;
	int timing_epg;
	int timing_infobar;
	int	timing_filebrowser;
	char timing_menu_string[4];
	char timing_chanlist_string[4];
	char timing_epg_string[4];
	char timing_infobar_string[4];
	char timing_filebrowser_string[4];

	//widget settings
	int	widget_fade;

	//colors
	unsigned char gtx_alpha1;
	unsigned char gtx_alpha2;

	unsigned char menu_Head_alpha;
	unsigned char menu_Head_red;
	unsigned char menu_Head_green;
	unsigned char menu_Head_blue;

	unsigned char menu_Head_Text_alpha;
	unsigned char menu_Head_Text_red;
	unsigned char menu_Head_Text_green;
	unsigned char menu_Head_Text_blue;

	unsigned char menu_Content_alpha;
	unsigned char menu_Content_red;
	unsigned char menu_Content_green;
	unsigned char menu_Content_blue;

	unsigned char menu_Content_Text_alpha;
	unsigned char menu_Content_Text_red;
	unsigned char menu_Content_Text_green;
	unsigned char menu_Content_Text_blue;

	unsigned char menu_Content_Selected_alpha;
	unsigned char menu_Content_Selected_red;
	unsigned char menu_Content_Selected_green;
	unsigned char menu_Content_Selected_blue;

	unsigned char menu_Content_Selected_Text_alpha;
	unsigned char menu_Content_Selected_Text_red;
	unsigned char menu_Content_Selected_Text_green;
	unsigned char menu_Content_Selected_Text_blue;

	unsigned char menu_Content_inactive_alpha;
	unsigned char menu_Content_inactive_red;
	unsigned char menu_Content_inactive_green;
	unsigned char menu_Content_inactive_blue;

	unsigned char menu_Content_inactive_Text_alpha;
	unsigned char menu_Content_inactive_Text_red;
	unsigned char menu_Content_inactive_Text_green;
	unsigned char menu_Content_inactive_Text_blue;

	unsigned char infobar_alpha;
	unsigned char infobar_red;
	unsigned char infobar_green;
	unsigned char infobar_blue;

	unsigned char infobar_Text_alpha;
	unsigned char infobar_Text_red;
	unsigned char infobar_Text_green;
	unsigned char infobar_Text_blue;

	//network
	std::string network_nfs_ip[4];
	char network_nfs_local_dir[4][100];
	char network_nfs_dir[4][100];
	int  network_nfs_automount[4];
	
	//streaming
	int  recording_type;
	int  recording_stopplayback;
	int  recording_stopsectionsd;
	std::string recording_server_ip;
	char recording_server_port[10];
	int  recording_server_wakeup;
	char recording_server_mac[31];
	int  recording_vcr_no_scart;

	//key configuration
	int key_tvradio_mode;

	int key_channelList_pageup;
	int key_channelList_pagedown;
	int key_channelList_cancel;
	int key_channelList_sort;
	int key_channelList_addrecord;
	int key_channelList_addremind;

	int key_quickzap_up;
	int key_quickzap_down;
	int key_bouquet_up;
	int key_bouquet_down;
	int key_subchannel_up;
	int key_subchannel_down;

	char repeat_blocker[4];
	char repeat_genericblocker[4];

	//screen configuration
	int	screen_StartX;
	int	screen_StartY;
	int	screen_EndX;
	int	screen_EndY;

	//Software-update
	int softupdate_mode;
	char softupdate_currentversion[20];
	char softupdate_proxyserver[31];
	char softupdate_proxyusername[31];
	char softupdate_proxypassword[31];

	//BouquetHandling
	int bouquetlist_mode;

	// parentallock
	int parentallock_prompt;
	int parentallock_lockage;
	char parentallock_pincode[5];


	// Font sizes
	char	fontsize_menu[4];
	char	fontsize_menu_title[4];
	char	fontsize_menu_info[4];

	char	fontsize_epg_title[4];
	char	fontsize_epg_info1[4];
	char	fontsize_epg_info2[4];
	char	fontsize_epg_date[4];
	char	fontsize_alert[4];
	char	fontsize_eventlist_title[4];
	char	fontsize_eventlist_itemlarge[4];
	char	fontsize_eventlist_itemsmall[4];
	char	fontsize_eventlist_datetime[4];

	char	fontsize_gamelist_itemlarge[4];
	char	fontsize_gamelist_itemsmall[4];

	char	fontsize_channellist[4];	
	char	fontsize_channellist_descr[4];
	char	fontsize_channellist_number[4];
	char	fontsize_channel_num_zap[4];

	char	fontsize_infobar_number[4];
	char	fontsize_infobar_channame[4];
	char	fontsize_infobar_info[4];
	char	fontsize_infobar_small[4];

	char	fontsize_filebrowser_item[4];

	// lcdd
	int	lcd_brightness;
	int	lcd_standbybrightness;
	int	lcd_contrast;
	int	lcd_power;
	int	lcd_inverse;

	// pictureviewer
	char   picviewer_slide_time[3];
	int    picviewer_scaling;

   //mp3player
   int   mp3player_display;
   int   mp3player_follow;
};

struct SglobalInfo
{
	unsigned char     box_Type;
	delivery_system_t delivery_system;
};


const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;

#define MAX_SATELLITES 20

class CScanSettings
{
 public:
	CConfigFile               configfile;
	CZapitClient::bouquetMode bouquetMode;
	diseqc_t                  diseqcMode;
	uint32_t                  diseqcRepeat;
	char                      satNameNoDiseqc[30];
	int                       satDiseqc[MAX_SATELLITES];
	char                      satName[30][MAX_SATELLITES];
	delivery_system_t         delivery_system;

	CScanSettings();
	
	int* diseqscOfSat( char* satname);
	char* CScanSettings::satOfDiseqc(int diseqc) const;
	void toSatList( CZapitClient::ScanSatelliteList& ) const;
	
	
	void useDefaults(const delivery_system_t _delivery_system);
	
	bool loadSettings(const std::string fileName, const delivery_system_t _delivery_system);
	bool saveSettings(const std::string fileName);
};


#endif


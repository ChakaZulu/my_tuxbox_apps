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


#ifndef __neutrino__
#define __neutrino__

#include <configfile.h>

#include <zapit/client/zapitclient.h>

#include "neutrinoMessages.h"
#include "driver/framebuffer.h"
#include "system/setting_helpers.h"
#include "timerdclient/timerdtypes.h"

#include <string>

#define widest_number "1"

#define ANNOUNCETIME (1 * 60)

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  main run-class                                             *
*                                                                                     *
**************************************************************************************/

class CNeutrinoApp : public CMenuTarget, COnPaintNotifier, CChangeObserver
{
	private:

		CFrameBuffer	*frameBuffer;

		enum
		{
			mode_unknown = -1,
			mode_tv = 1,
			mode_radio = 2,
			mode_scart = 3,
			mode_standby = 4,
			mode_mp3 = 5,
			mode_pic = 6,
			mode_mask = 0xFF,
			norezap = 0x100
		};

		CConfigFile			configfile;
		std::string			settingsFile;
		std::string			scanSettingsFile;
		CScanSettings			scanSettings;

		std::string			fontName;
		std::string			fontFile;
		int				fontsSizeOffset;

		int				mode;

		int				lastMode;
		bool				softupdate;
		bool				fromflash;
		int				recordingstatus;
		int				recording_id;
		CTimerd::RecordingInfo* nextRecordingInfo;
		//		bool				record_mode;

		long long 			standby_pressed_at;

		CZapitClient::responseGetLastChannel    firstchannel;
		st_rmsg				sendmessage;

		char				current_volume;
		bool				current_muted;

		bool				skipShutdownTimer;

		CColorSetupNotifier		*colorSetupNotifier;
		CAudioSetupNotifier		*audioSetupNotifier;
		CVideoSetupNotifier		*videoSetupNotifier;
		CLanguageSetupNotifier  	*languageSetupNotifier;
		CKeySetupNotifier       	*keySetupNotifier;
		CAPIDChangeExec         	*APIDChanger;
		CNVODChangeExec         	*NVODChanger;
		CUCodeCheckExec			*UCodeChecker;
		CStreamFeaturesChangeExec	*StreamFeaturesChanger;
		CIPChangeNotifier		*MyIPChanger;
//		CVCRControl			*vcrControl;
		CConsoleDestChangeNotifier	*ConsoleDestinationChanger;


		void doChecks();
		void firstChannel();
		void setupColors_classic();
		void setupColors_neutrino();
		void setupNetwork( bool force= false );
		void setupNFS();
		void setupRecordingDevice(void);
		void testNetwork();
		void showNetwork();
		
		void startNextRecording();

		void saveSetup();
		int loadSetup();

		void tvMode( bool rezap = true );
		void radioMode( bool rezap = true );
		void scartMode( bool bOnOff );
		void standbyMode( bool bOnOff );
		void setVolume(int key, bool bDoPaint = true);
		void AudioMute( bool newValue, bool isEvent= false );


		void ExitRun();
		void RealRun(CMenuWidget &mainSettings);
		void InitZapper();
		void InitKeySettings(CMenuWidget &);
		void InitServiceSettings(CMenuWidget &, CMenuWidget &);
		void InitColorSettingsMenuColors(CMenuWidget &);
		void InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier);
		void InitColorSettings(CMenuWidget &, CMenuWidget &);
		void InitLanguageSettings(CMenuWidget &);
		void InitColorThemesSettings(CMenuWidget &);
		void InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_menuColors);
		void InitColorSettingsTiming(CMenuWidget &colorSettings_timing);
		void InitLcdSettings(CMenuWidget &lcdSettings);
		void InitNetworkSettings(CMenuWidget &networkSettings);
		void AddFontSettingItem(CMenuWidget &fontSettings, std::string menuname, char *value);
		void InitFontSettings(CMenuWidget &fontSettings,CMenuWidget &fontSettings_Channellist ,CMenuWidget &fontSettings_Eventlist , CMenuWidget &fontSettings_Infobar ,CMenuWidget &fontSettings_Epg, CMenuWidget &fontSettings_Gamelist);
		void InitRecordingSettings(CMenuWidget &recordingSettings);
		void InitScreenSettings(CMenuWidget &);
		void InitMiscSettings(CMenuWidget &);
		void InitScanSettings(CMenuWidget &);
		void InitParentalLockSettings(CMenuWidget &);
		void InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier* videoSetupNotifier);
		void InitMainMenu(CMenuWidget &mainMenu, CMenuWidget &mainSettings, CMenuWidget &audioSettings,
		                  CMenuWidget &parentallockSettings, CMenuWidget &networkSettings, CMenuWidget &networkSettings,
		                  CMenuWidget &colorSettings, CMenuWidget &lcdSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings,
		                  CMenuWidget &languageSettings, CMenuWidget &miscSettings, CMenuWidget &service, CMenuWidget &fontSettings);
		void ClearFrameBuffer();
		void SetupFonts();
		void SetupTiming();
		void SetupFrameBuffer();
		void SelectAPID();
		void SelectNVOD();
		void CmdParser(int argc, char **argv);
		bool getEnvironment(const char* name, int* value);
		void ShowStreamFeatures();

		long long last_profile_call;

		CNeutrinoApp();
		~CNeutrinoApp();

	public:
		const CScanSettings& getScanSettings(){ return scanSettings;};

		CChannelList				*channelList;

		static CNeutrinoApp* getInstance();

		void channelsInit();
		int run(int argc, char **argv);
		//callback stuff only....
		int exec(CMenuTarget* parent, std::string actionKey);
		//callback for menue
		bool onPaintNotify(std::string MenuName);
		//onchange
		bool changeNotify(std::string OptionName, void *Data);

		int handleMsg(uint msg, uint data);
		void showProfiling(std::string text);

		int getMode() {return mode;}
		int getLastMode() {return lastMode;}
		bool isMuted() {return current_muted;}

	friend class CNeutrinoBouquetEditorEvents;
};


#endif

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
#ifndef __vcrcontrol__
#define __vcrcontrol__

#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <map>


class CVCRControl
{
	public:
		typedef enum CVCRStates 
		{
			CMD_VCR_UNKNOWN =	0,
			CMD_VCR_RECORD	=	1,
			CMD_VCR_STOP	=	2,
			CMD_VCR_PAUSE	=	3,
			CMD_VCR_RESUME	=	4,
			CMD_VCR_AVAILABLE =	5
		}CVCRCommand;

		enum CVCRDevices
		{
			DEVICE_VCR,
			DEVICE_SERVER
		};


		class CDeviceInfo
		{
			public:
				CDeviceInfo(){Name="";};
				std::string   Name;
		};

		class CVCRDeviceInfo : public CDeviceInfo
		{
			public:
				CVCRDeviceInfo() : CDeviceInfo(){};
				// nothing so far
				// perhaps ir stuff here
		};
		
		class CServerDeviceInfo : public CDeviceInfo		
		{
			public:
				CServerDeviceInfo() : CDeviceInfo() {ServerAddress = "";ServerPort = 0; StopPlayBack = false; StopSectionsd = true;};
				std::string	ServerAddress;
				int		ServerPort;
				bool	StopPlayBack;
				bool	StopSectionsd;
		};

	private:

		class CDevice			// basisklasse f�r die devices
		{
			public:
				int sock_fd;
				std::string Name;
				CVCRDevices deviceType;
				CVCRStates  deviceState;
				virtual bool Stop(){return false;};
				virtual bool Record(const t_channel_id channel_id = 0, unsigned long long epgid = 0, uint apid = 0){return false;};
				virtual bool Pause(){return false;};
				virtual bool Resume(){return false;};
				virtual bool IsAvailable(){return false;};
				CDevice(CVCRDevices devicetype){deviceType = devicetype; deviceState = CMD_VCR_STOP;};
				virtual ~CDevice(){};
		};

		class CVCRDevice : public CDevice		// VCR per IR
		{
			int last_mode;
			bool ParseFile(std::string filename);
			public:
				virtual bool Stop();		// TODO: VCR ansteuerung
				virtual bool Record(const t_channel_id channel_id = 0, unsigned long long epgid = 0, uint apid = 0);	
				virtual bool Pause();
				virtual bool Resume();
				virtual bool IsAvailable(){return true;};
				CVCRDevice() : CDevice(DEVICE_VCR) {};
				virtual ~CVCRDevice(){};
		};

		class CServerDevice : public CDevice	// externer Streamingserver per tcp
		{
			private:
				struct externalCommand			// command wird an externen server gesendet
				{
					unsigned char		messageType;		// egal
					unsigned char		version;			// momentan 1
					unsigned char		command;			// siehe externalcommands
					unsigned long long	epgID;				// may be zero
					t_channel_id		channel_id;			// may be zero
				};

				bool serverConnect();
				void serverDisconnect();

				bool sendCommand(CVCRCommand command, const t_channel_id channel_id = 0,unsigned long long epgid = 0, uint apid = 0);

			public:
				std::string	ServerAddress;
				int		ServerPort;
				bool	StopPlayBack;
				bool	StopSectionsd;

				virtual bool Stop();
				virtual bool Record(const t_channel_id channel_id = 0, unsigned long long epgid = 0, uint apid = 0);
				virtual bool Pause(){return false;};
				virtual bool Resume(){return false;};
				virtual bool IsAvailable(){return true;};
				CServerDevice() : CDevice(DEVICE_SERVER) {};			
				virtual ~CServerDevice(){};
		};
//		typedef std::map<int, CDevice*> CDeviceMap;
	

	public:
		CVCRControl();
		~CVCRControl();
		static CVCRControl* getInstance();

//		CDeviceMap Devices;
		CDevice *Device;
		
		bool registerDevice(CVCRDevices deviceType, CDeviceInfo *deviceInfo);
		void unregisterDevice();

//		int registeredDevices(){return Devices.size();};
		bool isDeviceRegistered(){return Device != NULL;};
		void setDeviceOptions(CDeviceInfo *deviceInfo);

		int getDeviceState(){return Device->deviceState;};
		bool Stop(){return Device->Stop();};
		bool Record(CTimerd::EventInfo *eventinfo)
		{
			return Device->Record(eventinfo->channel_id, eventinfo->epgID, eventinfo->apid); 
		};
		bool Pause(){return Device->Pause();};
		bool Resume(){return Device->Resume();};
};


#endif

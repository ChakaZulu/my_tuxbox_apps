/*
	Neutrino-GUI  -   DBoxII-Project

	Timerliste by Zwen
	
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

#ifndef __timerlist__
#define __timerlist__

#include <timerdclient/timerdclient.h>
#include <zapit/client/zapitclient.h>

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>

#include <string>


class CTimerList : public CMenuTarget
{
	private:
		CFrameBuffer		*frameBuffer;
		unsigned int		selected;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		int					fheight; // Fonthoehe Timerlist-Inhalt
		int					theight; // Fonthoehe Timerlist-Titel
		int               buttonHeight;
		bool				visible;			

		CTimerdClient *Timer;
		CTimerd::TimerList timerlist;             // List of timers		
		CZapitClient::BouquetChannelList channellist_tv;     
		CZapitClient::BouquetChannelList channellist_radio;     
		CTimerd::responseGetTimer timerNew;
		int timerNew_standby_on;
		char timerNew_channel_name[30];
		char m_weekdaysStr[8];
		
		int 			width;
		int 			height;
		int 			x;
		int 			y;

		int skipEventID;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();
		int modifyTimer();
		int newTimer();

	public:
		CTimerList();
		~CTimerList();
		void updateEvents(void);
		int  show();
		int  exec(CMenuTarget* parent, std::string actionKey);
		std::string convertTimerType2String(const CTimerd::CTimerEventTypes type); // UTF-8
		std::string convertTimerRepeat2String(const CTimerd::CTimerEventRepeat rep); // UTF-8
		std::string convertChannelId2String(const t_channel_id id, const CTimerd::CChannelMode mode); // UTF-8
};


#endif



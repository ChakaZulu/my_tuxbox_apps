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

#ifndef __channellist__
#define __channellist__
//
// $Id: channellist.h,v 1.28 2002/03/11 17:25:57 Simplex Exp $
//
// $Log: channellist.h,v $
// Revision 1.28  2002/03/11 17:25:57  Simplex
// locked bouquets work
//
// Revision 1.27  2002/02/27 22:51:13  field
// Tasten kaputt gefixt - sollte wieder gehen :)
//
// Revision 1.26  2002/02/27 16:08:27  field
// Boeser Tasten-Bug behoben, sollte wieder normal laufen :)
//
// Revision 1.25  2002/02/25 19:32:26  field
// Events <-> Key-Handling umgestellt! SEHR BETA!
//
// Revision 1.24  2002/01/29 23:23:06  field
// Mehr Details in Channellist (sectionsd updaten)
//
// Revision 1.23  2002/01/03 20:03:20  McClean
// cleanup
//
// Revision 1.22  2001/12/22 19:34:58  Simplex
// - selected channel in bouquetlist is correct after numzap and quickzap
// - dbox-key in channellist shows bouquetlist
//
// Revision 1.21  2001/11/26 02:34:04  McClean
// include (.../../stuff) changed - correct unix-formated files now
//
// Revision 1.20  2001/11/19 22:50:28  Simplex
// Neutrino can handle bouquets now.
// There are surely some bugs and todo's but it works
//
// Revision 1.19  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.18  2001/11/05 16:04:25  field
// nvods/subchannels ver"c++"ed
//
// Revision 1.17  2001/10/16 18:34:13  rasc
// -- QuickZap to last channel verbessert.
// -- Standard Kanal muss ca. 2-3 Sekunden aktiv sein fuer LastZap Speicherung.
// -- eigene Klasse fuer die Channel History...
//
// Revision 1.16  2001/10/10 17:17:13  field
// zappen auf onid_sid umgestellt
//
// Revision 1.15  2001/10/02 17:56:33  McClean
// time in infobar (thread probs?) and "0" quickzap added
//
// Revision 1.14  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.13  2001/09/20 19:21:37  fnbrd
// Channellist mit IDs.
//
// Revision 1.12  2001/09/20 00:36:32  field
// epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
// Revision 1.11  2001/09/18 11:48:43  fnbrd
// Changed some parameter to const string&
//
// Revision 1.10  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.9  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.8  2001/08/20 13:10:27  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.7  2001/08/20 01:51:12  McClean
// channellist bug fixed - faster channellist response
//
// Revision 1.6  2001/08/20 01:26:54  McClean
// stream info added
//
// Revision 1.5  2001/08/16 23:19:18  McClean
// epg-view and quickview changed
//
// Revision 1.4  2001/08/15 17:01:56  fnbrd
// Added id and log
//
//

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "daemonc/remotecontrol.h"
#include "helpers/infoviewer.h"
#include "helpers/settings.h"
#include "channels/lastchannel.h"
#include "eventlist.hpp"
#include "menue.h"
#include "color.h"

#include <string>
#include <vector>


using namespace std;

class CChannelList
{
	public:
		class CChannel
		{
			private:
				// flag if there is currently running a programm
				// that that should be locked with the actual youth
				// protection settings
				// ( for internal use of isCurrentlyLocked()-method )
				bool bLockedProgramIsRunning;

			public:
				int         key;
				int         number;
				string      name;
				unsigned int onid_sid;
				epg_event   currentEvent;

				// flag that tells if channel is staticly locked by bouquet-locking
				bool bAlwaysLocked;

				// constructor
				CChannel();

				// isCurrentlyLocked returns true if the channel is locked
				// considering youth-protection-settings, bouquet-locking
				// and currently running program
				bool isCurrentlyLocked();

				// lockedProgramStarts should be called when a locked program starts
				void lockedProgramStarts( uint age);
				// lockedProgramEnds should be called when a locked program ends
				void lockedProgramEnds();

				friend class CChannelList;
		};

	private:
		unsigned int		selected;
		unsigned int		tuned;
		CLastChannel		lastChList;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int			fheight; // Fonthoehe Channellist-Inhalt
		int			theight; // Fonthoehe Channellist-Titel

		string			name;
		vector<CChannel*>	chanlist;

		int 			width;
		int 			height;
		int 			x;
		int 			y;

		void paintDetails(int index);
		void paintItem(int pos);
		void paint();
		void paintHead();
		void hide();

	public:
		CChannelList( const std::string& Name="" );
		~CChannelList();
		void addChannel(int key, int number, const std::string& name, unsigned int ids = 0);
		void addChannel(CChannel* chan);
		CChannel* getChannel( int number);
		CChannel* operator[]( uint index) { if (chanlist.size() > index) return chanlist[index]; else return NULL;};
		void setName(const std::string& Name);
		int getKey(int);
		const std::string& getActiveChannelName();
		int getActiveChannelNumber();
		unsigned int CChannelList::getActiveChannelOnid_sid()
		{
			return chanlist[selected]->onid_sid;
		}
		const std::string getActiveChannelID();
		void zapTo(int pos);
		bool showInfo(int pos);
		void updateEvents(void);
		int numericZap(int key);
		int  show();
		int	 exec();
		void quickZap(int key);
		int  hasChannel(int nChannelNr);
		void setSelected( int nChannelNr); // for adjusting bouquet's channel list after numzap or quickzap
}
;


#endif



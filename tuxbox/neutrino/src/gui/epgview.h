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

#ifndef __epgview__
#define __epgview__

#include "driver/framebuffer.h"
#include "driver/rcinput.h"
#include "driver/fontrenderer.h"

#include "gui/color.h"
#include "system/settings.h"

#include "sections/sectionsdMsg.h"
#include "sectionsdclient.h"

#include <vector>
#include <string>

using namespace std;

class CEpgData
{
	private:
		CFrameBuffer		*frameBuffer;
		CChannelEventList	evtlist;
		CEPGData			epgData;

		string 		epg_date;
		string 		epg_start;
		string 		epg_end;
		int			epg_done;

		unsigned long long prev_id;
		time_t prev_zeit;
		unsigned long long next_id;
		time_t next_zeit;

		int			ox, oy, sx, sy, toph;
		int			emptyLineCount, info1_lines;
		int         textCount;
		vector<string>		epgText;
		int			topheight,topboxheight;
		int			botheight,botboxheight;
		int			medlineheight,medlinecount;

		void GetEPGData( unsigned int onid_sid, unsigned long long id, time_t* startzeit );
		void GetPrevNextEPGData( unsigned long long id, time_t* startzeit );
		void addTextToArray( string text );
		void processTextToArray( string text );
		void showText( int startPos, int ypos );
		int FollowScreenings(unsigned int onid_sid, string title);
		void showTimerEventBar(bool show);

	public:

		CEpgData();
		void start( );
		int show( unsigned int onid_tsid, unsigned long long id = 0, time_t* startzeit = NULL, bool doLoop = true );
		void hide();
};


#endif


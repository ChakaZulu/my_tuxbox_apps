/*
  Neutrino-GUI  -   DBoxII-Project

  Copyright (C) 2003 giggo
  Homepage: http://www.giggo.de/dbox

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

#ifndef __movieplayergui__
#define __movieplayergui__

#include <config.h>
#if HAVE_DVB_API_VERSION >= 3
#undef _FILE_OFFSET_BITS
#include "driver/framebuffer.h"
#include "gui/filebrowser.h"
#include "gui/widget/menue.h"
extern "C" {
           	#include "ringbuffer.h"
}           	
#include <stdio.h>

#include <string>
#include <vector>


/*class CTS
{
 public:
	std::string Filename;
	int Index;
};
*/

class CMoviePlayerGui : public CMenuTarget
{
 public:
	enum State
		{
			PLAY=0,
			STOP,
			PAUSE,
			FF,
			REV
		};

 private:
	pthread_t      rct;
	CFrameBuffer * frameBuffer;
	unsigned int   selected;
	int            current;
	unsigned int   liststart;
	unsigned int   listmaxshow;
	int            fheight; // Fonthoehe Playlist-Inhalt
	int            theight; // Fonthoehe Playlist-Titel
	int            sheight; // Fonthoehe MP Info
	int            buttonHeight;
	int            title_height;
	int            info_height;
	int            key_level;
	bool           visible;
        State          m_state;

	int            width;
	int            height;
	int            x;
	int            y;
	int            m_title_w;
	int            m_LastMode;
	const char     *filename;

	std::string Path;
	CFileBrowser * filebrowser;

	void PlayStream(int streamtype);
	void PlayFile();
	void paintItem(int pos);
	void paint();
	void paintHead();
	void paintImg();
	void paintFoot();
	void hide();

	CFileFilter videofilefilter;

 public:
	CMoviePlayerGui();
	~CMoviePlayerGui();
	int  show();
	int  exec(CMenuTarget* parent, std::string actionKey);
};


#endif

#endif

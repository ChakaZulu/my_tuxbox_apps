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


#ifndef __lcdcontroler__
#define __lcdcontroler__

#include <driver/framebuffer.h>

#include "menue.h"

#include <string>

class CLcdControler : public CMenuTarget
{
	private:
		CFrameBuffer	*frameBuffer;
		int x;
		int y;
		int width;
		int height;
		int hheight,mheight; // head/menu font height

		unsigned char contrast;
		unsigned char brightness;
		unsigned char brightnessstandby;


		std::string	name;

		CChangeObserver* observer;

		void paint();
		void setLcd();
		void paintSlider(int x, int y, unsigned int spos, float factor, const std::string text, std::string iconname, bool selected); // text in UTF-8

	public:

		CLcdControler(const char * const Name, CChangeObserver* Observer=NULL); // UTF-8

		void hide();
		int exec( CMenuTarget* parent, std::string actionKey );

};


#endif

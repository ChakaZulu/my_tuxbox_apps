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

#ifndef __color__
#define __color__

#define COL_INFOBAR_GREEN		254-8*5-5
#define COL_INFOBAR_RED 		254-8*5-4
#define COL_INFOBAR_YELLOW		254-8*5-3
#define COL_INFOBAR_BLUE		254-8*5-2
#define COL_INFOBAR_SHADOW		254-8*5-1
#define COL_INFOBAR			254-8*5

#define COL_MENUHEAD			254-8*4
#define COL_MENUCONTENT			254-8*3
#define COL_MENUCONTENTSELECTED		254-8*2
#define COL_MENUCONTENTINACTIVE		254-8*1

#define COL_BACKGROUND 			255

int convertSetupColor2RGB(unsigned char r, unsigned char g, unsigned char b);
int convertSetupAlpha2Alpha(unsigned char alpha);


#endif


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


#ifndef __messagebox__
#define __messagebox__

#include <gui/widget/hintbox.h>

#include <string>


class CMessageBox : CHintBox
{
 private:

	int  showbuttons;
	
	void paintButtons();

 public:
	enum result_
		{
			mbrYes    = 0,
			mbrNo     = 1,
			mbrCancel = 2,
			mbrBack   = 3
		} result;
	
	enum buttons_
		{
			mbYes= 0x01,
			mbNo = 0x02,
			mbCancel = 0x04,
			mbAll = 0x07,
			mbBack = 0x08
		} buttons;
	
	// Text & Caption are always UTF-8 encoded
	CMessageBox(const neutrino_locale_t Caption, const char * const Text, const int Width = 500, const char * const Icon = NULL, const CMessageBox::result_ Default = mbrYes, const uint ShowButtons = mbAll);

	int exec(int timeout = -1);
	
};

// Text is always UTF-8 encoded
int ShowMsgUTF(const neutrino_locale_t Caption, const char * const Text, const CMessageBox::result_ Default, const uint ShowButtons, const char * const Icon = NULL, const int Width = 450, const int timeout = -1); // UTF-8
int ShowMsgUTF(const neutrino_locale_t Caption, const std::string & Text, const CMessageBox::result_ Default, const uint ShowButtons, const char * const Icon = NULL, const int Width = 450, const int timeout = -1); // UTF-8

void DisplayErrorMessage(const char * const ErrorMsg); // UTF-8

#endif

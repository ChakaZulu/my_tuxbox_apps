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


#ifndef __update__
#define __update__

#include <gui/widget/menue.h>
#include <gui/widget/progressstatus.h>
#include <gui/widget/progresswindow.h>

#include <driver/framebuffer.h>

#include <string>

class CFlashUpdate : public CProgressWindow
{
	private:
		std::string	BasePath;
		std::string	ImageFile;
		std::string	VersionFile;

		std::string	installedVersion;
		std::string	newVersion;

		bool getInfo();
		bool getUpdateImage(std::string version);
		bool checkVersion4Update();

	public:
		CFlashUpdate();
		int exec( CMenuTarget* parent, std::string actionKey );

};

class CFlashExpert : public CProgressWindow
{
	private:
		int selectedMTD;

		void showMTDSelector(std::string actionkey);
		void showFileSelector(std::string actionkey);

		void readmtd(int readmtd);
		void writemtd(std::string filename, int mtdNumber);

	public:
		CFlashExpert();
		int exec( CMenuTarget* parent, std::string actionKey );

};


#endif

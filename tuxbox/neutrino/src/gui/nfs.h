/*
	Neutrino-GUI  -   DBoxII-Project

	NFS Mount/Umount GUI by Zwen
	
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

#ifndef __neutrino_nfs_gui__
#define __neutrino_nfs_gui__

#include <pthread.h>
#include "gui/widget/menue.h"

using namespace std;


class CNFSMountGui : public CMenuTarget
{
	public:
		enum FSType
		{
			NFS=0,
			CIFS
		};
	
   private:
		int menu();
		int menuEntry(int nr);
      static bool fsSupported(FSType fs);

		char m_entry[4][200];
      bool m_nfs_sup;
      bool m_cifs_sup;

   public:
		CNFSMountGui();
		~CNFSMountGui(){};
		int  exec(CMenuTarget* parent, string actionKey);
		static void mount(const char* ip, const char* dir, const char* local_dir, FSType fstype, 
								const char* username, const char* password, bool showerror=false);
		static void automount();
};

class CNFSUmountGui : public CMenuTarget
{
	private:

		int menu();

	public:
		CNFSUmountGui(){};
		~CNFSUmountGui(){};
		int  exec(CMenuTarget* parent, string actionKey);
		static void umount(string dir="");
};

class CNFSSmallMenu : public CMenuTarget
{
	private:

   public:
		CNFSSmallMenu(){};
		~CNFSSmallMenu(){};
		int exec( CMenuTarget* parent, string actionKey );
};

#endif



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


#include <config.h>

#include "localize.h"
#include <zapit/client/zapitclient.h> /* CZapitClient::Utf8_to_Latin1 */

#include <fstream>
#include <iostream>
#include <map>
#include <string>

static const char * iso639filename = "/share/iso-codes/iso-639.tab";
static std::map<std::string, std::string> iso639;

void initialize_iso639_map(void)
{
	std::string s, t, u, v;
	std::ifstream in(iso639filename);
	if (in.is_open())
	{
		while (in.peek() == '#')
			getline(in, s);
		while (in >> s >> t >> u)
		{
			getline(in, v);
			iso639[s] = v;
			if (s != t)
				iso639[t] = v;
		}
	}
	else
		std::cout << "Loading " << iso639filename << " failed." << std::endl;
}

const char * getISO639Description(const char *iso)
{
	std::map<std::string, std::string>::const_iterator it = iso639.find(std::string(iso));
	if (it == iso639.end())
		return iso;
	else
		return it->second.c_str();
}

void CLocaleManager::loadLocale(std::string locale)
{
	initialize_iso639_map();
	std::string filename[] = {"/var/tuxbox/config/locale/" + locale + ".locale",DATADIR  "/neutrino/locale/" + locale + ".locale"};
	FILE* fd = fopen(filename[0].c_str(), "r");
	if(!fd)
	{
		fd = fopen(filename[1].c_str(), "r");
		if(!fd)
		{		
			perror("cannot read locale");
			return;
		}
	}

	//	printf("read locale: %s\n", locale.c_str() );
	localeData.clear();

	char buf[1000];
	char valstr[1000];

	while(!feof(fd))
	{
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			char* tmpptr=buf;
			char* val= (char*) &valstr;
			bool keyfound = false;
			for(; (*tmpptr!=10) && (*tmpptr!=13);tmpptr++)
			{
				if((*tmpptr==' ') && (!keyfound))
				{
					keyfound=true;
					*tmpptr = 0;
				}
				else
				{
					if (keyfound)
					{
						*val = *tmpptr;
						val++;
					}
				}
			}
			*val = 0;

			std::string text= valstr;

			int pos;
			do
			{
				pos = text.find("\\n");
				if ( pos!=-1 )
				{
					text.replace(pos, 2, "\n", 1);
				}
			} while ( ( pos != -1 ) );

#warning cam.wrong is defined as locale but never used
#warning dhcp     is missing in locales (used in neutrino.cpp)
#warning NFS/CIFS is missing in locales (used in neutrino.cpp)
			if (
			    (buf[0] != 'c') &&
			    (buf[0] != 'k') &&
			    (buf[0] != 'l') &&
			    (buf[0] != 'n') &&
			    (buf[0] != 'o') &&
			    (buf[0] != 'v') &&
			    (buf[0] != 'z') &&
			    (strncmp(buf, "apidse", 6) != 0) &&
			    (strncmp(buf, "au", 2) != 0) &&
			    (strncmp(buf, "bouqueteditor", 13) != 0) &&
			    (strncmp(buf, "fa", 2) != 0) &&
			    (strncmp(buf, "fi", 2) != 0) &&
			    (strncmp(buf, "fo", 2) != 0) &&
			    (strncmp(buf, "me", 2) != 0) &&
			    (strncmp(buf, "mi", 2) != 0) &&
			    (strncmp(buf, "mp", 2) != 0) &&
			    (strncmp(buf, "pa", 2) != 0) &&
			    (strncmp(buf, "pic", 3) != 0) &&
			    (strncmp(buf, "ping", 4) != 0) &&
			    (strncmp(buf, "rec", 3) != 0) &&
			    (strncmp(buf, "sa", 2) != 0) &&
			    (strncmp(buf, "sc", 2) != 0) &&
			    (strncmp(buf, "st", 2) != 0) &&
			    (strncmp(buf, "timersettings", 13) != 0) &&
			    (strncmp(buf, "timing", 6) != 0) &&
			    ((strncmp(buf, "flashupdate", 11) != 0) || 
			     ((strcmp(buf, "flashupdate.actionreadflash") == 0) ||
			      (strcmp(buf, "flashupdate.getinfofile") == 0) ||
			      (strcmp(buf, "flashupdate.getupdatefile") == 0) ||
			      (strcmp(buf, "flashupdate.md5check") == 0) ||
			      (strncmp(buf, "flashupdate.msgbox", 18) == 0) ||
			      (strcmp(buf, "flashupdate.ready") == 0) ||
			      (strcmp(buf, "flashupdate.reallyflashmtd") == 0) ||
			      (strcmp(buf, "flashupdate.savesuccess") == 0) ||
			      (strcmp(buf, "flashupdate.versioncheck") == 0))) &&
			    ((strncmp(buf, "ma", 2) != 0) || (strcmp(buf, "mainsettings.savesettingsnow_hint") == 0)) &&
			    ((strncmp(buf, "se", 2) != 0) || (strcmp(buf, "servicemenu.reload_hint") == 0)) &&
			    ((strncmp(buf, "timerlist", 9) != 0) || (strncmp(buf, "timerlist.weekdays.hint", 23) == 0)) &&
			    (strcmp(buf, "bouquetlist.head") != 0) &&
			    (strcmp(buf, "dhcp") != 0) &&
			    (strcmp(buf, "epglist.head") != 0) &&
			    (strcmp(buf, "NFS/CIFS") != 0) &&
			    (strcmp(buf, "sleeptimerbox.title") != 0) &&
			    (strcmp(buf, "ucodecheck.head") != 0)
			     )
				text = CZapitClient::Utf8_to_Latin1(text);
			localeData[buf] = text;
		}
	}
	fclose(fd);
}

std::string CLocaleManager::getText(std::string keyName)
{
	std::string erg = localeData[keyName];
	if (erg == "")
		return keyName;
	else
		return erg;
}

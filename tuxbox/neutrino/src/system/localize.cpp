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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "localize.h"

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

	while(!feof(fd))
	{
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			char * val    = NULL;
			char * tmpptr = buf;

			for(; (*tmpptr!=10) && (*tmpptr!=13);tmpptr++)
			{
				if ((*tmpptr == ' ') && (val == NULL))
				{
					*tmpptr  = 0;
					val      = tmpptr + 1;
				}
			}
			*tmpptr = 0;

			if (val == NULL)
				continue;

			std::string text = val;

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

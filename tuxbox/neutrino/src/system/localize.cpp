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

#include <system/localize.h>

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

const char * getISO639Description(const char * const iso)
{
	std::map<std::string, std::string>::const_iterator it = iso639.find(std::string(iso));
	if (it == iso639.end())
		return iso;
	else
		return it->second.c_str();
}


const char * path[2] = {"/var/tuxbox/config/locale/", DATADIR "/neutrino/locale/"};

CLocaleManager::loadLocale_ret_t CLocaleManager::loadLocale(const char * const locale)
{
	int i;
	FILE * fd;

	initialize_iso639_map();

	for (i = 0; i < 2; i++)
	{
		std::string filename = path[i];
		filename += locale;
		filename += ".locale";
		
		fd = fopen(filename.c_str(), "r");
		if (fd)
			break;
	}
	
	if (i == 2)
	{		
		perror("cannot read locale");
		return NO_SUCH_LOCALE;
	}

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

#warning TODO: implement real check to determine whether we need a font with more than Basic Latin & Latin-1 Supplement characters
	return (
		(strcmp(locale, "bosanski") == 0) ||
		(strcmp(locale, "russkij") == 0) ||
		(strcmp(locale, "utf8") == 0)
		/* utf8.locale is a generic name that can be used for new locales which need characters outside the ISO-8859-1 character set */
		) ? UNICODE_FONT : ISO_8859_1_FONT;
}

const char * CLocaleManager::getText(const neutrino_locale_t keyName) const
{
	mapLocaleData::const_iterator it = localeData.find(keyName);
	if (it == localeData.end())
		return keyName;
	else
		return (it->second).c_str();
}

static const neutrino_locale_t locale_weekday[7] =
{
	LOCALE_DATE_SUN,
	LOCALE_DATE_MON,
	LOCALE_DATE_TUE,
	LOCALE_DATE_WED,
	LOCALE_DATE_THU,
	LOCALE_DATE_FRI,
	LOCALE_DATE_SAT
};

static const neutrino_locale_t locale_month[12] =
{
	LOCALE_DATE_JAN,
	LOCALE_DATE_FEB,
	LOCALE_DATE_MAR,
	LOCALE_DATE_APR,
	LOCALE_DATE_MAY,
	LOCALE_DATE_JUN,
	LOCALE_DATE_JUL,
	LOCALE_DATE_AUG,
	LOCALE_DATE_SEP,
	LOCALE_DATE_OCT,
	LOCALE_DATE_NOV,
	LOCALE_DATE_DEC
};


neutrino_locale_t CLocaleManager::getMonth(const struct tm * struct_tm_p)
{
	return locale_month[struct_tm_p->tm_mon];
}

neutrino_locale_t CLocaleManager::getWeekday(const struct tm * struct_tm_p)
{
	return locale_weekday[struct_tm_p->tm_wday];
}


/*
 * $Id: configfile.h,v 1.7 2002/08/31 00:23:38 dirch Exp $
 *
 * Copyright (C) 2001, 2002 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef __configfile_h__
#define __configfile_h__

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class CConfigFile
{
	private:

		typedef std::map <std::string, std::string> ConfigDataMap;

		ConfigDataMap configData;
		char delimiter;
		bool modifiedFlag;
		bool unknownKeyQueryedFlag;

	public:
		CConfigFile (const char p_delimiter);

		const bool loadConfig (const std::string p_filename);
		const bool saveConfig (const std::string p_filename);

		void clear();

		std::string getString (const std::string p_keyName, const std::string defaultValue = "");
		void setString (const std::string p_keyName, const std::string p_keyValue);
		int getInt (const std::string p_keyName, const int defaultValue = 0);
		void setInt (const std::string p_keyName, const int p_keyValue);
		long long getLongLong (const std::string p_keyName, const long long defaultValue = 0);
		void setLongLong (const std::string p_keyName, const long long p_keyValue);
		bool getBool (const std::string p_keyName, const bool defaultValue = false);
		void setBool (const std::string p_keyName, const bool p_keyValue);

		std::vector <std::string> getStringVector (const std::string p_keyName);
		void setStringVector (const std::string p_keyName, const std::vector <std::string> p_vec);

		std::vector <int> getIntVector (const std::string p_keyName);
		void setIntVector (const std::string p_keyName, const std::vector <int> p_vec);

		const bool getModifiedFlag () { return modifiedFlag; }
		void setModifiedFlag (const bool p_value) { modifiedFlag = p_value; }

		const bool getUnknownKeyQueryedFlag () { return unknownKeyQueryedFlag; }
		void setUnknownKeyQueryedFlag (const bool p_value) { unknownKeyQueryedFlag = p_value; }

};

#endif /* __configfile_h__ */

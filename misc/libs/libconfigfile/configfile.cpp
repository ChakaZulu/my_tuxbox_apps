/*
 * $Id: configfile.cpp,v 1.13 2002/12/11 16:39:34 thegoodguy Exp $
 *
 * configuration object for the d-box 2 linux project
 *
 * Copyright (C) 2001, 2002 Andreas Oberritter <obi@tuxbox.org>,
 *                          thegoodguy  <thegoodguy@tuxbox.org>
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

#include "configfile.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

CConfigFile::CConfigFile(const char p_delimiter)
{
	modifiedFlag = false;
	unknownKeyQueryedFlag = false;
	delimiter = p_delimiter;
}

void CConfigFile::clear()
{
	configData.clear();
}

//
// public file operation methods
//
const bool CConfigFile::loadConfig(const std::string filename)
{
	std::ifstream configFile(filename.c_str());

	if (configFile != NULL)
	{
		std::string s;
		clear();
		modifiedFlag = false;

		for (int linenr = 1; ; linenr++)
		{
			getline(configFile, s);
			if (configFile.fail())
				break;

			std::string::size_type i = s.find('=');
			if (i == std::string::npos)
			{
				std::cerr << filename << ": skipping line " << linenr << ": " << s << std::endl;
			}
			else
			{
				std::string::size_type j = s.find('#');
				if (j == std::string::npos)
					j = s.length();
				configData[s.substr(0, i)] = s.substr(i + 1, j - (i + 1));
			}
		}
		configFile.close();
		return true;
	}
	else
	{
		std::cerr << "[ConfigFile] Unable to open file " << filename << "for reading." << std::endl;
		return false;
	}
}

const bool CConfigFile::saveConfig(const std::string filename)
{
	std::ofstream configFile(filename.c_str());

	if (configFile != NULL)
	{
		for (ConfigDataMap::const_iterator it = configData.begin(); it != configData.end(); it++)
		{
			configFile << it->first << "=" << it->second << std::endl;
		}

		configFile.close();
		return true;
	}
	else
	{
		std::cerr << "[ConfigFile] Unable to open file " << filename << "for writing." << std::endl;
		return false;
	}
}



//
// private "store" methods
// 
void CConfigFile::storeBool(const std::string key, const bool val)
{
	if (val == true)
		configData[key] = std::string("true");
	else
		configData[key] = std::string("false");
}

void CConfigFile::storeInt32(const std::string key, const int32_t val)
{
	std::stringstream s;
	s << val;
	s >> configData[key];
}

void CConfigFile::storeInt64(const std::string key, const int64_t val)
{
	std::stringstream s;
	s << val;
	s >> configData[key];
}

void CConfigFile::storeString(const std::string key, const std::string val)
{
	configData[key] = val;
}



//
// public "get" methods
//
bool CConfigFile::getBool(const std::string key, const bool defaultVal)
{
	if (configData.find(key) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		storeBool(key, defaultVal);
	}

	return !((configData[key] == "false") || (configData[key] == "0"));
}

int32_t CConfigFile::getInt32(const std::string key, const int32_t defaultVal)
{
	if (configData.find(key) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		storeInt32(key, defaultVal);
	}

	return atoi(configData[key].c_str());
}

int64_t CConfigFile::getInt64(const std::string key, const int64_t defaultVal)
{
	if (configData.find(key) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		storeInt64(key, defaultVal);
	}

	return atoll(configData[key].c_str());
}

std::string CConfigFile::getString(const std::string key, const std::string defaultVal)
{
	if (configData.find(key) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		storeString(key, defaultVal);
	}

	return configData[key];
}

std::vector <int32_t> CConfigFile::getInt32Vector(const std::string key)
{
	std::string val = configData[key];
	std::vector <int32_t> vec;
	uint16_t length = 0;
	uint16_t pos = 0;
	uint16_t i;

	for (i = 0; i < val.length(); i++)
	{
		if (val[i] == delimiter)
		{
			vec.push_back(atoi(val.substr(pos, length).c_str()));
			pos = i + 1;
			length = 0;
		}
		else
		{
			length++;
		}
	}

	if (length == 0)
		unknownKeyQueryedFlag = true;
	else
		vec.push_back(atoi(val.substr(pos, length).c_str()));

	return vec;
}

std::vector <std::string> CConfigFile::getStringVector(const std::string key)
{
	std::string val = configData[key];
	std::vector <std::string> vec;
	uint16_t length = 0;
	uint16_t pos = 0;
	uint16_t i;

	for (i = 0; i < val.length(); i++)
	{
		if (val[i] == delimiter)
		{
			vec.push_back(val.substr(pos, length));
			pos = i + 1;
			length = 0;
		}
		else
		{
			length++;
		}
	}

	if (length == 0)
		unknownKeyQueryedFlag = true;
	else
		vec.push_back(val.substr(pos, length));

	return vec;
}



//
// public "set" methods
//
void CConfigFile::setBool(const std::string key, const bool val)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	bool oldVal = getBool(key);

	if ((oldVal != val) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		storeBool(key, val);
	}

	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

void CConfigFile::setInt32(const std::string key, int32_t val)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	int32_t oldVal = getInt32(key);

	if ((oldVal != val) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		storeInt32(key, val);
	}

	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

void CConfigFile::setInt64(const std::string key, const int64_t val)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	int64_t oldVal = getInt64(key);

	if ((oldVal != val) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		storeInt64(key, val);
	}

	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

void CConfigFile::setString(const std::string key, const std::string val)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	std::string oldVal = getString(key);
	
	if ((oldVal != val) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		storeString(key, val);
	}

	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

void CConfigFile::setInt32Vector(const std::string key, const std::vector<int32_t> vec)
{
	std::stringstream s;

	for (std::vector<int32_t>::const_iterator it = vec.begin(); ; )
	{
		s << (*it);
		it++;
		if (it == vec.end())
			break;
		s << delimiter;
	}
	s >> configData[key];
}

void CConfigFile::setStringVector(const std::string key, const std::vector<std::string> vec)
{
	configData[key] = "";

	for (std::vector<std::string>::const_iterator it = vec.begin(); ; )
	{
		configData[key] += *it;
		it++;
		if (it == vec.end())
			break;
		configData[key] += delimiter;
	}
}

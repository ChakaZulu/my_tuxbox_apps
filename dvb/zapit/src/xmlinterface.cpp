/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/src/Attic/xmlinterface.cpp,v 1.4 2002/09/30 12:58:04 thegoodguy Exp $
 *
 * xmlinterface for zapit - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "xmlinterface.h"

#include <stdio.h>


std::string convertForXML(const std::string s)
{
	std::string r;
	unsigned int i;
	for (i = 0; i < s.length(); i++)
	{
		switch (s[i])          // cf. xmltimpl.c: PREFIX(predefinedEntityName)
		{
		case '<':           
			r += "&lt;";
			break;
		case '>':
			r += "&gt;";
			break;
		case '&':
			r += "&amp;";
			break;
		case '\"':
			r += "&quot;";
			break;
		case '\'':
			r += "&apos;";
			break;
		default:
#ifdef MASK_SPECIAL_CHARACTERS
			if ((((unsigned char)s[i]) >= 32) && (((unsigned char)s[i]) < 128))
#else
			if (((unsigned char)s[i]) >= 32)    // skip strange characters
#endif
				r += s[i];
#ifdef MASK_SPECIAL_CHARACTERS
			else if (((unsigned char)s[i]) >= 128)
			{
				char val[5];
				sprintf(val, "%d", (unsigned char)s[i]);
				r = r + "&#" + val + ";";
			}
#endif
		}
	}
	return r;
}


std::string Utf8_to_Latin1(const std::string s)
{
	std::string r;
	unsigned int i;
	for (i = 0; i < s.length(); i++)
	{
		if ((i < s.length() - 3) && ((s[i] & 0xf0) == 0xf0))      // skip (can't be encoded in Latin1)
			i += 3;
		else if ((i < s.length() - 2) && ((s[i] & 0xe0) == 0xe0)) // skip (can't be encoded in Latin1)
			i += 2;
		else if ((i < s.length() - 1) && ((s[i] & 0xc0) == 0xc0))
		{
			r += ((s[i] & 3) << 6) | (s[i + 1] & 0x3f);
			i++;
		}
		else r += s[i];
	}
	return r;
}


XMLTreeParser* parseXmlFile(const std::string filename)
{
	char buffer[2048];
	XMLTreeParser* tree_parser;
	size_t done;
	size_t length;
	FILE* xml_file;

	xml_file = fopen(filename.c_str(), "r");

	if (xml_file == NULL)
	{
		perror(filename.c_str());
		return NULL;
	}

	tree_parser = new XMLTreeParser("ISO-8859-1");

	do
	{
		length = fread(buffer, 1, sizeof(buffer), xml_file);
		done = length < sizeof(buffer);

		if (!tree_parser->Parse(buffer, length, done))
		{
			printf("[xmlinterface.cpp] Error parsing \"%s\": %s at line %d\n",
			       filename.c_str(),
			       tree_parser->ErrorString(tree_parser->GetErrorCode()),
			       tree_parser->GetCurrentLineNumber());

			fclose(xml_file);
			delete tree_parser;
			return NULL;
		}
	}
	while (!done);

	fclose(xml_file);

	if (!tree_parser->RootNode())
	{
		delete tree_parser;
		return NULL;
	}
	return tree_parser;
}

/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/src/Attic/xmlinterface.cpp,v 1.22 2003/05/07 16:47:20 digi_casi Exp $
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

#include <cstdio>

#include <zapit/debug.h>
#include <zapit/xmlinterface.h>

#ifdef USE_LIBXML
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#else  /* USE_LIBXML */
#include <xmltok.h>
#endif /* USE_LIBXML */


unsigned long xmlGetNumericAttribute(const xmlNodePtr node, char *name, const int base)
{
	char *ptr = xmlGetAttribute(node, name);

	if (!ptr)
		return 0;

	return strtoul(ptr, 0, base);
}

long xmlGetSignedNumericAttribute(const xmlNodePtr node, char *name, const int base)
{
	char *ptr = xmlGetAttribute(node, name);

	if (!ptr)
		return 0;

	return strtol(ptr, 0, base);
}

xmlNodePtr xmlGetNextOccurence(xmlNodePtr cur, const char * s)
{
	while ((cur != NULL) && (strcmp(xmlGetName(cur), s) != 0))
		cur = cur->xmlNextNode;
	return cur;
}


std::string Unicode_Character_to_UTF8(const int character)
{
#ifdef USE_LIBXML
	xmlChar buf[5];
	int length = xmlCopyChar(4, buf, character);
	return std::string((char*)buf, length);
#else  /* USE_LIBXML */
	char buf[XML_UTF8_ENCODE_MAX];
	int length = XmlUtf8Encode(character, buf);
	return std::string(buf, length);
#endif /* USE_LIBXML */
}


std::string convert_UTF8_To_UTF8_XML(const std::string s)
{
	std::string r;

	for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
	{
		switch (*it)           // cf. http://www.w3.org/TR/xhtml1/dtds.html
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
			r += *it;      // all UTF8 chars with more than one byte are >= 0x80 !
/*
  default:
  // skip characters which are not part of ISO-8859-1
  // 0x00 - 0x1F & 0x80 - 0x9F
  // cf. http://czyborra.com/charsets/iso8859.html
  //
  // reason: sender name contain 0x86, 0x87 and characters below 0x20
  if ((((unsigned char)s[i]) & 0x60) != 0)
  r += *it;
*/
		}
	}
	return r;
}

std::string convert_to_UTF8(const std::string s)
{
	std::string r;
	
	for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
		r += Unicode_Character_to_UTF8((const unsigned char)*it);
		
	return r;
}

#ifdef USE_LIBXML
xmlDocPtr parseXmlFile(const std::string filename)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	
	doc = xmlParseFile(filename.c_str());

	if (doc == NULL)
	{
		WARN("Error parsing \"%s\"", filename.c_str());
		return NULL;
	}
	else
	{
		cur = xmlDocGetRootElement(doc);
		if (cur == NULL)
		{
			WARN("Empty document\n");
			xmlFreeDoc(doc);
			return NULL;
		}
		else
			return doc;
	}
}
#else /* USE_LIBXML */
xmlDocPtr parseXmlFile(const std::string filename)
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

	tree_parser = new XMLTreeParser(NULL);

	do
	{
		length = fread(buffer, 1, sizeof(buffer), xml_file);
		done = length < sizeof(buffer);

		if (!tree_parser->Parse(buffer, length, done))
		{
			WARN("Error parsing \"%s\": %s at line %d",
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
#endif /* USE_LIBXML */

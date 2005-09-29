/*
 * $Id: multilingual_network_name_descriptor.cpp,v 1.2 2005/09/29 23:49:44 ghostrider Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
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

#include <dvbsi++/multilingual_network_name_descriptor.h>

MultilingualNetworkName::MultilingualNetworkName(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	networkNameLength = buffer[3];
	networkName.assign((char *)&buffer[4], networkNameLength);
}

const std::string &MultilingualNetworkName::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &MultilingualNetworkName::getNetworkName(void) const
{
	return networkName;
}

MultilingualNetworkNameDescriptor::MultilingualNetworkNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 5] + 4)
		multilingualNetworkNames.push_back(new MultilingualNetworkName(&buffer[i + 2]));
}

MultilingualNetworkNameDescriptor::~MultilingualNetworkNameDescriptor(void)
{
	for (MultilingualNetworkNameIterator i = multilingualNetworkNames.begin(); i != multilingualNetworkNames.end(); ++i)
		delete *i;
}

const MultilingualNetworkNameList *MultilingualNetworkNameDescriptor::getMultilingualNetworkNames(void) const
{
	return &multilingualNetworkNames;
}


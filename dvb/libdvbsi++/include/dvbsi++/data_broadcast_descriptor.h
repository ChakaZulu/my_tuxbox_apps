/*
 * $Id: data_broadcast_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __data_broadcast_descriptor_h__
#define __data_broadcast_descriptor_h__

#include "descriptor.h"

typedef std::list<uint8_t> selectorByteList;
typedef selectorByteList::iterator selectorByteIterator;
typedef selectorByteList::const_iterator selectorByteConstIterator;

class DataBroadcastDescriptor : public Descriptor
{
	protected:
		unsigned dataBroadcastId			: 16;
		unsigned componentTag				: 8;
		unsigned selectorLength				: 8;
		selectorByteList selectorBytes;
		std::string iso639LanguageCode;
		unsigned textLength				: 8;
		std::string text;

	public:
		DataBroadcastDescriptor(const uint8_t * const buffer);

		uint16_t getDataBroadcastId(void) const;
		uint8_t getComponentTag(void) const;
		const selectorByteList *getSelectorBytes(void) const;
		const std::string &getIso639LanguageCode(void) const;
		const std::string &getText(void) const;
};

#endif /* __data_broadcast_descriptor_h__ */

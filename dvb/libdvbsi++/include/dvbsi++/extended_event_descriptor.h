/*
 * $Id: extended_event_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __extended_event_descriptor_h__
#define __extended_event_descriptor_h__

#include "descriptor.h"

class ExtendedEvent
{
	protected:
		unsigned itemDescriptionLength			: 8;
		std::string itemDescription;
		unsigned itemLength				: 8;
		std::string item;

	public:
		ExtendedEvent(const uint8_t * const buffer);

		const std::string &getItemDescription(void) const;
		const std::string &getItem(void) const;

	friend class ExtendedEventDescriptor;
};

typedef std::list<ExtendedEvent *> ExtendedEventList;
typedef ExtendedEventList::iterator ExtendedEventIterator;
typedef ExtendedEventList::const_iterator ExtendedEventConstIterator;

class ExtendedEventDescriptor : public Descriptor
{
	protected:
		unsigned descriptorNumber			: 4;
		unsigned lastDescriptorNumber			: 4;
		std::string iso639LanguageCode;
		unsigned lengthOfItems				: 8;
		ExtendedEventList items;
		unsigned textLength				: 8;
		std::string text;

	public:
		ExtendedEventDescriptor(const uint8_t * const buffer);
		~ExtendedEventDescriptor(void);

		uint8_t getDescriptorNumber(void) const;
		uint8_t getLastDescriptorNumber(void) const;
		const std::string &getIso639LanguageCode(void) const;
		const ExtendedEventList *getItems(void) const;
		const std::string &getText(void) const;
};

#endif /* __extended_event_descriptor_h__ */

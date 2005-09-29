/*
 * $Id: nvod_reference_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __nvod_reference_descriptor_h__
#define __nvod_reference_descriptor_h__

#include "descriptor.h"

class NvodReference
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned serviceId				: 16;

	public:
		NvodReference(const uint8_t * const buffer);

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getServiceId(void) const;
};

typedef std::list<NvodReference *> NvodReferenceList;
typedef NvodReferenceList::iterator NvodReferenceIterator;
typedef NvodReferenceList::const_iterator NvodReferenceConstIterator;

class NvodReferenceDescriptor : public Descriptor
{
	protected:
		NvodReferenceList nvodReferences;

	public:
		NvodReferenceDescriptor(const uint8_t * const buffer);
		~NvodReferenceDescriptor(void);

		const NvodReferenceList* getNvodReferences(void) const;
};

#endif /* __nvod_reference_descriptor_h__ */

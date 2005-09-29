/*
 * $Id: service_list_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __service_list_descriptor_h__
#define __service_list_descriptor_h__

#include "descriptor.h"

class ServiceListItem
{
	protected:
		unsigned serviceId				: 16;
		unsigned serviceType				: 8;

	public:
		ServiceListItem(const uint8_t * const buffer);

		uint16_t getServiceId(void) const;
		uint8_t getServiceType(void) const;
};

typedef std::list<ServiceListItem *> ServiceListItemList;
typedef ServiceListItemList::iterator ServiceListItemIterator;
typedef ServiceListItemList::const_iterator ServiceListItemConstIterator;

class ServiceListDescriptor : public Descriptor
{
	protected:
		ServiceListItemList serviceList;

	public:
		ServiceListDescriptor(const uint8_t * const buffer);
		~ServiceListDescriptor(void);

		const ServiceListItemList *getServiceList(void) const;
};

#endif /* __service_list_descriptor_h__ */

/*
 * $Id: service_description_section.h,v 1.1 2004/02/13 15:27:38 obi Exp $
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

#ifndef __service_description_section_h__
#define __service_description_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class ServiceDescription : public DescriptorContainer
{
	protected:
		unsigned serviceId				: 16;
		unsigned eitScheduleFlag			: 1;
		unsigned eitPresentFollowingFlag		: 1;
		unsigned runningStatus				: 3;
		unsigned freeCaMode				: 1;
		unsigned descriptorsLoopLength			: 12;

	public:
		ServiceDescription(const uint8_t * const buffer);

		uint16_t getServiceId(void) const;
		uint8_t getEitScheduleFlag(void) const;
		uint8_t getEitPresentFollowingFlag(void) const;
		uint8_t getRunningStatus(void) const;
		uint8_t getFreeCaMode(void) const;
};

typedef std::vector<ServiceDescription *> ServiceDescriptionVector;
typedef ServiceDescriptionVector::iterator ServiceDescriptionIterator;
typedef ServiceDescriptionVector::const_iterator ServiceDescriptionConstIterator;

class ServiceDescriptionSection : public LongCrcSection
{
	protected:
		unsigned originalNetworkId			: 16;
		ServiceDescriptionVector description;

	public:
		ServiceDescriptionSection(const uint8_t * const buffer);
		~ServiceDescriptionSection(void);

		static const enum PacketId PID = PID_SDT;
		static const enum TableId TID = TID_SDT_ACTUAL;
		static const uint32_t TIMEOUT = 3000;

		uint16_t getOriginalNetworkId(void) const;
		const ServiceDescriptionVector *getDescriptions(void) const;
};

typedef std::vector<ServiceDescriptionSection *> ServiceDescriptionSectionVector;
typedef ServiceDescriptionSectionVector::iterator ServiceDescriptionSectionIterator;
typedef ServiceDescriptionSectionVector::const_iterator ServiceDescriptionSectionConstIterator;

#endif /* __service_description_section_h__ */

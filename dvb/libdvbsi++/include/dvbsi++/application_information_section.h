/*
 * $Id: application_information_section.h,v 1.2 2004/05/31 21:21:22 obi Exp $
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

#ifndef __application_information_section_h__
#define __application_information_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"
#include "application_identifier.h"

class ApplicationInformation : public DescriptorContainer
{
	protected:
		ApplicationIdentifier *applicationIdentifier;
		unsigned applicationControlCode			: 8;
		unsigned applicationDescriptorsLoopLength	: 12;

	public:
		ApplicationInformation(const uint8_t * const buffer);
		~ApplicationInformation(void);

		const ApplicationIdentifier *getApplicationIdentifier(void) const;
		uint8_t getApplicationControlCode(void) const;

	friend class ApplicationInformationSection;
};

typedef std::vector<ApplicationInformation *> ApplicationInformationVector;
typedef ApplicationInformationVector::iterator ApplicationInformationIterator;
typedef ApplicationInformationVector::const_iterator ApplicationInformationConstIterator;

class ApplicationInformationSection : public LongCrcSection, public DescriptorContainer
{
	protected:
		unsigned commonDescriptorsLength		: 12;
		unsigned applicationLoopLength			: 12;
		ApplicationInformationVector applicationInformation;

	public:
		ApplicationInformationSection(const uint8_t * const buffer);
		~ApplicationInformationSection(void);

		static const enum TableId TID = TID_AIT;
		static const uint32_t TIMEOUT = 12000;

		const ApplicationInformationVector *getApplicationInformation(void) const;
};

typedef std::vector<ApplicationInformationSection *> ApplicationInformationSectionVector;
typedef ApplicationInformationSectionVector::iterator ApplicationInformationSectionIterator;
typedef ApplicationInformationSectionVector::const_iterator ApplicationInformationSectionConstIterator;

#endif /* __application_information_section_h__ */

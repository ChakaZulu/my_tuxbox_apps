/*
 * $Id: application_signalling_descriptor.cpp,v 1.2 2003/08/20 22:47:26 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#include <dvb/byte_stream.h>
#include <dvb/descriptor/application_signalling_descriptor.h>

ApplicationSignalling::ApplicationSignalling(const uint8_t * const buffer)
{
	applicationType = UINT16(&buffer[0]);
	aitVersionNumber = buffer[2] & 0x1f;
}

uint16_t ApplicationSignalling::getApplicationType(void) const
{
	return applicationType;
}

uint8_t ApplicationSignalling::getAitVersionNumber(void) const
{
	return aitVersionNumber;
}

ApplicationSignallingDescriptor::ApplicationSignallingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (uint16_t i = 0; i < descriptorLength; i += 3)
		applicationSignallings.push_back(new ApplicationSignalling(&buffer[i + 2]));
}

ApplicationSignallingDescriptor::~ApplicationSignallingDescriptor(void)
{
	for (ApplicationSignallingIterator i = applicationSignallings.begin(); i != applicationSignallings.end(); ++i)
		delete *i;
}

const ApplicationSignallingVector *ApplicationSignallingDescriptor::getApplicationSignallings(void) const
{
	return &applicationSignallings;
}


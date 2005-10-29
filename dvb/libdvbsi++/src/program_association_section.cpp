/*
 * $Id: program_association_section.cpp,v 1.4 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/program_association_section.h>

NetworkAssociation::NetworkAssociation(const uint8_t * const buffer)
{
	programNumber = UINT16(&buffer[0]);
	networkPid = DVB_PID(&buffer[2]);
}

uint16_t NetworkAssociation::getNetworkPid(void) const
{
	return networkPid;
}

ProgramAssociation::ProgramAssociation(const uint8_t * const buffer)
{
	programNumber = UINT16(&buffer[0]);
	programMapPid = DVB_PID(&buffer[2]);
}

uint16_t ProgramAssociation::getProgramNumber(void) const
{
	return programNumber;
}

uint16_t ProgramAssociation::getProgramMapPid(void) const
{
	return programMapPid;
}

ProgramAssociationSection::ProgramAssociationSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	for (size_t i = 8; i < sectionLength - 1; i += 4) {
		if (UINT16(&buffer[i]) == 0)
			networks.push_back(new NetworkAssociation(&buffer[i]));
		else
			programs.push_back(new ProgramAssociation(&buffer[i]));
	}
}

ProgramAssociationSection::~ProgramAssociationSection(void)
{
	for (NetworkAssociationIterator i = networks.begin(); i != networks.end(); ++i)
		delete *i;

	for (ProgramAssociationIterator i = programs.begin(); i != programs.end(); ++i)
		delete *i;
}

const NetworkAssociationList *ProgramAssociationSection::getNetworks(void) const
{
	return &networks;
}

const ProgramAssociationList *ProgramAssociationSection::getPrograms(void) const
{
	return &programs;
}


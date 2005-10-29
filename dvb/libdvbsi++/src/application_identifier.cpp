/*
 * $Id: application_identifier.cpp,v 1.2 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2004-2005 St�phane Est�-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/application_identifier.h>
#include <dvbsi++/byte_stream.h>
 
ApplicationIdentifier::ApplicationIdentifier(const uint8_t * const buffer)
{
	organisationId = r32(&buffer[0]);
	applicationId = r16(&buffer[4]);
}

uint32_t ApplicationIdentifier::getOrganisationId(void) const
{
	return organisationId;
}

uint16_t ApplicationIdentifier::getApplicationId(void) const
{
	return applicationId;
}

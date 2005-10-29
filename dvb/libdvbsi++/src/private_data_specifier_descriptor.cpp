/*
 * $Id: private_data_specifier_descriptor.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/private_data_specifier_descriptor.h>

PrivateDataSpecifierDescriptor::PrivateDataSpecifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	privateDataSpecifier = UINT32(&buffer[2]);
}

uint32_t PrivateDataSpecifierDescriptor::getPrivateDataSpecifier(void) const
{
	return privateDataSpecifier;
}


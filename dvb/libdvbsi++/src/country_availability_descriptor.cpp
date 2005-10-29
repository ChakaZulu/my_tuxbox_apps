/*
 * $Id: country_availability_descriptor.cpp,v 1.3 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/country_availability_descriptor.h>

CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	std::string countryCode;
	countryAvailabilityFlag = (buffer[2] >> 7) & 0x01;

	for (size_t i = 0; i < descriptorLength - 1; i += 3) {
		countryCode.assign((char *)&buffer[i + 3], 3);
		countryCodes.push_back(countryCode);
	}
}

uint8_t CountryAvailabilityDescriptor::getCountryAvailabilityFlag(void) const
{
	return countryAvailabilityFlag;
}

const CountryCodeList *CountryAvailabilityDescriptor::getCountryCodes(void) const
{
	return &countryCodes;
}


/*
 * $Id: parental_rating_descriptor.cpp,v 1.1 2003/07/17 01:07:42 obi Exp $
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

#include <dvb/descriptor/parental_rating_descriptor.h>

ParentalRating::ParentalRating(const uint8_t * const buffer)
{
	countryCode.assign((char *)&buffer[0], 3);
	rating = buffer[3];
}

std::string ParentalRating::getCountryCode(void) const
{
	return countryCode;
}

uint8_t ParentalRating::getRating(void) const
{
	return rating;
}

ParentalRatingDescriptor::ParentalRatingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (uint16_t i = 0; i < descriptorLength; i += 4)
		parentalRatings.push_back(new ParentalRating(&buffer[i + 2]));
}

ParentalRatingDescriptor::~ParentalRatingDescriptor(void)
{
	for (ParentalRatingIterator i = parentalRatings.begin(); i != parentalRatings.end(); ++i)
		delete *i;
}

const ParentalRatingVector *ParentalRatingDescriptor::getParentalRatings(void) const
{
	return &parentalRatings;
}


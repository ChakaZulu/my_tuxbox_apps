/*
 * $Id: bouquet_name_descriptor.cpp,v 1.1 2003/07/17 01:07:41 obi Exp $
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

#include <dvb/descriptor/bouquet_name_descriptor.h>

BouquetNameDescriptor::BouquetNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	bouquetName.assign((char *)&buffer[2], descriptorLength);
}

std::string BouquetNameDescriptor::getBouquetName(void) const
{
	return bouquetName;
}


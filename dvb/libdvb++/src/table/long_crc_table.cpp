/*
 * $Id: long_crc_table.cpp,v 1.1 2003/07/17 01:07:56 obi Exp $
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

#include <dvb/table/long_crc_table.h>

LongCrcTable::LongCrcTable(const uint8_t * const buffer) : LongTable(buffer)
{
	crc32 = (buffer[sectionLength - 1] << 24) |
		(buffer[sectionLength + 0] << 16) |
		(buffer[sectionLength + 1] << 8) |
		(buffer[sectionLength + 2]);
}

uint32_t LongCrcTable::getCrc32(void) const
{
	return crc32;
}


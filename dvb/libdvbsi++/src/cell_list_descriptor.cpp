/*
 * $Id: cell_list_descriptor.cpp,v 1.2 2004/02/13 17:51:08 obi Exp $
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

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/cell_list_descriptor.h>

Subcell::Subcell(const uint8_t * const buffer)
{
	cellIdExtension = buffer[0];
	subcellLatitude = UINT16(&buffer[1]);
	subcellLongitude = UINT16(&buffer[3]);
	subcellExtendOfLatitude = UINT16(&buffer[5]) >> 4;
	subcellExtendOfLongitude = UINT16(&buffer[6]) & 0x0fff;
}

uint8_t Subcell::getCellIdExtension(void) const
{
	return cellIdExtension;
}

uint16_t Subcell::getSubcellLatitude(void) const
{
	return subcellLatitude;
}

uint16_t Subcell::getSubcellLongtitude(void) const
{
	return subcellLongitude;
}

uint16_t Subcell::getSubcellExtendOfLatitude(void) const
{
	return subcellExtendOfLatitude;
}

uint16_t Subcell::getSubcellExtendOfLongtitude(void) const
{
	return subcellExtendOfLongitude;
}

Cell::Cell(const uint8_t * const buffer)
{
	cellId = UINT16(&buffer[0]);
	cellLatitude = UINT16(&buffer[2]);
	cellLongtitude = UINT16(&buffer[4]);
	cellExtendOfLatitude = UINT16(&buffer[6]) >> 4;
	cellExtendOfLongtitude = UINT16(&buffer[7]) & 0x0fff;
	subcellInfoLoopLength = buffer[9];

	for (size_t i = 0; i < subcellInfoLoopLength; i += 8)
		subcells.push_back(new Subcell(&buffer[i + 10]));
}

Cell::~Cell(void)
{
	for (SubcellIterator i = subcells.begin(); i != subcells.end(); ++i)
		delete *i;
}

uint16_t Cell::getCellId(void) const
{
	return cellId;
}

uint16_t Cell::getCellLatitude(void) const
{
	return cellLatitude;
}

uint16_t Cell::getCellLongtitude(void) const
{
	return cellLongtitude;
}

uint16_t Cell::getCellExtendOfLatitude(void) const
{
	return cellExtendOfLatitude;
}

uint16_t Cell::getCellExtendOfLongtitude(void) const
{
	return cellExtendOfLongtitude;
}

const SubcellVector *Cell::getSubcells(void) const
{
	return &subcells;
}

CellListDescriptor::CellListDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 11] + 10)
		cells.push_back(new Cell(&buffer[i + 2]));
}

CellListDescriptor::~CellListDescriptor(void)
{
	for (CellIterator i = cells.begin(); i != cells.end(); ++i)
		delete *i;
}

const CellVector *CellListDescriptor::getCells(void) const
{
	return &cells;
}


/*
 * $Id: eit.cpp,v 1.2 2003/08/20 22:47:35 obi Exp $
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
#include <dvb/table/eit.h>

Event::Event(const uint8_t * const buffer)
{
	eventId = UINT16(&buffer[0]);
	startTimeMjd = UINT16(&buffer[2]);
	startTimeBcd = (buffer[4] << 16) | UINT16(&buffer[5]);
	duration = (buffer[7] << 16) | UINT16(&buffer[8]);
	runningStatus = (buffer[10] >> 5) & 0x07;
	freeCaMode = (buffer[10] >> 4) & 0x01;
	descriptorsLoopLength = DVB_LENGTH(&buffer[10]);

	for (uint16_t i = 12; i < descriptorsLoopLength + 12; i += buffer[i + 1] + 2)
		descriptor(&buffer[i]);
}

uint16_t Event::getEventId(void) const
{
	return eventId;
}

uint16_t Event::getStartTimeMjd(void) const
{
	return startTimeMjd;
}

uint32_t Event::getStartTimeBcd(void) const
{
	return startTimeBcd;
}

uint32_t Event::getDuration(void) const
{
	return duration;
}

uint8_t Event::getRunningStatus(void) const
{
	return runningStatus;
}

uint8_t Event::getFreeCaMode(void) const
{
	return freeCaMode;
}

EventInformationTable::EventInformationTable(const uint8_t * const buffer) : LongCrcTable(buffer)
{
	transportStreamId = UINT16(&buffer[8]);
	originalNetworkId = UINT16(&buffer[10]);
	segmentLastSectionNumber = buffer[12];
	lastTableId = buffer[13];

	for (uint16_t i = 14; i < sectionLength - 1; i += DVB_LENGTH(&buffer[i + 10]) + 12)
		events.push_back(new Event(&buffer[i]));
}

EventInformationTable::~EventInformationTable(void)
{
	for (EventIterator i = events.begin(); i != events.end(); ++i)
		delete *i;
}

uint16_t EventInformationTable::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t EventInformationTable::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint8_t EventInformationTable::getLastSectionNumber(void) const
{
	return lastSectionNumber;
}

uint8_t EventInformationTable::getLastTableId(void) const
{
	return lastTableId;
}

const EventVector *EventInformationTable::getEvents(void) const
{
	return &events;
}


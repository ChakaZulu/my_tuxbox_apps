/*
 * $Id: tot.h,v 1.2 2003/08/20 22:47:24 obi Exp $
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

#ifndef __dvb_table_tot_h__
#define __dvb_table_tot_h__

#include <dvb/descriptor/container.h>
#include "short_crc_table.h"

class TimeOffsetTable : public ShortCrcTable, public DescriptorContainer
{
	protected:
		unsigned utcTimeMjd				: 16;
		unsigned utcTimeBcd				: 24;
		unsigned descriptorsLoopLength			: 12;

	public:
		TimeOffsetTable(const uint8_t * const buffer);

		static const enum PacketId PID = PID_TOT;
		static const enum TableId TID = TID_TOT;
		static const uint32_t TIMEOUT = 36000;

		uint16_t getUtcTimeMjd(void) const;
		uint32_t getUtcTimeBcd(void) const;
};

typedef std::vector<TimeOffsetTable *> TimeOffsetTableVector;
typedef TimeOffsetTableVector::iterator TimeOffsetTableIterator;
typedef TimeOffsetTableVector::const_iterator TimeOffsetTableConstIterator;

#endif /* __dvb_table_tot_h__ */

/*
 * $Id: nit.h,v 1.2 2003/08/20 22:47:24 obi Exp $
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

#ifndef __dvb_table_nit_h__
#define __dvb_table_nit_h__

#include <dvb/descriptor/container.h>
#include "long_crc_table.h"

class TransportStreamInfo : public DescriptorContainer
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned transportDescriptorsLength		: 12;

	public:
		TransportStreamInfo(const uint8_t * const buffer);

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
};

typedef std::vector<TransportStreamInfo *> TransportStreamInfoVector;
typedef TransportStreamInfoVector::iterator TransportStreamInfoIterator;
typedef TransportStreamInfoVector::const_iterator TransportStreamInfoConstIterator;

class NetworkInformationTable : public LongCrcTable, public DescriptorContainer
{
	protected:
		unsigned networkDescriptorsLength		: 12;
		unsigned transportStreamLoopLength		: 12;
		TransportStreamInfoVector tsInfo;

	public:
		NetworkInformationTable(const uint8_t * const buffer);
		~NetworkInformationTable(void);

		static const enum PacketId PID = PID_NIT;
		static const enum TableId TID = TID_NIT_ACTUAL;
		static const uint32_t TIMEOUT = 12000;

		const TransportStreamInfoVector *getTsInfo(void) const;
};

typedef std::vector<NetworkInformationTable *> NetworkInformationTableVector;
typedef NetworkInformationTableVector::iterator NetworkInformationTableIterator;
typedef NetworkInformationTableVector::const_iterator NetworkInformationTableConstIterator;

#endif /* __dvb_table_nit_h__ */

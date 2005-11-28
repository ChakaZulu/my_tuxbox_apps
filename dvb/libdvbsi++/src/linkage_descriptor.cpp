/*
 * $Id: linkage_descriptor.cpp,v 1.5 2005/11/28 16:20:24 ghostrider Exp $
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
#include <dvbsi++/linkage_descriptor.h>

LinkageDescriptor::LinkageDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	transportStreamId = UINT16(&buffer[2]);
	originalNetworkId = UINT16(&buffer[4]);
	serviceId = UINT16(&buffer[6]);
	linkageType = buffer[8];

	if (linkageType != 0x08)
		for (size_t i = 0; i < descriptorLength - 7; ++i)
			privateDataBytes.push_back(buffer[i + 9]);

	else {
		handOverType = (buffer[9] >> 4) & 0x0f;
		originType = buffer[9] & 0x01;

		uint8_t offset = 0;

		if ((handOverType >= 0x01) && (handOverType <= 0x03)) {
			networkId = UINT16(&buffer[10]);
			offset += 2;
		}

		if (originType == 0x00) {
			initialServiceId = UINT16(&buffer[offset + 10]);
			offset += 2;
		}

		for (size_t i = 0; i < descriptorLength - (offset + 8); ++i)
			privateDataBytes.push_back(buffer[i + offset + 10]);
	}
}

uint16_t LinkageDescriptor::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t LinkageDescriptor::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint16_t LinkageDescriptor::getServiceId(void) const
{
	return serviceId;
}

uint8_t LinkageDescriptor::getLinkageType(void) const
{
	return linkageType;
}

const PrivateDataByteVector *LinkageDescriptor::getPrivateDataBytes(void) const
{
	return &privateDataBytes;
}

uint8_t LinkageDescriptor::getHandOverType(void) const
{
	return handOverType;
}

uint8_t LinkageDescriptor::getOriginType(void) const
{
	return originType;
}

uint16_t LinkageDescriptor::getNetworkId(void) const
{
	return networkId;
}

uint16_t LinkageDescriptor::getInitialServiceId(void) const
{
	return initialServiceId;
}


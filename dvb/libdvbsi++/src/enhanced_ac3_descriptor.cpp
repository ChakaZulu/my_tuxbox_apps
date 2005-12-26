/*
 * $Id: enhanced_ac3_descriptor.cpp,v 1.1 2005/12/26 20:48:58 mws Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/enhanced_ac3_descriptor.h"

EnhancedAC3Descriptor::EnhancedAC3Descriptor(const uint8_t* const buffer):Descriptor(buffer)
{
	componentTypeFlag = (buffer[2] >> 7) & 0x01;
	bsidFlag = (buffer[2] >> 6) & 0x01;
	mainidFlag = (buffer[2] >> 5) & 0x01;
	asvcFlag = (buffer[2] >> 4) & 0x01;
	mixInfoExistsFlag = (buffer[2] >> 3) & 0x01;
	substream1Flag = (buffer[2] >> 2) & 0x01;
	substream2Flag = (buffer[2] >> 1) & 0x01;
	substream3Flag = buffer[2] & 0x01;

	size_t i = 3;
	if (componentTypeFlag == 0x01)
	{
		componentType = buffer[i++];
	}
	if (bsidFlag == 0x01)
	{
		bsid = buffer[i++];
	}
	if (mainidFlag == 0x01)
	{
		mainid = buffer[i++];
	}
	if (asvcFlag == 0x01)
	{
		avsc = buffer[i++];
	}
	if (substream1Flag == 0x01)
	{
		substream1 = buffer[i++];
	}
	if (substream2Flag == 0x01)
	{
		substream2 = buffer[i++];
	}
	if (substream3Flag == 0x01)
	{
		substream3 = buffer[i++];
	}

	additionalInfo.resize(descriptorLength - (i - 2));
	memcpy(&additionalInfo[0], buffer + i, descriptorLength - (i - 2));
}

EnhancedAC3Descriptor::~EnhancedAC3Descriptor()
{
}

uint8_t EnhancedAC3Descriptor::getComponentTypeFlag() const
{
	return componentTypeFlag;
}

uint8_t EnhancedAC3Descriptor::getBsidFlag() const
{
	return bsidFlag;
}

uint8_t EnhancedAC3Descriptor::getMainidFlag() const
{
	return mainidFlag;
}

uint8_t EnhancedAC3Descriptor::getAsvcFlag() const
{
	return asvcFlag;
}

uint8_t EnhancedAC3Descriptor::getMixInfoExistsFlag() const
{
	return mixInfoExistsFlag;
}

uint8_t EnhancedAC3Descriptor::getSubstream1Flag() const
{
	return substream1Flag;
}

uint8_t EnhancedAC3Descriptor::getSubstream2Flag() const
{
	return substream2Flag;
}

uint8_t EnhancedAC3Descriptor::getSubstream3Flag() const
{
	return substream3Flag;
}

uint8_t EnhancedAC3Descriptor::getComponentType() const
{
	return componentType;
}

uint8_t EnhancedAC3Descriptor::getBsid() const
{
	return bsid;
}

uint8_t EnhancedAC3Descriptor::getMainid() const
{
	return mainid;
}

uint8_t EnhancedAC3Descriptor::getAvsc() const
{
	return substream3Flag;
}

uint8_t EnhancedAC3Descriptor::getSubstream1() const
{
	return substream1;
}

uint8_t EnhancedAC3Descriptor::getSubstream2() const
{
	return substream2;
}

uint8_t EnhancedAC3Descriptor::getSubstream3() const
{
	return substream3;
}

const AdditionalInfoVector* EnhancedAC3Descriptor::getAdditionalInfo() const
{
	return &additionalInfo;
}

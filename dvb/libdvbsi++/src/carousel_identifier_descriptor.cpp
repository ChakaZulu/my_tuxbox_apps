/*
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

#include <dvbsi++/carousel_identifier_descriptor.h>
#include <dvbsi++/byte_stream.h>

CarouselIdentifierDescriptor::CarouselIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	carouselId = r32(&buffer[2]);
	formatId = buffer[6];
	switch ( formatId ) {
	case 0x00:
		for (size_t i = 0; i < descriptorLength - 5; ++i)
                        privateDataBytes.push_back(buffer[i + 7]);
		break;
	case 0x01:
		moduleVersion = buffer[7];
		moduleId = r16(&buffer[8]);
		blockSize = r16(&buffer[10]);
		moduleSize = r32(&buffer[12]);
		compressionMethod = buffer[16];
		originalSize = r32(&buffer[17]);
		timeout = buffer[21];
		objectKeyLength = buffer[22];
		objectKey.assign((char *)&buffer[23], objectKeyLength);
		for (size_t i = 0; i < descriptorLength - objectKeyLength - 21; ++i)
                        privateDataBytes.push_back(buffer[i + objectKeyLength + 23]);
		break;
	}
	
	
}

uint32_t CarouselIdentifierDescriptor::getCarouselId(void) const
{
	return carouselId;
}

uint8_t CarouselIdentifierDescriptor::getFormatId(void) const
{
	return formatId;
}

uint8_t CarouselIdentifierDescriptor::getModuleVersion(void) const
{
	return moduleVersion;
}

uint16_t CarouselIdentifierDescriptor::getModuleId(void) const
{
	return moduleId;
}

uint16_t CarouselIdentifierDescriptor::getBlockSize(void) const
{
	return blockSize;
}

uint32_t CarouselIdentifierDescriptor::getModuleSize(void) const
{
	return moduleSize;
}

uint8_t CarouselIdentifierDescriptor::getCompressionMethod(void) const
{
	return compressionMethod;
}

uint32_t CarouselIdentifierDescriptor::getOriginalSize(void) const
{
	return originalSize;
}

uint8_t CarouselIdentifierDescriptor::getTimeout(void) const
{
	return timeout;
}

const std::string &CarouselIdentifierDescriptor::getObjectKey(void) const
{
	return objectKey;
}

const PrivateDataByteVector *CarouselIdentifierDescriptor::getPrivateDataBytes(void) const
{
        return &privateDataBytes;
}

/*
 * $Id: crc32_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 St�phane Est�-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/crc32_descriptor.h>
#include <dvbsi++/byte_stream.h>

Crc32Descriptor::Crc32Descriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(4);

	crc32 = r32(&buffer[2]);
}

uint32_t Crc32Descriptor::getCrc32(void) const
{
	return crc32;
}

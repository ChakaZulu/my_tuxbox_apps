/*
 * $Id: audio_stream_descriptor.h,v 1.2 2003/08/20 22:47:17 obi Exp $
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

#ifndef __dvb_descriptor_audio_stream_descriptor_h__
#define __dvb_descriptor_audio_stream_descriptor_h__

#include "descriptor.h"

class AudioStreamDescriptor : public Descriptor
{
	protected:
		unsigned freeFormatFlag				: 1;
		unsigned id					: 1;
		unsigned layer					: 2;
		unsigned variableRateAudioIndicator		: 1;

	public:
		AudioStreamDescriptor(const uint8_t * const buffer);

		uint8_t getFreeFormatFlag(void) const;
		uint8_t getId(void) const;
		uint8_t getLayer(void) const;
		uint8_t getVariableRateAudioIndicator(void) const;
};

#endif /* __dvb_descriptor_audio_stream_descriptor_h__ */

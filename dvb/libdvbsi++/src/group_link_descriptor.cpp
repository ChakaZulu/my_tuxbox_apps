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

#include <dvbsi++/group_link_descriptor.h>
#include <dvbsi++/byte_stream.h>

GroupLinkDescriptor::GroupLinkDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	position = buffer[2];
	groupId = r32(&buffer[3]);
}

uint8_t GroupLinkDescriptor::getPosition(void) const
{
	return position;
}

uint32_t GroupLinkDescriptor::getGroupId(void) const
{
	return groupId;
}

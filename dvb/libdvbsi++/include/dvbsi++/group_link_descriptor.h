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

#ifndef __group_link_descriptor_h__
#define __group_link_descriptor_h__

#include "descriptor.h"

class GroupLinkDescriptor : public Descriptor
{
	protected:
		unsigned position				: 8;
		unsigned groupId				: 32;

	public:
		GroupLinkDescriptor(const uint8_t * const buffer);

		uint8_t getPosition(void) const;
		uint32_t getGroupId(void) const;
};

#endif /* __group_link_descriptor_h__ */

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

#ifndef __dvb_html_application_descriptor_h__
#define __dvb_html_application_descriptor_h__

#include "descriptor.h"

typedef std::list<uint16_t> ApplicationIdList;
typedef ApplicationIdList::iterator ApplicationIdIterator;
typedef ApplicationIdList::const_iterator ApplicationIdConstIterator;

class DvbHtmlApplicationDescriptor : public Descriptor
{
	protected:
		unsigned appidSetLength				: 8;
		ApplicationIdList applicationIds;
		std::string parameter;

	public:
		DvbHtmlApplicationDescriptor(const uint8_t * const buffer);

		const ApplicationIdList *getApplicationIds(void) const;
		const std::string &getParameter(void) const;
};

#endif /* __dvb_html_application_descriptor_h__ */

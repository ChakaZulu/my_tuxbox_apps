/*
 * $Id: cam.h,v 1.13 2002/05/13 17:17:04 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __cam_h__
#define __cam_h__

#include <ost/ca.h>

#include "ci.h"

class CCam
{
	private:
		unsigned char camdBuffer[1024];
		int camdSocket;

		bool camdConnect ();
		void camdDisconnect ();

		ca_msg_t CCam::getMessage (unsigned short length);
		int sendMessage (unsigned char * data, unsigned short length);

	public:
		CCam();
		~CCam();

		int reset (unsigned short originalNetworkId);
		int setCaPmt (CCaPmt * caPmt);
};

#endif /* __cam_h__ */

/*
 * $Id: cam.h,v 1.17 2002/09/21 17:58:42 thegoodguy Exp $
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

#include "ci.h"

class CCam
{
	private:
		int camdSocket;

		bool camdConnect ();
		void camdDisconnect ();

		int sendMessage (unsigned char * data, unsigned short length);

	public:
		CCam();

		int setCaPmt (CCaPmt * caPmt);
};

#endif /* __cam_h__ */

#ifndef __basicserver__
#define __basicserver__

/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/Attic/basicserver.h,v 1.1 2002/10/17 20:51:18 thegoodguy Exp $
 *
 * Basic Server Class (Neutrino) - DBoxII-Project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
 *
 * License: GPL
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

#include <zapit/client/basicmessage.h>

class CBasicServer
{
 public:
	bool run(const char* socketname, bool (parse_command)(CBasicMessage::Header &rmsg, int connfd));
};

#endif

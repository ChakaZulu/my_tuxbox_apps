/*
 * $Id: pat.h,v 1.10 2002/05/13 17:17:05 obi Exp $
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

#ifndef __pat_h__
#define __pat_h__

#include <channel.h>
#include <zapost/frontend.h>

int parse_pat (int demux_fd, CZapitChannel * channel);
int fake_pat (unsigned short original_network_id, FrontendParameters feparams);

#endif /* __pat_h__ */

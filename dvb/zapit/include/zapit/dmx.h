/*
 * $Id: dmx.h,v 1.6 2002/08/24 11:10:53 obi Exp $
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

#ifndef __dmx_h__
#define __dmx_h__

/* system c */
#include <stdint.h>
#include <time.h>

/* nokia api */
#include <ost/dmx.h>
#define DEMUX_DEV "/dev/dvb/card0/demux0"

int setDmxSctFilter (int fd, unsigned short pid, unsigned char * filter, unsigned char * mask);
int setDmxPesFilter (int fd, dmxOutput_t output, dmxPesType_t pesType, unsigned short pid);
int unsetDmxFilter (int fd);

#endif /* __dmx_h__ */

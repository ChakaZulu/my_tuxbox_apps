/*
 * $Id: dmx.h,v 1.4 2002/05/15 20:51:44 obi Exp $
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
#if (DVB_API_VERSION == 1)
#include <ost/dmx.h>
#define DEMUX_DEV "/dev/dvb/card0/demux0"
#else
#include <linux/dvb/dmx.h>
#define DEMUX_DEV "/dev/dvb/adapter0/demux0"
#endif

int setDmxSctFilter (int fd, dvb_pid_t pid, unsigned char filter0, unsigned char filter1 = 0x00, unsigned char filter2 = 0x00);
int setDmxPesFilter (int fd, dmxOutput_t output, dmxPesType_t pesType, dvb_pid_t pid);
int unsetDmxFilter (int fd);

#endif /* __dmx_h__ */

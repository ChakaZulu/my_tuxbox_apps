/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>
#include <iostream.h>

#include <ost/dmx.h>

#include "nit.h"

#define BSIZE 10000

int nit::getTransportStreams(channels *channels, int diseqc = 1)
{
	
	long fd, r;
	struct dmxSctFilterParams flt;
	unsigned char buffer[BSIZE];
	int countTS = 0;

	// Lies den NIT
	fd=open("/dev/ost/demux0", O_RDONLY);
	
	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	
	flt.pid            = 0x10;
	flt.filter.filter[0] = 0x41;
	flt.filter.mask[0] = 0xF0;
	flt.timeout        = 10000;
	flt.flags          = DMX_IMMEDIATE_START | DMX_CHECK_CRC; // | DMX_ONESHOT;
	
	ioctl(fd, DMX_SET_FILTER, &flt);
	
	int sec_counter = 0;
	do
	{
		r = BSIZE;	
		r=read(fd, buffer, r);
		if (r < 1)
			return 0;
		int descriptor_length = (((buffer[8] & 0x0f) << 8) | buffer[9]);
		int start = descriptor_length + 10;

		int counter = 0;
		int startbyte = start + 2;


		while (startbyte + counter < r - 5)
		{
			int transport_stream_id = (buffer[startbyte + counter] << 8) | buffer[startbyte + 1 + counter];
			int original_network_id = (buffer[startbyte + 2 + counter] << 8) | buffer[startbyte + 3 + counter];
			long frequ = 0;
			int symbol = 0;
			int polarization = -1;
			int fec = -1;
			
			int descriptors_length = ((buffer[startbyte + 4 + counter] & 0x0f) << 8) | buffer[startbyte + 5 + counter];
			int start = startbyte + counter + 6;
			while (start < startbyte + counter + 6 + descriptors_length)
			{
				
				if (buffer[start] == 0x44) // cable
				{
					frequ = (((buffer[start + 2] & 0xf0) >> 4) * 1000) + ((buffer[start + 2] & 0xf) * 100) + (((buffer[start + 3] & 0xf0) >> 4) * 10) + ((buffer[start + 3] & 0xf));
					frequ *= 10;
					symbol = (((buffer[start + 9] & 0xf0) >> 4) * 100000) + ((buffer[start + 9] & 0xf) * 10000) + (((buffer[start + 10] & 0xf0) >> 4) * 1000) + ((buffer[start + 10] & 0xf) * 100) + (((buffer[start + 11] & 0xf0) >> 4) * 10) + ((buffer[start + 11] & 0xf));
					printf ("%d %d %d %d %d %d %d %d\n", (buffer[start + 2] & 0xf0) >> 4, (buffer[start + 2] & 0x0f), ((buffer[start + 3] & 0xf0) >> 4), (buffer[start + 3] & 0xf), (buffer[start + 4] & 0xf0)>>4, (buffer[start + 4] & 0xf), (buffer[start + 5] & 0xf0)>>4, (buffer[start + 5] & 0xf));
					countTS++;
				}
				else if (buffer[start] == 0x43) // sat
				{
					frequ = (((buffer[start + 2] & 0xf0) >> 4) * 100000) + ((buffer[start + 2] & 0xf) * 10000) + (((buffer[start + 3] & 0xf0) >> 4) * 1000) + ((buffer[start + 3] & 0xf) * 100) + (((buffer[start + 4] & 0xf0) >> 4) * 10) + ((buffer[start + 4] & 0xf));
					symbol = (((buffer[start + 9] & 0xf0) >> 4) * 100000) + ((buffer[start + 9] & 0xf) * 10000) + (((buffer[start + 10] & 0xf0) >> 4) * 1000) + ((buffer[start + 10] & 0xf) * 100) + (((buffer[start + 11] & 0xf0) >> 4) * 10) + ((buffer[start + 11] & 0xf));
					polarization = ((buffer[start + 8] & 0x60) >> 5);
					fec = buffer[start + 12] & 0x0f;
					countTS++;
				}
				start += buffer[start + 1] + 2;
			
			}
			(*channels).addTS(transport_stream_id, original_network_id, frequ, symbol, polarization, fec, diseqc);

			counter += 6 + descriptors_length;
		}

	} while( buffer[7] != sec_counter++);
	ioctl(fd,DMX_STOP,0);
	close (fd);
	printf("Found Transponders: %d\n", countTS);

	return countTS;
}

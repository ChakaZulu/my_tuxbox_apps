/*
 * $Id: nit.cpp,v 1.19 2002/06/27 19:46:00 Homar Exp $
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

/* system c */
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* system c++ */
#include <map>

/* zapit */
#include <getservices.h>
#include <zapost/dmx.h>

#include "descriptors.h"
#include "nit.h"

extern std::map <unsigned int, transponder> transponders;

int parse_nit (unsigned char DiSEqC)
{
	int demux_fd;
	unsigned char buffer[1024];
	unsigned char section = 0;

	/* position in buffer */
	unsigned short pos;
	unsigned short pos2;

	/* network_information_section elements */
	unsigned short section_length;
	unsigned short network_descriptors_length;
	unsigned short transport_descriptors_length;
	unsigned short transport_stream_loop_length;
	unsigned short transport_stream_id;
	unsigned short original_network_id;

	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[nit.cpp] " DEMUX_DEV);
		return -1;
	}

	if (setDmxSctFilter(demux_fd, 0x0010, 0x40) < 0)
	{
		return -1;
	}

	do
	{
		if ((read(demux_fd, buffer, sizeof(buffer))) < 0)
		{
			perror("[nit.cpp] read");
			close(demux_fd);
			return -1;
		}

		section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
		network_descriptors_length = ((buffer[8] & 0x0F) << 8) | buffer[9];

		for (pos = 10; pos < network_descriptors_length + 10; pos += buffer[pos + 1] + 2)
		{
			switch (buffer[pos])
			{
			case 0x0F:
				Private_data_indicator_descriptor(buffer + pos);
				break;

			case 0x40:
				network_name_descriptor(buffer + pos);
				break;

			case 0x4A:
				linkage_descriptor(buffer + pos);
				break;

			case 0x5F:
				private_data_specifier_descriptor(buffer + pos);
				break;

			case 0x80: /* unknown, Eutelsat 13.0E */
				break;

			case 0x90: /* unknown, Eutelsat 13.0E */
				break;

			default:
				printf("[nit.cpp] descriptor_tag (a): %02x\n", buffer[pos]);
				break;
			}
		}

		transport_stream_loop_length = ((buffer[pos] & 0x0F) << 8) | buffer[pos + 1];

		for (pos += 2; pos < section_length - 3; pos += transport_descriptors_length + 6)
		{
			transport_stream_id = (buffer[pos] << 8) | buffer[pos + 1];
			original_network_id = (buffer[pos + 2] << 8) | buffer[pos + 3];
			transport_descriptors_length = ((buffer[pos + 4] & 0x0F) << 8) | buffer[pos + 5];

			if (transponders.count((transport_stream_id << 16) | original_network_id) == 0)
			{
				for (pos2 = pos + 6; pos2 < pos + transport_descriptors_length + 6; pos2 += buffer[pos2 + 1] + 2)
				{
					switch (buffer[pos2])
					{
					case 0x41:
						service_list_descriptor(buffer + pos2);
						break;

					case 0x43:
						if(!satellite_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, DiSEqC)) return -2;
						break;

					case 0x44:
						if(!cable_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id)) return -2;
						break;

					case 0x5F:
						private_data_specifier_descriptor(buffer + pos2);
						break;

					case 0x82: /* unknown, Eutelsat 13.0E */
						break;

					default:
						printf("[nit.cpp] descriptor_tag (b): %02x\n", buffer[pos2]);
						break;
					}
				}
			}
		}
	}
	while (section++ != buffer[7]);

	close(demux_fd);
	return 0;
}

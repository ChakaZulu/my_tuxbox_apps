/*
 * $Id: nit.cpp,v 1.13 2002/04/10 18:36:21 obi Exp $
 */

#include "nit.h"

int nit (uint8_t DiSEqC)
{
	struct dmxSctFilterParams flt;
	int demux_fd;
  	uint8_t buffer[1024];
	uint8_t section = 0;

	/* position in buffer */
	uint16_t pos;
	uint16_t pos2;

	/* network_information_section elements */
	uint16_t section_length;
	uint16_t network_descriptors_length;
	uint16_t transport_descriptors_length;
	uint16_t transport_stream_loop_length;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
  
  	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
    		perror("[nit.cpp] " DEMUX_DEV);
    		return -1;
  	}

  	memset (&flt.filter, 0, sizeof (struct dmxFilter));
  
  	flt.pid = 0x0010;
  	flt.filter.filter[0] = 0x40;
    	flt.filter.mask[0]  = 0xFF;
  	flt.timeout = 10000;		/* 10 Sec. Max. for NIT (ETSI)   (2002-04-04 rasc) */
  	flt.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
 
  	if (ioctl(demux_fd, DMX_SET_FILTER, &flt) < 0)
	{
    		perror("[nit.cpp] DMX_SET_FILTER");
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
#if 0
			case 0x40: /* network_name_descriptor */
				break;
			case 0x41: /* service_list_descriptor */
				break;
			case 0x42: /* stuffing_descriptor */
				break;
			case 0x43: /* satellite_delivery_system_descriptor */
				sat_deliv_system_desc(&buffer[pos], transport_stream_id, original_network_id, DiSEqC);
				break;
			case 0x44: /* cable_delivery_system_descriptor */
				cable_deliv_system_desc(&buffer[pos], transport_stream_id, original_network_id);
				break;
			case 0x4A: /* linkage_descriptor */
				break;
			case 0x5A: /* terrestrial_delivery_system_descriptor */
				break;
			case 0x5B: /* multilingual_network_name_descriptor */
				break;
			case 0x5F: /* private_data_specifier_descriptor */
				break;
			case 0x62: /* frequency_list_descriptor */
				break;
			case 0x6C: /* cell_list_descriptor */
				break;
			case 0x6D: /* cell_frequency_link_descriptor */
				break;
			case 0x6E: /* announcement_support_descriptor */
				break;
#endif
			default:
				printf("[nit.cpp] descriptor_tag: %02x\n", buffer[pos]);
				break;
			}
		}

		transport_stream_loop_length = ((buffer[pos] & 0x0F) << 8) | buffer[pos + 1];

		for (pos += 2; pos < section_length - 3; pos += transport_descriptors_length + 6)
		{
			transport_stream_id = (buffer[pos] << 8) | buffer[pos + 1];
			original_network_id = (buffer[pos + 2] << 8) | buffer[pos + 3];
			transport_descriptors_length = ((buffer[pos + 4] & 0x0F) << 8) | buffer[pos + 5];

			printf("[nit.cpp] TS-ID: %04x\n", transport_stream_id);
			printf("[nit.cpp] Original network-id: %04x\n", original_network_id);

			if (transponders.count(transport_stream_id) == 0)
			{
				for (pos2 = pos + 6; pos2 < pos + transport_descriptors_length + 6; pos2 += buffer[pos2 + 1] + 2)
				{
					switch (buffer[pos2])
					{
					case 0x40: /* network_name_descriptor */
						break;
					case 0x41: /* service_list_descriptor */
						break;
					case 0x42: /* stuffing_descriptor */
						break;
					case 0x43: /* satellite_delivery_system_descriptor */
						sat_deliv_system_desc(&buffer[pos2], transport_stream_id, original_network_id, DiSEqC);
						break;
					case 0x44: /* cable_delivery_system_descriptor */
						cable_deliv_system_desc(&buffer[pos2], transport_stream_id, original_network_id);
						break;
					case 0x4A: /* linkage_descriptor */
						break;
					case 0x5A: /* terrestrial_delivery_system_descriptor */
						break;
					case 0x5B: /* multilingual_network_name_descriptor */
						break;
					case 0x5F: /* private_data_specifier_descriptor */
						break;
					case 0x62: /* frequency_list_descriptor */
						break;
					case 0x6C: /* cell_list_descriptor */
						break;
					case 0x6D: /* cell_frequency_link_descriptor */
						break;
					case 0x6E: /* announcement_support_descriptor */
						break;
					default:
						printf("[nit.cpp] descriptor_tag: %02x\n", buffer[pos2]);
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


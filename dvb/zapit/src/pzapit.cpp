/*
 * $Id: pzapit.cpp,v 1.5 2002/03/28 18:07:34 obi Exp $
 *
 * simple commandline client for zapit
 *
 * Copyright (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#include "clientlib/zapitclient.h"
#include <iostream>

int usage(char *basename)
{
	printf("usage: %s                               - show bouquets\n", basename);
	printf("       %s bouquet-number                - show channels\n", basename);
	printf("       %s bouquet-number channel-number - zap\n", basename);
	return -1;
}

int main (int argc, char** argv)
{
	unsigned int bouquet;
	unsigned int channel;
	unsigned int count;
	bool show_bouquets;
	bool show_channels;
	bool zap;

	/* commandline parameters */
	if (argc == 1)
	{
		zap = false;
		show_bouquets = true;
		show_channels = false;
	}
	else if (argc == 2)
	{
		if ((!strncmp(argv[1], "-h", 2)) || (!strncmp(argv[1], "--help", 6)))
		{
			return usage(argv[0]);
		}
		else
		{
			zap = false;
			show_bouquets = false;
			show_channels = true;
			sscanf(argv[1], "%d", &bouquet);
		}
	}
	else if (argc == 3)
	{
		zap = true;
		show_bouquets = false;
		show_channels = false;
		sscanf(argv[1], "%d", &bouquet);
       		sscanf(argv[2], "%d", &channel);
	}
	else
	{
		return usage(argv[0]);
	}

	std::vector<CZapitClient::responseGetBouquets> bouquets;
	std::vector<CZapitClient::responseGetBouquetChannels> channels;

	CZapitClient *zapit = new CZapitClient();
	zapit->getBouquets(bouquets, true);
	zapit->getBouquetChannels(bouquet, channels);

	if (show_bouquets)
	{
		std::vector<CZapitClient::responseGetBouquets>::iterator b_resp;
		for (b_resp = bouquets.begin(); b_resp < bouquets.end(); b_resp++)
			std::cout << b_resp->bouquet_nr << ": " << b_resp->name << std::endl;
	}

	if (show_channels)
	{
		std::vector<CZapitClient::responseGetBouquetChannels>::iterator ch_resp;
		for (ch_resp = channels.begin(), count = 1; ch_resp < channels.end(); ch_resp++, count++)
			cout << count << ": " << ch_resp->name << endl;
	}

	if (zap)
	{
		std::cout << "zapping to bouquet " << bouquets[bouquet-1].name << ", channel " << channels[channel-1].name << "." << endl;
		zapit->zapTo(bouquet, channel);
	}

	std::cout << "this is the end... my only friend, the end..." << std::endl;
	return 0;
}


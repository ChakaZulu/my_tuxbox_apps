/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/lib/sectionsdclient/sectionsdcontrol.cpp,v 1.9 2009/01/24 17:47:03 seife Exp $
 *
 * Sectionsd command line interface - The Tuxbox Project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * (C) 2008-2009 Stefan Seyfried
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

#include <cstring>
#include <sectionsdclient/sectionsdclient.h>

CSectionsdClient client;

void usage(void) {
	printf("usage:  sectionsdcontrol --pause         stop sectionsd\n");
	printf("        sectionsdcontrol --nopause       restart sectionsd\n");
	printf("        sectionsdcontrol --state         get sectionsd runstate\n");
	printf("        sectionsdcontrol --wepg <epgdir> write epgfiles to dir\n");
	printf("        sectionsdcontrol --repg <epgdir> read epgfiles from dir\n");
	printf("        sectionsdcontrol --freemem       unloads all events\n");
	printf("        sectionsdcontrol --restart       restart sectionsd\n");
	printf("        sectionsdcontrol --ping          ping sectionsd\n");
	printf("        sectionsdcontrol --statistics    print statistics\n");
}

int main(int argc, char** argv)
{
	if (argc<2) {
		usage();
	}

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--h") || !strcmp(argv[i], "-h"))
		{
			usage();
		}
		else if (!strcmp(argv[i], "--pause"))
		{
			printf("setPauseScanning true\n");
			client.setPauseScanning(true);
		}
		else if (!strcmp(argv[i], "--nopause"))
		{
			printf("setPauseScanning false\n");
			client.setPauseScanning(false);
		}
		else if (!strcmp(argv[i], "--state"))
		{
			printf("Scanning is active: %s\n", client.getIsScanningActive()?"true":"false");
		}
		else if (!strcmp(argv[i], "--wepg"))
		{
			if (i+1 < argc) {
				printf("Writing epg files to %s...", argv[i+1]);
				client.writeSI2XML(argv[i+1]);
				printf("done!\n");
			} else {
				usage();
			}
		}
		else if (!strcmp(argv[i], "--repg"))
		{
			if (i+1 < argc) {
				printf("Reading epg files from %s...", argv[i+1]);
				client.readSIfromXML(argv[i+1]);
				printf("done!\n");
			} else {
				usage();
			}
		}
		else if (!strcmp(argv[i], "--freemem"))
		{
			printf("freeMemory\n");
			client.freeMemory();
		}
		else if (!strcmp(argv[i], "--restart"))
		{
			printf("restarting sectionsd\n");
			client.Restart();
		}
		else if (!strcmp(argv[i], "--statistics"))
		{
			std::string response;
			printf("statistics:\n");
			response = client.getStatusinformation();
			printf("%s", response.c_str());
		}
		else if (!strcmp(argv[i], "--ping"))
			printf("sectionsd %s\n", client.ping() ? "running" : "dead");
		else
		{
			usage();
			return 1;
		}
	}

	return 0;
}

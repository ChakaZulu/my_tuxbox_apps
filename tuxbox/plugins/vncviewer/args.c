/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * args.c - argument processing.
 */

#include <sys/utsname.h>
#include <vncviewer.h>

#define FLASHPORT  (5400)    /* Offset to listen for `flash' commands */
#define CLIENTPORT (5500)    /* Offset to listen for reverse connections */
#define SERVERPORT (5900)    /* Offset to server for regular connections */

char *programName;

char hostname[256];
int port;

Bool listenSpecified = False;
int listenPort = 0, flashPort = 0;

char *displayname = NULL;

Bool shareDesktop = False;

CARD32 explicitEncodings[MAX_ENCODINGS];
int nExplicitEncodings = 0;
Bool addCopyRect = True;
Bool addRRE = True;
Bool addCoRRE = True;
Bool addHextile = True;

Bool forceOwnCmap = False;
Bool forceTruecolour = False;
int requestedDepth = 0;
char *hwType = 0;
int kbdDelay = 250; /* milliseconds */
int kbdRate = 30; /* per second */
char *pnmFifo = NULL;
int pnmCacheSize = 8*1024*1024;

char *passwdFile = NULL;
char passwdString[9];

Bool outlineSolid = False;
int updateRequestPeriodms = 0;
int delay = 0;
int serverScaleFactor=1;
int rcCycleDuration=225000;
int rcTest=0;

int debug = 0;


void
usage()
{
    fprintf(stderr,"\n"
	    "usage: %s [<options>] <host>:<display#>\n"
	    "\n"
	    "<options> are:\n"
	    "              [-shared]\n"
	    "              [-raw] [-copyrect] [-rre] [-corre] [-hextile]\n"
	    "              [-nocopyrect] [-norre] [-nocorre] [-nohextile]\n"
	    "              [-bgr233] [-owncmap] [-truecolour] [-depth <d>]\n"
	    "              [-passwd <passwd-file>]\n"
	    "              [-period <ms>] [-delay <ms>]\n"
	    "              [-debug]\n"
	    "              [-hw <zaurus|ipaq>]\n"
	    "              [-kdelay <msec>] [-krate <n-per-second>]\n"
	    "              [-pnmfifo <path>] [-pnmcache <size>{k|M}]\n"
	    "\n"
	    ,programName);
    exit(1);
}


void
processArgs(int argc, char **argv)
{
    int i;

    programName = argv[0];
    
    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-')
	    break;
      
	if (strcmp(argv[i],"-display") == 0) {

	    if (++i >= argc) usage();
	    displayname = argv[i];

	} else if (strcmp(argv[i],"-listen") == 0) {

	    listenSpecified = True;
	    if (++i < argc) {
		listenPort = CLIENTPORT+atoi(argv[i]);
		flashPort = FLASHPORT+atoi(argv[i]);
	    }

	} else if (strcmp(argv[i],"-shared") == 0) {

	    shareDesktop = True;

	} else if (strcmp(argv[i],"-rre") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingRRE;
	    addRRE = False;

	} else if (strcmp(argv[i],"-corre") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingCoRRE;
	    addCoRRE = False;

	} else if (strcmp(argv[i],"-hextile") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingHextile;
	    addHextile = False;

	} else if (strcmp(argv[i],"-copyrect") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingCopyRect;
	    addCopyRect = False;

	} else if (strcmp(argv[i],"-raw") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingRaw;

	} else if (strcmp(argv[i],"-norre") == 0) {

	    addRRE = False;

	} else if (strcmp(argv[i],"-nocorre") == 0) {

	    addCoRRE = False;

	} else if (strcmp(argv[i],"-nohextile") == 0) {

	    addHextile = False;

	} else if (strcmp(argv[i],"-nocopyrect") == 0) {

	    addCopyRect = False;

	} else if (strcmp(argv[i],"-owncmap") == 0) {

	    forceOwnCmap = True;

	} else if (strcmp(argv[i],"-truecolour") == 0) {

	    forceTruecolour = True;

	} else if (strcmp(argv[i],"-depth") == 0) {

	    if (++i >= argc) usage();
	    requestedDepth = atoi(argv[i]);

	} else if (strcmp(argv[i],"-passwd") == 0) {

	    if (++i >= argc) usage();
	    passwdFile = argv[i];

	} else if (strcmp(argv[i],"-outlinesolid") == 0) {

	    outlineSolid = True;

	} else if (strcmp(argv[i],"-period") == 0) {

	    if (++i >= argc) usage();
	    updateRequestPeriodms = atoi(argv[i]);

	} else if (strcmp(argv[i],"-delay") == 0) {

	    if (++i >= argc) usage();
	    delay = atoi(argv[i]);

	} else if (strcmp(argv[i],"-debug") == 0) {

	    debug = True;

	} else if (strcmp(argv[i],"-hw") == 0) {

	    if (++i >= argc) usage();
	    hwType = argv[i];

	} else if (strcmp(argv[i],"-kdelay") == 0) {
	    if (++i >= argc) usage();
	    kbdDelay = atoi(argv[i]);
	} else if (strcmp(argv[i],"-krate") == 0) {
	    if (++i >= argc) usage();
	    kbdRate = atoi(argv[i]);

	} else if (strcmp(argv[i],"-pnmfifo") == 0) {
	    if (++i >= argc) usage();
	    pnmFifo = argv[i];
	} else if (strcmp(argv[i],"-pnmcache") == 0) {
	    int len;
	    int c;
	    if (++i >= argc) usage();
	    pnmCacheSize = atoi(argv[i]);

	    len=strlen(argv[i]);
	    if (!len) usage();
	    c=argv[i][len-1];

	    if (c=='k' || c=='K' || c=='M' || c=='m') pnmCacheSize *= 1024;
	    if (c=='M' || c=='m') pnmCacheSize *= 1024;

	} else {

	    usage();

	}
    }

    if (listenSpecified) {
	if (listenPort == 0) {
		fprintf (stderr, "listen not implemented for this viewer\n");
#if 0
	    char *display;
	    char *colonPos;
	    struct utsname hostinfo;

	    display = XDisplayName(displayname);
	    colonPos = strchr(display, ':');

	    uname(&hostinfo);

	    if (colonPos && ((colonPos == display) ||
			     (strncmp(hostinfo.nodename, display,
				      strlen(hostinfo.nodename)) == 0))) {

		listenPort = CLIENTPORT+atoi(colonPos+1);
		flashPort = FLASHPORT+atoi(colonPos+1);

	    } else {
		fprintf(stderr,"%s: "
			"cannot work out which display number to listen on.\n",
			programName);
		fprintf(stderr,
			"Please specify explicitly with -listen <num>\n");
		exit(1);
	    }
#endif
	}

    } else {	/* -listen not specified */

	if (((argc - i) != 1)
	    || (sscanf(argv[i], "%[^:]:%d", hostname, &port) != 2))
	{
	    usage();
	}

	if (port < 100)
	    port += SERVERPORT;
    }
}

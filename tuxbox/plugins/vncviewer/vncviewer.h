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
 * vncviewer.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <rfbproto.h>


#define USE_SVGALIB

extern int endianTest;

#define Swap16IfLE(s) \
    (*(char *)&endianTest ? ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)) : (s))

#define Swap32IfLE(l) \
    (*(char *)&endianTest ? ((((l) & 0xff000000) >> 24) | \
			     (((l) & 0x00ff0000) >> 8)  | \
			     (((l) & 0x0000ff00) << 8)  | \
			     (((l) & 0x000000ff) << 24))  : (l))

#define MAX_ENCODINGS 10

#define dprintf(fmt, args...) {if(debug) printf( "[vncv] " fmt, ## args);}

/* args.c */

extern char *programName;
extern char hostname[];
extern int port;
extern Bool listenSpecified;
extern int listenPort, flashPort;
extern char *displayname;
extern Bool shareDesktop;
extern CARD32 explicitEncodings[];
extern int nExplicitEncodings;
extern Bool addCopyRect;
extern Bool addRRE;
extern Bool addCoRRE;
extern Bool addHextile;
extern Bool forceOwnCmap;
extern Bool forceTruecolour;
extern int requestedDepth;
extern char *passwdFile;
extern char passwdString[];
extern Bool outlineSolid;
extern int serverScaleFactor;
extern int rcCycleDuration;
extern int rcTest;
extern int updateRequestPeriodms;
extern int delay;
extern char *hwType;
extern int kbdDelay;
extern int kbdRate;
extern char *pnmFifo;
extern int pnmCacheSize;
extern int debug;

extern void processArgs(int argc, char **argv);
extern void usage();


/* rfbproto.c */

extern int rfbsock;
extern Bool canUseCoRRE;
extern Bool canUseHextile;
extern char *desktopName;
extern rfbPixelFormat myFormat;
extern rfbServerInitMsg si;
extern struct timeval updateRequestTime;
extern Bool sendUpdateRequest;

extern Bool ConnectToRFBServer(const char *hostname, int port);
extern Bool DisconnectFromRFBServer();
extern Bool InitialiseRFBConnection(int sock);
extern Bool SetFormatAndEncodings();
extern Bool SetScaleFactor();
extern Bool SendIncrementalFramebufferUpdateRequest();
extern Bool SendFramebufferUpdateRequest(int x, int y, int w, int h,
					 Bool incremental);
extern Bool SendPointerEvent(int x, int y, int buttonMask);
extern Bool SendKeyEvent(CARD32 key, Bool down);
extern Bool SendClientCutText(char *str, int len);
extern Bool HandleRFBServerMessage();


#ifdef USE_X
/* x.c */

extern Display *dpy;
extern Window canvas;
extern Colormap cmap;
extern GC gc;

extern Bool CreateXWindow();
extern void ShutdownX();
extern Bool HandleXEvents();
extern Bool AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg);
extern void CopyDataToScreen(CARD8 *buf, int x, int y, int width, int height);

/* Should write FILL_RECT etc. here so that rfbproto.c is independent
 *  of display lib
 */
#endif /* USE_X */

#ifdef USE_SVGALIB
/* svga.c */

extern Bool InitSvga (void);
extern void ShutdownSvga (void);
extern void HandleKeyboardEvent (void);
extern void HandleMouseEvent (void);
extern void CopyDataToScreen(CARD8 *buf, int x, int y, int width, int height);
extern Bool svncKeyboardInit(void);

#define STORE_COLOUR(xc) gl_setpalettecolor ((xc).pixel, (xc).red >> 10, \
							(xc).green >> 10, (xc).blue >> 10)
#define COPY_AREA(x1 ,y1, w, h, x2, y2) gl_copybox (x1, y1, w, h, x2, y2)
#define FILL_RECT(x, y, w, h, col) gl_fillbox (x, y, w, h, col)
#define BELL 
#endif

/* sockets.c */

extern Bool errorMessageFromReadExact;

extern Bool ReadExact(int sock, char *buf, int n);
extern Bool WriteExact(int sock, char *buf, int n);
extern int ListenAtTcpPort(int port);
extern int ConnectToTcpAddr(unsigned int host, int port);
extern int AcceptTcpConnection(int listenSock);
extern int StringToIPAddr(const char *str, unsigned int *addr);
extern Bool SameMachine(int sock);


/* listen.c */

extern void listenForIncomingConnections();

/*
$Id: cmdline.h,v 1.8 2003/12/07 23:36:13 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




*/



#ifndef __CMDLINE_H
#define __CMDLINE_H 1


/*
 -- defs...
*/

enum PACKET_MODE  {SECT, TS, PES, PIDSCAN};
enum TIME_MODE    {NO_TIME, FULL_TIME, DELTA_TIME};

typedef struct _OPTIONS {
  int         packet_mode;
  int         packet_header_sync;	// Try to do a softsync of packet sync bytes
  int         printhex;
  int         printdecode;
  int         binary_out;
  char        *inpPidFile;		// read from file instead of dmux if not NULL
  int         help;
  char        *devDemux;
  char        *devDvr;
  u_int       pid;
  u_int       filter;
  u_int       mask;
  int         crc;
  long        packet_count;
  int         time_mode;
  int         hide_copyright;  	// suppress message at prog start
} OPTION;


/*
 -- prototypes
*/

int  cmdline_options (int argc, char **argv, OPTION *opt);
void usage (void);


#endif



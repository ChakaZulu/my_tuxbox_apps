/*
$Id: print_header.c,v 1.2 2004/01/01 20:09:26 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)




$Log: print_header.c,v $
Revision 1.2  2004/01/01 20:09:26  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.1  2003/12/28 14:00:27  rasc
bugfix: section read from input file
some changes on packet header output



*/


#include <stdio.h>

#include "dvbsnoop.h"
#include "print_header.h"
#include "misc/cmdline.h"
#include "misc/output.h"
#include "misc/pkt_time.h"




void  print_packet_header (OPTION *opt, char *packetTyp, int pid, int count, int length, int skipped_bytes)

{
   char  str[50];
   char  *s;


   if (pid != DUMMY_PID) {
   	sprintf (str,"%u (0x%04x)",pid,pid);
	s = str;
   } else {
	s = "(Unkown PID)";
   }

   out_nl (1,"\n----------------------------------------------------------");
   out_nl (1,"%s-Packet: %08ld   PID: %s, Length: %d (0x%04x)",
		packetTyp, count, s, length,length);

   if (opt->inpPidFile) {
   	out_nl (1,"from file: %s",opt->inpPidFile);
   } else {
   	out_receive_time (1, opt);
   }

   if (skipped_bytes) {
       out_nl (1,"Syncing %s... (%ld bytes skipped)",
		packetTyp,skipped_bytes);
   }

   out_nl (1,"----------------------------------------------------------");


}




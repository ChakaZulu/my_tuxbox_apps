/*
$Id: descriptor.h,v 1.5 2003/11/26 19:55:32 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- Descriptors Section




$Log: descriptor.h,v $
Revision 1.5  2003/11/26 19:55:32  rasc
no message

Revision 1.4  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


#ifndef __DESCRIPTOR_H
#define __DESCRIPTOR_H 1


int   descriptor (u_char *b);
void  descriptor_any (u_char *b);


#endif



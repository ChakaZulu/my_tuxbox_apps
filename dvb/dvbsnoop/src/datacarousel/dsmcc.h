/*
$Id: dsmcc.h,v 1.2 2004/01/01 20:09:16 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


*/


#ifndef __DSMCC_H
#define __DSMCC_H 1


void  decode_DSMCC_section (u_char *b, int len);


#endif




/*
$Id: dsmcc_carousel_descriptor.h,v 1.6 2004/01/01 20:09:19 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Private TAG Space  DSM-CC
 -- DSM-CC Descriptors  ISO 13818-6  // TR 102 006



$Log: dsmcc_carousel_descriptor.h,v $
Revision 1.6  2004/01/01 20:09:19  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.5  2003/12/27 18:17:17  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.4  2003/11/26 19:55:32  rasc
no message

Revision 1.3  2003/10/26 23:00:39  rasc
fix

Revision 1.2  2003/10/26 21:36:19  rasc
private DSM-CC descriptor Tags started,
INT-Section completed..

Revision 1.1  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


#ifndef __DSM_DESCRIPTOR_H
#define __DSM_DESCRIPTOR_H 1


int   descriptorDSMCC (u_char *b);
void  descriptorDSMCC_any (u_char *b);




#endif



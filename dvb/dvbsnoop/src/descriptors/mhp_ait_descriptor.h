/*
$Id: mhp_ait_descriptor.h,v 1.3 2004/02/10 22:57:54 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Private TAG Space  MHP AIT
 -- MHP AIT Descriptors 



$Log: mhp_ait_descriptor.h,v $
Revision 1.3  2004/02/10 22:57:54  rasc
MHP descriptor, missing DVB descriptor done

Revision 1.2  2004/02/09 21:24:58  rasc
AIT descriptors
minor redesign on output routines

Revision 1.1  2004/02/07 01:28:01  rasc
MHP Application  Information Table
some AIT descriptors





*/


#ifndef __MHP_AIT_DESCRIPTOR_H
#define __MHP_AIT_DESCRIPTOR_H 1


int  descriptorMHP_AIT (u_char *b);

void descriptorMHP_AIT_application (u_char *b);
void descriptorMHP_AIT_application_name (u_char *b);
void descriptorMHP_AIT_transport_protocol (u_char *b);
void descriptorMHP_AIT_dvb_j_application (u_char *b);
void descriptorMHP_AIT_dvb_j_application_location (u_char *b);
void descriptorMHP_AIT_external_application_authorisation (u_char *b);




#endif



/*
$Id: mpeg2_video.h,v 1.2 2005/11/23 23:06:10 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __MPEG2_VIDEO__
#define __MPEG2_VIDEO__


void MPEG2_decodeUserData (u_char *b, int len);
void MPEG2_decodeGroupOfPictures (u_char *b, int len);
void MPEG2_decodeSequenceHeader (u_char *b, int len);


#endif


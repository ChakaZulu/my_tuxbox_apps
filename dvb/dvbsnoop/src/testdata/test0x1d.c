/*
$Id: test0x1d.c,v 1.6 2004/01/01 20:09:42 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de



   -- test data section
   -- DVB test and measurement signalling channel
   -- ETSI TR 101 291



$Log: test0x1d.c,v $
Revision 1.6  2004/01/01 20:09:42  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.5  2003/12/28 00:01:15  rasc
some minor changes/adds...

Revision 1.4  2003/12/27 22:02:45  rasc
dsmcc INT UNT descriptors started

Revision 1.3  2003/11/26 20:31:51  rasc
no message

Revision 1.2  2003/11/26 16:27:48  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.1  2003/10/29 21:00:22  rasc
more PES stuff, DSM descriptors, testdata




*/




#include "dvbsnoop.h"
#include "test0x1d.h"
#include "misc/output.h"
#include "misc/hexprint.h"




void decode_TESTDATA (u_char *b, int len)
{

 typedef struct  _TESTDATA {
    u_int      table_id;
    u_int      priority_level;
    u_int      section_syntax_indicator;		

    unsigned long crc;
 } TESTDATA;



 TESTDATA   t;


 
 t.table_id 			 = getBits (b, 0, 0, 6);
 t.priority_level		 = getBits (b, 0, 6, 2);
 t.section_syntax_indicator	 = getBits (b, 0, 8, 1);


 out_nl (3,"TESTDATA-decoding....");
 out_SB_NL (3,"Table_ID: ",t.table_id);
 out_SB_NL (3,"priority_level: ",t.priority_level);

 out_SB_NL (3,"section_syntax_indicator: ",t.section_syntax_indicator);



 out_nl (3,"... $$$ TODO ....");
 out_nl (3,"... Report!!! if you find a transponder with test data!!! ....");
 // $$$ TODO   ...
 printhexdump_buf (5, b, len);

}





/*
$Id: nit.c,v 1.5 2003/10/24 22:17:21 rasc Exp $

   -- NIT section

   (c) rasc


$Log: nit.c,v $
Revision 1.5  2003/10/24 22:17:21  rasc
code reorg...

Revision 1.4  2003/10/16 19:02:29  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "nit.h"
#include "descriptors/descriptor.h"
#include "strings/dvb_str.h"
#include "misc/output.h"
#include "misc/hexprint.h"



void decode_NIT (u_char *b, int len)
{
 /* EN 300 468  5.2  */

 typedef struct  _NIT {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    u_int      network_id;
    u_int      reserved_3;
    u_int      version_number;
    u_int      current_next_indicator;
    u_int      section_number;
    u_int      last_section_number;
    u_int      reserved_4;
    u_int      network_descriptor_length;

    //  N ... descriptor

    u_int      reserved_5;
    u_int      transport_stream_loop_length;

    //  N1 ... transport stream loop

    u_long     CRC;
 } NIT;


 typedef struct  _NIT_TSL {
    u_int      transport_stream_id;
    u_int      original_network_id;
    u_int      reserved_1;
    u_int      transport_descriptor_length;

    //  N2 ... descriptor

 } NIT_TSL;


 NIT        n;
 NIT_TSL    nt;
 int        l1,l2;


 n.table_id 			 = b[0];
 n.section_syntax_indicator	 = getBits (b, 0,  8, 1);
 n.reserved_1 			 = getBits (b, 0,  9, 1);
 n.reserved_2 			 = getBits (b, 0, 10, 2);
 n.section_length		 = getBits (b, 0, 12, 12);
 n.network_id			 = getBits (b, 0, 24, 16);
 n.reserved_3 			 = getBits (b, 0, 40, 2);
 n.version_number 		 = getBits (b, 0, 42, 5);
 n.current_next_indicator	 = getBits (b, 0, 47, 1);
 n.section_number 		 = getBits (b, 0, 48, 8);
 n.last_section_number 		 = getBits (b, 0, 56, 8);
 n.reserved_4 			 = getBits (b, 0, 64, 4);
 n.network_descriptor_length	 = getBits (b, 0, 68, 12);



 out_nl (3,"NIT-decoding....");
 out_S2B_NL (3,"Table_ID: ",n.table_id, dvbstrTableID (n.table_id));
 if (n.table_id != 0x40 && n.table_id != 0x41) {
   out_nl (3,"wrong Table ID");
   return;
 }


 out_SB_NL (3,"section_syntax_indicator: ",n.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",n.reserved_1);
 out_SB_NL (6,"reserved_2: ",n.reserved_2);
 out_SW_NL (5,"Section_length: ",n.section_length);

 out_S2W_NL (3,"Network_ID: ",n.network_id,
	dvbstrNetworkIdent_ID(n.network_id)); 
 out_SB_NL (6,"reserved_3: ",n.reserved_3);
 out_SB_NL (3,"Version_number: ",n.version_number);
 out_SB_NL (3,"Current_next_indicator: ",n.current_next_indicator);
 out_SB_NL (3,"Section_number: ",n.section_number);
 out_SB_NL (3,"Last_Section_number: ",n.last_section_number);
 out_SB_NL (6,"reserved_4: ",n.reserved_4);

 out_SW_NL (5,"Network_descriptor_length: ",n.network_descriptor_length);


 // get network descriptors

 l1 = n.network_descriptor_length;
 b += 10;

 while ( l1 > 0 ) {
   int x;

   x = descriptor (b); 
   l1 -= x;
   b += x;
 }


 // get transport stream loop / descriptors...

 n.reserved_5 			 = getBits (b, 0, 0, 4);
 n.transport_stream_loop_length	 = getBits (b, 0, 4, 12);

 out_NL (3);
 out_SB_NL (6,"reserved_5: ",n.reserved_5);
 out_SW_NL (5,"Transport_stream_loop_length: ",n.transport_stream_loop_length);
 

 l1 = n.transport_stream_loop_length;
 b += 2;

 indent (+1);
 while ( l1 > 0 ) {
    nt.transport_stream_id	= getBits (b, 0,  0, 16);
    nt.original_network_id	= getBits (b, 0, 16, 16);
    nt.reserved_1		= getBits (b, 0, 32, 4);
    nt.transport_descriptor_length	= getBits (b, 0, 36, 12);


    out_NL (3);
    out_SW_NL  (3,"Transport_stream_ID: ",nt.transport_stream_id);
    out_S2W_NL (3,"Original_network_ID: ",nt.original_network_id,
        dvbstrOriginalNetwork_ID(nt.original_network_id));
    out_SB_NL  (6,"reserved_1: ",nt.reserved_1);
    out_SW_NL  (5,"Transport_descriptor_length: ",
        nt.transport_descriptor_length);


    // descriptor(s) 

    b  += 6;
    l1 -= 6;
    l2  = nt.transport_descriptor_length;

    while (l2 > 0) {
      int x;

      x = descriptor (b);
      b  += x;
      l2 -= x;
      l1 -= x;
    }

 }
 indent(-1);


 n.CRC 			 = getBits (b, 0, 0, 32);
 out_SL_NL (5,"CRC: ",n.CRC);

}










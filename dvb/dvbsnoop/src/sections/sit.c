/*
$Id: sit.c,v 1.1 2001/09/30 13:05:20 rasc Exp $

   -- SIT section
   -- Selection Information Table
   -- ETSI EN 300 469  7.1.2

   (c) rasc


$Log: sit.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "sit.h"




void decode_SIT (u_char *b, int len)
{

 typedef struct  _SIT {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    u_int      reserved_3;
    u_int      reserved_4;
    u_int      version_number;
    u_int      current_next_indicator;
    u_int      section_number;
    u_int      last_section_number;
    u_int      reserved_5;
    u_int      transmission_info_loop_length;

    // N  descriptor
    // N1 SIT_LIST2

    unsigned long crc;
 } SIT;


 typedef struct _SIT_LIST2 {
    u_int      service_id;
    u_int      reserved_1; 
    u_int      running_status;
    u_int      service_loop_length;

    // N2 descriptor

 } SIT_LIST2;



 SIT        s;
 SIT_LIST2  s2;
 int        len1,len2;


 
 s.table_id 			 = b[0];
 s.section_syntax_indicator	 = getBits (b, 0,  8,  1);
 s.reserved_1 			 = getBits (b, 0,  9,  1);
 s.reserved_2 			 = getBits (b, 0, 10,  2);
 s.section_length		 = getBits (b, 0, 12, 12);
 s.reserved_3 			 = getBits (b, 0, 24, 16);
 s.reserved_4 			 = getBits (b, 0, 40,  2);
 s.version_number 		 = getBits (b, 0, 42, 5);
 s.current_next_indicator	 = getBits (b, 0, 47, 1);
 s.section_number 		 = getBits (b, 0, 48, 8);
 s.last_section_number 		 = getBits (b, 0, 56, 8);
 s.reserved_5	 		 = getBits (b, 0, 64, 4);
 s.transmission_info_loop_length = getBits (b, 0, 68, 12);


 
 out_nl (3,"SIT-decoding....");
 out_S2B_NL (3,"Table_ID: ",s.table_id, dvbstrTableID (s.table_id));
 if (s.table_id != 0x7F) {
   out_nl (3,"wrong Table ID");
   return;
 }


 out_SB_NL (3,"section_syntax_indicator: ",s.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",s.reserved_1);
 out_SB_NL (6,"reserved_2: ",s.reserved_2);
 out_SW_NL (5,"Section_length: ",s.section_length);

 out_SW_NL (6,"reserved_3: ",s.reserved_3);
 out_SB_NL (6,"reserved_4: ",s.reserved_4);

 out_SB_NL (3,"Version_number: ",s.version_number);
 out_SB_NL (3,"Current_next_indicator: ",s.current_next_indicator);
 out_SB_NL (3,"Section_number: ",s.section_number);
 out_SB_NL (3,"Last_Section_number: ",s.last_section_number);

 out_SB_NL (6,"reserved_5: ",s.reserved_5);
 out_SW_NL (5,"Transmission_info_loop_length: ",s.transmission_info_loop_length);


 // - header data after length value
 len1 = s.section_length - 7;
 b   += 10;

 len2 = s.transmission_info_loop_length;
 while (len2 > 0) {
   int x;

   x = descriptor (b);
   len2 -= x;
   b += x;
   len1 -= x;
 }

 
 indent (+1);
 while (len1 > 4) {

   s2.service_id		 = getBits (b, 0,  0, 16);
   s2.reserved_1		 = getBits (b, 0, 16,  1);
   s2.running_status		 = getBits (b, 0, 17,  3);
   s2.service_loop_length	 = getBits (b, 0, 20, 12);

   b    += 4;
   len1 -= 4;


   out_NL (3);
   out_S2W_NL (3,"Service_ID: ",s2.service_id, 
          " --> refers to PMS program_number");
   out_SB_NL  (6,"reserved_1: ",s2.reserved_1);
   out_S2B_NL (3,"Running_status: ",s2.running_status,
          dvbstrRunningStatus_FLAG (s2.running_status));

   out_SW_NL  (5,"Service_loop_length: ",s2.service_loop_length);


   len2 = s2.service_loop_length;

   while (len2 > 0) {
      int x;

      x = descriptor (b);
      len2 -= x;
      b += x;
      len1 -= x;
   }

 } // while len1
 indent (-1);


 s.crc		 		 = getBits (b, 0, 0, 32);
 out_SL_NL (5,"CRC: ",s.crc);

}




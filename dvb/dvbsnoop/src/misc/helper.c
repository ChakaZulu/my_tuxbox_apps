/*
$Id: helper.c,v 1.8 2003/02/26 16:45:16 obi Exp $

 -- dvbsnoop
 -- a dvb sniffer tool
 -- mainly for me to learn the dvb streams

 -- (c) rasc


$Log: helper.c,v $
Revision 1.8  2003/02/26 16:45:16  obi
- make dvbsnoop work on little endian machines again
- fixed mask in getBits for bitlen >= 32

Revision 1.7  2003/02/09 23:11:07  rasc
no message

Revision 1.6  2003/02/09 23:02:47  rasc
-- endian check (bug fix)

Revision 1.5  2003/02/09 23:01:10  rasc
-- endian check (bug fix)

Revision 1.4  2003/02/09 22:59:33  rasc
-- endian check (bug fix)

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "helper.h"
#include "output.h"
#include "dvb_str.h"



/* 
  -- get bits out of buffer
  -- (getting more than 24 bits is not save)
  -- return: value
*/

unsigned long getBits (u_char *buf, int byte_offset, int startbit, int bitlen)

{
 u_char *b;
 unsigned long  v;
 unsigned long mask;
 unsigned long tmp_long;

 b = &buf[byte_offset + (startbit / 8)];
 startbit %= 8;

 tmp_long = (unsigned long)( ((*b)<<24) + (*(b+1)<<16) +
		 (*(b+2)<<8) + *(b+3) );

 startbit = 32 - startbit - bitlen;

 tmp_long = tmp_long >> startbit;

 // ja, das ULL muss so sein (fuer bitlen == 32 z.b.)...
 mask = (1ULL << bitlen) - 1;

 v = tmp_long & mask;

 return v;
}




/*
  -- print_name_Short 
  -- print_name_Long
  -- ETSI EN 300 468  Annex A

  -- evaluate string and look on DVB control codes
  -- print the string

*/

void print_name (int v, u_char *b, u_int len)

{
 //int i;

 if (len <= 0) {
    out (v,"\"\"");
 } else {
    out (v,"\"");
    print_name2 (v, b,len);
    out (v,"\"");
    out (v,"  -- Charset: %s", dvbstrTextCharset_TYPE (*b));
 }

}



void print_name2 (int v, u_char *b, u_int len)

{
  int    in_emphasis = 0;
  int    i;
  u_char c;
  u_char em_ON  = 0x86;
  u_char em_OFF = 0x87;

 
  for (i=0; i<len; i++) {
    c = b[i];

    if (i == 0 && c < 0x20) continue;   // opt. charset descript.

    if (c == em_ON) {
       in_emphasis = 1;
       out (v,"<EM>");
       continue;
    }
    if (c == em_OFF) {
       in_emphasis = 0;
       out (v,"</EM>");
       continue;
    }

       if (c == 0x8A)     out (v, "<BR>");
       else if (c < 0x20) out (v, ".");
       else               out (v, "%c", c);

  } // for
  

}





/*
 -- print time   40 bit
 --   16 Bit  MJD,  24 Bit UTC
*/ 

void print_time40 (int v, u_long mjd, u_long utc)

{

 out (v, "0x%lx%06lx (=",mjd, utc);

 if (mjd > 0) {
   long   y,m,d ,k;

   // algo: ETSI EN 300 468 - ANNEX C

   y =  (long) ((mjd  - 15078.2) / 365.25);
   m =  (long) ((mjd - 14956.1 - (long)(y * 365.25) ) / 30.6001);
   d =  (long) (mjd - 14956 - (long)(y * 365.25) - (long)(m * 30.6001));
   k =  (m == 14 || m == 15) ? 1 : 0;
   y = y + k + 1900;
   m = m - 1 - k*12;

   out (v, "%02d-%02d-%02d",y,m,d);
 }

 out (v, " %02lx:%02lx:%02lx [UTC])",
	 (utc>>16) &0xFF, (utc>>8) &0xFF, (utc) &0xFF);

}



/*
   -- str2i
   -- string to integer
   --   x, 0x ist Hex
   --   ansonsten Dezimal
   return:  long int

*/

long  str2i  (char *s)

{
 long v;
  
 v = strtol (s, NULL, 0);
 return v;

}





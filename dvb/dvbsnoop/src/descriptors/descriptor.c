/*
$Id: descriptor.c,v 1.9 2003/07/08 19:59:50 rasc Exp $

  dvbsnoop
  (c) Rainer Scherg 2001-2003


  Descriptor Section
  - MPEG
  - DVB
  - DSM-CC  (todo)



$Log: descriptor.c,v $
Revision 1.9  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?

Revision 1.8  2003/06/24 23:51:03  rasc
bugfixes and enhancements

Revision 1.7  2003/05/03 02:51:08  obi
skip descriptors with length == 0

Revision 1.6  2003/03/17 16:15:11  obi
fixed infinite loop
thanks to Johannes Stezenbach

Revision 1.5  2002/09/29 13:01:35  wjoost
kleiner Fehler



*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "mpeg_descriptor.h"
#include "dvb_descriptor.h"
#include "hexprint.h"







/*
  determine descriptor type and print it...
  return byte length
*/

int  descriptor  (u_char *b)

{
 int len;
 int id;



  // nothing to print here?
  if (getVerboseLevel() < 4) return len;

  id  =  (int)b[0];  
  len = ((int)b[1]) + 2;

  indent (+1);



  // Context of descriptors
  // $$$ To be improved!!!
 
  if (id < 0x40) {
	  descriptorMPEG (b);
  } else {
	  descriptorDVB (b);
  }


  indent (-1);

  return len;   // (descriptor total length)
}








/*
  Any  descriptor  (Basic Descriptor output)
  ETSI 300 468 
*/

void descriptor_any (u_char *b)

{

 typedef struct  _descANY {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // private data bytes

 } descANY;


 descANY  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 out_nl (4,"Descriptor-Data:");
 printhexdump_buf (4,b+2,d.descriptor_length);

}






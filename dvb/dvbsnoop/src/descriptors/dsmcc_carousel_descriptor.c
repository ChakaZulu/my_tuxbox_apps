/*
$Id: dsmcc_carousel_descriptor.c,v 1.12 2004/01/01 20:31:22 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Private TAG Space  DSM-CC
 -- DSM-CC Descriptors  ISO 13818-6  // TR 102 006




$Log: dsmcc_carousel_descriptor.c,v $
Revision 1.12  2004/01/01 20:31:22  rasc
PES program stream map, minor descriptor cleanup

Revision 1.11  2004/01/01 20:09:19  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.10  2003/12/27 18:17:17  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.9  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.8  2003/10/29 20:54:56  rasc
more PES stuff, DSM descriptors, testdata



*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "dsm_descriptor.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"





/*
  determine descriptor type and print it...
  !!! DSMCC descriptors are in a private tag space !!!

  return byte length
*/

int  descriptorDSMCC  (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"DSM_CC-DescriptorTag: ",id,
		  dsmccStrDSMCC_DescriptorTAG (id));
  out_SB_NL  (5,"Descriptor_length: ",b[1]);

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);



  switch (b[0]) {


     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (DSM-CC context), Report!");
	}
	descriptor_any (b);
	break;
  } 


  return len;   // (descriptor total length)
}














/* $$$ TODO   EN 301 192  / TR 102 006
 *
 * private DSM-CC descriptors
 *
 * */




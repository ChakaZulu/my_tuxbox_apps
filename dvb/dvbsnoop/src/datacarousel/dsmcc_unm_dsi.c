/*
$Id: dsmcc_unm_dsi.c,v 1.3 2004/02/17 23:54:12 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc download Server Initiate
 -- to be called from U-N-Message



$Log: dsmcc_unm_dsi.c,v $
Revision 1.3  2004/02/17 23:54:12  rasc
Bug (not fixed yet): DSM-CC  DII Carousel Descriptor Loop is strange

Revision 1.2  2004/02/15 20:46:09  rasc
DSM-CC  data/object carousell continued   (DSI, DII, DDB, DCancel)

Revision 1.1  2004/02/15 18:58:28  rasc
DSM-CC  data/object carousell continued   (DSI, DII, DDB, DCancel)





*/




#include "dvbsnoop.h"
#include "dsmcc_unm_dsi.h"
#include "dsmcc_misc.h"
#include "misc/output.h"
#include "misc/hexprint.h"





/*
 * ISO/IEC 13818-6
 * dsmcc_Download Server Initiate
 */


int dsmcc_DownloadServerInitiate (int v, u_char *b, u_int len)
{
   int   	len_org = len;
   int		len2;
   int		x;




	// already read  dsmcc_MessageHeader (v, b, len, &dmh);

	dsmcc_carousel_NSAP_address_B20 (v, "Server-ID", b);

	x = dsmcc_CompatibilityDescriptor (b+20);
	b += 20+x;
	len -= 20+x;

	len2 = outBit_Sx_NL (v,"privateDataLength: ",	b,  0, 16);
	b += 2;
	// $$$ TODO  Super-group-Info  && carousel descr.
	// GroupStruct + dsmcc_CarouselDescriptor_Loop ("GroupInfo", b, len);

	print_databytes (v, "private data: ", b, len2);

	return len_org;
}







// EN 301 192:
// 8.1.2 DownloadServerInitiate message
// 
// The DownloadServerInitiate message is used to build a SuperGroup.
// The semantics for DVB data carousels are as follows:
//
// serverId: this field shall be set to 20 bytes with the value of 0xFF.
//
// compatibilityDescriptor(): this structure shall only contain the
// compatibilityDescriptorLength field of the compatibilityDescriptor()
// as defined in DSM-CC (see ISO/IEC 13818-6 [5]). It shall be set to
// the value of 0x0000. The privateDataByte fields shall contain the
// GroupInfoIndication structure as defined in table 37.
//
// privateDataLength: this field defines the length in bytes of the
// following GroupInfoIndication structure.
//
// privateDataByte: these fields shall convey the GroupInfoIndication
// structure as defined in table 37.
//
// --> SuperGroupInfo


/* $$$ TODO
GroupInfoIndication() {
NumberOfGroups 2
for(i=0;i< numberOfGroups;i++) {
GroupId 4
GroupSize 4
GroupCompatibility()
GroupInfoLength 2
for(i=0;i<N;I++) {
groupInfoByte
}
1
}
PrivateDataLength 2
for(i=0;i< privateDataLength;i++) {
privateDataByte
}
1
}

// groupInfobyte may contain BIOP, descriptor_carousel_loopm etc.


*/

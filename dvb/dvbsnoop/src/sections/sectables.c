/*
$Id: sectables.c,v 1.12 2003/11/09 22:54:16 rasc Exp $

 --  For more information please see: ISO 13818 (-1) and ETSI 300 468
 -- Verbose Level >= 2
  (c) rasc


$Log: sectables.c,v $
Revision 1.12  2003/11/09 22:54:16  rasc
no message

Revision 1.11  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.10  2003/10/26 23:00:43  rasc
fix

Revision 1.9  2003/10/24 22:17:21  rasc
code reorg...

Revision 1.8  2003/10/21 20:12:51  rasc
no message

Revision 1.7  2003/10/21 19:54:43  rasc
no message

Revision 1.6  2003/10/19 22:22:57  rasc
- some datacarousell stuff started

Revision 1.5  2003/10/19 13:54:25  rasc
-more table decoding

Revision 1.4  2003/10/17 18:16:54  rasc
- started more work on newer ISO 13818  descriptors
- some reorg work started

Revision 1.3  2003/07/06 05:49:25  obi
CAMT fix and indentation

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "sectables.h"
#include "pat.h"
#include "cat.h"
#include "pmt.h"
#include "tsdt.h"
#include "nit.h"
#include "sdt.h"
#include "st.h"
#include "bat.h"
#include "tdt.h"
#include "tot.h"
#include "rst.h"
#include "dit.h"
#include "sit.h"
#include "eit.h"
#include "emm_ecm.h"
#include "userdef.h"
#include "datacarousel/dsmcc_addr.h"
#include "datacarousel/ints.h"
#include "testdata/test0x1d.h"




void  guess_table (u_char *buf, int len, u_int pid);





/* 
 -- decode Sections buffer by identifying the table IDs
 -- siehe EN 300 468 S.14 
*/

void decodeSections_buf (u_char *buf, int len, u_int pid)

{
  int table_id;


  // nothing to output ?  
  if (getVerboseLevel() < 2) return;


  out_nl (2,"PID:  %u (0x%04x)",pid,pid);
  table_id = buf[0];

  switch (pid) {

	case  0x000:		/* PAT  Program Association Table */
		decode_PAT  (buf, len);
		break; 

	case  0x001:		/* CAT  Conditional Access Table */
		decode_CAT  (buf, len);
		break; 

	case  0x002:		/* TSDT Transport Stream Description Sect */
		decode_TSDT  (buf, len);	/* Table ID == 0x03 */
		break; 

	case  0x010:		/* NIT, ST  */
		if (table_id == 0x72)   decode_ST   (buf,len);
		else                    decode_NIT  (buf, len);
		break; 

	case  0x011:		/* SDT, BAT, ST  */
		if      (table_id == 0x72) decode_ST  (buf,len);
		else if (table_id == 0x42) decode_SDT (buf,len); 
		else if (table_id == 0x46) decode_SDT (buf,len); 
		else                       decode_BAT (buf,len);
		break; 

	case  0x012:		/* EIT, ST  */
		if (table_id == 0x72)   decode_ST  (buf,len);
		else                    decode_EIT (buf,len);
		break; 

	case  0x013:		/* RST, ST  */
		if (table_id == 0x72) decode_ST   (buf,len);
		else                  decode_RST  (buf,len); 
		break; 

	case  0x014:		/* TDT, TOT, ST  */
		if      (table_id == 0x72) decode_ST   (buf,len);
		else if (table_id == 0x70) decode_TDT  (buf,len); 
		else                       decode_TOT  (buf,len);
		break; 

	case  0x015:		/* Net Sync  */
		// $$$$
		break; 

	case  0x01C:		/* Inband Signalling  */
		// $$$$
		break; 

	case  0x01D:		/* Measurement */
		decode_TESTDATA  (buf, len);
		break; 

	case  0x01E:		/* DIT */
		decode_DIT  (buf, len);
		break; 

	case  0x01F:		/* SIT */
		decode_SIT  (buf, len);
		break; 


	case  0x1FFC:		/* ATSC SI */
		out_nl (2,"ATSC SI Packet");
		break;

	case  0x1FFD:		/* ATSC Master Program Guide */
		out_nl (2,"ATSC Master Program Guide  Packet");
		break;

	case  0x1FFF:		/* Null packet */
		out_nl (2,"Null Packet");
		break;


        default:			// multiple PIDs possible
                guess_table (buf, len, pid);
		break;


  }

  fflush (stdout);


}







/*
  -- content of packet can not be determined via PID,
  -- so gess from first byte of packet header
*/




typedef struct _TABLE_IF_FUNC {
    u_int    from;          /* e.g. from id 1  */
    u_int    to;            /*      to   id 3  */
    void     (*func)();     /*      function for table decoding */
} TABLE_ID_FUNC;


/*
 * -- Decode mapping table  (TableID --> decode function)
 * -- Crosscheck this with dvb_str, when changing!!!
 */

static TABLE_ID_FUNC table_id_func[] = {

     {  0x00, 0x00,  decode_PAT	},
     {  0x01, 0x01,  decode_CAT	},
     {  0x02, 0x02,  decode_PMT	},
     {  0x03, 0x03,  decode_TSDT },
     /* res. */
     {  0x3e, 0x3e,  decode_DSMCC_ADDR },
     {  0x40, 0x41,  decode_NIT	},
     {  0x42, 0x42,  decode_SDT	},
     /* res. */
     {  0x46, 0x46,  decode_SDT	},
     /* res. */
     {  0x4A, 0x4A,  decode_BAT	},
     {  0x4C, 0x4C,  decode_INT_DSMCC },
     {  0x4E, 0x6E,  decode_EIT	},  /*  4 different types */
     {  0x70, 0x70,  decode_TDT },
     {  0x71, 0x71,  decode_RST },
     {  0x72, 0x72,  decode_ST  },
     {  0x73, 0x73,  decode_TOT },
     /* res. */
     {  0x7E, 0x7E,  decode_DIT },
     {  0x7F, 0x7F,  decode_SIT },
     {  0x80, 0x8F,  decode_EMM_ECM },   /* $$$ Conditional Access Message Section */

     {  0x90, 0xFE,  decode_PRIVATE },	 /* opps!? DSM-CC or other stuff?! */
     {  0,0, NULL }
  };






void  guess_table (u_char *buf, int len, u_int pid)

{

  TABLE_ID_FUNC *t = table_id_func;
  u_int		table_id;



  /* -- scan id table for decode function */

  table_id =  (u_int) buf[0];
  while (t->func) {
    if (t->from <= table_id && t->to >= table_id)
       break;
    t++;
  }



  out_nl (2,"Guess table from table id...");

  if (t->func == NULL) {
   	out_SB_NL (2,"Unknown, reserved or not implemented - TableID: ",table_id);
	out_nl    (2,"--> %s",dvbstrTableID (table_id));
	return;
  }


  (*(t->func))(buf,len);		/* exec decode function */
  return;
}







/*
$Id: dmx_ts.c,v 1.15 2003/12/30 14:05:37 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- Transport Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468




$Log: dmx_ts.c,v $
Revision 1.15  2003/12/30 14:05:37  rasc
just some annotations, so I do not forget these over Sylvester party...
(some alkohol may reformat parts of /devbrain/0 ... )
cheers!

Revision 1.14  2003/12/28 22:53:40  rasc
some minor changes/cleanup

Revision 1.13  2003/12/28 14:00:25  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.12  2003/12/15 20:09:48  rasc
no message

Revision 1.11  2003/12/10 22:54:11  obi
more tiny fixes

Revision 1.10  2003/11/24 23:52:16  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.9  2003/10/24 22:45:06  rasc
code reorg...

Revision 1.8  2003/10/24 22:17:18  rasc
code reorg...

Revision 1.7  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.6  2003/05/28 01:35:01  obi
fixed read() return code handling

Revision 1.5  2003/01/07 00:43:58  obi
set buffer size to 256kb

Revision 1.4  2002/11/01 20:38:40  Jolt
Changes for the new API

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "misc/cmdline.h"
#include "misc/output.h"
#include "misc/hexprint.h"
#include "misc/print_header.h"

#include "ts/tslayer.h"
#include "dvb_api.h"
#include "dmx_ts.h"



#define TS_BUF_SIZE  (256 * 1024)
#define TS_PACKET_LEN (188)              /* TS RDSIZE is fixed !! */
#define TS_SYNC_BYTE  (0x47)             /* SyncByte fuer TS  ISO 138181-1 */
#define READ_BUF_SIZE (3*TS_PACKET_LEN)	 /* min. 2x TS_PACKET_LEN!!! */



static long ts_SyncRead (int fd, u_char *buf, long buflen, long *skipped_bytes);





int  doReadTS (OPTION *opt)

{
  int     fd_dmx = 0, fd_dvr = 0;
  u_char  buf[READ_BUF_SIZE]; 	/* data buffer */
  long    count;
  int     i;
  char    *f;
  int     fileMode;


  

  if (opt->inpPidFile) {
  	f        = opt->inpPidFile;
        fileMode  = 1;
  } else {
  	f        = opt->devDvr;
        fileMode  = 0;
  } 


  if((fd_dvr = open(f,O_RDONLY)) < 0){
      perror(f);
      return -1;
  }


  


  /*
   -- init demux
  */

  if (!fileMode) {
    struct dmx_pes_filter_params flt;

    if((fd_dmx = open(opt->devDemux,O_RDWR)) < 0){
        perror(opt->devDemux);
	close (fd_dvr);
        return -1;
    }


    ioctl (fd_dmx,DMX_SET_BUFFER_SIZE, TS_BUF_SIZE);
    memset (&flt, 0, sizeof (struct dmx_pes_filter_params));

    flt.pid = opt->pid;
    flt.input  = DMX_IN_FRONTEND;
    flt.output = DMX_OUT_TS_TAP;
    flt.pes_type = DMX_PES_OTHER;
    flt.flags = DMX_IMMEDIATE_START;

    if ((i=ioctl(fd_dmx,DMX_SET_PES_FILTER,&flt)) < 0) {
      perror ("DMX_SET_PES_FILTER failed: ");
      return -1;
    }

}




/*
  -- read packets for pid
*/

  count = 0;
  while (1) {
    long   n;
    long   skipped_bytes = 0;


    if (opt->packet_header_sync) {
    	n = ts_SyncRead(fd_dvr,buf,sizeof(buf), &skipped_bytes);
    } else {
    	n = read(fd_dvr,buf,sizeof(buf));
    }


    // -- error or eof?
    if (n == -1) perror("read");
    if (n < 0)  continue;
    if (n == 0) {
	if (!fileMode) continue;	// DVRmode = no eof!
	else break;			// filemode eof 
    }


    count ++;

    if (opt->binary_out) {

       // direct write to FD 1 ( == stdout)
       write (1, buf,n);

    } else {

       indent (0);
       print_packet_header (opt, "TS", opt->pid, count, n, skipped_bytes);


       if (opt->printhex) {
           printhex_buf (0,buf, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodeTS_buf (buf,n ,opt->pid);
          out_nl (3,"==========================================================");
          out_nl (3,"");
       }
    } // bin_out



    // Clean Buffer
//    if (n > 0 && n < sizeof(buf)) memset (buf,0,n+1); 


    // count packets ?
    if (opt->packet_count > 0) {
       if (--opt->packet_count == 0) break;
    }


  } // while



  /*
    -- Stop Demux
  */

  if (!fileMode) {
     ioctl (fd_dmx, DMX_STOP, 0);

     close(fd_dmx);
  }

  close(fd_dvr);
  return 0;
}



/*
 * -- sync read
 * -- Seek TS sync-byte and read buffer
 * -- return: equivalent to read();
 */

static long  ts_SyncRead (int fd, u_char *buf, long buflen, long *skipped_bytes)

{
    int    n = 0;


    // -- simple TS sync...
    // -- $$$ to be improved:
    // -- $$$  (best would be: check if buf[188] is also a sync byte)
 
    // -- $$$ TODO speed optimize! (read TS_PACKET_LEN, seek Sync, read missing parts)

    *skipped_bytes = 0;
    while (1) {
    	n = read(fd,buf,1);
	if (n <= 0) return n;			// error or strange, abort

	if (buf[0] == TS_SYNC_BYTE) break;
	(*skipped_bytes)++;			// sync skip counter
    }

    // -- Sync found!
    // -- read buffersize-1

    n = read(fd,buf+1,TS_PACKET_LEN-1);		// read TS
    if (n >=0) n++;				// we already read one byte...

    return n;
}






/*
$Id: dmx_sect.c,v 1.25 2005/06/02 09:26:11 ghostrider Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 --  Sections Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468




$Log: dmx_sect.c,v $
Revision 1.25  2005/06/02 09:26:11  ghostrider
filtering sections up to 4 bytes is now possible,
do 'dvbsnoop -f 0x4e2f1c -m 0xFFFFFF 0x12' for filtering only now/next table for VOX on the correct transponder

Revision 1.24  2004/12/07 21:01:41  rasc
Large file support (> 2 GB) for -if cmd option. (tnx to K.Zheng,  Philips.com for reporting)

Revision 1.23  2004/10/12 20:37:47  rasc
 - Changed: TS pid filtering from file, behavior changed
 - New: new cmdline option -maxdmx <n>  (replaces -f using pidscan)
 - misc. changes

Revision 1.22  2004/09/01 20:20:34  rasc
new cmdline option: -buffersize KB  (set demux buffersize in KBytes)

Revision 1.21  2004/03/31 21:14:23  rasc
New: Spider section pids  (snoop referenced section pids),
some minor changes

Revision 1.20  2004/02/15 22:22:28  rasc
cmd option: -hexdumpbuffer -nohexdumpbuffer

Revision 1.19  2004/02/12 23:00:24  rasc
no message

Revision 1.18  2004/01/25 22:36:52  rasc
minor changes & enhancments

Revision 1.17  2004/01/22 22:26:35  rasc
pes_pack_header
section read timeout

Revision 1.16  2004/01/02 22:25:37  rasc
DSM-CC  MODULEs descriptors complete

Revision 1.15  2004/01/02 00:00:37  rasc
error output for buffer overflow

Revision 1.14  2004/01/01 20:09:23  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.13  2003/12/28 22:53:40  rasc
some minor changes/cleanup

Revision 1.12  2003/12/28 14:00:25  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.11  2003/12/15 20:09:48  rasc
no message

Revision 1.10  2003/12/10 22:46:34  obi
tiny section filter fixes

Revision 1.9  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.8  2003/10/24 22:45:06  rasc
code reorg...

Revision 1.7  2003/10/24 22:17:18  rasc
code reorg...

Revision 1.6  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.5  2003/05/28 01:35:01  obi
fixed read() return code handling

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
#include "misc/pid_mem.h"

#include "sections/sectables.h"
#include "dvb_api.h"
#include "file_io.h"
#include "dmx_error.h"
#include "dmx_sect.h"



#define SECT_BUF_SIZE (256*1024)	/* default DMX buffer size */
#define READ_BUF_SIZE (32*1024)		/* section work buffer (section is @5K) */


static long  sect_read (int fd, u_char *buf, long buflen);
static int   doReadSECT_2 (OPTION *opt);





/* 
 -- read sections
 -- single pid sections or spider pid sections (opt.)
 */

int  doReadSECT (OPTION *opt)
{
   int    status;


   // -- first pid
   status = doReadSECT_2 (opt);
   mark_PidMem_as_used (opt->pid);

   if (status) return status;


   // -- spider option requested?
   if (opt->spider_pid) {
   	u_int  pid;

	while (1) {
		pid = get_UnusedPidFromMem ();
		if (pid == INVALID_PID) break;

		// new spidered pid
		opt->pid = pid;
		status = doReadSECT_2 (opt);
		mark_PidMem_as_used (pid);

		if (status) return status;
	}

   }

   return status;
}





/* 
 -- read single section
 */

static int  doReadSECT_2 (OPTION *opt)

{
  int     fd;
  u_char  buf[READ_BUF_SIZE]; 		/* data buffer */
  long    count;
  char    *f;
  int     openMode;
  int     dmxMode;
  long    dmx_buffer_size = SECT_BUF_SIZE;
  int     idx=0;



  if (opt->inpPidFile) {
  	f        = opt->inpPidFile;
  	openMode = O_RDONLY | O_LARGEFILE;
        dmxMode  = 0;
  } else {
  	f        = opt->devDemux;
  	openMode = O_RDWR;
        dmxMode  = 1;
  }


  if((fd = open(f,openMode)) < 0){
      IO_error(f);
      return -1;
  }


  /*
   -- init demux
  */

  if (dmxMode) {
    struct dmx_sct_filter_params flt;


    // -- alloc dmx buffer for SECTION
    if (opt->rd_buffer_size > 0) {
	    dmx_buffer_size = opt->rd_buffer_size;
    }

    if (ioctl(fd,DMX_SET_BUFFER_SIZE, dmx_buffer_size) < 0) {
	IO_error ("DMX_SET_BUFFER_SIZE failed: ");
	close (fd);
	return -1;
    }


    memset (&flt, 0, sizeof (struct dmx_sct_filter_params));

    flt.pid = opt->pid;
    if ( opt->filter > 0xFFFFFF )  // we have 4 byte filter
	flt.filter.filter[idx++] = (opt->filter >> 24)&0xFF;
    if ( opt->filter > 0xFFFF )
	flt.filter.filter[idx++] = (opt->filter >> 16)&0xFF;
    if ( opt->filter > 0xFF )
	flt.filter.filter[idx++] = (opt->filter >> 8)&0xFF;
    flt.filter.filter[idx++] = opt->filter&0xFF;

    idx=0;
    if ( opt->mask > 0xFFFFFF )  // we have 4 byte filter
	flt.filter.mask[idx++] = (opt->mask >> 24)&0xFF;
    if ( opt->mask > 0xFFFF )
	flt.filter.mask[idx++] = (opt->mask >> 16)&0xFF;
    if ( opt->mask > 0xFF )
	flt.filter.mask[idx++] = (opt->mask >> 8)&0xFF;
    flt.filter.mask[idx++] = opt->mask&0xFF;

    flt.timeout = opt->timeout_ms;
    flt.flags = DMX_IMMEDIATE_START;
    if (opt->crc) flt.flags |= DMX_CHECK_CRC;

    if (ioctl (fd, DMX_SET_FILTER, &flt) < 0) {
	IO_error ("DMX_SET_FILTER failed: ");
 	close(fd);
	return -1;
    }

  }




/*
  -- read SECTION packets for pid
*/

  count = 0;
  while (1) {
    long   n;


    n = sect_read(fd,buf,sizeof(buf));

    // -- error or eof?
    if (n < 0) {
	int err;
	
	err = IO_error("read");
	if (err == ETIMEDOUT) break;		// Timout, abort
	continue;
    }

    if (n == 0) {
	if (dmxMode) continue;	// dmxmode = no eof!
	else break;		// filemode eof 
    }



    count ++;

    if (opt->binary_out) {

       // direct write to FD 1 ( == stdout)
       write (1, buf,n);

    } else {

       indent (0);
       print_packet_header (opt, "SECT", opt->pid, count, n, 0);

// $$$ TODO: check pid in -if  (warnung: dmx read may use mask!)
// $$$ TODO: mask may also be used with -if

       if (opt->buffer_hexdump) {
           printhex_buf (0,buf, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodeSections_buf (buf,n ,opt->pid);
          out_nl (3,"==========================================================");
          out_NL (3);
       }
    } // bin_out



    // Clean Buffer
//    if (n > 0 && n < sizeof(buf)) memset (buf,0,n+1); 


    // count packets ?
    if (opt->rd_packet_count > 0) {
       if (count >= opt->rd_packet_count) break;
    }


  } // while


  /*
    -- Stop Demux
  */

  if (dmxMode) {
    ioctl (fd, DMX_STOP, 0);
  }


  close(fd);
  return 0;
}







/*
 * -- section read
 * -- read one section
 * -- return: equivalent to read();
 */

static long  sect_read (int fd, u_char *buf, long buflen)
{
    int    n;
    int    sect_len;


    n = read(fd,buf,3);				// read section header
    if (n <= 0) return n;			// error or strange, abort

    // section size
    // -- table_id 	8  uimsbf
    // -- some stuff   	4  bits
    // -- sectionlength 12 uimsbf

    sect_len = ((buf[1] & 0x0F) << 8) + buf[2];	// get section size
    if (sect_len > (buflen-3)) return -1;	// something odd?

    n = read(fd,buf+3,sect_len);		// read 1 section
    if (n >=0) n += 3;				// we already read header bytes

    return n;
}



/*
  Annotation:
    We could also do a soft-CRC32 check here.
    But to do this properly, we have to do this "read" byte shifted, because
    we do not know, when a payload_unit_start is indicated in TS.
    Also this would be necessary, when reading from playback stream files.
    so: currently we only offer hardware supported crc check...
 */



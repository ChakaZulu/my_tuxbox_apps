/*
  Zapit  -   DBoxII-Project
  
  $Id: zapit.cpp,v 1.2 2001/09/28 17:05:42 faralla Exp $
  
  Done 2001 by Philipp Leusmann using many parts of code from older 
  applications by the DBoxII-Project.
  
  Kommentar:
  
  Dies ist ein zapper der f�r die kommunikation �ber tcp/ip ausgelegt ist.
  Er benutzt Port 1505
  Die Kanalliste mu� als /var/zapit/settings.xml erstellt vorhanden sein.
  
  
  cmd = 1 zap to channel (numeric)
  param = channelnumber
  
  cmd = 2 kill current zap for streaming
  
  
  cmd = 3 zap to channel (channelname)
  param3 = channelname
  
  cmd = 4 shutdown the box
  
  cmd = 5 Get the Channellist
  
  cmd = 6 Switch to RadioMode
  
  cmd = 7 Switch to TVMode
  
  cmd = 8 Get back a struct of current Apid-descriptions. 
  
  cmd = 9 Change apid 
  param = apid-number (0 .. count_apids-1)
  
  cmd = 'a' Get last channel
  
  cmd = 'b' Get current vpid and apid
  
  cmd = 'c'  - wie cmd 5, nur mit onid_tsid
  
  Bei Fehlschlagen eines Kommandos wird der negative Wert des kommandos zur�ckgegeben.
  
  License: GPL
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  
  $Log: zapit.cpp,v $
  Revision 1.2  2001/09/28 17:05:42  faralla
  bugfix

  Revision 1.1  2001/09/28 14:34:30  faralla
  fake-cpp redesign

  Revision 1.36  2001/09/26 16:26:26  field
  BUGFIX (argh)
  
  Revision 1.35  2001/09/26 15:01:55  field
  Crypted Sender-Handling verbessert
  
  Revision 1.33  2001/09/26 11:05:43  field
  Sprach/Tonhandling verbessert
  
  Revision 1.32  2001/09/26 09:55:31  field
  Tontraeger-Auswahl
  
  Revision 1.31  2001/09/25 12:46:24  field
  AC3 gefixt
  
  Revision 1.30  2001/09/24 16:30:31  field
  Sprachnamen (ausser bei PW)
  
  Revision 1.27  2001/09/22 01:45:21  field
  kleiner fix
  
  Revision 1.26  2001/09/22 01:18:27  field
  Sprachauswahl gefixt, Bug behoben
  
  Revision 1.25  2001/09/19 23:30:31  field
  sid integriert
  
  Revision 1.23  2001/09/19 20:46:33  field
  audio-handling gefixt!

  Revision 1.22  2001/09/14 16:34:51  fnbrd
  Added id and log
  
  
*/



#include "zapit.h"
#include "../../apps//mczap/lcdd/lcdd.h"

uint16_t old_tsid  = 0;
uint curr_onid_sid = 0;
secVoltage Oldvoltage = -1;
secToneMode OldTone = -1;
boolean OldAC3 = false;
char    OldParam;

uint8_t fec_inner = 0; 
uint16_t vpid, apid, pmt = 0;
boolean im_gone = true;

int video = -1;
int audio = -1;

//chanptr tv_channels, radio_channels, current;
std::map<uint, uint> allnumchannels_tv;
std::map<uint, uint> allnumchannels_radio;
std::map<std::string, uint> allnamechannels_tv;
std::map<std::string, uint> allnamechannels_radio;

std::map<uint, transponder>transponders;
std::map<uint, channel> allchans_tv;
std::map<uint, uint> numchans_tv;
std::map<std::string, uint> namechans_tv;
std::map<uint, channel> allchans_radio;
std::map<uint, uint> numchans_radio;
std::map<std::string, uint> namechans_radio;

typedef std::map<uint, transponder>::iterator titerator;

boolean Radiomode_on = false;
pids pids_desc;
boolean caid_set = false;

volatile sig_atomic_t keep_going = 1; /* controls program termination */

void termination_handler (int signum)
{
  keep_going = 0;
  signal (signum, termination_handler);
}

void write_lcd(char *name) {
  int lcdd_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct lcdd_msg lmsg;
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(1510);
  inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
  
  //printf("Writing name %s to lcdd", name);
  if(connect(lcdd_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
    {
      lmsg.version=LCDD_VERSION;
      lmsg.cmd=LC_CHANNEL;
      strcpy(lmsg.param3, name);
      write(lcdd_fd,&lmsg,sizeof(lmsg));
    } else {
      perror("connect: ");
    }
  close(lcdd_fd);
}

// war descriptor
int parsePMTInfo(char *buffer, int len, int ca_system_id)
{
  int count=0;
  int desc,len2,ca_id, ca_pid=0;
  
  while(count<len)
    {
      desc=buffer[count++];
      len2=buffer[count++];
      if (desc == 0x09)
        {
	  ca_id=(buffer[count]<<8)|buffer[count+1];
	  count+=2;
	  if ((ca_id == ca_system_id) && ((ca_id>>8) == ((0x18|0x27)&0xD7)))
	    {
	      ca_pid= ( ( buffer[count]& 0x1F )<<8 ) | buffer[count+1];
	    }
	  count+=2;
	  count+=(len2-4);
        }
      else
	count+=len2;
    }
  return ca_pid;
}



pids parse_pmt(int pid, int ca_system_id)
{
  char buffer[1000];
  int fd, r=1000;
  int pt;
  int ap_count=0;
  int vp_count=0;
  int ecm_pid=0;
  struct dmxSctFilterParams flt;
  pids ret_pids;
  struct pollfd dmx_fd;
  
  
  
  //printf("Starting parsepmt()\n");
  memset(&ret_pids,0,sizeof(ret_pids));
  
  fd=open(DEMUX_DEV, O_RDWR);
  if (fd<0)
    {
      perror("/dev/ost/demux0");
      return ret_pids;
    }
  
  memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
  memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);
  
  flt.pid=pid;
  flt.filter.filter[0]=2;
  
  flt.filter.mask[0]  =0xFF;
  flt.timeout=5000;
  flt.flags=DMX_IMMEDIATE_START;
  
  //printf("parsepmt is setting DMX-FILTER\n");
  if (ioctl(fd, DMX_SET_FILTER, &flt)<0)
    {
      perror("DMX_SET_FILTER");
      close(fd);
      return ret_pids;
    }
  //printf("parsepmt is starting DEMUX\n");
  ioctl(fd, DMX_START, 0);
  
  dmx_fd.fd = fd;
  dmx_fd.events = POLLIN;
  dmx_fd.revents = 0;
  
  pt = poll(&dmx_fd, 1, 800);  // war 500;
  
  if (!pt)
    {
      printf("Poll Timeout\n");
      close(fd);
      return ret_pids;
    }
  else if (pt < 0)
    {
      perror ("pmt-poll");
      close(fd);
      return ret_pids;
    }
  else
    {
      //printf("parsepmt is reading DMX\n");
      if ( (r=read(fd, buffer, 3))<=0 )
        {
	  perror("read");
	  close(fd);
	  return ret_pids;
        }
      
      //printf("parsepmt is parsing pmts\n");
      int PMTInfoLen, dp, sec_len;
      
      sec_len = (((buffer[1]&0xF)<<8) + buffer[2]);
      
      if ((r=read(fd, buffer+3, sec_len))<=0)
        {
	  perror("read");
	  close(fd);
	  return ret_pids;
        }
      
      /*
	FILE *file=fopen("zapit.pmt", "wb");
	if(file) {
        fwrite(buffer, sec_len+ 3, 1, file);
        fclose(file);
	}
      */
      PMTInfoLen = ( (buffer[10]&0xF)<<8 )| buffer[11];
      dp= 12;
      
      //        while (dp<(pilen+12))
      // wozu so kompliziert?
      if ( PMTInfoLen> 0 )
        {
	  ecm_pid= parsePMTInfo(&buffer[12], PMTInfoLen, ca_system_id);
	  dp+= PMTInfoLen;
        }
      else
	{
	  ecm_pid = no_ecmpid_found; // not scrambled...
	}
      
      while ( dp < ( r- 4 ) )
        {
	  int epid, esinfo, stype;
	  stype = buffer[dp++];
	  //            printf("stream type: %x\n", stype);
	  
	  epid = (buffer[dp++]&0x1F)<<8;
	  epid|= buffer[dp++];
	  esinfo = (buffer[dp++]&0xF)<<8;
	  esinfo|= buffer[dp++];
	  
	  
	  if (((stype == 1) || (stype == 2)) && (epid != 0))
            {
	      ret_pids.vpid = epid;
	      vp_count++;
	      dp+= esinfo;
            }
	  else if ( ( (stype == 3) || (stype == 4) || (stype == 6) ) && (ap_count <max_num_apids ) && (epid != 0) )
            {
	      int i_pt= dp;
	      int tag_type, tag_len;
	      ret_pids.apids[ap_count].component_tag= -1;
	      dp+= esinfo;
	      
	      ret_pids.apids[ap_count].is_ac3 = false;
	      ret_pids.apids[ap_count].desc[0] = 0;
	      
	      while ( i_pt< dp )
                {
		  tag_type = buffer[i_pt++];
		  tag_len = buffer[i_pt++];
		  if ( tag_type == 0x6A ) // AC3
                    {
		      ret_pids.apids[ap_count].is_ac3 = true;
                    }
		  else if ( tag_type == 0x0A ) // LangDescriptor
                    {
		      if ( ret_pids.apids[ap_count].desc[0] == 0 )
                        {
			  buffer[i_pt+ 3]= 0; // quick'n'dirty
			  
			  strcpy( ret_pids.apids[ap_count].desc, &(buffer[i_pt]) );
                        }
                    }
		  else if ( tag_type == 0x52 ) // STREAM_IDENTIFIER_DESCR
                    {
		      ret_pids.apids[ap_count].component_tag = buffer[i_pt];
                    }
		  
		  i_pt+= tag_len;
                }
	      if ( (stype == 3) || (stype == 4) || ( ret_pids.apids[ap_count].is_ac3 ) )
                {
		  if ( ret_pids.apids[ap_count].desc[0] == 0 )
		    sprintf(ret_pids.apids[ap_count].desc, "%02d", ap_count+ 1);
		  
		  ret_pids.apids[ap_count].pid = epid;
		  ap_count++;
                }
	      
            }
	  else
	    dp+= esinfo;
        }
      ret_pids.count_apids = ap_count;
      ret_pids.count_vpids = vp_count;
      ret_pids.ecmpid = ecm_pid;
    }
  //printf("parsepmt is nearly over\n");
  close(fd);
  return ret_pids;
}

int find_emmpid(int ca_system_id) 
{           
  char buffer[1000];           
  int fd, r=1000 ,count;           
  struct dmxSctFilterParams flt;              

  fd=open("/dev/ost/demux0", O_RDWR);           
  if (fd<0)           
    {                   
      perror("/dev/ost/demux0");                   
      return 0;           
    }              

  memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);           
  memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);              
  flt.pid=1;           
  flt.filter.filter[0]=1;              
  flt.filter.mask[0]        =0xFF;           
  flt.timeout=1000;           
  flt.flags=DMX_ONESHOT;              
  //flt.flags=0;           
  if (ioctl(fd, DMX_SET_FILTER, &flt)<0)
    {           
      close(fd);                   
      perror("DMX_SET_FILTER");                   
      return 0;
    }              
  
  ioctl(fd, DMX_START, 0);           
  if ((r=read(fd, buffer, r))<=0)
    {           
      close(fd);                   
      perror("read");                   
      return 0;
    }              
  close(fd);              
  
  if (r<=0) return 0;              
  r=((buffer[1]&0x0F)<<8)|buffer[2];
  //for(count=0;count<r-1;count++)          
  //        printf("%02d: %02X\n",count,buffer[count]);              
  count=8;           

  while(count<r-1) {                    
    //printf("CAID %04X EMM: %04X\n",((buffer[count+2]<<8)|buffer[count+3]),((buffer[count+4]<<8)|buffer[count+5])&0x1FFF);      
    if ((((buffer[count+2]<<8)|buffer[count+3]) == ca_system_id) && (buffer[count+2] == ((0x18|0x27)&0xD7)))                           
      return (((buffer[count+4]<<8)|buffer[count+5])&0x1FFF);                          
    count+=buffer[count+1]+2;
  }           
  
  return 0;  
}

void _writecam(int cmd, unsigned char *data, int len)
{
  int camfd;
  ca_msg_t ca_msg;
  char buffer[128];
  int csum=0, i;
  
  camfd=open("/dev/ost/ca0", O_RDWR);
  
  if( camfd <= 0 )
    {
      close(camfd);
      perror("_writecam: open ca0");
      return;
    }
  
  buffer[0]=0x6E;
  buffer[1]=0x50;
  buffer[2]=(len+1)|((cmd!=0x23)?0x80:0);
  buffer[3]=cmd;
  memcpy(buffer+4, data, len);
  len+=4;
  for (i=0; i<len; i++)
    csum^=buffer[i];
  buffer[len++]=csum;
  
  /* init ca message */
  ca_msg.index = 0;
  ca_msg.type = 0;
  
  ca_msg.length = len-1;
  memcpy(ca_msg.msg,buffer+1,len-1);
  
  
  if ( ioctl(camfd,CA_SEND_MSG,&ca_msg) != 0 )
    {
      perror("_writecam: ioctl");
    }
  /*
    printf("%d >",len);
    for (i=0; i<len; i++)
    printf(" %02x", buffer[i]);
    printf("\n");
  */
  close(camfd);
}

void writecam(unsigned char *data, int len)
{
  _writecam(0x23, data, len);
}

void descramble(int onID, int serviceID, int unknown, int caID, int ecmpid, int vpid, int apid)
{
  unsigned char buffer[20];
  buffer[0]=0x0D;
  buffer[1]=onID>>8;
  buffer[2]=onID&0xFF;
  buffer[3]=serviceID>>8;
  buffer[4]=serviceID&0xFF;
  buffer[5]=unknown>>8;
  buffer[6]=unknown&0xFF;
  buffer[7]=caID>>8;
  buffer[8]=caID&0xFF;
  buffer[9]=ecmpid>>8;
  buffer[10]=ecmpid&0xFF;
  buffer[11]=0x02;
  buffer[12]=vpid>>8;
  buffer[13]=vpid&0xFF;
  buffer[14]=0x80;
  buffer[15]=0;
  buffer[16]=apid>>8;
  buffer[17]=apid&0xFF;
  buffer[18]=0x80;
  buffer[19]=0;
  writecam(buffer, 20);
}

void cam_reset(void)
{
  unsigned char buffer[1];
  buffer[0]=0x9;
  writecam(buffer, 1);
}

void setemm(int unknown, int caID, int emmpid)
{
	unsigned char buffer[7];
	buffer[0]=0x84;
	buffer[1]=unknown>>8;
	buffer[2]=unknown&0xFF;
	buffer[3]=caID>>8;
	buffer[4]=caID&0xFF;
	buffer[5]=emmpid>>8;
	buffer[6]=emmpid&0xFF;
	writecam(buffer, 7);
}


void save_settings()
{
  FILE *channel_settings;
  std::map<uint, channel>::iterator cit;
  channel_settings = fopen("/var/zapit/last_chan", "w");
  
  if (channel_settings == NULL)
    {
      perror("fopen: ");
    }
  
  if (Radiomode_on)
    {
    fprintf(channel_settings, "radio\n");
    cit = allchans_radio.find(curr_onid_sid);
    }
  else
    {
    fprintf(channel_settings, "tv\n");
    cit = allchans_tv.find(curr_onid_sid);
    }

  
  fprintf(channel_settings, "%06d\n", cit->second.chan_nr);
  fprintf(channel_settings, "%s\n", cit->second.name.c_str());
  
  fclose(channel_settings);
  //printf("Saved settings\n");
}

channel_msg load_settings()
{
  FILE *channel_settings;
  channel_msg output_msg;
  char *buffer;
  
  buffer = (char*) malloc(31);
  
  memset(&output_msg, 0, sizeof(output_msg));
  
  channel_settings = fopen("/var/zapit/last_chan", "r");
  
  if (channel_settings == NULL)
    {
      perror("fopen: /var/zapit/last_chan");
      output_msg.mode = 't';
      output_msg.chan_nr = 1;
      return output_msg;
    }
  
  fscanf(channel_settings, "%s", buffer);
  if (!strcmp(buffer, "tv"))
    output_msg.mode = 't';
  else if (!strcmp(buffer, "radio"))
    output_msg.mode = 'r';
  else
    {
      printf("No valid settings found\n");
      output_msg.mode = 't';
      output_msg.chan_nr = 1;
      return output_msg;
    }
  
  
  
  fscanf(channel_settings, "%s", buffer);
  output_msg.chan_nr = atoi(buffer);
  
  fscanf(channel_settings, "%s", buffer);
  strncpy(output_msg.name, buffer, 30);
  
  fclose(channel_settings);
  
  return output_msg;
}

int tune(uint tsid)
{
  struct qpskParameters front;
  struct secCmdSequence seq;
  struct secCommand cmd;
  struct secDiseqcCmd diseqc;
  int dosec;
  unsigned char bits;
  int sec_device = -1;
  uint8_t Fec_inner;
  int device = -1;
  uint Frequency, Symbolrate;

  if (transponders.count(tsid) == 0)
    {
      printf("No transponder with tsid %04x found\n", tsid);
      return -1;
    }
  
  titerator transponder = transponders.find(tsid);

  
  diseqc.addr=0x10;
  diseqc.cmd=0x38;
  diseqc.numParams=1;
  diseqc.params[0]=0xF0;
  cmd.type=SEC_CMDTYPE_DISEQC;
  bits=transponder->second.diseqc << 2;

   if (transponder->second.frequency<8000)
    {
      // cable
      Frequency = transponder->second.frequency*100;
      dosec = 0;
    }
  else if (transponder->second.frequency>11700)
    {
      Frequency = transponder->second.frequency*1000-10600000;
      seq.continuousTone = SEC_TONE_ON;
      bits|=1;
      dosec = 1;
    }
  else
    {
      Frequency = transponder->second.frequency*1000-9750000;
      seq.continuousTone = SEC_TONE_OFF;
      bits|=0;
      dosec = 1;
    }
  Frequency += offset;
  Symbolrate = transponder->second.symbolrate*1000;
  
  if (dosec)
    {
      if (transponder->second.polarity)
        {
	  seq.voltage=SEC_VOLTAGE_13;
	  bits|=0;
        }
      else
        {
	  seq.voltage=SEC_VOLTAGE_18;
	  bits|=2;
        }
      
      diseqc.params[0]|=bits;
      
      if ( ( Oldvoltage != seq.voltage ) ||
	   ( OldTone != seq.continuousTone ) ||
	   ( OldParam != diseqc.params[0] ) )
        {
	  if((sec_device = open(SEC_DEV, O_RDWR)) < 0)
            {
	      printf("Cannot open SEC device \"%s\"\n",SEC_DEV);
	      exit(-3);
            }
	  
	  cmd.u.diseqc=diseqc;
	  seq.miniCommand=SEC_MINI_NONE;
	  seq.numCommands=1;
	  seq.commands=&cmd;
	  
	  Oldvoltage = seq.voltage;
	  OldTone = seq.continuousTone;
	  OldParam = diseqc.params[0];
	  
	  ioctl(sec_device,SEC_SEND_SEQUENCE,&seq);
	  
	  close(sec_device);
	  sec_device= -1;
        }
    }
  
  if (caid == 0)
    caid=0x1722;

  Fec_inner = 0;
  
  if ( ( device = open(FRONT_DEV, O_RDWR) ) < 0 )
    {
      printf("Cannot open frontend device \"%s\"\n",FRONT_DEV);
      //close(device);
      
      if ( video>= 0 )
	close(video);
      if ( audio>= 0 )
	close(audio);
      
      exit(-3);
    };
  printf ("Have to tune to freq: %d, sr: %d\n", Frequency, Symbolrate);
  front.iFrequency = Frequency;
  front.SymbolRate = Symbolrate;
  front.FEC_inner = Fec_inner;
  ioctl(device,QPSK_TUNE,&front);
  
  close(device);
  device= -1;
  
  old_tsid = tsid;

  return 23;
}

int zapit (uint onid_sid) {

  struct dmxPesFilterParams pes_filter;
  int vid = -1;
  uint16_t Pmt, Vpid, Apid;
  pids parse_pmt_pids;
  std::map<uint, channel>::iterator cit;
  //  time_t current_time = 0;
  uint16_t emmpid;
  boolean do_search_emmpid;
 
  if (Radiomode_on)
    cit = allchans_radio.find(onid_sid);
  else
    cit = allchans_tv.find(onid_sid);

  
  if ( ( vid = open(VIDEO_DEV, O_RDWR) ) < 0)
    {
      printf("Cannot open video device \"%s\"\n",FRONT_DEV);
      exit(1);
    }
  
  ioctl(video,DMX_STOP,0);
  
  if ( video>= 0 )
    {
      ioctl(vid, VIDEO_STOP, false);
      close(video);
      video = -1;
    }
  
  if ( audio>= 0 )
    {
      ioctl(audio,DMX_STOP,0);
      close(audio);
      audio = -1;
    }
  
  if (cit->second.tsid != old_tsid)
    {
      printf("[zapit] tunig to tsid %04x\n", cit->second.tsid);
      if (tune(cit->second.tsid) < 0)
	{
	  printf("No transponder with tsid %04x found\n", cit->second.tsid);
	  exit(-1);
	}
      do_search_emmpid = true;
    }
else
  do_search_emmpid = false;

  memset(&parse_pmt_pids,0,sizeof(parse_pmt_pids));
  parse_pmt_pids = parse_pmt(cit->second.pmt, caid);
  
  //printf("VPID parsed from pmt: %x\n", parse_pmt_pids.vpid);
  //for (i = 0;i<parse_pmt_pids.count_apids;i++) {
  //   printf("Audio-PID %d from parse_pmt() ist: %x\n",i,parse_pmt_pids.apid[i]);
  //}
  
  if (parse_pmt_pids.count_vpids >0)
    {
      cit->second.vpid = parse_pmt_pids.vpid;
      if (cit->second.vpid == 0)
	cit->second.vpid = 0x1fff;
      //  cit->second.last_update = current_time;
    }
  else
    {
      printf("Using standard-Pids\n");
    }
  
  if (parse_pmt_pids.count_apids >0)
    {
      cit->second.apid = parse_pmt_pids.apids[0].pid;
      //  cit->second.last_update = current_time;
    }
  
  //    if (parse_pmt_pids.ecmpid != 0)
  {
    cit->second.ecmpid = parse_pmt_pids.ecmpid;
    //	cit->second.last_update = current_time;
  }
  
  write_lcd((char*) cit->second.name.c_str());
  
  if (cit->second.vpid != 0x1fff || cit->second.apid != 0x8191)
    {
      pids_desc = parse_pmt_pids;
      
      Vpid = cit->second.vpid;
      Apid = cit->second.apid;
      Pmt = cit->second.pmt;
      
      printf("Zapping to %s. VPID: 0x%04x. APID: 0x%04x, PMT: 0x%04x\n", cit->second.name.c_str(), cit->second.vpid, cit->second.apid, cit->second.pmt);
      
      
      //        descramble(0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff);
      cam_reset();
      if ( ( cit->second.ecmpid > 0 ) && ( cit->second.ecmpid != no_ecmpid_found ) )
	{
	  descramble(cit->second.onid, cit->second.tsid, 0x104, caid, cit->second.ecmpid, Apid, Vpid);
	  if (do_search_emmpid)
	    {
	      if((emmpid = find_emmpid(caid)) != 0)
		{
		  printf("[zapit] emmpid >0x%04x< found for caid 0x%04x\n", emmpid, caid);
		  setemm(0x104, caid, emmpid);
		}
	      else
		printf("[zapit] no emmpid found...\n");
	    }
	}

      if ( (video = open(DEMUX_DEV, O_RDWR)) < 0)
        {
	  printf("[zapit] cannot open demux device \"%s\"\n",DEMUX_DEV);
	  exit(1);
        }
      
      /* vpid */
      pes_filter.pid     = Vpid;
      pes_filter.input   = DMX_IN_FRONTEND;
      pes_filter.output  = DMX_OUT_DECODER;
      pes_filter.pesType = DMX_PES_VIDEO;
      pes_filter.flags   = 0;
      ioctl(video,DMX_SET_PES_FILTER,&pes_filter);
      vpid = Vpid;
     
      if((audio = open(DEMUX_DEV, O_RDWR)) < 0)
        {
	  printf("Cannot open demux device \"%s\"\n",DEMUX_DEV);
	  exit(1);
        }
      
      /* apid */
      pes_filter.pid     = Apid;
      pes_filter.input   = DMX_IN_FRONTEND;
      pes_filter.output  = DMX_OUT_DECODER;
      pes_filter.pesType = DMX_PES_AUDIO;
      pes_filter.flags   = 0;
      //printf("changing filtered apid to %d\n", Apid);
      ioctl(audio,DMX_SET_PES_FILTER,&pes_filter);
      apid = Apid;
      
      if ( parse_pmt_pids.apids[0].is_ac3 != OldAC3 )
        {
	  OldAC3 = parse_pmt_pids.apids[0].is_ac3;
	  int ac3d=open(AUDIO_DEV, O_RDWR);
	  if( ac3d < 0 )
            {
	      printf("Cannot open audio device \"%s\"\n",AUDIO_DEV);
	      exit(1);
            }
	  else
	    {
	      //printf("Setting audiomode to %d", ( OldAC3 )?0:1);
	      ioctl(ac3d, AUDIO_SET_BYPASS_MODE, ( OldAC3 )?0:1);
	      close(ac3d);
	    }
    	}
      
      if (ioctl(audio,DMX_START,0) < 0)
	printf("\t\tATTENTION audio-ioctl not succesfull\n");
      if (ioctl(video,DMX_START,0)<0)
	printf("\t\tATTENTION video-ioctl not succesfull\n");
      
      ioctl(vid, VIDEO_SELECT_SOURCE, (videoStreamSource_t)VIDEO_SOURCE_DEMUX);
      ioctl(vid, VIDEO_PLAY, 0);
      
      close(vid);
      vid= -1;
      //printf("Saving settings\n");
      curr_onid_sid = onid_sid;
      save_settings();
    }
  else
    {
      printf("[zapit] Not a channel. Won�t zap\n");
      
      close(vid);
      return -3;
    }
  return 3;
}


int numzap(int channel_number) {
  std::map<uint, uint>::iterator cit;
  
if (Radiomode_on)
    {
      if (allnumchannels_radio.count(channel_number) == 0)
	{
	  printf("[zapit] Given channel not found");
	  return -2;
	}
      cit  = allnumchannels_radio.find(channel_number);
    }
  else
    {
      if (allnumchannels_tv.count(channel_number) == 0)
	{
	  printf("[zapit] Given channel not found");
	  return -2;
	}
      cit = allnumchannels_tv.find(channel_number);
    }
  
 printf("Zapping to onid_sid %04x\n", cit->second);
 if (zapit(cit->second) > 0)
   return 2;
 else
   return -2;
}


int namezap(std::string channel_name) {
  std::map<std::string,uint>::iterator cit;
  // Search current channel
  if (Radiomode_on)
    {
      if (allnamechannels_radio.count(channel_name) == 0)
	{
	  printf("Given channel not found");
	  return -2;
	}
      cit  = allnamechannels_radio.find(channel_name);
    }
  else
    {
      if (allnamechannels_tv.count(channel_name) == 0)
	{
	  printf("Given channel not found");
	  return -2;
	}
      cit = allnamechannels_tv.find(channel_name);
    }
  
  printf("Zapping to onid_sid %04x\n", cit->second);
  if (zapit(cit->second) > 0)
    return 3;
  else
    return -3;
}

int changeapid(ushort pid_nr) 
{
  struct dmxPesFilterParams pes_filter;
  std::map<uint,channel>::iterator cit;

  if (Radiomode_on)
    cit = allchans_radio.find(curr_onid_sid);
  else
    cit = allchans_tv.find(curr_onid_sid);

  if (pid_nr <= pids_desc.count_apids)
    {
      if ( audio>= 0 )
        {
	  close(audio);
	  audio = -1;
        }
      
      //        descramble(0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff);
      
      if ( ( cit->second.ecmpid > 0 ) && ( cit->second.ecmpid != no_ecmpid_found ) )
        {
	  cam_reset();
	  descramble(cit->second.onid, cit->second.tsid, 0x104, caid, cit->second.ecmpid,  pids_desc.apids[pid_nr].pid , vpid);
        }
      
      //        printf("Changing APID of %s. VPID: 0x%04x. APID: 0x%04x, PMT: 0x%04x\n", current->name, current->vpid, pids_desc.apid[pid_nr], current->pmt);
      
      if((audio = open(DEMUX_DEV, O_RDWR)) < 0)
        {
	  printf("Cannot open demux device \"%s\"\n",DEMUX_DEV);
	  exit(1);
        }
      
      /* apid */
      pes_filter.pid     = pids_desc.apids[pid_nr].pid;
      pes_filter.input   = DMX_IN_FRONTEND;
      pes_filter.output  = DMX_OUT_DECODER;
      pes_filter.pesType = DMX_PES_AUDIO;
      pes_filter.flags   = 0;
      
      //        printf("changing filtered apid to %d [%d]\n",  pids_desc.apid[pid_nr], pid_nr);
      ioctl(audio,DMX_STOP);
      ioctl(audio,DMX_SET_PES_FILTER,&pes_filter);
      apid = pids_desc.apids[pid_nr].pid;
      
      if ( pids_desc.apids[pid_nr].is_ac3 != OldAC3 )
        {
	  OldAC3 = pids_desc.apids[pid_nr].is_ac3;
	  int ac3d=open(AUDIO_DEV, O_RDWR);
	  if( ac3d < 0 )
            {
	      printf("[zapit] cannot open audio device \"%s\"\n",AUDIO_DEV);
	      exit(1);
            }
	  else
	    {
	      //    			printf("Setting audiomode to %d", ( OldAC3 )?0:1);
	      ioctl(ac3d, AUDIO_SET_BYPASS_MODE, ( OldAC3 )?0:1);
	      close(ac3d);
	    }
    	}
      ioctl(audio,DMX_START,0);
      return 8;
    }
  else
    {
      return -8;
    }
}


/*
  void display_pic()
  {
  struct videoDisplayStillPicture sp;
  char *picture;
  int pic_fd;
  struct stat pic_desc;
  int vid, error;
  
  pic_fd = open("/root/ilogo.mpeg", O_RDONLY);
  if (pic_fd < 0) {
  perror("open");
  exit(0);
  }
  
  if((vid = open(VIDEO_DEV, O_RDWR|O_NONBLOCK)) < 0) {
  printf("Cannot open video device \"%s\"\n",FRONT_DEV);
  exit(1);
  }
  
  fstat(pic_fd, &pic_desc);
  
  
  sp.iFrame = (char *) malloc(pic_desc.st_size);
  sp.size = pic_desc.st_size;
  printf("I-frame size: %d\n", sp.size);
	
	if(!sp.iFrame) {
		printf("No memory for I-Frame\n");
		exit(0);
	}
	
	printf("read: %d bytes\n",read(pic_fd,sp.iFrame,sp.size));
	
	if ( ioctl(vid,VIDEO_STOP,false) < 0){
		perror("VIDEO PLAY: ");
		//exit(0);
	}
	
	if ( (error = ioctl(vid,VIDEO_STILLPICTURE, sp) < 0)){
		perror("VIDEO STILLPICTURE: ");
		//exit(0);
	}
	
	sleep(3);
	
	if ( ioctl(vid,VIDEO_PLAY,0) < 0){
		perror("VIDEO PLAY: ");
		//exit(0);
	}
	
      close(vid);

}

*/		

	

void endzap()
{
  if ( video>= 0 )
    {
      close(video);
      audio = -1;
    }
  if ( audio>= 0 )
    {
      close(audio);
      audio = -1;
    }
}


void shutdownBox()
{
  if (execlp("/sbin/halt", "/sbin/halt", 0)<0)
    {
      perror("exec failed - halt\n");
    }
}

int sleepBox() {
  int device;
  if((device = open(FRONT_DEV, O_RDWR)) < 0) {
    printf("Cannot open frontend device \"%s\"\n",FRONT_DEV);
    return -4;
  };
  
  if (ioctl(device, OST_SET_POWER_STATE, OST_POWER_SUSPEND)!=0){
    printf("Cannot set suspend-mode");
    return -4;
  }
  return(4);
}

void setRadioMode() {
  printf("Switching to Radio-Mode\n");
  Radiomode_on = true;
}

void setTVMode() {
  Radiomode_on = false;
}

int prepare_channels()
{
  std::map<uint, uint>::iterator numit;
  std::map<std::string, uint>::iterator nameit;
  std::map<uint, channel>::iterator cit;

  if (LoadServices() > 0)
    {
      int number = 1;
      for (numit = numchans_tv.begin(); numit != numchans_tv.end(); numit++)
	{
	  cit = allchans_tv.find(numit->second);
	  cit->second.chan_nr = number;
	  allnumchannels_tv.insert(std::pair<uint,uint>(number++, (cit->second.onid<<16)+cit->second.sid));
	  allnamechannels_tv.insert(std::pair<std::string, uint>(cit->second.name, (cit->second.onid<<16)+cit->second.sid));
	  //printf("Inserted %d %s\n", cit->second.chan_nr, cit->second.name.c_str());
	}
      numchans_tv.clear();
      for (nameit = namechans_tv.begin(); nameit != namechans_tv.end(); nameit++)
	{
	  cit = allchans_tv.find(nameit->second);
	  cit->second.chan_nr = number;
	  allnumchannels_tv.insert(std::pair<uint, uint>(number++, (cit->second.onid<<16)+cit->second.sid));
	  allnamechannels_tv.insert(std::pair<std::string, uint>(nameit->first, (cit->second.onid<<16)+cit->second.sid));
	  //printf("Inserted %d %s\n", cit->second.chan_nr, cit->second.name.c_str());
	}
      namechans_tv.clear();
      number = 1;
      for (numit = numchans_radio.begin(); numit != numchans_radio.end(); numit++)
	{
	  cit = allchans_radio.find(numit->second);
	  cit->second.chan_nr = number;
	  allnumchannels_radio.insert(std::pair<uint,uint>(number++, (cit->second.onid<<16)+cit->second.sid));
	  allnamechannels_radio.insert(std::pair<std::string, uint>(cit->second.name, (cit->second.onid<<16)+cit->second.sid));
	  //printf("Inserted %s\n", cit->second.name.c_str());
	}
      numchans_radio.clear();
      for (nameit = namechans_radio.begin(); nameit != namechans_radio.end(); nameit++)
	{
	  cit = allchans_radio.find(nameit->second);
	  cit->second.chan_nr = number;
	  allnumchannels_radio.insert(std::pair<uint, uint>(number++, (cit->second.onid<<16)+cit->second.sid));
	  allnamechannels_radio.insert(std::pair<std::string, uint>(nameit->first, (cit->second.onid<<16)+cit->second.sid));
	  //printf("Inserted %s\n", cit->second.name.c_str());
	}
      namechans_radio.clear();
    }
else
  {
    printf("No channels found\n");
    return -1;
  }
  return 23;
}


void parse_command()
{
  char *status;
  short carsten;
  std::map<uint,uint>::iterator sit;
  std::map<uint,channel>::iterator cit;
  
  //printf ("parse_command\n");
  
  //byteorder!!!!!!
  rmsg.param2 = ((rmsg.param2 & 0x00ff) << 8) | ((rmsg.param2 & 0xff00) >> 8);
  
  /*
    printf("Command received\n");
    printf("  Version: %d\n", rmsg.version);  
    printf("  Command: %d\n", rmsg.cmd);
    printf("  Param: %c\n", rmsg.param);
    printf("  Param2: %d\n", rmsg.param2);
    printf("  Param3: %s\n", rmsg.param3);
  */
  if(rmsg.version!=1)
    {
      perror("unknown version\n");
      return;
    }
  
  switch (rmsg.cmd)
    {
    case 1:
      printf("zapping by number\n");
      if (numzap( atoi((const char*) &rmsg.param) ) > 0)
      	status = "001";
      else
      	status = "-01";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 2:
      printf("killing zap\n");
      endzap();
      status = "002";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("Could not send any retun\n");
	return;
      }	
      break;
    case 3:
      printf("zapping by name\n");
      if (namezap(rmsg.param3) == 3) 
      	status = "003";
      else
      	status = "-03";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 4:
      status = "004";
      
      printf("shutdown\n");
      shutdownBox();
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 5:
      if (Radiomode_on)
    	{ 
	  if (!allchans_radio.empty())
	    {
	      status = "005";
	      if (send(connfd, status, strlen(status),0) == -1) {
		perror("Could not send any retun\n");
		return;
	      }
	      for (sit = allnumchannels_radio.begin(); sit != allnumchannels_radio.end(); sit++)
		{
		  cit = allchans_radio.find(sit->second);
		  channel_msg chanmsg;
		  strncpy(chanmsg.name, cit->second.name.c_str(),29);
		  chanmsg.chan_nr = sit->first;
		  chanmsg.mode = 'r';
		  if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1) 
		    {
		      perror("Could not send any retun\n");
		      return;
		    }
		}
	    }
	  else
	    {
	      printf("radio_channellist is empty\n");
	      status = "-05";
	      if (send(connfd, status, strlen(status),0) == -1) 
	       {
		 perror("Could not send any retun\n");
		 return;
	       }
	    }
      	} else {
	  if (!allchans_tv.empty())
	    {
	    status = "005";
	    if (send(connfd, status, strlen(status),0) == -1) 
	      {
		perror("Could not send any retun\n");
		return;
	      }
	    for (sit = allnumchannels_tv.begin(); sit != allnumchannels_tv.end(); sit++)
		{
		  cit = allchans_tv.find(sit->second);
		  channel_msg chanmsg;
		  strncpy(chanmsg.name, cit->second.name.c_str(),29);
		  chanmsg.chan_nr = sit->first;
		  chanmsg.mode = 't';
		  if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1) 
		    {
		      perror("Could not send any retun\n");
		      return;
		    }
		}
	    }
	  else
	    {
	      printf("tv_channellist is empty\n");
	      status = "-05";
	      if (send(connfd, status, strlen(status),0) == -1) 
		{
		  perror("Could not send any retun\n");
		  return;
		}
	    }
	}
      usleep(200000);
      break;
    case 6:
      status = "006";
      setRadioMode();
      if (!allchans_radio.empty())
      	status = "-06";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 7:
      status = "007";
      setTVMode();
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 8:
      status = "008";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      if (send(connfd, &pids_desc , sizeof(pids),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 9:
      if (changeapid(atoi((const char*) &rmsg.param)) > 0) 
	status = "009";
      else
	status = "-09";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    case 'a':
      status = "00a";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      {
	channel_msg settings = load_settings();
      	if (send(connfd, &settings, sizeof(settings),0) == -1) {
	  perror("Could not send any retun\n");
	  return;
	}
      }
      break;
    case 'b':
      if (Radiomode_on)
	cit = allchans_radio.find(curr_onid_sid);
      else
	cit =allchans_tv.find(curr_onid_sid);

      if (curr_onid_sid == 0)
	{
	  status = "-0b";
	  break;
	}
      else
	status = "00b";
      //printf("zapit is sending back a status-msg %s\n", status);	
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      carsten = (short) cit->second.vpid;
      if (send(connfd, &carsten, 2,0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      carsten = (short) cit->second.apid;
      if (send(connfd, &carsten, 2,0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      break;		
    case 'c':
       if (Radiomode_on)
    	{ 
	  if (!allchans_radio.empty())
	    {
	      status = "00c";
	      if (send(connfd, status, strlen(status),0) == -1) {
		perror("Could not send any retun\n");
		return;
	      }
	      for (sit = allnumchannels_radio.begin(); sit != allnumchannels_radio.end(); sit++)
		{
		  cit = allchans_radio.find(sit->second);
		  channel_msg_2 chanmsg;
		  strncpy(chanmsg.name, cit->second.name.c_str(),30);
		  chanmsg.onid_tsid = (cit->second.onid<<16)|cit->second.sid;
		  chanmsg.chan_nr = sit->first;

		  if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1) 
		    {
		      perror("Could not send any retun\n");
		      return;
		    }
		}
	    }
	  else
	    {
	      printf("tv_channellist is empty\n");
	      status = "-0c";
	      if (send(connfd, status, strlen(status),0) == -1) 
	       {
		 perror("Could not send any retun\n");
		 return;
	       }
	    }
      	} else {
	  if (!allchans_tv.empty())
	    {
	    status = "00c";
	    if (send(connfd, status, strlen(status),0) == -1) 
	      {
		perror("Could not send any retun\n");
		return;
	      }
	    for (sit = allnumchannels_tv.begin(); sit != allnumchannels_tv.end(); sit++)
		{
		  cit = allchans_tv.find(sit->second);
		  channel_msg_2 chanmsg;
		  strncpy(chanmsg.name, cit->second.name.c_str(),30);
		  chanmsg.chan_nr = sit->first;
		  chanmsg.onid_tsid = (cit->second.onid<<16)|cit->second.sid;

		  if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1) 
		    {
		      perror("Could not send any retun\n");
		      return;
		    }
		}
	    }
	  else
	    {
	      printf("tv_channellist is empty\n");
	      status = "-0c";
	      if (send(connfd, status, strlen(status),0) == -1) 
		{
		  perror("Could not send any retun\n");
		  return;
		}
	    }
	}
       break;
    case 'd':
      printf("zapping by number\n");
      int number;
      sscanf((const char*) &rmsg.param, "%x", &number);

      if (zapit(number) > 0)
      	status = "00d";
      else
      	status = "-0d";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("Could not send any retun\n");
	return;
      }
      break;
    default:  
      status = "000";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("Could not send any retun\n");
	return;
      }
      printf("unknown command\n");
    }
  
  
}

int network_setup() {
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(1505);
  
  if ( bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) !=0)
    {
      perror("bind failed...\n");
      exit(-1);
    }
  
  
  if (listen(listenfd, 5) !=0)
    {
      perror("listen failed...\n");
      exit( -1 );
    }
  
  return 0;
}

int main(int argc, char **argv) {
  
  channel_msg testmsg;
  int channelcount = 0;
  
  if (argc > 1) {
    if (!strcmp(argv[1], "-o")) {
      offset = atoi(argv[2]);
    }
    else
      {
	printf("Usage: zapit [-o offset in Hz]\n");
	exit(0);
      }
  }
  
  system("/usr/bin/killall camd");
  printf("Zapit $Id: zapit.cpp,v 1.2 2001/09/28 17:05:42 faralla Exp $\n\n");
  //  printf("Zapit 0.1\n\n");
  
  testmsg = load_settings();
  
  if (testmsg.mode== 'r')
    Radiomode_on = true;
 
  caid = get_caid();
  
  memset(&pids_desc, 0, sizeof(pids));
  
  if (prepare_channels() <0) {
    printf("Error parsing Services\n");
    exit(-1);
  }
  
  printf("Channels have been loaded succesfully\n");
  
  printf("We�ve got ");
  if (!allnumchannels_tv.empty())
    channelcount = allnumchannels_tv.rbegin()->first;
  printf("%d tv- and ", channelcount);
  if (!allnumchannels_radio.empty())
    channelcount = allnumchannels_radio.rbegin()->first;
  else
    channelcount = 0;
  printf("%d radio-channels\n", channelcount);
  
  if (network_setup()!=0){
    printf("Error during network_setup\n");
    exit(0);
  }
  
  
  switch (fork ())
    {
    case -1:                    /* can't fork */
      perror ("fork()");
      exit (3);
    case 0:                     /* child, process becomes a daemon: */
      //close (STDIN_FILENO);
      //close (STDOUT_FILENO);
      //close (STDERR_FILENO);
      if (setsid () == -1)      /* request a new session (job control) */
	{
	  exit (4);
	}
      break;
    default:                    /* parent returns to calling process: */
      return 0;
    }
  
  /* Establish signal handler to clean up before termination: */
  if (signal (SIGTERM, termination_handler) == SIG_IGN)
    signal (SIGTERM, SIG_IGN);
  signal (SIGINT, SIG_IGN);
  signal (SIGHUP, SIG_IGN);
  
  /* Main program loop */
  descramble(0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff);
  while (keep_going)
    {      
      clilen = sizeof(cliaddr);
      connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
      memset(&rmsg, 0, sizeof(rmsg));
      read(connfd,&rmsg,sizeof(rmsg));
      parse_command();
      close(connfd);
    }
  return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

/* NAPI */
#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#include <ost/ca.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "getservices.h"

/*Thread stuff */
#include <pthread.h>
#define errexit(code,str)                          \
	fprintf(stderr,"%s: %s\n",(str),strerror(code)); \
	exit(1);


#define EXTERNAL_CAMD
#define FRONT_DEV "/dev/ost/qpskfe0"
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV   "/dev/ost/sec0"
#define VIDEO_DEV   "/dev/ost/video0"
int dvb_device;

typedef enum {
  DVB_FEC_1_2,
  DVB_FEC_2_3,
  DVB_FEC_3_4,
  DVB_FEC_4_5,
  DVB_FEC_5_6,
  DVB_FEC_6_7,
  DVB_FEC_7_8,
  DVB_FEC_8_9,
  DVB_FEC_AUTO
} DvbFEC;

typedef enum {
  DVB_POL_HOR,
  DVB_POL_VERT
} DvbPol;

#ifdef EXTERNAL_CAMD
  char buffer[3][5];
#endif

#define SA struct sockaddr
#define SAI struct sockaddr_in

int listenfd, connfd;
socklen_t clilen;
SAI cliaddr, servaddr;

struct rmsg {
  		unsigned char version;
  		unsigned char cmd;
  		unsigned char param;
  		unsigned short param2;
  		char param3[30];

} rmsg;





extern chanptr LoadServices(int serv_mode);

void *start_camd(void *args);
void end_cam();
int descriptor(char *buffer, int len, int ca_system_id);

#ifndef __rcinput__
#define __rcinput__

#include <dbox/fp.h>
#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "pthread.h"
#include "semaphore.h"

#include "ringbuffer.h"

#include <string>

using namespace std;

class CRCInput
{
	private:

		int				fd;
		CRingBuffer		ringbuffer;
		pthread_t       thrInput;
		pthread_t       thrTimer;
		sem_t			waitforkey;
		int				timeout;

		__u16			prevrccode;

		int translate(int code);
		int getKeyInt();	//don't use!
		void start();
		
		static void * InputThread (void *arg);
		static void * TimerThread (void *arg);

	public:
		//rc-code definitions
		enum
		{
			RC_standby=0x10, RC_home=0x1F, RC_setup=0x18, RC_0=0x0, RC_1=0x1,
			RC_2=0x2, RC_3=0x3, RC_4=0x4, RC_5=0x5, RC_6=0x6, RC_7=0x7,
			RC_8=0x8, RC_9=0x9, RC_blue=0x14, RC_yellow=0x12, RC_green=0x11,
			RC_red=0x13, RC_page_up=0x54, RC_page_down=0x53, RC_up=0xC, RC_down=0xD,
			RC_left=0xB, RC_right=0xA, RC_ok=0xE, RC_plus=0x15, RC_minus=0x16,
			RC_spkr=0xF, RC_help=0x17, RC_top_left=27, RC_top_right=28, RC_bottom_left=29, RC_bottom_right=30,
			RC_timeout=-1, RC_nokey=-2
		};
		
		//constructor - opens rc-device and starts needed threads
		CRCInput();
		//destructor - closes rc-device
		~CRCInput();
		
		//get key from the input-device
		int getKey(int Timeout=-1);
		void addKey2Buffer(int);
		static string getKeyName(int);
};

#endif

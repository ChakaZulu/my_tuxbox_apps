#ifndef __remotecontrol__
#define __remotecontrol__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include <string>

#include "pthread.h"
#include "semaphore.h"
#include <sys/wait.h>
#include <signal.h>
#include "../../../zapit/getservices.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

struct st_rmsg
{
	unsigned char version;
 	unsigned char cmd;
	unsigned short param;
	unsigned short param2;
	char param3[30];
};

struct st_audio_info
{
    char    name[100];
    ushort  count_apids;
    char    apid_names[100][5];
    int     selected;
};

class CRemoteControl
{
		st_rmsg	remotemsg;
        st_audio_info apids;

		void send();
		bool zapit_mode;

        pthread_t       thrSender;
        pthread_cond_t  send_cond;
        pthread_mutex_t send_mutex;

        static void * RemoteControlThread (void *arg);

	public:
        st_audio_info   apid_info;

		CRemoteControl();
		void zapTo(int key,  string chnlname );
        void queryAPIDs();
        void setAPID(int APID);
		void shutdown();
		void setZapper (bool zapper);
		void radioMode();
		void tvMode();

	    void CopyAPIDs();
};


#endif




/******************************************************************************
 *                       <<< TuxMailD - POP3 Daemon >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmaild.h,v $
 * Revision 1.2  2003/04/29 10:36:43  lazyt
 * enable/disable audio via .conf
 *
 *
 * Revision 1.1  2003/04/21 09:24:52  lazyt
 * add tuxmail, todo: sync (filelocking?) between daemon and plugin
 ******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <curl/curl.h>

#include "audio.h"

#define DSP "/dev/sound/dsp"

#define CFGPATH "/var/tuxbox/config/tuxmail/"
#define CFGFILE "tuxmail.conf"
#define SCKFILE "/tmp/tuxmaild.sock"
#define LOGFILE "/tmp/tuxmaild.log"
#define PIDFILE "/tmp/tuxmaild.pid"

//pop3 commands

enum {INIT, USER, PASS, STAT, UIDL, TOP, DELE, QUIT};

//account database

struct
{
	char name[32];
	char host[64];
	char user[64];
	char pass[64];
	int  mail_all;
	int  mail_new;

}account_db[10];

//http command

char http_command[256];

//some data

FILE *fd_pid;
int pid;
int startdelay, intervall;
char online = 1;
char pop3log;
char logmode;
char audio;
int accounts;
int sock;
int messages, deleted_messages;
int stringindex;
char uid[128];
char header[1024];
char timeinfo[22];
time_t tt;

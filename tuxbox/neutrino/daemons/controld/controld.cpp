/*
	Control-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://mcclean.cyberphoria.org/



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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "dbox/avs_core.h"
#include <sys/ioctl.h>
 
#include "../lcdd/lcdd.h"


#define CONF_FILE "/var/controld.conf"


struct rmsg {
  unsigned char version;
  unsigned char cmd;
  unsigned short param;
  unsigned short param2;
  char param3[30];
 
} rmsg;

struct Ssettings
{
	char gen_Volume;
}settings;


void sig_catch(int);



int loadSettings()
{
	int fd;
	fd = open(CONF_FILE, O_RDONLY );
	
	if (fd==-1)
	{
		printf("[controld] error while loading settings: %s\n", CONF_FILE );
		return 0;
	}
	if(read(fd, &settings, sizeof(settings))!=sizeof(settings))
	{
		printf("[controld] error while loading settings: %s - config from old version?\n", CONF_FILE );
		return 0;
	}
	close(fd);
	return 1;
}

void saveSettings()
{
	int fd;
	fd = open(CONF_FILE, O_WRONLY | O_CREAT );
	
	if (fd==-1)
	{
		printf("[controld] error while saving settings: %s\n", CONF_FILE );
		return;
	}
	write(fd, &settings,  sizeof(settings) );
	close(fd);
}


void sendto_lcdd(unsigned char cmd, unsigned char param) {
	int sock_fd;
	SAI servaddr;
	struct lcdd_msg lmsg;
	
	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(LCDD_PORT);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
	{
		lmsg.version=LCDD_VERSION;
		lmsg.cmd=cmd;
		lmsg.param = param;
		write(sock_fd,&lmsg,sizeof(lmsg));
		close(sock_fd);
	}

}

void shutdownBox()
{
    sendto_lcdd(LC_POWEROFF, 0);
	sig_catch(1);
    if (execlp("/sbin/halt", "/sbin/halt", 0)<0)
    {
      perror("exec failed - halt\n");
    }
}


void setVideoRGB()
{
	if(fork()==0)
	{
		if (execlp("/bin/switch", "/bin/switch", "-fnc", "2", "-fblk", "1", 0)<0)
		{
			perror("exec failed - /bin/switch\n");
		}
	}
}



void setVolume(char volume)
{
	int fd;
	settings.gen_Volume = volume;

	int i = 64-int(volume*64.0/100.0);
	printf("[controld] set volume: %d\n", i );
	if (i < 0)
	{
		i=0;
	}
	else if (i > 63)
	{
		i=63;
	}

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSVOL,&i)< 0)
	{
		perror("AVSIOGVOL:");
		return;
	}
	close(fd);

	sendto_lcdd(LC_VOLUME, volume);
}

void Mute()
{
	int i;
	int fd;
	i=AVS_MUTE;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return;
	}
	close(fd);

	sendto_lcdd(LC_MUTE, LC_MUTE_ON);
}

void UnMute()
{
	int i;
	int fd;
	i=AVS_UNMUTE;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return;
	}
	close(fd);

	sendto_lcdd(LC_MUTE, LC_MUTE_OFF);
}


void parse_command(int connfd)
{
  //byteorder!!!!!!
  rmsg.param = ((rmsg.param & 0x00ff) << 8) | ((rmsg.param & 0xff00) >> 8);
  rmsg.param2 = ((rmsg.param2 & 0x00ff) << 8) | ((rmsg.param2 & 0xff00) >> 8);
 
 
  printf("Command received\n");
  printf("  Version: %d\n", rmsg.version);
  printf("  Command: %d\n", rmsg.cmd);
  printf("  Param: %d\n", rmsg.param);
  printf("  Param2: %d\n", rmsg.param2);
  printf("  Param3: %s\n", rmsg.param3);
 
 
  if(rmsg.version!=1)
  {
    perror("unknown version\n");
    return;
  }
 
  switch (rmsg.cmd)
  {
    case 1:
      printf("shutdown\n");
      shutdownBox();
      break;
    case 2:
      printf("set volume\n");
      setVolume(rmsg.param);
      break;
    case 3:
      printf("mute\n");
      Mute();
      break;
    case 4:
      printf("unmute\n");
      UnMute();
      break;
	case 128:
		printf("get volume\n");
		write(connfd,&settings.gen_Volume,sizeof(settings.gen_Volume));
		break;

    default:
		printf("unknown command\n");
  }
 
}


void sig_catch(int)
{
	printf("[controld] shutdown\n");
	saveSettings();
	printf("[controld] data saved\n");
}


int main(int argc, char **argv)
{

	int listenfd, connfd;
	socklen_t clilen;
	SAI cliaddr, servaddr;
 
	printf("Controld  0.1\n\n");
 
	if (fork() != 0) return 0;
 
	//network-setup
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
 
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(1610);
 
	if ( bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) !=0)
	{
		perror("[controld] bind failed...\n");
		exit(-1);
	}
 
 
	if (listen(listenfd, 5) !=0)
	{
		perror("[controld] listen failed...\n");
		exit( -1 );
	}

	signal(SIGHUP,sig_catch);
	signal(SIGKILL,sig_catch);
	signal(SIGTERM,sig_catch);

	if (!loadSettings())
	{
		printf("[controld] useing defaults\n");
		settings.gen_Volume = 100;
	}


	//init 
	setVolume(settings.gen_Volume);
	while(1)
	{
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
 
		memset(&rmsg, 0, sizeof(rmsg));
		read(connfd,&rmsg,sizeof(rmsg));
 
		parse_command(connfd);
 
		close(connfd);
  }
 
}

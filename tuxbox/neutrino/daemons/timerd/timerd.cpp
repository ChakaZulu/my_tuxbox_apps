/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

//#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "timermanager.h"
#include "timerdMsg.h"
#include "debug.h"

static CTimerManager* TimerManager = NULL;

void parse_command(int connfd, CTimerd::commandHead* rmessage)
{

	if(rmessage->version!=CTimerd::ACTVERSION)
	{
		dperror("command with unknown version\n");
		return;
	}

	switch (rmessage->cmd)
	{
		case CTimerd::CMD_ADDTIMER:
			CTimerd::commandAddTimer msgAddTimer;
			read(connfd,&msgAddTimer, sizeof(msgAddTimer));

			CTimerEvent* event;
			switch (msgAddTimer.evType)
			{
				case CTimerdClient::TIMER_SHUTDOWN :
					event = new CTimerEvent_Shutdown(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min,
						msgAddTimer.evID);
				break;
				case CTimerdClient::TIMER_NEXTPROGRAM :
					event = new CTimerEvent_NextProgram(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min,
						msgAddTimer.evID);
					static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = *((CTimerEvent_NextProgram::EventInfo*)msgAddTimer.data);
				break;
				default:
					event = new CTimerEvent(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min,
						msgAddTimer.evID, msgAddTimer.evType);
			}

			TimerManager->addEvent( event);
			break;
		case CTimerd::CMD_REMOVETIMER:
			break;
		default:
		dprintf("unknown command\n");
	}
}

void sig_catch(int)
{
	//dummy
}

int main(int argc, char **argv)
{
	int listenfd, connfd;
	dprintf("startup\n\n");

	/*
	if (fork() != 0)
	{
		return 0;
	}
	*/

	struct sockaddr_un servaddr;
	int clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, TIMERD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(TIMERD_UDS_NAME);

	//network-setup
	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		dperror("error while socket create");
	}

	if ( bind(listenfd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		dperror("bind failed...");
		exit(-1);
	}

	if (listen(listenfd, 5) !=0)
	{
		perror("listen failed...");
		exit( -1 );
	}

	//busyBox
	/*
	signal(SIGHUP,sig_catch);
	signal(SIGINT,sig_catch);
	signal(SIGQUIT,sig_catch);
	signal(SIGTERM,sig_catch);
*/

	//startup Timer
	TimerManager = CTimerManager::getInstance();
	try
	{
		struct CTimerd::commandHead rmessage;
		while(1)
		{
			connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
			memset(&rmessage, 0, sizeof(rmessage));
			read(connfd,&rmessage,sizeof(rmessage));

			parse_command(connfd, &rmessage);
			close(connfd);
		}
	}
	catch (std::exception& e)
	{
		dprintf("caught std-exception in main-thread %s!\n", e.what());
	}
	catch (...)
	{
		dprintf("caught exception in main-thread!\n");
	}
}

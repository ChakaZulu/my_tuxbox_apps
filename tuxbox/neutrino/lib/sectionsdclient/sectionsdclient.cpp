/*
  Client-Interface f�r zapit  -   DBoxII-Project

  $Id: sectionsdclient.cpp,v 1.3 2002/03/12 16:12:55 field Exp $

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

  $Log: sectionsdclient.cpp,v $
  Revision 1.3  2002/03/12 16:12:55  field
  Bugfixes

  Revision 1.2  2002/03/07 18:33:43  field
  ClientLib angegangen, Events angefangen

  Revision 1.1  2002/01/07 21:28:22  McClean
  initial

  Revision 1.1  2002/01/06 19:10:06  Simplex
  made clientlib for zapit
  implemented bouquet-editor functions in lib


*/

#include "sectionsdclient.h"

CSectionsdClient::CSectionsdClient()
{
	sock_fd = 0;
}

bool CSectionsdClient::sectionsd_connect()
{
	sockaddr_in servaddr;
	char rip[]="127.0.0.1";
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;

	inet_pton(AF_INET, rip, &servaddr.sin_addr);
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	servaddr.sin_port=htons(sectionsd::portNumber);
	if(connect(sock_fd, (sockaddr *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[sectionsdclient] couldn't connect to sectionsd!");
		return false;
	}
	return true;
}

int CSectionsdClient::readResponse(char* data, int size)
{
	struct sectionsd::msgResponseHeader responseHeader;
    receive((char*)&responseHeader, sizeof(responseHeader));

	if ( data != NULL )
	{
		if ( responseHeader.dataLength != size )
			return -1;
		else
			return receive(data, size);
	}
	else
		return responseHeader.dataLength;
}


bool CSectionsdClient::sectionsd_close()
{
	if(sock_fd!=0)
	{
		close(sock_fd);
		sock_fd=0;
	}
}

bool CSectionsdClient::send(char* data, int size)
{
	if(sock_fd)
	{
		write(sock_fd, data, size);
	}
}

bool CSectionsdClient::receive(char* data, int size)
{
	if(sock_fd)
	{
		read(sock_fd, data, size);
	}
}

void CSectionsdClient::registerEvent(unsigned int eventID, unsigned int clientID, string udsName)
{
	sectionsd::msgRequestHeader msg;
	CEventServer::commandRegisterEvent msg2;

	msg.version = 3;
	msg.command = sectionsd::CMD_registerEvents;
	msg.dataLength = sizeof( msg2 );

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	strcpy(msg2.udsName, udsName.c_str());
	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	sectionsd_close();
}

void CSectionsdClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	sectionsd::msgRequestHeader msg;
	CEventServer::commandUnRegisterEvent msg2;

	msg.version = 3;
	msg.command = sectionsd::CMD_unregisterEvents;
	msg.dataLength = sizeof( msg2 );

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	sectionsd_close();
}

bool CSectionsdClient::getIsTimeSet()
{
	sectionsd::msgRequestHeader msg;
	sectionsd::responseIsTimeSet rmsg;

	msg.version = 2;
	msg.command = sectionsd::getIsTimeSet;
	msg.dataLength = 0;

	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	readResponse((char*)&rmsg, sizeof(rmsg));
	sectionsd_close();
	return rmsg.IsTimeSet;
}

void CSectionsdClient::setPauseScanning( bool doPause )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::pauseScanning;
	int PauseIt = ( doPause ) ? 1 : 0;
	msg.dataLength = sizeof( PauseIt );

	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&PauseIt, msg.dataLength);
	readResponse();
	sectionsd_close();
}

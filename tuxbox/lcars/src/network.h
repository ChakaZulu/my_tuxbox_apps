/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: network.h,v $
Revision 1.6  2002/05/21 04:37:42  TheDOC
http-update... new web-frontend in http://dbox/file/start.htm... will be main index soon

Revision 1.5  2002/05/20 20:08:12  TheDOC
some new timer and epg-stuff

Revision 1.4  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.3  2001/12/17 03:52:42  tux
Netzwerkfernbedienung fertig

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef NETWORK_H
#define NETWORK_H

#include <termios.h>
#include <pthread.h>

#include "container.h"
#include "xmlrpc.h"
#include "rc.h"
#include "control.h"
#include "variables.h"

#define PORT 80

class network
{
	pthread_t thread;
	
public:
	control *control_obj;
	variables *vars;
	std::string replace_vars(std::string tmp_string);
	std::string getfile(std::string name);
	bool update_enabled;
	xmlrpc xmlrpc_obj;
	container cont;
	rc *rc_obj;
	void writetext(std::string text);
	network(container &container, rc *r, control *c, variables *v);
	int fd;
	int inbound_connection;
	static void *startlistening(void *object);
	void startThread();
};

#endif

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

#ifndef TOT_H
#define TOT_H

#include "settings.h"

class tot
{
	pthread_t timeThread;
 
    static void* start_timereader( void * );
    
public:	
	tot(settings *s);
	settings *setting;
	int start_thread();
};

#endif


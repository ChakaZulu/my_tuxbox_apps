/*
 * $Id: movieplayer.h,v 1.18 2005/11/10 21:54:50 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#ifndef __lib_movieplayer_h
#define __lib_movieplayer_h

#include <lib/base/estring.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>
#include <lib/movieplayer/mpconfig.h>

struct player_value
{
	int STAT, BUFFERTIME, JUMPMIN;
	bool ACTIVE, ACT_AC3, BUFFERFILLED, AVPIDS_FOUND;
	unsigned short PIDA, PIDV;
	short AC3;
};

class eMoviePlayer: public eMainloop, private eThread, public Object
{
	struct Message
	{
		int type;
		const char *filename;
		enum
		{
			start,
			start2,
			stop,
			play,
			pause,
			forward,
			rewind,
			jump,
			terminate,
			quit
		};
		Message(int type = 0, const char *filename = 0)
			:type(type), filename(filename)
		{}
	};
	eFixedMessagePump<Message> messages;
	static eMoviePlayer *instance;
	eServiceReference suspendedServiceReference;
	void gotMessage(const Message &message);
	void thread();
	int requestStream();
	int playStream(eString mrl);
	void setErrorStatus();
public:
	eMoviePlayer();
	~eMoviePlayer();
	player_value status;
	struct serverConfig server;
	eMPConfig mpconfig;
	int sendRequest2VLC(eString command);
	void control(const char *command, const char *filename);
	void leaveStreamingClient();
	player_value getStatus() { return status; }
	eString sout(eString mrl);
	static eMoviePlayer *getInstance() { return instance; }
	enum {STOPPED, STREAMERROR, PLAY, PAUSE, FORWARD, REWIND, RESYNC, SKIP};
};

#endif

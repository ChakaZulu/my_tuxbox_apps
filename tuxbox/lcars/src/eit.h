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

#ifndef EIT_H
#define EIT_H

#include "osd.h"
#include "settings.h"

#include <time.h>
#include <string>
#include <map>
#include <vector>

struct linkage
{
	int TS;
	int ONID;
	int SID;
	char name[100];
	int VPID;
	int APIDcount;
	int APID[10];
};

struct event
{
	int eventid;
	int running_status;
	char event_name[300];
	char event_short_text[500];
	char event_extended_text[5000];
	int number_components;
	int component_tag[10];
	int stream_content[10];
	int component_type[10];
	char audio_description[10][20];
	time_t starttime;
	int duration;
	int TS;
	int ONID;
	int SID;
	int section;
	int par_rating;
	int number_perspectives;
	linkage linkage_descr[10];
};

struct eit_header
{
	unsigned char table_id:8;
	unsigned char section_syntax_indicator :1;
	unsigned char reserved_future_use:1;
	unsigned char reserved:2;
	unsigned short section_length:12;
	unsigned short service_id:16;
	unsigned char reserved2:2;
	unsigned char version_number:5;
	unsigned char current_next_indicator:1;
	unsigned char section_number:8;
	unsigned char last_section_number:8;
	unsigned short transport_stream_id:16;
	unsigned short original_network_id:16;
	unsigned char segment_last_section_number:8;
	unsigned char last_table_id:8;
}__attribute__ ((packed));

struct event_header
{
	unsigned short event_id:16;
	unsigned short start_time_mjd:16;
	unsigned int start_time_time:24;
	unsigned int duration:24;
	unsigned char running_status:3;
	unsigned char free_CA_mode:1;
	unsigned short descriptors_loop_length:12;
}__attribute__ ((packed));

struct sid
{
	unsigned short TS;
	unsigned short ONID;
	unsigned short SID;
};

struct ltstr
{
	bool operator()(struct sid sid1, struct sid sid2) const
	{
		if (sid1.ONID < sid2.ONID)
			return true;
		else if (sid1.ONID == sid1.ONID)
		{
			if (sid1.TS < sid2.TS)
				return true;
			else if (sid1.TS == sid2.TS)
			{
				if (sid1.SID < sid2.SID)
					return true;
				else
					return false;
			}
			else
				return false;
		}
		else
			return false;

	}
};


class eit
{
	event now;
	event next;
	linkage linkage_descr[10];
	int number_perspectives, curr_linkage;
	std::multimap<int, struct event> eventlist;
	settings *setting;
	osd *osd_obj;

	std::queue<std::string> command_queue;
	
	int lastSID;
	int audio_comp;

	pthread_t eitThread;
	pthread_mutex_t mutex;
 
    static void* start_eitqueue( void * );
	std::multimap<int, struct event> eventid_event;
	std::multimap<time_t, int> time_eventid;
	std::multimap<struct sid, std::multimap<time_t, int>, ltstr> sid_eventid;

public:	
	eit(settings *s, osd *o);
	event getNow() { return now; }
	event getNext() { return next; }
	void receiveNow(int SID);

	int start_thread();
	void addCommand(std::string command);
	void executeCommand();
	void executeQueue();
	bool isEmpty() { return command_queue.empty(); }

	void setAudioComponent(int comp);
	bool isMultiPerspective();
	void beginLinkage();
	int numberPerspectives() { return now.number_perspectives; }
	linkage nextLinkage();
	void readSchedule(int SID, osd *osd);
	void dumpSchedule(int TS, int ONID, int SID, osd *osd);
	event getEvent(int eventid);

	void eit::dumpSchedule(int SID, osd *osd);
};

#endif

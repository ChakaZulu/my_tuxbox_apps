#ifndef SECTIONSDMSG_H
#define SECTIONSDMSG_H
//
//  $Id: sectionsdMsg.h,v 1.2 2002/11/03 22:26:55 thegoodguy Exp $
//
//	sectionsdMsg.h (header file with msg-definitions for sectionsd)
//	(dbox-II-project)
//
//	Copyright (C) 2001 by fnbrd,
//                    2002 by thegoodguy
//
//    Homepage: http://dbox2.elxsi.de
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include <string>
#include <vector>


#include <zapit/client/zapittypes.h>  // t_channel_id, t_service_id, t_original_network_id, t_transport_stream_id;


using namespace std;

#define SECTIONSD_UDS_NAME "/tmp/sectionsd.sock"

struct sectionsd
{
	struct epgflags {
		enum
		{
			has_anything = 0x01,
			has_later = 0x02,
			has_current = 0x04,
			not_broadcast = 0x08,
			has_next = 0x10,
			has_no_current= 0x20,
			current_has_linkagedescriptors= 0x40
		};
	};

	struct msgRequestHeader
	{
		char version;
		char command;
		unsigned short dataLength;
	} __attribute__ ((packed)) ;

	struct msgResponseHeader
	{
		unsigned short dataLength;
	} __attribute__ ((packed)) ;

	struct sectionsdTime
	{
		time_t startzeit;
		unsigned dauer;
	} __attribute__ ((packed)) ;

	static const int numberOfCommands=26;
	enum commands
	{
		actualEPGchannelName=0,
		actualEventListTVshort,
		currentNextInformation,
		dumpStatusinformation,
		allEventsChannelName,
		setHoursToCache,
		setEventsAreOldInMinutes,
		dumpAllServices,
		actualEventListRadioshort,
		getNextEPG,
		getNextShort,
		pauseScanning, // for the grabbers ;)
		actualEPGchannelID,
		actualEventListTVshortIDs,
		actualEventListRadioShortIDs,
		currentNextInformationID,
		epgEPGid,
		epgEPGidShort,
		ComponentTagsUniqueKey,
		allEventsChannelID_,
		timesNVODservice,
		getEPGPrevNext,
		getIsTimeSet,
		serviceChanged,
		LinkageDescriptorsUniqueKey,
		pauseSorting
	};

	static const int numberOfCommands_v3=2;
	enum commands_3
	{
		CMD_registerEvents,
		CMD_unregisterEvents
	};

	struct commandSetServiceChanged
	{
		t_channel_id channel_id;
		bool         requestEvent;
	};

	struct responseIsTimeSet
	{
		bool IsTimeSet;
	};


	struct responseGetComponentTags
	{
		std::string component; 			// Text aus dem Component Descriptor
    	unsigned char componentType; 	// Component Descriptor
		unsigned char componentTag; 	// Component Descriptor
		unsigned char streamContent; 	// Component Descriptor
	};

    typedef std::vector<responseGetComponentTags> ComponentTagList;

	struct responseGetLinkageDescriptors
	{
		std::string name;
		t_transport_stream_id transportStreamId;
		t_original_network_id originalNetworkId;
		t_service_id          serviceId;
	};

    typedef std::vector<responseGetLinkageDescriptors> LinkageDescriptorList;

	struct responseGetNVODTimes
	{
		t_service_id          service_id;
		t_original_network_id original_network_id;
		t_transport_stream_id transport_stream_id;
		sectionsd::sectionsdTime zeit;
	};

    typedef std::vector<responseGetNVODTimes> NVODTimesList;

    struct responseGetCurrentNextInfoChannelID
	{
		unsigned long long 			current_uniqueKey;
		sectionsd::sectionsdTime 	current_zeit;
		std::string					current_name;
		char						current_fsk;
		unsigned long long 			next_uniqueKey;
		sectionsd::sectionsdTime 	next_zeit;
		std::string					next_name;
		unsigned					flags;
	};

	struct CurrentNextInfo : public responseGetCurrentNextInfoChannelID
	{};
};

//
// Description of Commands:
//
// If a command is recognize then sectionsd will always send a response.
// When requested datas are not found the data length of the response is 0.
//
// actualEPGchannelName:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the EPG (for
//     compatibility with old epgd)
//
// actualEventListTVshort:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached actual events,
//     3 lines per service, first line unique-event-key, second line service name, third line event name
//
// currentNextInformation:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the current/next EPGs
//
// dumpStatusinformation:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) describing current status of sectionsd
//
// allEventsChannelName:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the cached events for the requestet channel
//     1 line per event, format: uniqueEventKey DD.MM HH:MM durationInMinutes Event name
//
// setHoursToCache
//   data of request:
//     unsigned short (hours to cache)
//   data of response:
//     -
//
// setEventsAreOldInMinutes
//   data of request:
//     unsigned short (minutes after events are old (after their end time))
//   data of response:
//     -
//
// dumpAllServicesinformation:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached services
//     3 lines per service
//     1. line: unique-service-key, service-ID, service-type, eitScheduleFlag (bool),
//              eitPresentFollowingFlag (bool), runningStatus (bool),
//              freeCAmode (bool), number of nvod services
//     2. line: service name
//     3. line: provider name
//
// actualEventListRadioshort:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached actual events,
//     3 lines per service, first line unique-event-key, second line service name, third line event name
//
// getNextEPG:
//   data of request:
//     8 bytes (long long in 32 bit world) with unique key (binary) of the event for wich the next should be delivered
//     4 bytes with start time (ctime) of the above event
//   data of response:
//     is a string (c-string) describing the EPG:
//     unique key (long long, hex) 0xff name  0xff text  0xff extended text  0xff start time GMT (ctime, hex ) 0xff duration (seconds, hex)
//
// getNextShort:
//   data of request:
//     8 bytes (long long in 32 bit world) with unique key (binary) of the event for wich the next should be delivered
//     4 bytes with start time (ctime) of the above event
//   data of response:
//     is a string (c-string) describing the Event in short terms:
//     1. line unique key (long long, hex), 2. line name, 3. line start time GMT (ctime, hex ), 4 line  duration (seconds, hex)
//
// pauseScanning:
//   data of request:
//     int (1 = pause, 0 = continue)
//   data of response:
//     -
//
// actualEPGchannelID:
//   data of request:
//     is channel ID
//   data of response:
//     is a string (c-string) describing the EPG:
//     unique key (long long, hex) 0xff name  0xff text  0xff extended text  0xff start time GMT (ctime, hex ) 0xff duration (seconds, hex) 0xff
//
// actualEventListTVshortIDs:
//   data of request:
//     -
//   data of response:
//     for every service:
//       1. unique-service-key (4 bytes)
//       2. unique-event-key (8 bytes)
//       3. event name (c-string with 0)
//
// actualEventListRadioShortIDs:
//   data of request:
//     -
//   data of response:
//       1. unique-service-key (4 bytes)
//       2. unique-event-key (8 bytes)
//       3. event name (c-string with 0)
//
// currentNextInformationID:
//   data of request:
//     4 byte channel ID (4 byte, onid<<16+sid)
//     1 byte number of Events (noch nicht implementiert)
//   data of response:
//     is a string (c-string) describing the current/next EPGs
//     every event:
//       1. 8 bytes unique key (unsigned long long),
//       2. struct sectionsdTime (8 bytes)
//       3. name (c-string with 0)
//
// epgEPGid:
//   data of request:
//     unique epg ID (8 byte)
//     time_t starttime GMT (4 bytes)
//   data of response:
//     is a string (c-string) describing the EPG:
//     name 0xff text 0xff extended text 0xff start time GMT (ctime, hex ) 0xff duration (seconds, hex) 0xff
//
// epgEPGidShort:
//   data of request:
//     unique epg ID (8 byte)
//   data of response:
//     is a string (c-string) describing the EPG:
//     name  0xff text  0xff extended text 0xff
//
// CurrentComponentTags
//   - gets all ComponentDescriptor-Tags for the currently running Event
//
//   data of request:
//     is channel ID (4 byte, onid<<16+sid)
//   data of response:
//      for each component-descriptor (zB %02hhx %02hhx %02hhx\n%s\n)
//          componentTag
//          componentType
//          streamContent
//          component.c_str
//
// allEventsChannelID:
//   data of request:
//     is channel ID
//   data of response:
//     is a string (c-string) describing the cached events for the requestet channel
//     1 line per event, format: uniqueEventKey DD.MM HH:MM durationInMinutes Event name
//
// timesNVODservice
//   data of request:
//     is channel ID
//   data of response:
//     for every (sub-)channel
//       channelID (4 byte, onid<<16+sid)
//       transportStreamID (2 byte)
//       start time (4 bytes ctime)
//       duration (4 bytes unsigned)
//
//  getEPGPrevNext
//   data of request:
//     8 bytes (long long in 32 bit world) with unique key (binary) of the event for wich the next should be delivered
//     4 bytes with start time (ctime) of the above event
//   data of response:
//     is a string (c-string) describing the EPG:
//     unique key (long long, hex) 0xff start time GMT (ctime, hex ) for previous event
//     unique key (long long, hex) 0xff start time GMT (ctime, hex ) for next event

//
#endif // SECTIONSDMSG_H

/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerd.cpp,v 1.39 2002/12/03 11:15:11 thegoodguy Exp $

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
#include <string.h>
#include <sstream>

#include <unistd.h>

#include <configfile.h>
#include <timermanager.h>
#include <debug.h>
#include <sectionsdclient/sectionsdMsg.h>
#include <sectionsdclient/sectionsdclient.h>

#include <connection/basicserver.h>
#include <timerdclient/timerdmsg.h>

bool parse_command(CBasicMessage::Header &rmsg, int connfd)
{
//	CTimerEvent_NextProgram::EventMap::iterator it = NULL;
	CTimerEventMap events;
	CTimerdMsg::commandModifyTimer msgModifyTimer;
	CTimerdMsg::responseGetSleeptimer rspGetSleeptimer;
	CTimerEventMap::iterator pos;
	switch (rmsg.cmd)
	{
		
		case CTimerdMsg::CMD_REGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->registerEvent( connfd );
			break;

		case CTimerdMsg::CMD_UNREGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->unRegisterEvent( connfd );
			break;

		case CTimerdMsg::CMD_GETSLEEPTIMER:
			rspGetSleeptimer.eventID = 0;
			if(CTimerManager::getInstance()->listEvents(events))
			{
				if(events.size() > 0)
				{
					for(pos = events.begin();(pos != events.end());pos++)
					{
						printf("ID: %u type: %u\n",pos->second->eventID,pos->second->eventType);
						if(pos->second->eventType == CTimerd::TIMER_SLEEPTIMER)
						{
							rspGetSleeptimer.eventID = pos->second->eventID;
							break;
						}
					}
				}
			}
			write( connfd, &rspGetSleeptimer, sizeof(rspGetSleeptimer));
			break;

		case CTimerdMsg::CMD_GETTIMER:						// timer daten abfragen
			CTimerdMsg::commandGetTimer msgGetTimer;
			CTimerd::responseGetTimer resp;
			read(connfd,&msgGetTimer, sizeof(msgGetTimer));
			if(CTimerManager::getInstance()->listEvents(events))
			{
				if(events[msgGetTimer.eventID])
				{
					CTimerEvent *event = events[msgGetTimer.eventID];
					resp.eventID = event->eventID;
					resp.eventState = event->eventState;
					resp.eventType = event->eventType;
					resp.eventRepeat = event->eventRepeat;
					resp.announceTime = event->announceTime;
					resp.alarmTime = event->alarmTime;
					resp.stopTime = event->stopTime;

					if(event->eventType == CTimerd::TIMER_STANDBY)
						resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
					else if(event->eventID == CTimerd::TIMER_NEXTPROGRAM)
					{
						resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
						resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
						resp.apid = static_cast<CTimerEvent_Record*>(event)->eventInfo.apid;
						resp.mode = static_cast<CTimerEvent_Record*>(event)->eventInfo.mode;
					}
					else if(event->eventID == CTimerd::TIMER_RECORD)
					{
						resp.epgID = static_cast<CTimerEvent_Record*>(event)->eventInfo.epgID;
						resp.channel_id = static_cast<CTimerEvent_Record*>(event)->eventInfo.channel_id;
						resp.apid = static_cast<CTimerEvent_Record*>(event)->eventInfo.apid;
						resp.mode = static_cast<CTimerEvent_Record*>(event)->eventInfo.mode;
					}
					else if(event->eventID == CTimerd::TIMER_ZAPTO)
					{
						resp.epgID = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.epgID;
						resp.channel_id = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.channel_id;
						resp.mode = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.mode;
						resp.apid = static_cast<CTimerEvent_Record*>(event)->eventInfo.apid;
					}
					else if(event->eventID == CTimerd::TIMER_REMIND)
					{
						memset(resp.message, 0, sizeof(resp.message));
						strncpy(resp.message, static_cast<CTimerEvent_Remind*>(event)->message, sizeof(resp.message)-1);
					}
				}
			}
			write( connfd, &resp, sizeof(CTimerd::responseGetTimer));
			break;

		case CTimerdMsg::CMD_GETTIMERLIST:				// liste aller timer 
			if(CTimerManager::getInstance()->listEvents(events))
			{
				for(CTimerEventMap::iterator pos = events.begin();pos != events.end();pos++)
				{
					CTimerd::responseGetTimer resp;

					CTimerEvent *event = pos->second;

					resp.eventID = event->eventID;
					resp.eventState = event->eventState;
					resp.eventType = event->eventType;
					resp.eventRepeat = event->eventRepeat;
					resp.announceTime = event->announceTime;
					resp.alarmTime = event->alarmTime;
					resp.stopTime = event->stopTime;
					if(event->eventType == CTimerd::TIMER_STANDBY)
						resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
					else if(event->eventType == CTimerd::TIMER_NEXTPROGRAM)
					{
						resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
						resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
						resp.apid = static_cast<CTimerEvent_Record*>(event)->eventInfo.apid;
						resp.mode = static_cast<CTimerEvent_Record*>(event)->eventInfo.mode;
					}
					else if(event->eventType == CTimerd::TIMER_RECORD)
					{
						resp.epgID = static_cast<CTimerEvent_Record*>(event)->eventInfo.epgID;
						resp.channel_id = static_cast<CTimerEvent_Record*>(event)->eventInfo.channel_id;
						resp.apid = static_cast<CTimerEvent_Record*>(event)->eventInfo.apid;
						resp.mode = static_cast<CTimerEvent_Record*>(event)->eventInfo.mode;
					}
					else if(event->eventType == CTimerd::TIMER_ZAPTO)
					{
						resp.epgID = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.epgID;
						resp.channel_id = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.channel_id;
						resp.mode = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.mode;
						resp.apid = static_cast<CTimerEvent_Record*>(event)->eventInfo.apid;
					}
					else if(event->eventType == CTimerd::TIMER_REMIND)
					{
						strcpy(resp.message, static_cast<CTimerEvent_Remind*>(event)->message);
					}
					write( connfd, &resp, sizeof(CTimerd::responseGetTimer));
				}
			}
			break;

		case CTimerdMsg::CMD_RESCHEDULETIMER:			// event nach vorne oder hinten schieben
			{
				read(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
				int ret=CTimerManager::getInstance()->rescheduleEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime);
				CTimerdMsg::responseStatus rspStatus;
				rspStatus.status = (ret!=0);
				write( connfd, &rspStatus, sizeof(rspStatus));
				break;
			}

		case CTimerdMsg::CMD_MODIFYTIMER:				// neue zeiten setzen
			{
				read(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
				int ret=CTimerManager::getInstance()->modifyEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime,
																				  msgModifyTimer.eventRepeat );
				CTimerdMsg::responseStatus rspStatus;
				rspStatus.status = (ret!=0);
				write( connfd, &rspStatus, sizeof(rspStatus));
				break;
			}

		case CTimerdMsg::CMD_ADDTIMER:						// neuen timer hinzuf�gen
			CTimerdMsg::commandAddTimer msgAddTimer;
			read(connfd,&msgAddTimer, sizeof(msgAddTimer));

			CTimerdMsg::responseAddTimer rspAddTimer;
			CTimerEvent* event;
			CTimerd::EventInfo evInfo;
			switch(msgAddTimer.eventType)
			{
				case CTimerd::TIMER_STANDBY :
					CTimerdMsg::commandSetStandby standby;
					read( connfd, &standby, sizeof(CTimerdMsg::commandSetStandby));

					event = new CTimerEvent_Standby(
															 msgAddTimer.announceTime,
															 msgAddTimer.alarmTime,
															 standby.standby_on,
															 msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					break;

				case CTimerd::TIMER_SHUTDOWN :
					event = new CTimerEvent_Shutdown(
															  msgAddTimer.announceTime,
															  msgAddTimer.alarmTime,
															  msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					break;

				case CTimerd::TIMER_SLEEPTIMER :
					event = new CTimerEvent_Sleeptimer(
																 msgAddTimer.announceTime,
																 msgAddTimer.alarmTime,
																 msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					break;

				case CTimerd::TIMER_RECORD :
					read( connfd, &evInfo, sizeof(CTimerd::EventInfo));
					event = new CTimerEvent_Record(
															msgAddTimer.announceTime,
															msgAddTimer.alarmTime,
															msgAddTimer.stopTime,
															evInfo.channel_id,
															evInfo.epgID,
															evInfo.apid,
															evInfo.mode,
															msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					break;

				case CTimerd::TIMER_ZAPTO :
					read( connfd, &evInfo, sizeof(CTimerd::EventInfo));
					if(evInfo.channel_id > 0)
					{
						event = new CTimerEvent_Zapto(
															  msgAddTimer.announceTime,
															  msgAddTimer.alarmTime,
															  evInfo.channel_id,
															  evInfo.epgID,
															  evInfo.mode,
															  msgAddTimer.eventRepeat);
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					}
					break;

				case CTimerd::TIMER_NEXTPROGRAM :
//					CTimerd::EventInfo evInfo;
					read( connfd, &evInfo, sizeof(CTimerd::EventInfo));
/*
					it = CTimerEvent_NextProgram::events.find( evInfo.uniqueKey);
					if (it == CTimerEvent_NextProgram::events.end())
					{
						event = new CTimerEvent_NextProgram(
							msgAddTimer.announceTime,
							msgAddTimer.alarmTime,
							msgAddTimer.stopTime,
							msgAddTimer.eventRepeat);
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = evInfo;
						CTimerEvent_NextProgram::events.insert(make_pair(static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.uniqueKey, static_cast<CTimerEvent_NextProgram*>(event)));
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					}
					else
					{
						event = it->second;
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = evInfo;
						event->alarmtime.tm_mon  = msgAddTimer.month;
						event->alarmtime.tm_mday = msgAddTimer.day;
						event->alarmtime.tm_hour = msgAddTimer.hour;
						event->alarmtime.tm_min  = msgAddTimer.min;
						rspAddTimer.eventID = event->eventID;
					}
*/
					break;
				case CTimerd::TIMER_REMIND :
					CTimerdMsg::commandRemind remind;
					read( connfd, &remind, sizeof(CTimerdMsg::commandRemind));
					event = new CTimerEvent_Remind(msgAddTimer.announceTime,
															 msgAddTimer.alarmTime,
															 remind.message,
															 msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					break;

				default:
					printf("[timerd] Unknown TimerType\n");
			}

			write( connfd, &rspAddTimer, sizeof(rspAddTimer));

			break;
		case CTimerdMsg::CMD_REMOVETIMER:						//	timer entfernen
			dprintf("TIMERD: command remove\n");
			CTimerdMsg::commandRemoveTimer msgRemoveTimer;
			read(connfd,&msgRemoveTimer, sizeof(msgRemoveTimer));
			dprintf("TIMERD: command remove %d\n",msgRemoveTimer.eventID );
			CTimerManager::getInstance()->removeEvent( msgRemoveTimer.eventID);
			break;

		case CTimerdMsg::CMD_TIMERDAVAILABLE:					// testen ob server l�uft ;)
			{
				CTimerdMsg::responseAvailable rspAvailable;
				rspAvailable.available = true;
				write( connfd, &rspAvailable, sizeof(rspAvailable));
			}
			break;
		case CTimerdMsg::CMD_SHUTDOWN:
			{
				bool ret=CTimerManager::getInstance()->shutdown();
				CTimerdMsg::responseStatus rspStatus;
				rspStatus.status = ret;
				write( connfd, &rspStatus, sizeof(rspStatus));
				return false;
			}
			break;
		case CTimerdMsg::CMD_SETAPID:				  // apid setzen
			{
				CTimerdMsg::commandSetAPid data;
				read(connfd,&data, sizeof(data));
				CTimerManager::getInstance()->modifyEvent(data.eventID , data.apid );
			}
			break;
		default:
			dprintf("unknown command\n");
	}
	return true;
}

int main(int argc, char **argv)
{
	bool do_fork = true;

	dprintf("startup\n");
	if(argc > 1)
	{
		for(int i = 1; i < argc; i++)
		{

			if(strncmp(argv[i], "-f", 2) == 0)
			{
				do_fork = false;
			}
		}
	}

	CBasicServer timerd_server;

	if (!timerd_server.prepare(TIMERD_UDS_NAME))
		return -1;

	if(do_fork)
	{
		switch(fork())
		{
			case -1:
				perror("[timerd] fork");
				return -1;
			case 0:
				break;
			default:
				return 0;
		}
		if(setsid() == -1)
		{
			perror("[timerd] setsid");
			return -1;
		}
	}

	//startup Timer
	try
	{
		timerd_server.run(parse_command, CTimerdMsg::ACTVERSION);
	}
	catch(std::exception& e)
	{
		dprintf("caught std-exception in main-thread %s!\n", e.what());
	}
	catch(...)
	{
		dprintf("caught exception in main-thread!\n");
	}
}

/*
   Timer-Daemon  -   DBoxII-Project

   Copyright (C) 2001 Steffen Hehn 'McClean'
   Homepage: http://dbox.cyberphoria.org/

   $Id: timerd.cpp,v 1.22 2002/09/27 07:34:23 Zwen Exp $

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
#include <sstream>

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

#include "timermanager.h"
#include "timerdMsg.h"
#include "debug.h"
#include "config.h"
#include <configfile.h>

bool doLoop;

void loadTimersFromConfig()
{
   CConfigFile *config = new CConfigFile(',');

   if(!config->loadConfig(CONFIGFILE))
   {
      /* set defaults if no configuration file exists */
      dprintf("%s not found\n", CONFIGFILE);
   }
   else
   {
      vector<int> savedIDs;
      savedIDs = config->getInt32Vector ("IDS");
      printf("[TIMERD]IDS: %d\n",savedIDs.size());
      for(unsigned int i=0; i < savedIDs.size(); i++)
      {
         stringstream ostr;
         ostr << savedIDs[i];
         string id=ostr.str();
         CTimerEvent::CTimerEventTypes type=(CTimerEvent::CTimerEventTypes)config->getInt32 ("EVENT_TYPE_"+id,0);
         time_t now = time(NULL);
         switch(type)
         {
            case CTimerEvent::TIMER_SHUTDOWN :
               {
                  CTimerEvent_Shutdown *event=
                  new CTimerEvent_Shutdown(config, savedIDs[i]);
                  if((event->alarmTime >= now) || (event->stopTime > now))
                  {
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else if (event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
                  {
                     // old periodic timers need to be rescheduled
                     event->eventState = CTimerEvent::TIMERSTATE_HASFINISHED;
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else
                  {
                     dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
                     delete event;
                  }
                  break;
               }       
            case CTimerEvent::TIMER_NEXTPROGRAM :
               {
                  CTimerEvent_NextProgram *event=
                  new CTimerEvent_NextProgram(config, savedIDs[i]);
                  if((event->alarmTime >= now) || (event->stopTime > now))
                  {
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else if (event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
                  {
                     // old periodic timers need to be rescheduled
                     event->eventState = CTimerEvent::TIMERSTATE_HASFINISHED;
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else
                  {
                     dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
                     delete event;
                  }
                  break;
               }       
            case CTimerEvent::TIMER_ZAPTO :
               {
                  CTimerEvent_Zapto *event=
                  new CTimerEvent_Zapto(config, savedIDs[i]);
                  if((event->alarmTime >= now) || (event->stopTime > now))
                  {
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else if (event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
                  {
                     // old periodic timers need to be rescheduled
                     event->eventState = CTimerEvent::TIMERSTATE_HASFINISHED;
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else
                  {
                     dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
                     delete event;
                  }
                  break;
               }          
            case CTimerEvent::TIMER_STANDBY :
               {
                  CTimerEvent_Standby *event=
                  new CTimerEvent_Standby(config, savedIDs[i]);
                  if((event->alarmTime >= now) || (event->stopTime > now))
                  {
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else if (event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
                  {
                     // old periodic timers need to be rescheduled
                     event->eventState = CTimerEvent::TIMERSTATE_HASFINISHED;
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else
                  {
                     dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
                     delete event;
                  }
                  break;
               }           
            case CTimerEvent::TIMER_RECORD :
               {
                  CTimerEvent_Record *event=
                  new CTimerEvent_Record(config, savedIDs[i]);
                  if((event->alarmTime >= now) || (event->stopTime > now))
                  {
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else if (event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
                  {
                     // old periodic timers need to be rescheduled
                     event->eventState = CTimerEvent::TIMERSTATE_HASFINISHED;
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else
                  {
                     dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
                     delete event;
                  }
                  break;
               }          
            case CTimerEvent::TIMER_SLEEPTIMER :
               {
                  CTimerEvent_Sleeptimer *event=
                  new CTimerEvent_Sleeptimer(config, savedIDs[i]);
                  if((event->alarmTime >= now) || (event->stopTime > now))
                  {
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else if (event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
                  {
                     // old periodic timers need to be rescheduled
                     event->eventState = CTimerEvent::TIMERSTATE_HASFINISHED;
                     CTimerManager::getInstance()->addEvent(event,false);
                  }
                  else
                  {
                     dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
                     delete event;
                  }
                  break;
               }
            default:
               dprintf("Unknown timer on load %d\n",type);
         }
      }
   }
   delete config;
/*	char cmd[80];
	sprintf(cmd,"cp /var/tuxbox/config/timerd.conf /var/tuxbox/config/timerd.conf.%d",
			  (int) time(NULL));
	system(cmd);*/
   CTimerManager::getInstance()->saveEventsToConfig();
}

void parse_command(int connfd, CTimerd::commandHead* rmessage)
{

   if(rmessage->version!=CTimerd::ACTVERSION)
   {
      dperror("command with unknown version\n");
      return;
   }

//	CTimerEvent_NextProgram::EventMap::iterator it = NULL;
   CTimerEventMap events;
   CTimerd::commandModifyTimer msgModifyTimer;
   CTimerd::responseGetSleeptimer rspGetSleeptimer;
   CTimerEventMap::iterator pos;
   switch(rmessage->cmd)
   {
      
      case CTimerd::CMD_REGISTEREVENT :
         CTimerManager::getInstance()->getEventServer()->registerEvent( connfd );
         break;

      case CTimerd::CMD_UNREGISTEREVENT :
         CTimerManager::getInstance()->getEventServer()->unRegisterEvent( connfd );
         break;

      case CTimerd::CMD_GETSLEEPTIMER:
         rspGetSleeptimer.eventID = 0;
         if(CTimerManager::getInstance()->listEvents(events))
         {
            if(events.size() > 0)
            {
               for(pos = events.begin();(pos != events.end());pos++)
					{
                  printf("ID: %u type: %u\n",pos->second->eventID,pos->second->eventType);
						if(pos->second->eventType == CTimerEvent::TIMER_SLEEPTIMER)
						{
							rspGetSleeptimer.eventID = pos->second->eventID;
							break;
						}
					}
            }
         }
         write( connfd, &rspGetSleeptimer, sizeof(rspGetSleeptimer));
         break;

      case CTimerd::CMD_GETTIMER:                  // timer daten abfragen
         CTimerd::commandGetTimer msgGetTimer;
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

               if(event->eventType == CTimerEvent::TIMER_STANDBY)
                  resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
               else if(event->eventID == CTimerEvent::TIMER_NEXTPROGRAM)
               {
                  resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
                  resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
               }
               else if(event->eventID == CTimerEvent::TIMER_RECORD)
               {
                  resp.epgID = static_cast<CTimerEvent_Record*>(event)->eventInfo.epgID;
                  resp.channel_id = static_cast<CTimerEvent_Record*>(event)->eventInfo.channel_id;
               }
               else if(event->eventID == CTimerEvent::TIMER_ZAPTO)
               {
                  resp.epgID = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.epgID;
                  resp.channel_id = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.channel_id;
               }
            }
         }
         write( connfd, &resp, sizeof(CTimerd::responseGetTimer));
         break;

      case CTimerd::CMD_GETTIMERLIST:           // liste aller timer 
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
               if(event->eventType == CTimerEvent::TIMER_STANDBY)
                  resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
               else if(event->eventType == CTimerEvent::TIMER_NEXTPROGRAM)
               {
                  resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
                  resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
               }
               else if(event->eventType == CTimerEvent::TIMER_RECORD)
               {
                  resp.epgID = static_cast<CTimerEvent_Record*>(event)->eventInfo.epgID;
                  resp.channel_id = static_cast<CTimerEvent_Record*>(event)->eventInfo.channel_id;
               }
               else if(event->eventType == CTimerEvent::TIMER_ZAPTO)
               {
                  resp.epgID = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.epgID;
                  resp.channel_id = static_cast<CTimerEvent_Zapto*>(event)->eventInfo.channel_id;
               }
               write( connfd, &resp, sizeof(CTimerd::responseGetTimer));
            }
         }
         break;

      case CTimerd::CMD_RESCHEDULETIMER:        // event nach vorne oder hinten schieben
		{
         read(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
         int ret=CTimerManager::getInstance()->rescheduleEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime);
			CTimerd::responseStatus rspStatus;
         rspStatus.status = (ret!=0);
         write( connfd, &rspStatus, sizeof(rspStatus));
         break;
		}

      case CTimerd::CMD_MODIFYTIMER:            // neue zeiten setzen
		{
         read(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
		   int ret=CTimerManager::getInstance()->modifyEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime,
																			  msgModifyTimer.eventRepeat );
			CTimerd::responseStatus rspStatus;
         rspStatus.status = (ret!=0);
         write( connfd, &rspStatus, sizeof(rspStatus));
         break;
		}

      case CTimerd::CMD_ADDTIMER:                  // neuen timer hinzuf�gen
         CTimerd::commandAddTimer msgAddTimer;
         read(connfd,&msgAddTimer, sizeof(msgAddTimer));

         CTimerd::responseAddTimer rspAddTimer;
         CTimerEvent* event;
         CTimerEvent::EventInfo evInfo;
         switch(msgAddTimer.eventType)
         {
            case CTimerEvent::TIMER_STANDBY :
               CTimerd::commandSetStandby standby;
               read( connfd, &standby, sizeof(CTimerd::commandSetStandby));

               event = new CTimerEvent_Standby(
                                              msgAddTimer.announceTime,
                                              msgAddTimer.alarmTime,
                                              msgAddTimer.stopTime,
                                              msgAddTimer.eventRepeat);

               static_cast<CTimerEvent_Standby*>(event)->standby_on = standby.standby_on;
               rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
               break;

            case CTimerEvent::TIMER_SHUTDOWN :
               event = new CTimerEvent_Shutdown(
                                               msgAddTimer.announceTime,
                                               msgAddTimer.alarmTime,
                                               msgAddTimer.stopTime,
                                               msgAddTimer.eventRepeat);
               rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
               break;

            case CTimerEvent::TIMER_SLEEPTIMER :
               event = new CTimerEvent_Sleeptimer(
                                                 msgAddTimer.announceTime,
                                                 msgAddTimer.alarmTime,
                                                 msgAddTimer.stopTime,
                                                 msgAddTimer.eventRepeat);
               rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
               break;

            case CTimerEvent::TIMER_RECORD :
               read( connfd, &evInfo, sizeof(CTimerEvent::EventInfo));
               event = new CTimerEvent_Record(
                                             msgAddTimer.announceTime,
                                             msgAddTimer.alarmTime,
                                             msgAddTimer.stopTime,
                                             msgAddTimer.eventRepeat);
               static_cast<CTimerEvent_Record*>(event)->eventInfo.channel_id = evInfo.channel_id;
               static_cast<CTimerEvent_Record*>(event)->eventInfo.epgID = evInfo.epgID;
               rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
               break;

            case CTimerEvent::TIMER_ZAPTO :
               read( connfd, &evInfo, sizeof(CTimerEvent::EventInfo));
               if(evInfo.channel_id > 0)
               {
                  event = new CTimerEvent_Zapto(
                                               msgAddTimer.announceTime,
                                               msgAddTimer.alarmTime,
                                               msgAddTimer.stopTime,
                                               msgAddTimer.eventRepeat);
                  static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id = evInfo.channel_id;
                  static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID = evInfo.epgID;
                  rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
               }
               break;

            case CTimerEvent::TIMER_NEXTPROGRAM :
//					CTimerd::EventInfo evInfo;
               read( connfd, &evInfo, sizeof(CTimerEvent::EventInfo));
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
            default:
               printf("[timerd] Unknown TimerType\n");
         }

         write( connfd, &rspAddTimer, sizeof(rspAddTimer));

         break;
      case CTimerd::CMD_REMOVETIMER:                  //	timer entfernen
         dprintf("TIMERD: command remove\n");
         CTimerd::commandRemoveTimer msgRemoveTimer;
         read(connfd,&msgRemoveTimer, sizeof(msgRemoveTimer));
         dprintf("TIMERD: command remove %d\n",msgRemoveTimer.eventID );
         CTimerManager::getInstance()->removeEvent( msgRemoveTimer.eventID);
         break;

      case CTimerd::CMD_TIMERDAVAILABLE:              // testen ob server l�uft ;)
         CTimerd::responseAvailable rspAvailable;
         rspAvailable.available = true;
         write( connfd, &rspAvailable, sizeof(rspAvailable));
         break;
		case CTimerd::CMD_SHUTDOWN:
		{
			bool ret=CTimerManager::getInstance()->shutdown();
			CTimerd::responseStatus rspStatus;
			rspStatus.status = ret;
			write( connfd, &rspStatus, sizeof(rspStatus));
			doLoop=false;
		}
		break;
      default:
         dprintf("unknown command\n");
   }
}

int main(int argc, char **argv)
{
   int listenfd, connfd;
   struct sockaddr_un servaddr;
   int clilen;
   bool do_fork = true;
	bool no_wait = false;
	doLoop=true;

   dprintf("startup!!!\n\n");
   if(argc > 1)
   {
      for(int i = 1; i < argc; i++)
      {

         if(strncmp(argv[i], "-f", 2) == 0)
         {
            do_fork = false;
         }
         else if(strncmp(argv[i], "-w", 2) == 0)
         {
				no_wait=true;
			}
      }
   }

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

   memset(&servaddr, 0, sizeof(struct sockaddr_un));
   servaddr.sun_family = AF_UNIX;
   strcpy(servaddr.sun_path, TIMERD_UDS_NAME);
   clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
   unlink(TIMERD_UDS_NAME);

   //network-setup
   if((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
   {
      dperror("error while socket create");
   }

   if( bind(listenfd, (struct sockaddr*) &servaddr, clilen) <0 )
   {
      dperror("bind failed...");
      exit(-1);
   }

   if(listen(listenfd, 15) !=0)
   {
      perror("listen failed...");
      exit( -1 );
   }

	if(!no_wait)
	{
		sleep(30); // wait for correct date to be set...
	}
   loadTimersFromConfig();
   
	//startup Timer
   try
   {
      struct CTimerd::commandHead rmessage;
      while(doLoop)                      // wait for incomming messages
      {
         connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
         memset(&rmessage, 0, sizeof(rmessage));
         read(connfd,&rmessage,sizeof(rmessage));
         parse_command(connfd, &rmessage);
         close(connfd);
      }
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

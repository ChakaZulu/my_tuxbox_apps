//
//  $Id: sectionsd.cpp,v 1.28 2001/07/20 00:02:47 fnbrd Exp $
//
//	sectionsd.cpp (network daemon for SI-sections)
//	(dbox-II-project)
//
//	Copyright (C) 2001 by fnbrd
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
//  $Log: sectionsd.cpp,v $
//  Revision 1.28  2001/07/20 00:02:47  fnbrd
//  Kleiner Hack fuer besseres Zusammenspiel mit Neutrino.
//
//  Revision 1.26  2001/07/19 14:12:30  fnbrd
//  Noch ein paar Kleinigkeiten verbessert.
//
//  Revision 1.25  2001/07/19 10:33:52  fnbrd
//  Beschleunigt, interne Strukturen geaendert, Ausgaben sortiert.
//
//  Revision 1.24  2001/07/18 13:51:05  fnbrd
//  Datumsfehler behoben.
//
//  Revision 1.23  2001/07/18 03:26:45  fnbrd
//  Speicherloch gefixed.
//
//  Revision 1.22  2001/07/17 14:15:52  fnbrd
//  Kleine Aenderung damit auch static geht.
//
//  Revision 1.21  2001/07/17 13:14:59  fnbrd
//  Noch ne Verbesserung in Bezug auf alte Events.
//
//  Revision 1.19  2001/07/17 02:38:56  fnbrd
//  Fehlertoleranter
//
//  Revision 1.18  2001/07/16 15:57:58  fnbrd
//  Parameter -d fuer debugausgaben
//
//  Revision 1.17  2001/07/16 13:08:34  fnbrd
//  Noch ein Fehler beseitigt.
//
//  Revision 1.16  2001/07/16 12:56:50  fnbrd
//  Noch ein Fehler behoben.
//
//  Revision 1.15  2001/07/16 12:52:30  fnbrd
//  Fehler behoben.
//
//  Revision 1.14  2001/07/16 11:49:31  fnbrd
//  Neuer Befehl, Zeichen fuer codetable aus den Texten entfernt
//
//  Revision 1.13  2001/07/15 15:09:27  fnbrd
//  Informative Ausgabe.
//
//  Revision 1.12  2001/07/15 15:05:09  fnbrd
//  Speichert jetzt alle Events die bis zu 24h in der Zukunft liegen.
//
//  Revision 1.11  2001/07/15 11:58:20  fnbrd
//  Vergangene Zeit in Prozent beim EPG
//
//  Revision 1.10  2001/07/15 04:32:46  fnbrd
//  neuer sectionsd (mit event-liste)
//
//  Revision 1.9  2001/07/14 22:59:58  fnbrd
//  removeOldEvents() in SIevents
//
//  Revision 1.8  2001/07/14 17:36:04  fnbrd
//  Verbindungsthreads sind jetzt detached (kein Mem-leak mehr)
//
//  Revision 1.7  2001/07/14 16:41:44  fnbrd
//  fork angemacht
//
//  Revision 1.6  2001/07/14 16:38:46  fnbrd
//  Mit workaround fuer defektes mktime der glibc
//
//  Revision 1.5  2001/07/14 10:19:26  fnbrd
//  Mit funktionierendem time-thread (mktime der glibc muss aber gefixt werden)
//
//  Revision 1.4  2001/07/12 22:51:25  fnbrd
//  Time-Thread im sectionsd (noch disabled, da prob mit mktime)
//
//  Revision 1.3  2001/07/11 22:08:55  fnbrd
//  wegen gcc 3.0
//
//  Revision 1.2  2001/07/06 10:25:04  fnbrd
//  Debug-Zeug raus.
//
//  Revision 1.1  2001/06/27 11:59:44  fnbrd
//  Angepasst an gcc 3.0
//
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <set>
#include <map>
#include <algorithm>
#include <string>

#include <loki/SmartPtr.h>

#include <ost/dmx.h>

#include "sectionsdMsg.h"
#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

#define PORT_NUMBER 1600
// Wieviele Sekunden EPG gecached werden sollen
static long secondsToCache=24*60L*60L; // 24h
// Ab wann ein Event als alt gilt (in Sekunden)
static long oldEventsAre=120*60L; // 2h
static int debug=0;

#define dprintf(fmt, args...) {if(debug) printf(fmt, ## args);}
#define dputs(str) {if(debug) puts(str);}

static pthread_mutex_t eventsLock=PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge events geschrieben und gelesen wird
static pthread_mutex_t servicesLock=PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge services geschrieben und gelesen wird
static pthread_mutex_t dmxEITlock=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dmxSDTlock=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dmxEITnvodLock=PTHREAD_MUTEX_INITIALIZER;
static int dmxEITfd=0;
static int dmxSDTfd=0;
//static int dmxEITfd2=0;
static int timeset=0;
static SIevent nullEvt; // Null-Event, falls keins gefunden

//------------------------------------------------------------
// Wir verwalten die events in SmartPointers
// und nutzen verschieden sortierte Menge zum Zugriff
//------------------------------------------------------------
using namespace Loki;

//typedef SmartPtr<class SIevent, RefCounted, DisallowConversion, AssertCheckStrict>
typedef SmartPtr<class SIevent, RefCounted, DisallowConversion, NoCheck>
  SIeventPtr;

// Key ist unsigned short (Event-ID), data ist ein SIeventPtr
typedef map<unsigned short, SIeventPtr, less<unsigned short> > MySIeventsOrderEventID;
MySIeventsOrderEventID mySIeventsOrderEventID;

struct OrderServiceIDFirstStartTimeEventID
{
    bool operator()(const SIeventPtr &p1, const SIeventPtr &p2) {

      return
        p1->serviceID == p2->serviceID ?
        (p1->times.begin()->startzeit == p2->times.begin()->startzeit ? p1->eventID < p2->eventID : p1->times.begin()->startzeit < p2->times.begin()->startzeit )
        :
        (p1->serviceID < p2->serviceID );
    }
};

typedef map<const SIeventPtr, SIeventPtr, OrderServiceIDFirstStartTimeEventID > MySIeventsOrderServiceIDFirstStartTimeEventID;
MySIeventsOrderServiceIDFirstStartTimeEventID mySIeventsOrderServiceIDFirstStartTimeEventID;


struct OrderFirstEndTimeServiceIDEventID
{
    bool operator()(const SIeventPtr &p1, const SIeventPtr &p2) {

      return
        p1->times.begin()->startzeit + (long)p1->times.begin()->dauer == p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ?
        ( p1->serviceID == p2->serviceID ? p1->eventID < p2->eventID : p1->serviceID < p2->serviceID )
        :
	( p1->times.begin()->startzeit + (long)p1->times.begin()->dauer < p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ) ;
    }
};

typedef map<const SIeventPtr, SIeventPtr, OrderFirstEndTimeServiceIDEventID > MySIeventsOrderFirstEndTimeServiceIDEventID;
MySIeventsOrderFirstEndTimeServiceIDEventID mySIeventsOrderFirstEndTimeServiceIDEventID;

// Loescht ein Event aus allen Mengen
static void deleteEvent(const unsigned short eventID)
{
  MySIeventsOrderEventID::iterator e=mySIeventsOrderEventID.find(eventID);
  if(e!=mySIeventsOrderEventID.end()) {
    mySIeventsOrderFirstEndTimeServiceIDEventID.erase(e->second);
    mySIeventsOrderServiceIDFirstStartTimeEventID.erase(e->second);
    mySIeventsOrderEventID.erase(eventID);
  }
}

// Fuegt ein Event in alle Mengen ein
static void addEvent(const SIevent &e)
{
  SIeventPtr s(new SIevent(e));
  // Damit in den nicht nach Event-ID sortierten Mengen
  // Mehrere Events mit gleicher ID sind, diese vorher loeschen
  deleteEvent(e.eventID);
  mySIeventsOrderEventID.insert(make_pair(e.eventID, s));
  mySIeventsOrderServiceIDFirstStartTimeEventID.insert(make_pair(s, SIeventPtr(s)));
  mySIeventsOrderFirstEndTimeServiceIDEventID.insert(make_pair(s, SIeventPtr(s)));
}

static void removeOldEvents(long seconds)
{
  // Alte events loeschen
  time_t zeit=time(NULL);
  for(MySIeventsOrderFirstEndTimeServiceIDEventID::iterator e=mySIeventsOrderFirstEndTimeServiceIDEventID.begin(); e!=mySIeventsOrderFirstEndTimeServiceIDEventID.end(); e++)
    if(e->first->times.size()) {
      if(e->first->times.begin()->startzeit+(long)e->first->times.begin()->dauer<zeit-seconds)
        deleteEvent(e->first->eventID);
      else
        break; // sortiert nach Endzeit, daher weiter Suchen unnoetig
    }
  return;
}

//typedef SmartPtr<class SIservice, RefCounted, DisallowConversion, AssertCheckStrict>
typedef SmartPtr<class SIservice, RefCounted, DisallowConversion, NoCheck>
  SIservicePtr;

// Key ist unsigned short (Sevice-ID), data ist ein SIservicePtr
typedef map<unsigned short, SIservicePtr, less<unsigned short> > MySIservicesOrderServiceID;
MySIservicesOrderServiceID mySIservicesOrderServiceID;

struct OrderServiceName
{
  // Evtl. waere es schneller die Controlcodes beim einfuegen zu loeschen
  bool operator()(const SIservicePtr &p1, const SIservicePtr &p2) {
/*
    // Erst mal die Controlcodes entfernen
    char servicename1[50];
    strncpy(servicename1, p1->serviceName.c_str(), sizeof(servicename1)-1);
    servicename1[sizeof(servicename1)-1]=0;
    removeControlCodes(servicename1);
    char servicename2[50];
    strncpy(servicename2, p2->serviceName.c_str(), sizeof(servicename2)-1);
    servicename2[sizeof(servicename2)-1]=0;
    removeControlCodes(servicename2);
    return strcasecmp(servicename1, servicename2) < 0;
*/
    return strcasecmp(p1->serviceName.c_str(), p2->serviceName.c_str()) < 0;
  }
};

typedef map<const SIservicePtr, SIservicePtr, OrderServiceName > MySIservicesOrderServiceName;
MySIservicesOrderServiceName mySIservicesOrderServiceName;

/*
// Loescht ein Event aus allen Mengen
static void deleteService(const unsigned short serviceID)
{
  MySIservicesOrderServiceID::iterator s=mySIservicesOrderServiceID.find(serviceID);
  if(s!=mySIservicesOrderServiceID.end()) {
    mySIservicesOrderServiceName.erase(s->second);
    mySIservicesOrderServiceID.erase(serviceID);
  }
}
*/
// Fuegt ein Event in alle Mengen ein
static void addService(const SIservice &s)
{
  SIservicePtr sptr(new SIservice(s));
  // Controlcodes entfernen
  char servicename[50];
  strncpy(servicename, sptr->serviceName.c_str(), sizeof(servicename)-1);
  servicename[sizeof(servicename)-1]=0;
  removeControlCodes(servicename);
  sptr->serviceName=servicename;
  mySIservicesOrderServiceID.insert(make_pair(s.serviceID, sptr));
  mySIservicesOrderServiceName.insert(make_pair(sptr, SIservicePtr(sptr)));
}

//------------------------------------------------------------
// other stuff
//------------------------------------------------------------

static int startDMXeit(void)
{
  if ((dmxEITfd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 1;
  }
  if (ioctl (dmxEITfd, DMX_SET_BUFFER_SIZE, 384*1024) == -1) {
    close(dmxEITfd);
    dmxEITfd=0;
    perror ("DMX_SET_BUFFER_SIZE");
    return 2;
  }
  struct dmxSctFilterParams flt;
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x12;
  flt.filter.filter[0] = 0x50; // schedule
  flt.filter.mask[0]   = 0xf0; // -> 5x
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if (ioctl (dmxEITfd, DMX_SET_FILTER, &flt) == -1) {
    close(dmxEITfd);
    dmxEITfd=0;
    perror ("DMX_SET_FILTER");
    return 3;
  }
  pthread_mutex_unlock(&dmxEITlock);
  return 0;
}

static int stopDMXeit(void)
{
  pthread_mutex_lock(&dmxEITlock);
  close(dmxEITfd);
  dmxEITfd=0;
  return 0;
}

static int pauseDMXeit(void)
{
  pthread_mutex_lock(&dmxEITlock);
  if (ioctl (dmxEITfd, DMX_STOP, 0) == -1) {
    close(dmxEITfd);
    dmxEITfd=0;
    perror ("DMX_STOP");
    return 1;
  }
  return 0;
}

static int unpauseDMXeit(void)
{
  if (ioctl (dmxEITfd, DMX_START, 0) == -1) {
    close(dmxEITfd);
    dmxEITfd=0;
    perror ("DMX_START");
    pthread_mutex_unlock(&dmxEITlock);
    return 4;
  }
  pthread_mutex_unlock(&dmxEITlock);
  return 0;
}

static int startDMXsdt(void)
{
  if ((dmxSDTfd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 1;
  }
  struct dmxSctFilterParams flt;
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x11;
  flt.filter.filter[0] = 0x42;
  flt.filter.mask[0]   = 0xff;
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
  if (ioctl (dmxSDTfd, DMX_SET_FILTER, &flt) == -1) {
    close(dmxSDTfd);
    dmxSDTfd=0;
    perror ("DMX_SET_FILTER");
    return 2;
  }
  pthread_mutex_unlock(&dmxSDTlock);
  return 0;
}

static int stopDMXsdt(void)
{
  pthread_mutex_lock(&dmxSDTlock);
  close(dmxSDTfd);
  dmxSDTfd=0;
  return 0;
}

#ifdef NO_ZAPD_NEUTRINO_HACK
static int pauseDMXsdt(void)
{
  pthread_mutex_lock(&dmxSDTlock);
  if (ioctl (dmxSDTfd, DMX_STOP, 0) == -1) {
    close(dmxSDTfd);
    dmxSDTfd=0;
    perror ("DMX_STOP");
    return 1;
  }
  return 0;
}

static int unpauseDMXsdt(void)
{
  if (ioctl (dmxSDTfd, DMX_START, 0) == -1) {
    close(dmxSDTfd);
    dmxSDTfd=0;
    perror ("DMX_START");
    pthread_mutex_unlock(&dmxSDTlock);
    return 4;
  }
  pthread_mutex_unlock(&dmxSDTlock);
  return 0;
}
#endif

/*
static int startDMXeitNVOD(void)
{
  if (ioctl (dmxEITfd2, DMX_START, 0) == -1) {
    close(dmxEITfd2);
    dmxEITfd2=0;
    perror ("DMX_START");
    pthread_mutex_unlock(&dmxEITnvodLock);
    return -1;
  }
  pthread_mutex_unlock(&dmxEITnvodLock);
  return 0;
}

static int stopDMXeitNVOD(void)
{
  pthread_mutex_lock(&dmxEITnvodLock);
  if (ioctl (dmxEITfd2, DMX_STOP, 0) == -1) {
    close(dmxEITfd2);
    dmxEITfd2=0;
    perror ("DMX_STOP");
    return -1;
  }
  return 0;
}
*/

// Liefert die ServiceID zu einem Namen
// 0 bei Misserfolg
static unsigned short findServiceIDforServiceName(const char *serviceName)
{
  SIservicePtr s(new SIservice((unsigned short)0));
  s->serviceName=serviceName;
  dprintf("Search for Service '%s'\n", serviceName);
  MySIservicesOrderServiceName::iterator si=mySIservicesOrderServiceName.find(s);
  if(si!=mySIservicesOrderServiceName.end())
    return si->first->serviceID;
  dputs("Service not found");
  return 0;
}

static const SIevent &findActualSIeventForServiceID(const unsigned serviceID)
{
  time_t zeit=time(NULL);
  // Event (serviceid) suchen
  int serviceIDfound=0;
  for(MySIeventsOrderServiceIDFirstStartTimeEventID::iterator e=mySIeventsOrderServiceIDFirstStartTimeEventID.begin(); e!=mySIeventsOrderServiceIDFirstStartTimeEventID.end(); e++)
    if(e->first->serviceID==serviceID) {
      serviceIDfound=1;
      for(SItimes::iterator t=e->first->times.begin(); t!=e->first->times.end(); t++)
        if(t->startzeit<=zeit && zeit<=(long)(t->startzeit+t->dauer))
          return *(e->first);
    } // if = serviceID
    else if(serviceIDfound)
      break; // sind nach serviceID und startzeit sortiert, daher weiter Suchen unnoetig
  return nullEvt;
}

static const SIevent &findActualSIeventForServiceName(const char *serviceName)
{
  unsigned short serviceID=findServiceIDforServiceName(serviceName);
  if(serviceID)
    return findActualSIeventForServiceID(serviceID);
  return nullEvt;
}

static const SIevent &findNextSIevent(const unsigned short eventID)
{
  MySIeventsOrderEventID::iterator eFirst=mySIeventsOrderEventID.find(eventID);
  if(eFirst!=mySIeventsOrderEventID.end()) {
    MySIeventsOrderServiceIDFirstStartTimeEventID::iterator eNext=mySIeventsOrderServiceIDFirstStartTimeEventID.find(eFirst->second);
    eNext++;
    if(eNext!=mySIeventsOrderServiceIDFirstStartTimeEventID.end())
      return *(eNext->second);
  }
  return nullEvt;
}

// Liefert 1 wenn das Event entweder ein zu einem NVOD-Service gehoert
// oder selbst nur NVOD-Zeiten hat
static int isNVODevent(const SIevent &e)
{
  return 0;
}

// Liest n Bytes aus einem Socket per read
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl gelesener Bytes
inline int readNbytes(int fd, char *buf, int n, unsigned timeoutInSeconds)
{
int j;

  for(j=0; j<n;) {
    struct pollfd ufds;
    ufds.fd=fd;
    ufds.events=POLLIN;
    ufds.revents=0;
    int rc=poll(&ufds, 1, timeoutInSeconds*1000);
    if(!rc)
      return 0; // timeout
    else if(rc<0 && errno==EINTR)
      continue; // interuppted
    else if(rc<0) {
      perror ("poll");
//      printf("errno: %d\n", errno);
      return -1;
    }
    int r=read (fd, buf, n-j);
    if(r>0) {
      j+=r;
      buf+=r;
    }
    else if(r<=0 && errno!=EINTR && errno!=EAGAIN) {
      perror ("read");
      return -1;
    }
  }
  return j;
}

//*********************************************************************
//			connection-thread
// handles incoming requests
//*********************************************************************
struct connectionData {
  int connectionSocket;
  struct sockaddr_in clientAddr;
};

static void commandDumpAllServices(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of service list.\n");
  char *serviceList=new char[65*1024]; // 65kb should be enough and dataLength is unsigned short
  if(!serviceList) {
    fprintf(stderr, "low on memory!\n");
    return;
  }
  *serviceList=0;
  pthread_mutex_lock(&servicesLock);
  char daten[200];
  for(MySIservicesOrderServiceName::iterator s=mySIservicesOrderServiceName.begin(); s!=mySIservicesOrderServiceName.end(); s++) {
    sprintf(daten, "%hu %hhu %d %d %d %d %u ",
      s->first->serviceID, s->first->serviceTyp,
      s->first->eitScheduleFlag(), s->first->eitPresentFollowingFlag(),
      s->first->runningStatus(), s->first->freeCAmode(),
      s->first->nvods.size());
    strcat(serviceList, daten);
    strcat(serviceList, "\n");
    strcat(serviceList, s->first->serviceName.c_str());
    strcat(serviceList, "\n");
    strcat(serviceList, s->first->providerName.c_str());
    strcat(serviceList, "\n");
  }
  pthread_mutex_unlock(&servicesLock);
  struct msgSectionsdResponseHeader msgResponse;
  msgResponse.dataLength=strlen(serviceList)+1;
  if(msgResponse.dataLength==1)
    msgResponse.dataLength=0;
  write(client->connectionSocket, &msgResponse, sizeof(msgResponse));
  if(msgResponse.dataLength)
    write(client->connectionSocket, serviceList, msgResponse.dataLength);
  delete[] serviceList;
  return;
}

static void commandSetEventsAreOldInMinutes(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength!=2)
    return;
  dprintf("Set events are old after minutes: %hd\n", *((unsigned short*)data));
  oldEventsAre=*((unsigned short*)data)*60L;
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=0;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  return;
}

static void commandSetHoursToCache(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength!=2)
    return;
  dprintf("Set hours to cache: %hd\n", *((unsigned short*)data));
  secondsToCache=*((unsigned short*)data)*60L*60L;
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=0;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  return;
}

static void commandAllEventsChannelName(struct connectionData *client, char *data, unsigned dataLength)
{
  data[dataLength-1]=0; // to be sure it has an trailing 0
  dprintf("Request of all events for '%s'\n", data);
  pthread_mutex_lock(&servicesLock);
  unsigned short serviceID=findServiceIDforServiceName(data);
  pthread_mutex_unlock(&servicesLock);
  char *evtList=new char[65*1024]; // 65kb should be enough and dataLength is unsigned short
  if(!evtList) {
    fprintf(stderr, "low on memory!\n");
    return;
  }
  *evtList=0;
  if(serviceID!=0) {
    // service Found
    if(pauseDMXeit()) {
      delete[] evtList;
      return;
    }
    pthread_mutex_lock(&eventsLock);
    int serviceIDfound=0;
    for(MySIeventsOrderServiceIDFirstStartTimeEventID::iterator e=mySIeventsOrderServiceIDFirstStartTimeEventID.begin(); e!=mySIeventsOrderServiceIDFirstStartTimeEventID.end(); e++)
      if(e->first->serviceID==serviceID) {
        serviceIDfound=1;
        if(e->first->times.size()) { // Nur events mit Zeiten
	  char strZeit[50];
	  struct tm *tmZeit;
          tmZeit=localtime(&(e->first->times.begin()->startzeit));
	  sprintf(strZeit, "%02d.%02d %02d:%02d %u ",
	    tmZeit->tm_mday, tmZeit->tm_mon+1, tmZeit->tm_hour, tmZeit->tm_min, e->first->times.begin()->dauer/60);
	  strcat(evtList, strZeit);
	  strcat(evtList, e->first->name.c_str());
	  strcat(evtList, "\n");
	} // if times.size
      } // if = serviceID
      else if(serviceIDfound)
        break; // sind nach serviceID und startzeit sortiert -> nicht weiter suchen
    pthread_mutex_unlock(&eventsLock);
    if(unpauseDMXeit()) {
      delete[] evtList;
      return;
    }
  }
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=strlen(evtList)+1;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  if(responseHeader.dataLength)
    write(client->connectionSocket, evtList, responseHeader.dataLength);
  delete[] evtList;
  return;
}

static void commandDumpStatusInformation(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of status information");
  pthread_mutex_lock(&eventsLock);
  unsigned anzEvents=mySIeventsOrderEventID.size();
  pthread_mutex_unlock(&eventsLock);
  pthread_mutex_lock(&servicesLock);
  unsigned anzServices=mySIservicesOrderServiceID.size();
//  unsigned anzServices=services.size();
  pthread_mutex_unlock(&servicesLock);
  struct mallinfo speicherinfo=mallinfo();
  time_t zeit=time(NULL);
  char stati[1024];
  sprintf(stati,
    "Current time: %s"
    "Hours to cache: %ld\n"
    "Events are old %ldmin after their end time\n"
    "Number of cached services: %u\n"
    "Number of cached events: %u\n"
    "Total size of memory occupied by chunks handed out by malloc: %d\n"
    "Total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n",
    ctime(&zeit),
    secondsToCache/(60*60L), oldEventsAre/60, anzServices, anzEvents, speicherinfo.uordblks,
    speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024.)
    );
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=strlen(stati)+1;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  if(responseHeader.dataLength)
    write(client->connectionSocket, stati, responseHeader.dataLength);
  return;
}

#ifdef NO_ZAPD_NEUTRINO_HACK
static int currentNextWasOk=0;
#endif

// Mostly copied from epgd (something bugfixed ;) )
static void commandCurrentNextInfoChannelName(struct connectionData *client, char *data, unsigned dataLength)
{
  int nResultDataSize=0;
  char* pResultData=0;

  data[dataLength-1]=0; // to be sure it has an trailing 0
  dprintf("Request of current/next information for '%s'\n", data);

  if(pauseDMXeit())
    return;
  pthread_mutex_lock(&eventsLock);
  pthread_mutex_lock(&servicesLock);
  const SIevent &evt=findActualSIeventForServiceName(data);
  pthread_mutex_unlock(&servicesLock);

//  readSection(request.Name, &pResultData, &nResultDataSize);

  if(evt.serviceID!=0) {//Found
    dprintf("current EPG found.\n");
    SItime siStart = *(evt.times.begin());
    const SIevent &nextEvt=findNextSIevent(evt.eventID);
//    const SIevent &nextEvt=findNextSIeventForService(evt.serviceID, siStart.startzeit+siStart.dauer);
    if(nextEvt.serviceID!=0) {
      dprintf("next EPG found.\n");

      // Folgendes ist grauenvoll, habs aber einfach kopiert aus epgd
      // und keine Lust das grossartig zu verschoenern
      nResultDataSize=
        strlen(evt.name.c_str())+1+		//Name + del
        3+2+1+					//std:min + del
        4+1+					//dauer (mmmm) + del
        3+1+					//100 + del
        strlen(nextEvt.name.c_str())+1+		//Name + del
        3+2+1+					//std:min + del
        4+1+1;					//dauer (mmmm) + del + 0
      pResultData = new char[nResultDataSize];
      struct tm *pStartZeit = localtime(&siStart.startzeit);
      int nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);
      unsigned dauer=siStart.dauer/60;
      unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-siStart.startzeit)/(float)siStart.dauer*100.);

      siStart = *(nextEvt.times.begin());
      pStartZeit = localtime(&siStart.startzeit);
      int nSH2(pStartZeit->tm_hour), nSM2(pStartZeit->tm_min);
      unsigned dauer2=siStart.dauer/60;

      sprintf(pResultData,
      "%s\n%02d:%02d\n%04u\n%03u\n%s\n%02d:%02d\n%04u\n",
        evt.name.c_str(),
        nSH, nSM, dauer, nProcentagePassed,
        nextEvt.name.c_str(),
        nSH2, nSM2, dauer2);
    }
  }
  pthread_mutex_unlock(&eventsLock);
  unpauseDMXeit();

  // response
  struct msgSectionsdResponseHeader pmResponse;
  pmResponse.dataLength=nResultDataSize;
  write(client->connectionSocket, &pmResponse, sizeof(pmResponse));
  if( nResultDataSize > 0 ) {
    write(client->connectionSocket, pResultData, nResultDataSize);
    delete[] pResultData;
#ifdef NO_ZAPD_NEUTRINO_HACK
    currentNextWasOk=1;
#endif
  }
  else
    dprintf("current/next EPG not found!\n");
}

// Mostly copied from epgd (something bugfixed ;) )
static void commandActualEPGchannelName(struct connectionData *client, char *data, unsigned dataLength)
{
  int nResultDataSize=0;
  char* pResultData=0;

  data[dataLength-1]=0; // to be sure it has an trailing 0
  dprintf("Request of actual EPG for '%s'\n", data);

  if(pauseDMXeit())
    return;
  pthread_mutex_lock(&eventsLock);
  pthread_mutex_lock(&servicesLock);
  const SIevent &evt=findActualSIeventForServiceName(data);
  pthread_mutex_unlock(&servicesLock);

//  readSection(request.Name, &pResultData, &nResultDataSize);

  if(evt.serviceID!=0) {//Found
    dprintf("EPG found.\n");
    nResultDataSize=strlen(evt.name.c_str())+1+		//Name + del
      strlen(evt.text.c_str())+1+		//Text + del
      strlen(evt.extendedText.c_str())+1+	//ext + del
      3+3+4+1+					//dd.mm.yyyy + del
      3+2+1+					//std:min + del
      3+2+1+					//std:min+ del
      3+1+1;					//100 + del + 0
    pResultData = new char[nResultDataSize];
    SItime siStart = *(evt.times.begin());
    struct tm *pStartZeit = localtime(&siStart.startzeit);
    int nSDay(pStartZeit->tm_mday), nSMon(pStartZeit->tm_mon+1), nSYear(pStartZeit->tm_year+1900),
     nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);

    long int uiEndTime(siStart.startzeit+siStart.dauer);
    struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
    int nFH(pEndeZeit->tm_hour), nFM(pEndeZeit->tm_min);

    unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-siStart.startzeit)/(float)siStart.dauer*100.);

    sprintf(pResultData, "%s\xFF%s\xFF%s\xFF%02d.%02d.%04d\xFF%02d:%02d\xFF%02d:%02d\xFF%03u\xFF",
      evt.name.c_str(),
      evt.text.c_str(),
      evt.extendedText.c_str(), nSDay, nSMon, nSYear, nSH, nSM, nFH, nFM, nProcentagePassed );
  }
  else
    dprintf("actual EPG not found!\n");
  pthread_mutex_unlock(&eventsLock);
  unpauseDMXeit();

  // response
  struct msgSectionsdResponseHeader pmResponse;
  pmResponse.dataLength=nResultDataSize;
  write(client->connectionSocket, &pmResponse, sizeof(pmResponse));
  if( nResultDataSize > 0 ) {
    write(client->connectionSocket, pResultData, nResultDataSize);
    delete[] pResultData;
  }
}

static void sendEventList(struct connectionData *client, unsigned char serviceTyp)
{
  char *evtList=new char[65*1024]; // 65kb should be enough and dataLength is unsigned short
  if(!evtList) {
    fprintf(stderr, "low on memory!\n");
    return;
  }
  *evtList=0;
  if(pauseDMXeit()) {
    delete[] evtList;
    return;
  }
  pthread_mutex_lock(&servicesLock);
  pthread_mutex_lock(&eventsLock);
  for(MySIservicesOrderServiceName::iterator s=mySIservicesOrderServiceName.begin(); s!=mySIservicesOrderServiceName.end(); s++)
    if(s->first->serviceTyp==serviceTyp) {
      const SIevent &evt=findActualSIeventForServiceID(s->first->serviceID);
      strcat(evtList, s->first->serviceName.c_str());
      strcat(evtList, "\n");
      if(evt.serviceID!=0)
        //Found
        strcat(evtList, evt.name.c_str());
      strcat(evtList, "\n");
    } // if TV
  pthread_mutex_unlock(&eventsLock);
  pthread_mutex_unlock(&servicesLock);
  unpauseDMXeit();
  struct msgSectionsdResponseHeader msgResponse;
  msgResponse.dataLength=strlen(evtList)+1;
  if(msgResponse.dataLength==1)
    msgResponse.dataLength=0;
  write(client->connectionSocket, &msgResponse, sizeof(msgResponse));
  if(msgResponse.dataLength)
    write(client->connectionSocket, evtList, msgResponse.dataLength);
  delete[] evtList;
}

static void commandEventListTV(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of TV event list.\n");
  sendEventList(client, 0x01);
  return;
}

static void commandEventListRadio(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of radio event list.\n");
  sendEventList(client, 0x02);
  return;
}

static void (*connectionCommands[NUMBER_OF_SECTIONSD_COMMANDS]) (struct connectionData *, char *, unsigned)  = {
  commandActualEPGchannelName,
  commandEventListTV,
  commandCurrentNextInfoChannelName,
  commandDumpStatusInformation,
  commandAllEventsChannelName,
  commandSetHoursToCache,
  commandSetEventsAreOldInMinutes,
  commandDumpAllServices,
  commandEventListRadio
};

static void *connectionThread(void *conn)
{
struct connectionData *client=(struct connectionData *)conn;

  dprintf("Connection from %s\n", inet_ntoa(client->clientAddr.sin_addr));
  struct msgSectionsdRequestHeader header;
  memset(&header, 0, sizeof(header));

  if(readNbytes(client->connectionSocket, (char *)&header, sizeof(header) , 2)>0) {
    dprintf("version: %hhd, cmd: %hhd\n", header.version, header.command);
    if(header.version==2 && header.command<NUMBER_OF_SECTIONSD_COMMANDS) {
      dprintf("data length: %hd\n", header.dataLength);
      char *data=new char[header.dataLength+1];
      if(!data)
        fprintf(stderr, "low on memory!\n");
      else {
        int rc=1;
        if(header.dataLength)
	  rc=readNbytes(client->connectionSocket, data, header.dataLength, 2);
        if(rc>0) {
          dprintf("Starting command %hhd\n", header.command);
          connectionCommands[header.command](client, data, header.dataLength);
        }
        delete[] data;
      }
    }
    else
      dputs("Unknow format or version of request!");
  }
//  oldDaemonCommands(client);
  close(client->connectionSocket);
  dprintf("Connection from %s closed!\n", inet_ntoa(client->clientAddr.sin_addr));
  delete client;
#ifdef NO_ZAPD_NEUTRINO_HACK
  if(currentNextWasOk) {
    // Damit nach dem umschalten der camd/pzap usw. schneller anlaeuft.
    currentNextWasOk=0;
    if(pauseDMXeit())
      return 0;
    if(pauseDMXsdt())
      return 0;
    int rc=5;
    while(rc)
      rc=sleep(rc);
    if(unpauseDMXsdt())
      return 0;
    if(unpauseDMXeit())
      return 0;
  }
#endif
  return 0;
}

//*********************************************************************
//			sdt-thread
// reads sdt for service list
//*********************************************************************
static void *sdtThread(void *)
{
struct SI_section_header header;
char *buf;
const unsigned timeoutInSeconds=2;

  dprintf("sdt-thread started.\n");
  pthread_mutex_lock(&dmxSDTlock);
  if(startDMXsdt()) // -> unlock
    return 0;
  for(;;) {
    pthread_mutex_lock(&dmxSDTlock);
    int rc=readNbytes(dmxSDTfd, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      pthread_mutex_unlock(&dmxSDTlock);
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      pthread_mutex_unlock(&dmxSDTlock);
      break;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      pthread_mutex_unlock(&dmxSDTlock);
      fprintf(stderr, "Not enough memory!\n");
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(dmxSDTfd, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    pthread_mutex_unlock(&dmxSDTlock);
    if(!rc) {
      delete[] buf;
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      delete[] buf;
      break;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionSDT sdt(SIsection(sizeof(header)+header.section_length-5, buf));
      pthread_mutex_lock(&servicesLock);
      for(SIservices::iterator s=sdt.services().begin(); s!=sdt.services().end(); s++)
        addService(*s);
      pthread_mutex_unlock(&servicesLock);
    } // if
    else
      delete[] buf;
  } // for
  close(dmxSDTfd);
  dmxSDTfd=0;
  dprintf("sdt-thread ended\n");
  return 0;
}

//*********************************************************************
//			Time-thread
// updates system time according TOT every 30 minutes
//*********************************************************************
struct SI_section_TOT_header {
      unsigned char table_id : 8;
      // 1 byte
      unsigned char section_syntax_indicator : 1;
      unsigned char reserved_future_use : 1;
      unsigned char reserved1 : 2;
      unsigned short section_length : 12;
      // 3 bytes
      unsigned long long UTC_time : 40;
      // 8 bytes
      unsigned char reserved2 : 4;
      unsigned short descriptors_loop_length : 12;
} __attribute__ ((packed)) ; // 10 bytes

struct SI_section_TDT_header {
      unsigned char table_id : 8;
      // 1 byte
      unsigned char section_syntax_indicator : 1;
      unsigned char reserved_future_use : 1;
      unsigned char reserved1 : 2;
      unsigned short section_length : 12;
      // 3 bytes
      unsigned long long UTC_time : 40;
} __attribute__ ((packed)) ; // 10 bytes

/*
// BR schickt falschen Time-Offset, daher per TZ und Rest hier auskommentiert

struct descr_gen_struct {
  unsigned char descriptor_tag : 8;
  unsigned char descriptor_length : 8;
} __attribute__ ((packed)) ;

struct local_time_offset {
  char country_code1 : 8;
  char country_code2 : 8;
  char country_code3 : 8;
  unsigned char country_region_id : 6;
  unsigned char reserved : 1;
  unsigned char local_time_offset_polarity : 1;
  unsigned short local_time_offs : 16;
  unsigned long long time_of_chng : 40;
  unsigned short next_time_offset : 8;
} __attribute__ ((packed)) ;

static int timeOffsetMinutes=0; // minutes
static int timeOffsetFound=0;

static void parseLocalTimeOffsetDescriptor(const char *buf, const char *countryCode)
{
  struct descr_gen_struct *desc=(struct descr_gen_struct *)buf;
  buf+=2;
  while(buf<((char *)desc)+2+desc->descriptor_length-sizeof(struct local_time_offset)) {
    struct local_time_offset *lto=(struct local_time_offset *)buf;
    if(!strncmp(countryCode, buf, 3)) {
      timeOffsetMinutes=(((lto->local_time_offs)>>12)&0x0f)*10*60L+(((lto->local_time_offs)>>8)&0x0f)*60L+
	(((lto->local_time_offs)>>4)&0x0f)*10+((lto->local_time_offs)&0x0f);
      if(lto->local_time_offset_polarity)
        timeOffsetMinutes=-timeOffsetMinutes;
      timeOffsetFound=1;
      break;
    }
//    else
//      printf("Code: %c%c%c\n", lto->country_code1, lto->country_code2, lto->country_code3);
    buf+=sizeof(struct local_time_offset);
  }
}

static void parseDescriptors(const char *des, unsigned len, const char *countryCode)
{
  struct descr_gen_struct *desc;
  while(len>=sizeof(struct descr_gen_struct)) {
    desc=(struct descr_gen_struct *)des;
    if(desc->descriptor_tag==0x58) {
//      printf("Found time descriptor\n");
      parseLocalTimeOffsetDescriptor((const char *)desc, countryCode);
      if(timeOffsetFound)
        break;
    }
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

*/
static void *timeThread(void *)
{
int fd;
struct dmxSctFilterParams flt;
const unsigned timeoutInSeconds=31;
char *buf;

  dprintf("time-thread started.\n");
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x14;
  flt.filter.filter[0] = 0x70; // TDT
  flt.filter.mask[0]   = 0xff;
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START;

  // Zuerst per TDT (schneller)
  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 0;
  }
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    close(fd);
    perror ("DMX_SET_FILTER");
    return 0;
  }
  {
  struct SI_section_TDT_header tdt_header;
  int rc=readNbytes(fd, (char *)&tdt_header, sizeof(tdt_header), timeoutInSeconds);
  if(rc>0) {
    time_t tim=changeUTCtoCtime(((const unsigned char *)&tdt_header)+3);
    if(tim) {
      if(stime(&tim)< 0) {
        perror("cannot set date");
	close(fd);
	return 0;
      }
      timeset=1;
      time_t t=time(NULL);
      dprintf("local time: %s", ctime(&t));
    }
  }
  }
  if (ioctl (fd, DMX_STOP, 0) == -1) {
    close(fd);
    perror ("DMX_STOP");
    return 0;
  }
  flt.filter.filter[0] = 0x73; // TOT
  flt.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    close(fd);
    perror ("DMX_SET_FILTER");
    return 0;
  }
  // Jetzt wird die Uhrzeit nur noch per TOT gesetzt (CRC)
  for(;;) {
    if(!fd) {
      if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
        perror ("/dev/ost/demux0");
        return 0;
      }
      if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
        close(fd);
        perror ("DMX_SET_FILTER");
        return 0;
      }
    }
    struct SI_section_TOT_header header;
    int rc=readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(fd);
      break;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      fprintf(stderr, "Not enough memory!\n");
      close(fd);
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(fd, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    if(!rc) {
      delete[] buf;
      continue; // timeout -> kein TDT
    }
    else if(rc<0) {
      delete[] buf;
      break;
    }
    time_t tim=changeUTCtoCtime(((const unsigned char *)&header)+3);
//    printf("time: %ld\n", tim);
    if(tim) {
//      timeOffsetFound=0;
//      parseDescriptors(buf+sizeof(struct SI_section_TOT_header), ((struct SI_section_TOT_header *)buf)->descriptors_loop_length, "DEU");
//      printf("local time: %s", ctime(&tim));
//      printf("Time offset %d", timeOffsetMinutes);
//      if(timeOffsetFound)
//        tim+=timeOffsetMinutes*60L;
      if(stime(&tim)< 0) {
        perror("cannot set date");
	break;
      }
      timeset=1;
      time_t t=time(NULL);
      dprintf("local time: %s", ctime(&t));
    }
    delete[] buf;
    close(fd);
    fd=0;
    if(timeset)
      rc=60*30;  // sleep 30 minutes
    else
      rc=60;  // sleep 1 minute
    while(rc)
      rc=sleep(rc);
  } // for
  dprintf("time-thread ended\n");
  return 0;
}

//*********************************************************************
//			EIT-thread
// reads EPG-datas (scheduled)
//*********************************************************************
static void *eitThread(void *)
{
struct SI_section_header header;
char *buf;
const unsigned timeoutInSeconds=2;

  dprintf("eit-thread started.\n");
  pthread_mutex_lock(&dmxEITlock);
  if(startDMXeit()) // -> unlock
    return 0;
  for(;;) {
    pthread_mutex_lock(&dmxEITlock);
    int rc=readNbytes(dmxEITfd, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      pthread_mutex_unlock(&dmxEITlock);
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(dmxEITfd);
      dmxEITfd=0;
      pthread_mutex_unlock(&dmxEITlock);
      break;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      close(dmxEITfd);
      dmxEITfd=0;
      pthread_mutex_unlock(&dmxEITlock);
      fprintf(stderr, "Not enough memory!\n");
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(dmxEITfd, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    pthread_mutex_unlock(&dmxEITlock);
    if(!rc) {
      delete[] buf;
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(dmxEITfd);
      dmxEITfd=0;
      delete[] buf;
      break;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionEIT eit(SIsection(sizeof(header)+header.section_length-5, buf));
      time_t zeit=time(NULL);
      // Nicht alle Events speichern
      for(SIevents::iterator e=eit.events().begin(); e!=eit.events().end(); e++)
        if(e->times.size()>0) {
	  if(e->times.begin()->startzeit < zeit+secondsToCache &&
	    e->times.begin()->startzeit+(long)e->times.begin()->dauer > zeit-oldEventsAre
	  ) {
            pthread_mutex_lock(&eventsLock);
	    addEvent(*e);
            pthread_mutex_unlock(&eventsLock);
          }
	}
    } // if
    else
      delete[] buf;
  } // for

  dprintf("eit-thread ended\n");
  return 0;
}

//*********************************************************************
//			EIT-nvod-thread
// reads EPG-datas (nvod)
//*********************************************************************
/*
static void *eitNVODthread(void *)
{
struct SI_section_header header;
struct dmxSctFilterParams flt;
char *buf;
const unsigned timeoutInSeconds=2;

  dprintf("eit-thread (nvod) started.\n");
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x12;
  flt.filter.filter[0] = 0x4f; // present/following
  flt.filter.mask[0]   = 0xff;
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if ((dmxEITfd2 = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 0;
//    return 1;
  }
  if (ioctl (dmxEITfd2, DMX_SET_FILTER, &flt) == -1) {
    close(dmxEITfd2);
    dmxEITfd2=0;
    perror ("DMX_SET_FILTER");
    return 0;
//    return 2;
  }
  for(;;) {
    pthread_mutex_lock(&dmxEITnvodLock);
    int rc=readNbytes(dmxEITfd2, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      pthread_mutex_unlock(&dmxEITnvodLock);
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(dmxEITfd2);
      dmxEITfd2=0;
      pthread_mutex_unlock(&dmxEITnvodLock);
      break;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      close(dmxEITfd2);
      dmxEITfd2=0;
      pthread_mutex_unlock(&dmxEITnvodLock);
      fprintf(stderr, "Not enough memory!\n");
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(dmxEITfd2, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    pthread_mutex_unlock(&dmxEITnvodLock);
    if(!rc) {
      delete[] buf;
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(dmxEITfd2);
      dmxEITfd2=0;
      delete[] buf;
      break;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionEIT eit(SIsection(sizeof(header)+header.section_length-5, buf));
      time_t zeit=time(NULL);
      // Nicht alle Events speichern
      // nur mit nvod und im Zeitrahmen
      for(SIevents::iterator e=eit.events().begin(); e!=eit.events().end(); e++)
        if(isNVODevent(*e)) {
          if(e->times.size()>0) { // Geht schief bei nvods
            if(e->times.begin()->startzeit<zeit+(long)HOURS_TO_CACHE*60L*60L) {
              pthread_mutex_lock(&eventsLock);
	      addEvent(*e);
              pthread_mutex_unlock(&eventsLock);
            }
	  }
        }
    } // if
    else
      delete[] buf;
  } // for
  dprintf("eit-thread (nvod) ended\n");
  return 0;
}
*/

//*********************************************************************
//			housekeeping-thread
// does cleaning on fetched datas
//*********************************************************************
static void *houseKeepingThread(void *)
{
  dprintf("housekeeping-thread started.\n");
  for(;;) {
    int rc=5*60;  // sleep 2 minutes
    while(rc)
      rc=sleep(rc);
    dprintf("housekeeping.\n");
    if(stopDMXeit())
      return 0;
    if(stopDMXsdt())
      return 0;
//    if(stopDMXeitNVOD())
//      return 0;
    if(debug) {
      // Speicher-Info abfragen
      struct mallinfo speicherinfo=mallinfo();
      dprintf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo.uordblks);
      dprintf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %fMB)\n",speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024));
    }
    pthread_mutex_lock(&eventsLock);
//    unsigned anzEventsAlt=events.size();
/*
    pthread_mutex_lock(&servicesLock);
    events.mergeAndRemoveTimeShiftedEvents(services);
    pthread_mutex_unlock(&servicesLock);
    if(events.size()!=anzEventsAlt)
      printf("Removed %d time-shifted events.\n", anzEventsAlt-events.size());
*/
    unsigned anzEventsAlt=mySIeventsOrderEventID.size();
    removeOldEvents(oldEventsAre); // alte Events
    if(mySIeventsOrderEventID.size()!=anzEventsAlt)
      dprintf("Removed %d old events.\n", anzEventsAlt-mySIeventsOrderEventID.size());
    dprintf("Number of sptr events (event-ID): %u\n", mySIeventsOrderEventID.size());
    dprintf("Number of sptr events (service-id, start time, event-id): %u\n", mySIeventsOrderServiceIDFirstStartTimeEventID.size());
    dprintf("Number of sptr events (end time, service-id, event-id): %u\n", mySIeventsOrderFirstEndTimeServiceIDEventID.size());
    pthread_mutex_unlock(&eventsLock);
    if(debug) {
      pthread_mutex_lock(&servicesLock);
      dprintf("Number of services: %u\n", mySIservicesOrderServiceID.size());
//      dprintf("Number of services: %u\n", services.size());
      pthread_mutex_unlock(&servicesLock);
    }
    if(startDMXeit())
      return 0;
    if(startDMXsdt())
      return 0;
//    if(startDMXeitNVOD())
//      return 0;
    if(debug) {
      // Speicher-Info abfragen
      struct mallinfo speicherinfo=mallinfo();
      dprintf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo.uordblks);
      dprintf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %fMB)\n",speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024));
    }
  } // for endlos
}

static void printHelp(void)
{
    printf("\nUsage: sectionsd [-d]\n\n");
}

int main(int argc, char **argv)
{
pthread_t threadTOT, threadEIT, threadEITnvod, threadSDT, threadHouseKeeping;
int rc;
int listenSocket;
struct sockaddr_in serverAddr;

  printf("$Id: sectionsd.cpp,v 1.28 2001/07/20 00:02:47 fnbrd Exp $\n");

  if(argc!=1 && argc!=2) {
    printHelp();
    return 1;
  }
  if(argc==2) {
    if(!strcmp(argv[1], "-d"))
      debug=1;
    else {
      printHelp();
      return 1;
    }
  }
  printf("caching %ld hours\n", secondsToCache/(60*60L));
  printf("events are old %ldmin after their end time\n", oldEventsAre/60);
  tzset(); // TZ auswerten

  if( fork()!= 0 ) // switching to background
    return 0;

  // from here on forked

  // den Port f�r die Clients �ffnen
  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  memset( &serverAddr, 0, sizeof(serverAddr) );
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port = htons(PORT_NUMBER);
  if(bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr) )) {
    perror("[sectionsd] bind");
    return 2;
  }
  if(listen(listenSocket, 5)) { // max. 5 Clients
    perror("[sectionsd] listen");
    return 3;
  }

  // SDT-Thread starten

  rc=pthread_create(&threadSDT, 0, sdtThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create sdt-thread (rc=%d)\n", rc);
    return 1;
  }

  // EIT-Thread starten
  rc=pthread_create(&threadEIT, 0, eitThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create eit-thread (rc=%d)\n", rc);
    return 1;
  }

  // EIT-Thread (NVOD) starten
//  rc=pthread_create(&threadEITnvod, 0, eitNVODthread, 0);
//  if(rc) {
//    fprintf(stderr, "failed to create eit-thread (rc=%d)\n", rc);
//    return 1;
//  }

  // time-Thread starten
  rc=pthread_create(&threadTOT, 0, timeThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create time-thread (rc=%d)\n", rc);
    return 1;
  }

  // housekeeping-Thread starten
  rc=pthread_create(&threadHouseKeeping, 0, houseKeepingThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create houskeeping-thread (rc=%d)\n", rc);
    return 1;
  }

  pthread_attr_t conn_attrs;
  pthread_attr_init(&conn_attrs);
  pthread_attr_setdetachstate(&conn_attrs, PTHREAD_CREATE_DETACHED);
  // Unsere Endlosschliefe
  socklen_t clientInputLen = sizeof(serverAddr);
  for(;;) {
    // wir warten auf eine Verbindung
    struct connectionData *client=new connectionData; // Wird vom Thread freigegeben
    do {
      client->connectionSocket = accept(listenSocket, (struct sockaddr *)&(client->clientAddr), &clientInputLen);
    } while(client->connectionSocket == -1);
    pthread_t threadConnection;
    rc=pthread_create(&threadConnection, &conn_attrs, connectionThread, client);
    if(rc) {
      fprintf(stderr, "failed to create connection-thread (rc=%d)\n", rc);
      return 4;
    }
  }
  printf("sectionsd ended\n");
  return 0;
}

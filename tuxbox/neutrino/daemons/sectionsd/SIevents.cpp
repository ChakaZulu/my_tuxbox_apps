//
// $Id: SIevents.cpp,v 1.6 2001/06/11 19:22:54 fnbrd Exp $
//
// classes SIevent and SIevents (dbox-II-project)
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2001 fnbrd (fnbrd@gmx.de)
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
// $Log: SIevents.cpp,v $
// Revision 1.6  2001/06/11 19:22:54  fnbrd
// Events haben jetzt mehrere Zeiten, fuer den Fall von NVODs (cinedoms)
//
// Revision 1.5  2001/06/10 15:48:31  fnbrd
// Noch einen kleinen Fehler behoben.
//
// Revision 1.4  2001/06/10 14:55:51  fnbrd
// Kleiner Aenderungen und Ergaenzungen (epgMini).
//
// Revision 1.3  2001/05/20 14:40:15  fnbrd
// Mit parental_rating
//
// Revision 1.2  2001/05/19 22:46:50  fnbrd
// Jetzt wellformed xml.
//
//

#include <stdio.h>
#include <time.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <ost/dmx.h>

#include <set>
#include <algorithm>
#include <string>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

SIevent::SIevent(const struct eit_event *e)
{
  eventID=e->event_id;
  time_t startzeit=changeUTCtoCtime(((const unsigned char *)e)+2);
  unsigned dauer=0;
  if(e->duration!=0xffffff)
    dauer=((e->duration)>>20)*10*3600L+(((e->duration)>>16)&0x0f)*3600L+
      (((e->duration)>>12)&0x0f)*10*60L+(((e->duration)>>8)&0x0f)*60L+
      (((e->duration)>>4)&0x0f)*10+((e->duration)&0x0f);
  if(startzeit && dauer)
    times.insert(SItime(startzeit, dauer));
  serviceID=0;
}

// Std-Copy
SIevent::SIevent(const SIevent &e)
{
  eventID=e.eventID;
  name=e.name;
  text=e.text;
//  startzeit=e.startzeit;
//  dauer=e.dauer;
  times=e.times;
  serviceID=e.serviceID;
  itemDescription=e.itemDescription;
  item=e.item;
  extendedText=e.extendedText;
  contentClassification=e.contentClassification;
  userClassification=e.userClassification;
  components=e.components;
  ratings=e.ratings;
}

int SIevent::saveXML(FILE *file, const char *serviceName) const
{
  if(saveXML0(file))
    return 1;
  if(serviceName) {
    if(fprintf(file, "    <service_name>")<0)
      return 2;
    saveStringToXMLfile(file, serviceName);
    if(fprintf(file, "</service_name>\n")<0)
      return 3;
  }
  return saveXML2(file);
}

int SIevent::saveXML0(FILE *file) const
{
  if(fprintf(file, "  <event service_id=\"%hd\" event_id=\"%hd\">\n", serviceID, eventID)<0)
    return 1;
  return 0;
}

int SIevent::saveXML2(FILE *file) const
{
  if(name.length()) {
    fprintf(file, "    <name>");
    saveStringToXMLfile(file, name.c_str());
    fprintf(file, "</name>\n");
  }
  if(text.length()) {
    fprintf(file, "    <text>");
    saveStringToXMLfile(file, text.c_str());
    fprintf(file, "</text>\n");
  }
  if(item.length()) {
    fprintf(file, "    <item>");
    saveStringToXMLfile(file, item.c_str());
    fprintf(file, "</item>\n");
  }
  if(itemDescription.length()) {
    fprintf(file, "    <item_description>");
    saveStringToXMLfile(file, itemDescription.c_str());
    fprintf(file, "</item_description>\n");
  }
  if(extendedText.length()) {
    fprintf(file, "    <extended_text>");
    saveStringToXMLfile(file, extendedText.c_str());
    fprintf(file, "</extended_text>\n");
  }
/*
  if(startzeit) {
    struct tm *zeit=localtime(&startzeit);
    fprintf(file, "    <time>%02d:%02d:%02d</time>\n", zeit->tm_hour, zeit->tm_min, zeit->tm_sec);
    fprintf(file, "    <date>%02d.%02d.%04d</date>\n", zeit->tm_mday, zeit->tm_mon+1, zeit->tm_year+1900);
  }
  if(dauer)
    fprintf(file, "    <duration>%u</duration>\n", dauer);
*/
  for_each(times.begin(), times.end(), saveSItimeXML(file));
  for(unsigned i=0; i<contentClassification.length(); i++)
    fprintf(file, "    <content_classification>0x%02hhx</content_classification>\n", contentClassification[i]);
  for(unsigned i=0; i<userClassification.length(); i++)
    fprintf(file, "    <user_classification>0x%02hhx</user_classification>\n", userClassification[i]);
  for_each(components.begin(), components.end(), saveSIcomponentXML(file));
  for_each(ratings.begin(), ratings.end(), saveSIparentalRatingXML(file));
  fprintf(file, "  </event>\n");
  return 0;
}

void SIevent::dump(void) const
{
  if(serviceID)
    printf("Service-ID: %hu\n", serviceID);
  printf("Event-ID: %hu\n", eventID);
  if(item.length())
    printf("Item: %s\n", item.c_str());
  if(itemDescription.length())
    printf("Item-Description: %s\n", itemDescription.c_str());
  if(name.length())
    printf("Name: %s\n", name.c_str());
  if(text.length())
    printf("Text: %s\n", text.c_str());
  if(extendedText.length())
    printf("Extended-Text: %s\n", extendedText.c_str());
  if(contentClassification.length()) {
    printf("Content classification:");
    for(unsigned i=0; i<contentClassification.length(); i++)
      printf(" 0x%02hhx", contentClassification[i]);
    printf("\n");
  }
  if(userClassification.length()) {
    printf("User classification:");
    for(unsigned i=0; i<userClassification.length(); i++)
      printf(" 0x%02hhx", userClassification[i]);
    printf("\n");
  }
/*
  if(startzeit)
    printf("Startzeit: %s", ctime(&startzeit));
  if(dauer)
    printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
*/
  for_each(times.begin(), times.end(), printSItime());
  for_each(components.begin(), components.end(), printSIcomponent());
  for_each(ratings.begin(), ratings.end(), printSIparentalRating());
}

void SIevent::dumpSmall(void) const
{
  if(name.length())
    printf("Name: %s\n", name.c_str());
  if(text.length())
    printf("Text: %s\n", text.c_str());
  if(extendedText.length())
    printf("Extended-Text: %s\n", extendedText.c_str());
/*
  if(startzeit)
    printf("Startzeit: %s", ctime(&startzeit));
  if(dauer)
    printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
*/
  for_each(times.begin(), times.end(), printSItime());
  for_each(ratings.begin(), ratings.end(), printSIparentalRating());
}

// Liest n Bytes aus einem Socket per read
inline int readNbytes(int fd, char *buf, int n)
{
int j;
  for(j=0; j<n;) {
    int r=read (fd, buf, n-j);
    if(r<=0) {
      perror ("read");
      return -1;
    }
    j+=r;
    buf+=r;
  }
  return j;
}

SIevent SIevent::readActualEvent(unsigned short serviceID, unsigned timeoutInSeconds)
{
  int fd;
  SIevent evt; // Std-Event das bei Fehler zurueckgeliefert wird
  struct SI_section_header header;
  struct dmxSctFilterParams flt;
  char *buf;

  memset (&flt.filter, 0, sizeof (struct dmxFilter));

  flt.pid              = 0x12;
  flt.filter.filter[0] = 0x4e;
  flt.filter.mask[0]   = 0xff;
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return evt;
  }
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    close(fd);
    perror ("DMX_SET_FILTER");
    return evt;
  }

  time_t szeit=time(NULL);

//  printf("reading first\n");
  // Segment mit Event fuer sid suchen
  do {
    if(readNbytes(fd, (char *)&header, sizeof(header))<0) {
      close(fd);
      perror ("read header");
      return evt;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      close(fd);
      printf("Not enough memory!\n");
      return evt;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    if(readNbytes(fd, buf+sizeof(header), header.section_length-5)<0) {
      close(fd);
      delete[] buf;
      perror ("read section");
      return evt;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionEIT e(SIsection(sizeof(header)+header.section_length-5, buf));
      time_t zeit=time(NULL);
      for(SIevents::iterator k=e.events().begin(); k!=e.events().end(); k++)
        if(k->serviceID==serviceID)
          for(SItimes::iterator t=k->times.begin(); t!=k->times.end(); t++)
            if(t->startzeit<=zeit && zeit<=(long)(t->startzeit+t->dauer)) {
	      close(fd);
	      return SIevent(*k);
            }
    }
    else
      delete[] buf;
  } while (time(NULL)<szeit+(long)(timeoutInSeconds));
  close(fd);
  return evt;
}

// Entfernt anhand der Services alle time shifted events (ohne Text,
// mit service-id welcher im nvod steht)
// und sortiert deren Zeiten in die Events mit dem Text ein.
void SIevents::mergeAndRemoveTimeShiftedEvents(const SIservices &services)
{
  // Wir gehen alle services durch, suchen uns die services mit nvods raus
  // und fuegen dann die Zeiten der Events mit der service-id eines nvods
  // in das entsprechende Event mit der service-id das die nvods hat ein.
  // die 'nvod-events' werden auch geloescht

//  SIevents eventsToDelete; // Hier rein kommen Events die geloescht werden sollen
  for(SIservices::iterator k=services.begin(); k!=services.end(); k++)
    if(k->nvods.size()) {
      // NVOD-Referenzen gefunden
      // Zuerst mal das Event mit dem Text holen
//      iterator e;
      iterator e;
      for(e=begin(); e!=end(); e++)
        if(e->serviceID==k->serviceID)
          break;
      if(e!=end()) {
        // *e == event mit dem Text
	SIevent newEvent(*e); // Kopie des Events
	// Jetzt die nvods druchgehen und deren Uhrzeiten in obiges Event einfuegen
        for(SInvodReferences::iterator n=k->nvods.begin(); n!=k->nvods.end(); n++) {
          // Alle druchgehen und deren Events suchen
          for(iterator en=begin(); en!=end(); en++) {
	    if(en->serviceID==n->serviceID) {
              newEvent.times.insert(en->times.begin(), en->times.end());
//              newEvent.times.insert(SItime(en->startzeit, en->dauer));
//	      eventsToDelete.insert(SIevent(*en));
            }
	  }
        }
	erase(e); // Altes Event loeschen -> iterator (e) ung�ltig
	insert(newEvent); // und das erweiterte Event wieder einfuegen
      }
    }
  // Jetzt loeschen wir alle Events die eine Service-ID haben deren Service vom Typ 0 ist
  // Untenstehender Algo ist so zwar relativ langsam, aber es funktioniert
  // Bei Gelegenheit mach ich das mal anders
  for(;;) {
    int erased=0;
    for(iterator e=begin(); e!=end(); e++) {
      SIservices::iterator s=services.find(e->serviceID);
      if(s!=services.end())
        if(s->serviceTyp==0) {
	  erase(e); // -> e wird ungueltig
	  erased=1;
	  break;
        }
    }
    if(!erased)
      break;
  }
  // Hiermit loeschen wir ungewollte Events (oben aussortierte)
//  for(iterator e=eventsToDelete.begin(); e!=eventsToDelete.end(); e++)
//    erase(*e);
}

//
// $Id: SIevents.cpp,v 1.2 2001/05/19 22:46:50 fnbrd Exp $
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
// Revision 1.2  2001/05/19 22:46:50  fnbrd
// Jetzt wellformed xml.
//
//

#include <stdio.h>
#include <time.h>

#include <set>
#include <algorithm>
#include <string>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"

SIevent::SIevent(const struct eit_event *e)
{
  eventID=e->event_id;
  startzeit=changeUTCtoCtime(((const unsigned char *)e)+2);
  if(e->duration==0xffffff)
    dauer=0; // keine Dauer
  else
    dauer=((e->duration)>>20)*10*3600L+(((e->duration)>>16)&0x0f)*3600L+
      (((e->duration)>>12)&0x0f)*10*60L+(((e->duration)>>8)&0x0f)*60L+
      (((e->duration)>>4)&0x0f)*10+((e->duration)&0x0f);
  serviceID=0;
}

// Std-Copy
SIevent::SIevent(const SIevent &e)
{
  eventID=e.eventID;
  name=e.name;
  text=e.text;
  startzeit=e.startzeit;
  dauer=e.dauer;
  serviceID=e.serviceID;
  itemDescription=e.itemDescription;
  item=e.item;
  extendedText=e.extendedText;
  contentClassification=e.contentClassification;
  userClassification=e.userClassification;
  components=e.components;
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
  if(startzeit) {
    struct tm *zeit=localtime(&startzeit);
    fprintf(file, "    <time>%02d:%02d:%02d</time>\n", zeit->tm_hour, zeit->tm_min, zeit->tm_sec);
    fprintf(file, "    <date>%02d.%02d.%04d</date>\n", zeit->tm_mday, zeit->tm_mon+1, zeit->tm_year+1900);
  }
  if(dauer)
    fprintf(file, "    <duration>%u</duration>\n", dauer);
  for(unsigned i=0; i<contentClassification.length(); i++)
    fprintf(file, "    <content_classification>0x%02hhx</content_classification>\n", contentClassification[i]);
  for(unsigned i=0; i<userClassification.length(); i++)
    fprintf(file, "    <user_classification>0x%02hhx</user_classification>\n", userClassification[i]);
  for_each(components.begin(), components.end(), saveSIcomponentXML(file));
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
  if(startzeit)
    printf("Startzeit: %s", ctime(&startzeit));
  if(dauer)
    printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
  printf("\n");
  for_each(components.begin(), components.end(), printSIcomponent());
}

void SIevent::dumpSmall(void) const
{
  if(name.length())
    printf("Name: %s\n", name.c_str());
  if(text.length())
    printf("Text: %s\n", text.c_str());
  if(extendedText.length())
    printf("Extended-Text: %s\n", extendedText.c_str());
  if(startzeit)
    printf("Startzeit: %s", ctime(&startzeit));
  if(dauer)
    printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
  printf("\n");
}

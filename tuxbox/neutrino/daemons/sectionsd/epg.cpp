//
// $Id: epg.cpp,v 1.12 2001/05/16 15:23:47 fnbrd Exp $
//
// Beispiel zur Benutzung der SI class lib (dbox-II-project)
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
// $Log: epg.cpp,v $
// Revision 1.12  2001/05/16 15:23:47  fnbrd
// Alles neu macht der Mai.
//
// Revision 1.11  2001/05/16 07:17:10  fnbrd
// SDT geht, epg erweitert.
//
// Revision 1.10  2001/05/15 19:51:55  fnbrd
// epgSmall funktioniert jetzt zufriedenstellend.
//
// Revision 1.9  2001/05/15 05:02:55  fnbrd
// Weiter gearbeitet.
//
// Revision 1.8  2001/05/14 13:44:23  fnbrd
// Erweitert.
//
// Revision 1.7  2001/05/13 12:42:00  fnbrd
// Unnoetiges Zeug entfernt.
//
// Revision 1.6  2001/05/13 12:37:11  fnbrd
// Noch etwas verbessert.
//
// Revision 1.5  2001/05/13 00:39:30  fnbrd
// Etwas aufgeraeumt.
//
// Revision 1.4  2001/05/13 00:08:54  fnbrd
// Kleine Debugausgabe dazu.
//
// Revision 1.2  2001/05/12 23:55:04  fnbrd
// Ueberarbeitet, geht aber noch nicht ganz.
//

//#define READ_PRESENT_INFOS

#include <stdio.h>
#include <time.h>

#include <set>
#include <algorithm>
#include <string>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

int main(int argc, char **argv)
{
  time_t starttime, endtime;
#ifdef READ_PRESENT_INFOS
  SIsectionsEIT epgset;
  SIsectionsSDT sdtset;
#else
  SIsectionsEITschedule epgset;
#endif

  tzset(); // TZ auswerten

  starttime=time(NULL);
  epgset.readSections();
#ifdef READ_PRESENT_INFOS
  sdtset.readSections();
#endif
  endtime=time(NULL);
  printf("EIT Sections read: %d\n", epgset.size());
#ifdef READ_PRESENT_INFOS
  printf("SDT Sections read: %d\n", sdtset.size());
#endif
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
//  for_each(epgset.begin(), epgset.end(), printSmallSectionHeader());
//  for_each(epgset.begin(), epgset.end(), printSIsection());
#ifdef READ_PRESENT_INFOS
  SIevents events;
  for(SIsectionsEIT::iterator k=epgset.begin(); k!=epgset.end(); k++)
    events.insert((k->events()).begin(), (k->events()).end());

  SIservices services;
  for(SIsectionsSDT::iterator k=sdtset.begin(); k!=sdtset.end(); k++)
    services.insert((k->services()).begin(), (k->services()).end());

  for_each(events.begin(), events.end(), printSIeventWithService(services));
//  for_each(events.begin(), events.end(), printSIevent());
//  for_each(epgset.begin(), epgset.end(), printSIsectionEIT());
#endif
  return 0;
}

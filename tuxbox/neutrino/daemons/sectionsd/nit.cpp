//
// $Id: nit.cpp,v 1.5 2003/03/14 04:27:55 obi Exp $
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
// $Log: nit.cpp,v $
// Revision 1.5  2003/03/14 04:27:55  obi
// make it compileable with -W -Werror
//
// Revision 1.4  2001/06/11 19:22:54  fnbrd
// Events haben jetzt mehrere Zeiten, fuer den Fall von NVODs (cinedoms)
//
// Revision 1.3  2001/05/19 20:15:08  fnbrd
// Kleine Aenderungen (und epgXML).
//
// Revision 1.2  2001/05/16 15:23:47  fnbrd
// Alles neu macht der Mai.
//
// Revision 1.1  2001/05/14 13:45:32  fnbrd
// Erweitert.
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
#include "SIsections.hpp"

int main(int /*argc*/, char** /*argv*/)
{
  time_t starttime, endtime;
  SIsectionsNIT nitset;

  starttime=time(NULL);
  nitset.readSections();
  endtime=time(NULL);
  printf("Sections read: %d\n", nitset.size());
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
//  for_each(nitset.begin(), nitset.end(), printSmallSectionHeader());
//  for_each(nitset.begin(), nitset.end(), printSIsection());
  for_each(nitset.begin(), nitset.end(), printSIsectionNIT());
  return 0;
}

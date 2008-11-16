/*
  Neutrino-GUI  -   DBoxII-Project

  Part of Movieplayer (c) 2003, 2004 by gagga
  Based on code by Zwen. Thanks.

  $Id: bookmarkmanager.cpp,v 1.15 2008/11/16 21:46:40 seife Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/bookmarkmanager.h>

#include <global.h>
#include <neutrino.h>

#include <system/settings.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define info_height 60


CBookmark::CBookmark(const std::string & inName, const std::string & inUrl, const std::string & inTime)
{
	name = inName;
	url = inUrl;
	time = inTime;
}

//------------------------------

int CBookmarkManager::addBookmark (CBookmark inBookmark) {
	if (bookmarks.size() < MAXBOOKMARKS)
	{
		bookmarks.push_back(inBookmark);
		printf("CBookmarkManager: addBookmark: %s %s\n", inBookmark.getName(), inBookmark.getTime());
		bookmarksmodified = true;
		return 0;
	}
    // TODO:show dialog to delete old bookmark
    return -1;    
}

//------------------------------------------------------------------------
inline int CBookmarkManager::createBookmark (const std::string & name, const std::string & url, const std::string & time) {
	return addBookmark(CBookmark(name, url, time));
}

int CBookmarkManager::createBookmark (const std::string & url, const std::string & time) {
    char bookmarkname[26]="";
    CStringInputSMS bookmarkname_input(LOCALE_MOVIEPLAYER_BOOKMARKNAME, bookmarkname, 25, LOCALE_MOVIEPLAYER_BOOKMARKNAME_HINT1, LOCALE_MOVIEPLAYER_BOOKMARKNAME_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-_");
    bookmarkname_input.exec(NULL, "");
    // TODO: return -1 if no name was entered
    return createBookmark(std::string(bookmarkname), url, time);
}

//------------------------------------------------------------------------

void CBookmarkManager::removeBookmark (unsigned int index) {
	std::vector<CBookmark>::iterator p = bookmarks.begin()+index;
	bookmarks.erase(p);
	bookmarksmodified=true;
}

//------------------------------------------------------------------------

void CBookmarkManager::renameBookmark (unsigned int index) {
	if (index >= bookmarks.size())
		return;

	CBookmark & theBookmark = bookmarks[index];
	char bookmarkname[26];
	strncpy (bookmarkname, theBookmark.getName(), 25);
	CStringInputSMS bookmarkname_input(LOCALE_MOVIEPLAYER_BOOKMARKNAME, bookmarkname, 25, LOCALE_MOVIEPLAYER_BOOKMARKNAME_HINT1, LOCALE_MOVIEPLAYER_BOOKMARKNAME_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-_");
	bookmarkname_input.exec(NULL, "");

	if (strcmp(theBookmark.getName(), bookmarkname) != 0)
	{
		theBookmark.setName(std::string(bookmarkname));
		bookmarksmodified=true;
	}
}

#define BOOKMARKSTRINGLENGTH (10 + 1)
#define BOOKMARKSTRINGMODIFICATIONPOINT 8
const char * const BOOKMARKSTRING = "bookmark0.";

//------------------------------------------------------------------------
void CBookmarkManager::readBookmarkFile() {
	if (bookmarkfile.loadConfig(BOOKMARKFILE))
	{
		char bookmarkstring[BOOKMARKSTRINGLENGTH];
		strcpy(bookmarkstring, BOOKMARKSTRING);
	
		bookmarksmodified = false;
		bookmarks.clear();

		unsigned int bookmarkcount = bookmarkfile.getInt32("bookmarkcount", 0);

		if (bookmarkcount > MAXBOOKMARKS)
			bookmarkcount = MAXBOOKMARKS;

		while (bookmarkcount-- > 0)
		{
			std::string tmp = bookmarkstring;
			tmp += "name";
			std::string bookmarkname = bookmarkfile.getString(tmp, "");
			tmp = bookmarkstring;
			tmp += "url";
			std::string bookmarkurl = bookmarkfile.getString(tmp, "");
			tmp = bookmarkstring;
			tmp += "time";
			std::string bookmarktime = bookmarkfile.getString(tmp, "");
			
			bookmarks.push_back(CBookmark(bookmarkname, bookmarkurl, bookmarktime));

			bookmarkstring[BOOKMARKSTRINGMODIFICATIONPOINT]++;
		}
	}
	else
		bookmarkfile.clear();
}

//------------------------------------------------------------------------
void CBookmarkManager::writeBookmarkFile() {
	char bookmarkstring[BOOKMARKSTRINGLENGTH];
	strcpy(bookmarkstring, BOOKMARKSTRING);

	printf("CBookmarkManager: Writing bookmark file\n");
	
	for (std::vector<CBookmark>::const_iterator it = bookmarks.begin(); it != bookmarks.end(); it++)
	{
		std::string tmp = bookmarkstring;
		tmp += "name";
		bookmarkfile.setString(tmp, it->getName());
		tmp = bookmarkstring;
		tmp += "url";
		bookmarkfile.setString(tmp, it->getUrl());
		tmp = bookmarkstring;
		tmp += "time";
		bookmarkfile.setString(tmp, it->getTime());

		bookmarkstring[BOOKMARKSTRINGMODIFICATIONPOINT]++;
	}
	bookmarkfile.setInt32("bookmarkcount", bookmarks.size());
	bookmarkfile.saveConfig(BOOKMARKFILE);
}

//------------------------------------------------------------------------

CBookmarkManager::CBookmarkManager() : bookmarkfile ('\t')
{
	readBookmarkFile();
}

//------------------------------------------------------------------------

CBookmarkManager::~CBookmarkManager () {
	flush();   
}

//------------------------------------------------------------------------

int CBookmarkManager::getBookmarkCount(void) const {
	return bookmarks.size();
}

//------------------------------------------------------------------------

int CBookmarkManager::getMaxBookmarkCount(void) const {
	return MAXBOOKMARKS;    
}

//------------------------------------------------------------------------

void CBookmarkManager::flush() {
    if (bookmarksmodified) {
        writeBookmarkFile();   
    }   
}

//------------------------------------------------------------------------

const CBookmark * CBookmarkManager::getBookmark(CMenuTarget* parent)
{
	if(parent)
	{
		parent->hide();
	}
	
	frameBuffer = CFrameBuffer::getInstance();
	visible = false;
	selected = 0;
	// Max
	width = 720;
	if(g_settings.screen_EndX-g_settings.screen_StartX < width)
		width=g_settings.screen_EndX-g_settings.screen_StartX-10;
	buttonHeight = 25;
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
	listmaxshow = (height-theight-0)/(fheight*2);
	liststart = 0;
	
	height = (g_settings.screen_EndY-g_settings.screen_StartY)-(info_height+50);
	listmaxshow = (height-theight-0)/(fheight*2);
	height = theight+0+listmaxshow*fheight*2;	// recalc height
	if(bookmarks.size() < listmaxshow)
	{
		listmaxshow=bookmarks.size();
		height = theight+0+listmaxshow*fheight*2;	// recalc height
	}
	if(selected==bookmarks.size() && !(bookmarks.empty()))
	{
		selected=bookmarks.size()-1;
		liststart = (selected/listmaxshow)*listmaxshow;
	}
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;


	int res = -1;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
	uint msg; uint data;

	bool loop=true;
	bool update=true;
	while(loop)
	{
		if(update)
		{
			hide();
			update=false;
			paint();
		}
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		if( ( msg == CRCInput::RC_timeout ) ||
			 ( msg == CRCInput::RC_home) )
		{ //Exit after timeout or cancel key
			res = -1;
			loop=false;
		}
		else if ((msg == CRCInput::RC_up) && !(bookmarks.empty()))
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = bookmarks.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ((msg == CRCInput::RC_down) && !(bookmarks.empty()))
		{
			int prevselected=selected;
			selected = (selected+1)%bookmarks.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ((msg == CRCInput::RC_ok) && !(bookmarks.empty()))
		{
    		res = selected;
    		loop=false;
		}
		else if((msg == CRCInput::RC_red) && !(bookmarks.empty()))
		{
			removeBookmark(selected);
			update=true;
		}
		else if((msg==CRCInput::RC_yellow) && !(bookmarks.empty()))
		{
			renameBookmark(selected);
			update=true;
		}
		else if((msg==CRCInput::RC_blue)||(msg==CRCInput::RC_green)||
				  (CRCInput::isNumeric(msg)) )
		{
			//Ignore
		}
		else if(msg==CRCInput::RC_setup)
		{
			res=-1;
			loop=false;
		}
		else if( msg == CRCInput::RC_help )
		{
			// TODO Add Help
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = menu_return::RETURN_EXIT_ALL;
			}
		}
	}
	hide();

	if ((res >=0) && (((unsigned int)res) < bookmarks.size()))
		return &bookmarks[res];
	else
		return NULL;
}

//------------------------------------------------------------------------
void CBookmarkManager::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight*2;
	
	uint8_t    color;
	fb_pixel_t bgcolor;

	if (pos & 1)
	{
		color   = COL_MENUCONTENTDARK;
		bgcolor = COL_MENUCONTENTDARK_PLUS_0;
	}
	else
	{
		color   = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
	}

	if (liststart + pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}

	int real_width=width;
	if(bookmarks.size()>listmaxshow)
	{
		real_width-=15; //scrollbar
	}
	
	frameBuffer->paintBoxRel(x,ypos, real_width, 2*fheight, bgcolor);
	if(liststart+pos<bookmarks.size())
	{
		CBookmark theBookmark = bookmarks[liststart+pos];
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+fheight, real_width-10, theBookmark.getName(), color, fheight, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+2*fheight, real_width-10, theBookmark.getUrl(), color, fheight, true); // UTF-8

		// LCD Display
		if(liststart+pos==selected)
		{
			CLCD::getInstance()->showMenuText(0, theBookmark.getName(), -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, theBookmark.getUrl(), -1, true); // UTF-8
		}
	}
}

//------------------------------------------------------------------------

void CBookmarkManager::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x, y, width, height+ info_height+ 5);
		visible = false;
	}
}

//------------------------------------------------------------------------
void CBookmarkManager::paintHead()
{
	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	frameBuffer->paintIcon("bookmarkmanager.raw",x+5,y+4);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+35,y+theight+0, width- 45, g_Locale->getText(LOCALE_BOOKMARKMANAGER_NAME), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ width- 30, y+ 5 );
}

const struct button_label BookmarkmanagerButtons[2] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_BOOKMARKMANAGER_DELETE },
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_BOOKMARKMANAGER_RENAME }
};

//------------------------------------------------------------------------
void CBookmarkManager::paintFoot()
{
	int ButtonWidth = (width - 20) / 4;
	frameBuffer->paintBoxRel(x,y+height, width,buttonHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);

	if (bookmarks.empty()) {
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x+width- 1* ButtonWidth + 10, y+height);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + width - 1 * ButtonWidth + 38, y + height + 24 - 2, ButtonWidth - 28, g_Locale->getText(LOCALE_BOOKMARKMANAGER_SELECT), COL_INFOBAR_SHADOW_PLUS_1, 0, true); // UTF-8
    }    	
	else
	{
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + height + 4, ButtonWidth, 2, BookmarkmanagerButtons);

		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x+width- 1* ButtonWidth + 10, y+height);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width-1 * ButtonWidth + 38, y+height+24 - 2, ButtonWidth- 28, g_Locale->getText(LOCALE_BOOKMARKMANAGER_SELECT), COL_INFOBAR, 0, true); // UTF-8
	}
}

//------------------------------------------------------------------------
void CBookmarkManager::paint()
{
	unsigned int page_nr = (listmaxshow == 0) ? 0 : (selected / listmaxshow);
	liststart = page_nr * listmaxshow;

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_BOOKMARKMANAGER_NAME));

	paintHead();
	
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}
	if(bookmarks.size()>listmaxshow)
	{
		int ypos = y+ theight;
		int sb = 2*fheight* listmaxshow;
		frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

		int sbc= ((bookmarks.size()- 1)/ listmaxshow)+ 1;

		frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ page_nr*(sb-4)/sbc, 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);
	}

	paintFoot();
	visible = true;
}

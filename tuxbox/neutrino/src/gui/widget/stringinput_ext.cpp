/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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


#include "stringinput_ext.h"

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>

#include "messagebox.h"


CExtendedInput::CExtendedInput(const char * const Name, char* Value, const char * const Hint_1, const char * const Hint_2, CChangeObserver* Observ, bool Localizing)
{
	frameBuffer = CFrameBuffer::getInstance();
	name = Name;
	value = Value;

	hint_1 = Hint_1 ? Hint_1 : "";
	hint_2 = Hint_2 ? Hint_2 : "";

	observ = Observ;

	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	iheight = g_Fonts->menu_info->getHeight();

	localizing = Localizing;
	width = g_Fonts->menu_title->getRenderWidth(localizing ? g_Locale->getText(name) : name, true)+20; // UTF-8
	height = hheight+ mheight+ 20;

	if ( hint_1.length()> 0 )
		height+= iheight;
	if ( hint_2.length()> 0 )
		height+= iheight;

	x = ((720-width)>>1);
	y = ((500-height)>>1);
}

void CExtendedInput::addInputField( CExtendedInput_Item* fld )
{
	inputFields.push_back(fld);
}


void CExtendedInput::calculateDialog()
{
	int ix = 0;
	int iy = 0;
	int maxX = 0;
	int maxY = 0;
	selectedChar = -1;
	for(unsigned int i=0; i<inputFields.size();i++)
	{
		inputFields[i]->init( ix, iy);
		inputFields[i]->setDataPointer( &value[i] );
		if ((selectedChar==-1) && (inputFields[i]->isSelectable()))
		{
			selectedChar = i;
		}
		maxX = ix>maxX?ix:maxX;
		maxY = iy>maxY?iy:maxY;
	}

	width = width>maxX+40?width:maxX+40;
	height = height>maxY+hheight+ mheight?height:maxY+hheight+ mheight;

	hintPosY = y + height -10;

	if ( hint_1.length()> 0 )
		height+= iheight;
	if ( hint_2.length()> 0 )
		height+= iheight;

	x = ((720-width)>>1);
	y = ((500-height)>>1);
}


int CExtendedInput::exec( CMenuTarget* parent, std::string )
{
	onBeforeExec();
	int res = menu_return::RETURN_REPAINT;
	char oldval[inputFields.size()+10], dispval[inputFields.size()+10];

	if (parent)
	{
		parent->hide();
	}

	strcpy(oldval, value);
	paint();

	uint msg; uint data;
	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );

	bool loop=true;
	while (loop)
	{
		if ( strcmp(value, dispval) != 0)
		{
			CLCD::getInstance()->showMenuText(1, value, selectedChar+1);
			strcpy(dispval, value);
		}

		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd, true );

		if (msg==CRCInput::RC_left)
		{
			if(selectedChar>0)
			{
				bool found = false;
				int oldSelectedChar = selectedChar;
				for(int i=selectedChar-1; i>=0;i--)
				{
					if ((!found) && (inputFields[i]->isSelectable()))
					{
						found = true;
						selectedChar = i;
					}
				}
				if(found)
				{
					inputFields[oldSelectedChar]->paint( x+20, y+hheight +20, false );
					inputFields[selectedChar]->paint( x+20, y+hheight +20, true );
					CLCD::getInstance()->showMenuText(1, value, selectedChar+1);
				}
			}
		}
		else if (msg==CRCInput::RC_right)
		{
			if(selectedChar< (int) inputFields.size()-1)
			{
				bool found = false;
				int oldSelectedChar = selectedChar;
				for(unsigned int i=selectedChar+1; i<inputFields.size();i++)
				{
					if ((!found) && (inputFields[i]->isSelectable()))
					{
						found = true;
						selectedChar = i;
					}
				}
				if(found)
				{
					inputFields[oldSelectedChar]->paint( x+20, y+hheight +20, false );
					inputFields[selectedChar]->paint( x+20, y+hheight +20, true );
					CLCD::getInstance()->showMenuText(1, value, selectedChar+1);
				}
			}
		}
		else if ( (CRCInput::getUnicodeValue(msg) != -1) || (msg == CRCInput::RC_red) || (msg == CRCInput::RC_green) || (msg == CRCInput::RC_blue) || (msg == CRCInput::RC_yellow)
					|| (msg == CRCInput::RC_up) || (msg == CRCInput::RC_down))
		{
			inputFields[selectedChar]->keyPressed(msg);
			inputFields[selectedChar]->paint( x+20, y+hheight +20, true );
		}
		else if (msg==CRCInput::RC_ok)
		{
			loop=false;
		}
		else if ( (msg==CRCInput::RC_home) || (msg==CRCInput::RC_timeout) )
		{
			if(strcmp(value, oldval)!= 0){
				int erg=ShowMsg(name, g_Locale->getText("messagebox.discard"), CMessageBox::mbrYes, CMessageBox::mbNo | CMessageBox::mbYes | CMessageBox::mbCancel, "", 380, -1, true); // UTF-8
				 if(erg==CMessageBox::mbrYes){
					strcpy(value, oldval);
					loop=false;
				 }
				 else if(erg==CMessageBox::mbrNo){
					 loop=false;
				 }
				 else if(erg==CMessageBox::mbrCancel){
				 }
			} else {
				//keine �nderungen - beenden ok
				loop=false;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}
	}

	hide();

	onAfterExec();

	if ( (observ) && (msg==CRCInput::RC_ok) )
	{
		observ->changeNotify( name, value );
	}

	return res;
}

void CExtendedInput::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CExtendedInput::paint()
{
	frameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width- 10, localizing ? g_Locale->getText(name) : name, COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x, y+ hheight, width, height-hheight, COL_MENUCONTENT);

	if (hint_1.length() > 0)
	{
		g_Fonts->menu_info->RenderString(x+ 20, hintPosY, width- 20, localizing ? g_Locale->getText(hint_1) : hint_1, COL_MENUCONTENT, 0, true); // UTF-8
		if (hint_2.length() > 0)
			g_Fonts->menu_info->RenderString(x+ 20, hintPosY + iheight, width- 20, localizing ? g_Locale->getText(hint_2) : hint_2, COL_MENUCONTENT, 0, true); // UTF-8
	}

	for(unsigned int i=0; i<inputFields.size();i++)
	{
		inputFields[i]->paint( x+20, y+hheight +20, (i== (unsigned int) selectedChar) );
	}


}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CExtendedInput_Item_Char::CExtendedInput_Item_Char(std::string Chars, bool Selectable )
{
	frameBuffer = CFrameBuffer::getInstance();
	idx = 20;
	idy = g_Fonts->menu->getHeight();
	allowedChars = Chars;
	selectable = Selectable;
}

void CExtendedInput_Item_Char::init(int &x, int &y)
{
	ix = x;
	iy = y;
	x += idx;
}

void CExtendedInput_Item_Char::setAllowedChars( std::string ac )
{
	allowedChars = ac;
}

void CExtendedInput_Item_Char::paint(int x, int y, bool focusGained )
{
	int startx = ix + x;
	int starty = iy + y;

	int color = COL_MENUCONTENT;
	if (focusGained)
		color = COL_MENUCONTENTSELECTED;

	frameBuffer->paintBoxRel( startx, starty, idx, idy, COL_MENUCONTENT+4);
	frameBuffer->paintBoxRel( startx+1, starty+1, idx-2, idy-2, color);

	char text[2];
	text[0] = *data;
	text[1] = 0;
	int xfpos = startx + 1 + ((idx- g_Fonts->menu->getRenderWidth( text ))>>1);

	g_Fonts->menu->RenderString(xfpos,starty+idy, idx, text, color);
}

bool CExtendedInput_Item_Char::isAllowedChar( char ch )
{
	return ( (int) allowedChars.find(ch) != -1);
}

int CExtendedInput_Item_Char::getCharID( char ch )
{
	return allowedChars.find(ch);
}

void CExtendedInput_Item_Char::keyPressed(const int key)
{
	int value = CRCInput::getUnicodeValue(key);
	if (value != -1)
	{
		if (isAllowedChar((char)value))
		{
			*data = (char)value;
			g_RCInput->postMsg( CRCInput::RC_right, 0 );
		}
	}
	else
	{
		unsigned int pos = getCharID( *data );
		if (key==CRCInput::RC_up)
		{
			if(pos<allowedChars.size()-1)
			{
				*data = allowedChars[pos+1];
			}
			else
			{
				*data = allowedChars[0];
			}
		}
		else if (key==CRCInput::RC_down)
		{
			if(pos>0)
			{
				*data = allowedChars[pos-1];
			}
			else
			{
				*data = allowedChars[allowedChars.size()-1];
			}
		}
	}
}

//-----------------------------#################################-------------------------------------------------------

CIPInput::CIPInput(const char * const Name, std::string &Value, const char * const Hint_1, const char * const Hint_2, CChangeObserver* Observ)
	: CExtendedInput(Name, IP, Hint_1, Hint_2, Observ)
{
	ip = &Value;
	frameBuffer = CFrameBuffer::getInstance();
	addInputField( new CExtendedInput_Item_Char("012") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("012") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("012") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("012") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_newLiner(30) );
	calculateDialog();
}

void CIPInput::onBeforeExec()
{
	if (ip->empty())
	{
		strcpy(value, "000.000.000.000");
		printf("[neutrino] value-before(2): %s\n", value);
		return;
	}
	unsigned char _ip[4];
	sscanf(ip->c_str(), "%hhu.%hhu.%hhu.%hhu", &_ip[0], &_ip[1], &_ip[2], &_ip[3]);
	sprintf( value, "%03hhu.%03hhu.%03hhu.%03hhu", _ip[0], _ip[1], _ip[2], _ip[3]);
}

void CIPInput::onAfterExec()
{
	int _ip[4];
	sscanf( value, "%3d.%3d.%3d.%3d", &_ip[0], &_ip[1], &_ip[2], &_ip[3] );
	sprintf( value, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
	if(strcmp(value,"0.0.0.0")==0)
	{
		(*ip) = "";
	}
	else
		(*ip) = value;
}

//-----------------------------#################################-------------------------------------------------------
CDateInput::CDateInput(const char * const Name, time_t* Time, const char * const Hint_1, const char * const Hint_2, CChangeObserver* Observ)
	: CExtendedInput(Name, "", Hint_1, Hint_2, Observ)
{
	time=Time;
	value= new char[20];
	struct tm *tmTime = localtime(time);
	sprintf( value, "%02d.%02d.%04d %02d:%02d", tmTime->tm_mday, tmTime->tm_mon+1,
				tmTime->tm_year+1900,
				tmTime->tm_hour, tmTime->tm_min);
	
	frameBuffer = CFrameBuffer::getInstance();
	addInputField( new CExtendedInput_Item_Char("0123") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char(".",false) );
	addInputField( new CExtendedInput_Item_Char("01") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char(".",false) );
	addInputField( new CExtendedInput_Item_Char("2",false) );
	addInputField( new CExtendedInput_Item_Char("0",false) );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("012") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char(":",false) );
	addInputField( new CExtendedInput_Item_Char("012345") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_newLiner(30) );
	calculateDialog();
}
CDateInput::~CDateInput()
{
	delete value;
}
void CDateInput::onBeforeExec()
{
	struct tm *tmTime = localtime(time);
	sprintf( value, "%02d.%02d.%04d %02d:%02d", tmTime->tm_mday, tmTime->tm_mon+1,
				tmTime->tm_year+1900,
				tmTime->tm_hour, tmTime->tm_min);
}

void CDateInput::onAfterExec()
{
	struct tm tmTime;
	sscanf( value, "%02d.%02d.%04d %02d:%02d", &tmTime.tm_mday, &tmTime.tm_mon,
				&tmTime.tm_year,
				&tmTime.tm_hour, &tmTime.tm_min);
	tmTime.tm_mon-=1;
	tmTime.tm_year-=1900;
	tmTime.tm_sec=0;

	if(tmTime.tm_year>129)
      tmTime.tm_year=129;
   if(tmTime.tm_year<0)
      tmTime.tm_year=0;
   if(tmTime.tm_mon>11)
      tmTime.tm_mon=11;
   if(tmTime.tm_mon<0)
      tmTime.tm_mon=0;
   if(tmTime.tm_mday>31) //-> eine etwas laxe pruefung, aber mktime biegt das wieder grade
      tmTime.tm_mday=31;
   if(tmTime.tm_mday<1)
      tmTime.tm_mday=1;
   if(tmTime.tm_hour>23)
      tmTime.tm_hour=23;
   if(tmTime.tm_hour<0)
      tmTime.tm_hour=0;
   if(tmTime.tm_min>59)
      tmTime.tm_min=59;
   if(tmTime.tm_min<0)
      tmTime.tm_min=0;
   if(tmTime.tm_sec>59)
      tmTime.tm_sec=59;
   if(tmTime.tm_sec<0)
      tmTime.tm_sec=0;
	*time=mktime(&tmTime);
	struct tm *tmTime2 = localtime(time);
	sprintf( value, "%02d.%02d.%04d %02d:%02d", tmTime2->tm_mday, tmTime2->tm_mon+1,
				tmTime2->tm_year+1900,
				tmTime2->tm_hour, tmTime2->tm_min);
}
//-----------------------------#################################-------------------------------------------------------

CMACInput::CMACInput(const char * const Name, char* Value, const char * const Hint_1, const char * const Hint_2, CChangeObserver* Observ)
	: CExtendedInput(Name, Value, Hint_1, Hint_2, Observ)
{
	frameBuffer = CFrameBuffer::getInstance();
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_Char("0123456789ABCDEF") );
	addInputField( new CExtendedInput_Item_newLiner(30) );
	calculateDialog();
}

void CMACInput::onBeforeExec()
{
	if(strcmp(value,"")==0)
	{
		strcpy(value, "00:00:00:00:00:00");
		printf("[neutrino] value-before(2): %s\n", value);
		return;
	}
	int _mac[6];
	sscanf( value, "%x:%x:%x:%x:%x:%x", &_mac[0], &_mac[1], &_mac[2], &_mac[3], &_mac[4], &_mac[5] );
	sprintf( value, "%02x:%02x:%02x:%02x:%02x:%02x", _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
}

void CMACInput::onAfterExec()
{
	int _mac[6];
	sscanf( value, "%x:%x:%x:%x:%x:%x", &_mac[0], &_mac[1], &_mac[2], &_mac[3], &_mac[4], &_mac[5] );
	sprintf( value, "%02x:%02x:%02x:%02x:%02x:%02x", _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
	if(strcmp(value,"00:00:00:00:00:00")==0)
	{
		strcpy(value, "");
	}
}

//-----------------------------#################################-------------------------------------------------------

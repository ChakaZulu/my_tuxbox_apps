/*
	Neutrino-GUI  -   DBoxII-Project

	Timerliste by Zwen
	
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

#include "global.h"

#include "timerlist.h"
#include "neutrino.h"

#include "eventlist.h"

#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "daemonc/remotecontrol.h"
#include "system/settings.h"

#include "gui/widget/menue.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/progresswindow.h"
#include "gui/color.h"
#include "gui/infoviewer.h"



#define info_height 60



CTimerList::CTimerList()
{
	frameBuffer = CFrameBuffer::getInstance();
	visible = false;
	selected = 0;
	width = 505;
	buttonHeight = 25;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->menu->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
	liststart = 0;
	Timer = new CTimerdClient();
}

CTimerList::~CTimerList()
{
	timerlist.clear();
	delete Timer;
}

int CTimerList::exec(CMenuTarget* parent, string actionKey)
{
	if (parent)
	{
		parent->hide();
	}

//	updateEvents();
	channellist.clear();
	int nNewChannel = show();

	if ( nNewChannel > -1)
	{
		return menu_return::RETURN_REPAINT;
	}
	else if ( nNewChannel == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}
}

void CTimerList::updateEvents(void)
{
	timerlist.clear();
	Timer->getTimerList (timerlist);
	height = 450;
	listmaxshow = (height-theight-0)/(fheight*2);
	height = theight+0+listmaxshow*fheight*2; // recalc height
	if(timerlist.size() < listmaxshow)
	{
		listmaxshow=timerlist.size();
		height = theight+0+listmaxshow*fheight*2; // recalc height
	}
	if(selected==timerlist.size())
	{
		selected=timerlist.size()-1;
		liststart = (selected/listmaxshow)*listmaxshow;
	}
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
}


int CTimerList::show()
{
	int res = -1;

/*!!!	g_lcdd->setMode(CLcddClient::MODE_MENU, g_Locale->getText(name) );*/

	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );
	uint msg; uint data;

	bool loop=true;
	bool update=true;
	while (loop)
	{
		if(update)
		{
			hide();
			updateEvents();
			update=false;
			if(timerlist.size()==0)
			{
				//evtl. anzeige dass keine kanalliste....
				ShowHint ( "messagebox.info", g_Locale->getText("timerlist.empty"));		
				return -1;
			}
			paint();
		}
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );

		if ( ( msg == CRCInput::RC_timeout ) ||
			 ( msg == CRCInput::RC_home) )
		{ //Exit after timeout or cancel key
			loop=false;
		}
		else if ( msg == CRCInput::RC_up )
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = timerlist.size()-1;
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
		else if ( msg == CRCInput::RC_down )
		{
			int prevselected=selected;
			selected = (selected+1)%timerlist.size();
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
		else if ( msg == CRCInput::RC_ok )
		{
			// OK button
		}
		else if(msg==CRCInput::RC_red)
		{
			Timer->removeTimerEvent(timerlist[selected].eventID);
			update=true;
		}
		else if((msg==CRCInput::RC_green) ||
				 (msg==CRCInput::RC_yellow) ||
				 (msg==CRCInput::RC_blue) ||
		         (CRCInput::isNumeric(msg)) )
		{
			//pushback key if...
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else if ( msg == CRCInput::RC_help )
		{
			// help key
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) == messages_return::cancel_all )
			{
				loop = false;
				res = - 2;
			}
		}
	}
	hide();

	/*!!!g_lcdd->setMode(CLcddClient::MODE_TVRADIO, g_Locale->getText(name) );*/

	return(res);
}

void CTimerList::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x, y, width, height+ info_height+ 5);
		visible = false;
	}
}

void CTimerList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight*2;
	int color;
	if(pos % 2)
		color = COL_MENUCONTENTDARK;
	else
		color	= COL_MENUCONTENT;

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	frameBuffer->paintBoxRel(x,ypos, width-15, 2*fheight, color);
	if(liststart+pos<timerlist.size())
	{
		CTimerd::responseGetTimer & timer = timerlist[liststart+pos];
      char zAlarmTime[25] = {0};
      struct tm *alarmTime = localtime(&(timer.alarmTime));
      strftime(zAlarmTime,20,"%d.%m. %H:%M",alarmTime);
      char zStopTime[25] = {0};
      struct tm *stopTime = localtime(&(timer.stopTime));
      strftime(zStopTime,20,"%d.%m. %H:%M",stopTime);
		g_Fonts->menu->RenderString(x+10,ypos+fheight, 160, zAlarmTime, color, fheight);
		if(timer.stopTime != 0)
		{			
			g_Fonts->menu->RenderString(x+10,ypos+2*fheight, 160, zStopTime, color, fheight);
		}
		g_Fonts->menu->RenderString(x+170,ypos+fheight, 165, convertTimerRepeat2String(timer.eventRepeat), color, fheight);
		g_Fonts->menu->RenderString(x+335,ypos+fheight, 155, convertTimerType2String(timer.eventType), color, fheight);
      string zAddData("");
      switch(timer.eventType)
      {
         case CTimerEvent::TIMER_NEXTPROGRAM :
         case CTimerEvent::TIMER_ZAPTO :
         case CTimerEvent::TIMER_RECORD :
         {
				if(channellist.size()==0)
				{
					CZapitClient *Zapit = new CZapitClient();
					Zapit->getChannels(channellist);
					delete Zapit;
				}
				CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
				for(; channel != channellist.end();channel++)
				{
					if(channel->onid_sid==timer.onidSid)
				   {
						zAddData = channel->name;
						break;
					}
               if(channel == channellist.end())
						zAddData=g_Locale->getText("timerlist.program.unknown");
            }
			}
			break;
			case CTimerEvent::TIMER_STANDBY :
			{
				if(timer.standby_on)
					zAddData = g_Locale->getText("timerlist.standby.on");
				else
					zAddData = g_Locale->getText("timerlist.standby.off");
			}
			break;
			default:{}
		}
		g_Fonts->menu->RenderString(x+170,ypos+2*fheight, 320, zAddData, color, fheight);
	}
}

void CTimerList::paintHead()
{
	string strCaption = g_Locale->getText("timerlist.name");

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width- 20, strCaption.c_str(), COL_MENUHEAD);

/*	frameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );
	if (bouquetList!=NULL)
		frameBuffer->paintIcon("dbox.raw", x+ width- 60, y+ 5 );*/
}

void CTimerList::paintFoot()
{
	int ButtonWidth = (width-28) / 4;
	frameBuffer->paintBoxRel(x,y+height, width,buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW);

	frameBuffer->paintIcon("rot.raw", x+width- 4* ButtonWidth - 20, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 4* ButtonWidth, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("timerlist.delete").c_str(), COL_INFOBAR);
/*
	frameBuffer->paintIcon("gruen.raw", x+width- 3* ButtonWidth - 30, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.add").c_str(), COL_INFOBAR);

	frameBuffer->paintIcon("gelb.raw", x+width- 2* ButtonWidth - 30, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 2* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.move").c_str(), COL_INFOBAR);
*/
}

void CTimerList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	paintHead();
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = 2*fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((timerlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);
	paintFoot();
	visible = true;
}

string CTimerList::convertTimerType2String(CTimerEvent::CTimerEventTypes type)
{
   switch(type)
   {
      case CTimerEvent::TIMER_SHUTDOWN : return g_Locale->getText("timerlist.type.shutdown");
      case CTimerEvent::TIMER_NEXTPROGRAM : return g_Locale->getText("timerlist.type.nextprogramm");
      case CTimerEvent::TIMER_ZAPTO : return g_Locale->getText("timerlist.type.zapto");
      case CTimerEvent::TIMER_STANDBY : return g_Locale->getText("timerlist.type.standby");
      case CTimerEvent::TIMER_RECORD : return g_Locale->getText("timerlist.type.record");
      case CTimerEvent::TIMER_REMIND : return g_Locale->getText("timerlist.type.remind");
      case CTimerEvent::TIMER_SLEEPTIMER: return g_Locale->getText("timerlist.type.sleeptimer");
      default: return g_Locale->getText("timerlist.type.unknown");
   }
}

string CTimerList::convertTimerRepeat2String(CTimerEvent::CTimerEventRepeat rep)
{
   switch(rep)
   {
      case CTimerEvent::TIMERREPEAT_ONCE : return g_Locale->getText("timerlist.repeat.once");
      case CTimerEvent::TIMERREPEAT_DAILY : return g_Locale->getText("timerlist.repeat.daily");
      case CTimerEvent::TIMERREPEAT_WEEKLY : return g_Locale->getText("timerlist.repeat.weekly");
      case CTimerEvent::TIMERREPEAT_BIWEEKLY : return g_Locale->getText("timerlist.repeat.biweekly");
      case CTimerEvent::TIMERREPEAT_FOURWEEKLY : return g_Locale->getText("timerlist.repeat.fourweekly");
      case CTimerEvent::TIMERREPEAT_MONTHLY : return g_Locale->getText("timerlist.repeat.monthly");
      case CTimerEvent::TIMERREPEAT_BYEVENTDESCRIPTION : return g_Locale->getText("timerlist.repeat.byeventdescription");
      default: return g_Locale->getText("timerlist.repeat.unknown");
	}
}


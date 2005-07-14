/*
	Neutrino-GUI  -   DBoxII-Project


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

#define SCREEN_X	720
#define SCREEN_Y	572

#include <gui/imageinfo.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <daemonc/remotecontrol.h>

#include <system/flashtool.h>

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CImageInfo::CImageInfo()
{
	frameBuffer = CFrameBuffer::getInstance();

	font_head   = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	font_small  = SNeutrinoSettings::FONT_TYPE_IMAGEINFO_SMALL;
	font_info   = SNeutrinoSettings::FONT_TYPE_IMAGEINFO_INFO;

	hheight     = g_Font[font_head]->getHeight();
	iheight     = g_Font[font_info]->getHeight();
	sheight     = g_Font[font_small]->getHeight();

	width  = w_max (710, 5);
	height = h_max (560, 5);

	max_height = SCREEN_Y-1;
	max_width  = SCREEN_X-1;

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CImageInfo::~CImageInfo()
{
	delete pig;
}

int CImageInfo::exec(CMenuTarget* parent, const std::string &)
{
	if (parent)
	{
 		parent->hide();
	}

	paint();

	pig = new CPIG (0);
 	paint_pig( width-170, y, 215, 170);

	neutrino_msg_t msg;

	while (1)
	{
		neutrino_msg_data_t data;
		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd_MS(100);
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (msg <= CRCInput::RC_MaxRC)
		{
			break;
		}

		if ( msg >  CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
	}

	delete pig;
	hide();

	return menu_return::RETURN_REPAINT;
}

void CImageInfo::hide()
{
	pig->hide();
	frameBuffer->paintBackgroundBoxRel(0,0, max_width,max_height);
}

void CImageInfo::paint_pig(int x, int y, int w, int h)
{
  	frameBuffer->paintBoxRel(x,y,w,h, COL_BACKGROUND);
	pig->show (x,y,w,h);
}

void CImageInfo::paintLine(int xpos, int font, const char* text)
{
	char buf[100];
	sprintf((char*) buf, "%s", text);
	g_Font[font]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true);
}

void CImageInfo::paint()
{
	const char * head_string;
	const char * releaseCycle;
	char imagedate[18] = "";
 	int  xpos = x+10;

	ypos = y+5;

	head_string = g_Locale->getText(LOCALE_IMAGEINFO_HEAD);
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, head_string);

	frameBuffer->paintBoxRel(0, 0, max_width, max_height, COL_MENUHEAD_PLUS_0);
	g_Font[font_head]->RenderString(xpos, ypos+ hheight+1, width, head_string, COL_MENUHEAD, 0, true);

	ypos += hheight;
	ypos += (iheight >>1);


	CConfigFile config('\t');
	config.loadConfig("/.version");

	const char * homepage  = config.getString("homepage",  "n/a").c_str();
	const char * creator   = config.getString("creator",   "n/a").c_str();
	const char * imagename = config.getString("imagename", "self compiled").c_str();
	const char * version   = config.getString("version",   "????????????????").c_str();

	static CFlashVersionInfo versionInfo(version);

	sprintf((char*) releaseCycle, "%s (%s)", versionInfo.getReleaseCycle() ,versionInfo.getType());
	sprintf((char*) imagedate, "%s  %s", versionInfo.getDate(), versionInfo.getTime());

	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGE));
	paintLine(xpos+125, font_info, imagename);

	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DATE));
	paintLine(xpos+125, font_info, imagedate);

	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_VERSION));
	paintLine(xpos+125, font_info, releaseCycle);

	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CREATOR));
	paintLine(xpos+125, font_info, creator);

	ypos += iheight+10;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_HOMEPAGE));
	paintLine(xpos+125, font_info, homepage);

	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DOKUMENTATION));
	paintLine(xpos+125, font_info, "http://wiki.tuxbox.org");

	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_FORUM));
	paintLine(xpos+125, font_info, "http://forum.tuxbox.org");

	ypos += iheight+10;
	paintLine(xpos, font_info,g_Locale->getText(LOCALE_IMAGEINFO_LICENSE));
	paintLine(xpos+125, font_small, "This program is free software; you can redistribute it and/or modify");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "it under the terms of the GNU General Public License as published by");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "the Free Software Foundation; either version 2 of the License, or");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "(at your option) any later version.");

	ypos+= sheight+10;
	paintLine(xpos+125, font_small, "This program is distributed in the hope that it will be useful,");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "but WITHOUT ANY WARRANTY; without even the implied warranty of");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "See the GNU General Public License for more details.");

	ypos+= sheight+10;
	paintLine(xpos+125, font_small, "You should have received a copy of the GNU General Public License");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "along with this program; if not, write to the Free Software");

	ypos+= sheight;
	paintLine(xpos+125, font_small, "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.");
}

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

//
// -- this module is a evil hack
// -- Neutrino lacks a proper OSD class
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define SCREEN_X	720
#define SCREEN_Y	572


#include <gui/streaminfo2.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>
#include <gui/color.h>
#include <gui/widget/icons.h>

#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CStreamInfo2::CStreamInfo2()
{
	pig = new CPIG (0);
  	frameBuffer = CFrameBuffer::getInstance();


	font_head = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	font_info = SNeutrinoSettings::FONT_TYPE_MENU;
	font_small = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;

	hheight     = g_Font[font_head]->getHeight();
	iheight     = g_Font[font_info]->getHeight();
	sheight     = g_Font[font_small]->getHeight();

	width  = w_max (710, 5);
	height = h_max (560, 5); 

	max_height = SCREEN_Y-1;
	max_width  = SCREEN_X-1;


    	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	frameBuffer->paletteSetColor(COL_WHITE,   0x00FFFFFF, 0);
	frameBuffer->paletteSetColor(COL_RED,     0x00FF0000, 0);
	frameBuffer->paletteSetColor(COL_GREEN,   0x0000FF00, 0);
	frameBuffer->paletteSetColor(COL_BLUE,    0x000000FF, 0);
	frameBuffer->paletteSetColor(COL_YELLOW,  0x0000FFFF, 0);
	frameBuffer->paletteSetColor(COL_BLACK,   0x00000000, 0);

	frameBuffer->paletteSet();

	sigBox_pos = 0;

}



CStreamInfo2::~CStreamInfo2()
{
	delete pig;

}





int CStreamInfo2::exec(CMenuTarget* parent, const std::string &)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int res = g_RCInput->messageLoop();

	hide();

        res = menu_return::RETURN_EXIT_ALL;
	return res;
}




void CStreamInfo2::hide()
{
	pig->hide();
	frameBuffer->paintBackgroundBoxRel(0,0, max_width,max_height);
}



void CStreamInfo2::paint_pig(int x, int y, int w, int h)
{
  	frameBuffer->paintBoxRel(x,y,w,h, COL_BACKGROUND);
	pig->show (x,y,w,h);
}




void CStreamInfo2::paint_signal_fe_box(int x, int y, int w, int h)
{
   int y1;
   int xd = w/3;


	g_Font[font_info]->RenderString(x, y+iheight, width-10,
			g_Locale->getText("streaminfo.signal"),
			COL_MENUCONTENT, 0, true);


	sigBox_x = x;
	sigBox_y = y+iheight;
	sigBox_w = w;
	sigBox_h = h-iheight*3;
  	frameBuffer->paintBoxRel(sigBox_x,sigBox_y,sigBox_w,sigBox_h, COL_BLACK);

	y1  = y + h;

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, x+2+xd*0 , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*0 , y1, 50, "BER", COL_MENUCONTENT, 0, true);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x+2+xd*1  , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*1, y1, 50, "SNR", COL_MENUCONTENT, 0, true);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, x+2+xd*2  , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*2,y1, 50, "SIG", COL_MENUCONTENT, 0, true);


	{
	  int i;  // test
	  for (i=0; i++ < 1000; ) {
		paint_signal_fe(200,30000,4444 );
	  }
	}

}




void CStreamInfo2::paint_signal_fe(long ber, long snr, long sig )
{
   int   x_now = sigBox_pos;


	sigBox_pos = (++sigBox_pos) % sigBox_w;

	frameBuffer->paintVLine(sigBox_x+sigBox_pos,sigBox_y,sigBox_y+sigBox_h,COL_WHITE);
	frameBuffer->paintVLine(sigBox_x+x_now,sigBox_y,sigBox_y+sigBox_h,COL_BLACK);



	// test
frameBuffer->paintLine(sigBox_x+10, sigBox_y+20, sigBox_w+sigBox_x-10, sigBox_h+sigBox_y, COL_BLUE);
x_now = ber+snr+sig;


}





void CStreamInfo2::paint()
{
	const char * head_string;
	int ypos = y;
	int xpos = x;




	head_string = g_Locale->getText("streaminfo.head");

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, head_string);

	// paint backround, title pig, etc.
	frameBuffer->paintBoxRel(0, 0, max_width, max_height, COL_MENUHEAD_PLUS_0);

	g_Font[font_head]->RenderString(xpos, ypos+ hheight+1, width, head_string, COL_MENUHEAD, 0, true); // UTF-8

	ypos+= hheight;



	// paint PIG
	paint_pig( width-240,  y+10 , 240, 190);



	// Info Output
	ypos += (iheight >>1);
	paint_techinfo ( xpos, ypos);

	paint_signal_fe_box ( width-240,  (y + 190 + hheight), 240, 190);

}






void CStreamInfo2::paint_techinfo(int xpos, int ypos)
{


	// Info Output

	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return;
	}

	long bitInfo[10];

	char *key,*tmpptr,buf[100], buf2[100];
	long value;
	int pos=0;
	fgets(buf,35,fd);//dummy
	while(!feof(fd))
	{
		if(fgets(buf,35,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			key=strsep(&tmpptr,":");
			value=strtoul(tmpptr,NULL,0);
			bitInfo[pos]= value;
			pos++;
		}
	}
	fclose(fd);


	ypos+= iheight;
	sprintf((char*) buf, "%s: %dx%d", g_Locale->getText("streaminfo.resolution"), (int)bitInfo[0], (int)bitInfo[1] );
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8


	ypos += iheight;
	sprintf((char*) buf, "%s: %d bits/sec", g_Locale->getText("streaminfo.bitrate"), (int)bitInfo[4]*50);
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8


	ypos += iheight;
	switch (bitInfo[2])
	{
	case 2:
		sprintf((char*) buf, "%s: 4:3", g_Locale->getText("streaminfo.aratio"));
		break;
	case 3:
		sprintf((char*) buf, "%s: 16:9", g_Locale->getText("streaminfo.aratio"));
		break;
	case 4:
		sprintf((char*) buf, "%s: 2.21:1", g_Locale->getText("streaminfo.aratio"));
		break;
	default:
		strncpy(buf, g_Locale->getText("streaminfo.aratio_unknown"), sizeof(buf));
	}
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8



	ypos+= iheight;
	switch ( bitInfo[3] )
	{
			case 3:
			sprintf((char*) buf, "%s: 25fps", g_Locale->getText("streaminfo.framerate"));
			break;
			case 6:
			sprintf((char*) buf, "%s: 50fps", g_Locale->getText("streaminfo.framerate"));
			break;
			default:
			strncpy(buf, g_Locale->getText("streaminfo.framerate_unknown"), sizeof(buf));
	}
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8



	if (!bitInfo[7]) strncpy(buf, g_Locale->getText("streaminfo.audiotype_unknown"), sizeof(buf));
	else {
		const char* layernames[4]={"res","III","II","I"};
		const char* sampfreqnames[4]={"44,1k","48k","32k","res"};
		const char* modenames[4]={"stereo","joint_st","dual_ch","single_ch"};

		long header = bitInfo[7];

		unsigned char layer =	(header>>17)&3;
		unsigned char sampfreq = (header>>10)&3;
		unsigned char mode =	(header>> 6)&3;
		unsigned char copy =	(header>> 3)&1;

		sprintf((char*) buf, "%s: %s (%s/%s) %s", g_Locale->getText("streaminfo.audiotype"),
								modenames[mode],
								sampfreqnames[sampfreq],
								layernames[layer],
								copy?"c":"");
	}
	g_Font[font_info]->RenderString(xpos, ypos+ iheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= iheight+ 10;




	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();

	//onid
	ypos+= iheight;
	sprintf((char*) buf, "%s: 0x%04x", "ONid", si.onid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//sid
	ypos+= sheight;
	sprintf((char*) buf, "%s: 0x%04x", "Sid", si.sid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//tsid
	ypos+= sheight;
	sprintf((char*) buf, "%s: 0x%04x", "TSid", si.tsid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//tsfrequenz
	ypos+= sheight;
	sprintf((char*) buf, "%s: %d.%d MHz (%c)", "Freq", si.tsfrequency/1000, si.tsfrequency%1000,
			(si.polarisation == HORIZONTAL) ? 'h' : 'v');
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//vpid
	ypos+= sheight;
	if ( g_RemoteControl->current_PIDs.PIDs.vpid == 0 )
		sprintf((char*) buf, "%s: %s", "Vpid", g_Locale->getText("streaminfo.not_available"));
	else
		sprintf((char*) buf, "%s: 0x%04x", "Vpid", g_RemoteControl->current_PIDs.PIDs.vpid );
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//apid	
	ypos+= sheight;
	if (g_RemoteControl->current_PIDs.APIDs.empty())
		sprintf((char*) buf, "%s: %s", "Apid(s)", g_Locale->getText("streaminfo.not_available"));
	else
	{
		sprintf((char*) buf, "%s: ", "Apid(s)" );
		for (unsigned int i= 0; i< g_RemoteControl->current_PIDs.APIDs.size(); i++)
		{
			sprintf((char*) buf2, " 0x%04x",  g_RemoteControl->current_PIDs.APIDs[i].pid );

			if (i > 0)
			{
				strcat((char*) buf, ",");
				strcat((char*) buf, buf2+4);
			}
			else
				strcat((char*) buf, buf2);
		}
	}
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//vtxtpid
	if ( g_RemoteControl->current_PIDs.PIDs.vtxtpid == 0 )
        	sprintf((char*) buf, "%s: %s", "VTXTpid", g_Locale->getText("streaminfo.not_available"));
	else
        	sprintf((char*) buf, "%s: 0x%04x", "VTXTpid", g_RemoteControl->current_PIDs.PIDs.vtxtpid );
	g_Font[font_small]->RenderString(xpos, ypos+ iheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= sheight+ 10;
	
	//satellite
	sprintf((char*) buf, "Provider / Sat: %s",CNeutrinoApp::getInstance()->getScanSettings().satOfDiseqc(si.diseqc));
	g_Font[font_info]->RenderString(xpos, ypos+ iheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
}



























































































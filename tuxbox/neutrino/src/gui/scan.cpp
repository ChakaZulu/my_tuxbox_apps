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


#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>

#include "color.h"
#include "scan.h"

#include "widget/menue.h"
#include "widget/messagebox.h"


CScanTs::CScanTs()
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 500;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight + (10*mheight);		//space for infolines
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

int CScanTs::exec(CMenuTarget* parent, string)
{
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;

	frameBuffer->loadPal("scan.pal", 37, COL_MAXFREE);
	frameBuffer->loadPicture2FrameBuffer("scan.raw");

	g_Sectionsd->setPauseScanning( true );

	g_Zapit->setDiseqcType( CNeutrinoApp::getInstance()->getScanSettings().diseqcMode);
	g_Zapit->setDiseqcRepeat( CNeutrinoApp::getInstance()->getScanSettings().diseqcRepeat);
	g_Zapit->setScanBouquetMode( CNeutrinoApp::getInstance()->getScanSettings().bouquetMode);

	CZapitClient::ScanSatelliteList satList;
	CNeutrinoApp::getInstance()->getScanSettings().toSatList( satList);
	g_Zapit->setScanSatelliteList( satList);

	bool success = g_Zapit->startScan();

	paint();

	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
	ypos= y+ hheight + (mheight >>1);
	int xpos3 = 0;;
	if (g_info.delivery_system == DVB_S)
	{	//sat only
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.actsatellite").c_str(), COL_MENUCONTENT);
		xpos3 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.actsatellite").c_str());
	}
	if (g_info.delivery_system == DVB_C)		// maybe add DVB_T later:)
	{	//cable
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.actcable").c_str(), COL_MENUCONTENT);
		xpos3 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.actcable").c_str());
	}
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.transponders").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.freqdata").c_str(), COL_MENUCONTENT);
	ypos+= mheight;
	ypos+= mheight; // blank line

	ypos+= mheight;	//providername
	ypos+= mheight; // channelname

	ypos+= mheight; // blank line

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.servicenames").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	int xpos1 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.transponders").c_str());
 	int xpos2 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.services").c_str());
 	int xpos4 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.freqdata").c_str());
	int zwxpos = 0;

	if (xpos1 > zwxpos)
          zwxpos = xpos1;
	if (xpos2 > zwxpos)
          zwxpos = xpos2;
	if (xpos3 > zwxpos)
          zwxpos = xpos3;
	if (xpos4 > zwxpos)
          zwxpos = xpos4;

	xpos1 = zwxpos;
	xpos2 = zwxpos;
	xpos3 = zwxpos;
	xpos4 = zwxpos;


	frameBuffer->loadPal("radar.pal", 17, 37);
	int pos = 0;

	ypos= y+ hheight + (mheight >>1);

	uint msg;
	uint data;
	uint found_transponder = 0;
	bool istheend = !success;

	while (!istheend)
	{
		char filename[30];
		char cb[10];
		char cb1[21];
 		char cb2[5];
 		char cb3[30];
		sprintf(filename, "radar%d.raw", pos);
		pos = (pos+1)%10;
		frameBuffer->paintIcon8(filename, x+400,ypos+15, 17);

		unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd_MS( 250 );
		msg = CRCInput::RC_nokey;

		while ( ! ( msg == CRCInput::RC_timeout ) )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

			switch (msg)
			{

				case NeutrinoMessages::EVT_SCAN_SATELLITE:
					frameBuffer->paintBoxRel(xpos3, ypos, width-xpos3-145, mheight, COL_MENUCONTENT);//new position set
					g_Fonts->menu->RenderString(xpos3, ypos+mheight, width-xpos3, (char*)data, COL_MENUCONTENT);
					delete (unsigned char*) data;
					break;
	//todo: merge the follwing 2 cases:
				case NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS:	//willbe obsolete soon
					sprintf(cb, "%d", data);
					frameBuffer->paintBoxRel(xpos1, ypos+1*mheight, width-xpos1-105, mheight, COL_MENUCONTENT); //new position set
					g_Fonts->menu->RenderString(xpos1, ypos+ 2*mheight, width-xpos1, cb, COL_MENUCONTENT);
					found_transponder = data;
					break;

				case NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
					if (found_transponder == 0) data = 0;
					sprintf(cb1, "%d/%d", data,found_transponder);
					frameBuffer->paintBoxRel(xpos1, ypos+1*mheight, width-xpos1-105, mheight, COL_MENUCONTENT); // new position set
					g_Fonts->menu->RenderString(xpos1, ypos+ 2*mheight, width-xpos1, cb1, COL_MENUCONTENT);
					break;

				case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY:          //will be  enlarged to receive a structure
					sprintf(cb1, "%u",data );
					frameBuffer->paintBoxRel(xpos4, ypos+2*mheight,100, mheight, COL_MENUCONTENT); // new position set
					g_Fonts->menu->RenderString(xpos4, ypos+3*mheight,100, cb1, COL_MENUCONTENT);
					break;
				case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP:          //will be  enlarged to receive a structure
					if ( data == 0 )  {
						sprintf(cb2, "-H" );
					}
					else
						sprintf(cb2, "-V" );

					frameBuffer->paintBoxRel(xpos4+100,ypos+2*mheight,30, mheight, COL_MENUCONTENT); // new position set
					g_Fonts->menu->RenderString(xpos4+100, ypos+3*mheight,30, cb2, COL_MENUCONTENT);
					break;
				case NeutrinoMessages::EVT_SCAN_PROVIDER:
					frameBuffer->paintBoxRel(x+ 10, ypos+ 4* mheight+2, width-x-10, mheight, COL_MENUCONTENT);
					g_Fonts->menu->RenderString(x+ 10, ypos+ 5* mheight, width-x-10, (char*)data, COL_MENUCONTENT, 0, true); // UTF-8
					delete (unsigned char*) data;
					break;
				case NeutrinoMessages::EVT_SCAN_SERVICENAME:
					frameBuffer->paintBoxRel(x+ 8, ypos+ 5* mheight+2, width-x-10, mheight, COL_MENUCONTENT);
					g_Fonts->menu->RenderString(x+ 10, ypos+ 6* mheight, width-x-10, (char*)data, COL_MENUCONTENT, 0, true); // UTF-8
					delete (unsigned char*) data;
					break;
				case NeutrinoMessages::EVT_SCAN_NUM_CHANNELS:
					sprintf(cb, " = %d", data);
					frameBuffer->paintBoxRel(x +210, ypos+ 8*mheight, 70, mheight , COL_MENUCONTENT);   //ist nen bischen zu tief
					g_Fonts->menu->RenderString(x + 210, ypos+ 9*mheight,130, cb, COL_MENUCONTENT);
					break;
				case NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN:
					sprintf(cb, "%d", data);
					frameBuffer->paintBoxRel(x +8, ypos+8*mheight,60, mheight , COL_MENUCONTENT);   //ist nen bischen zu tief
					g_Fonts->menu->RenderString(x + 10, ypos+ 9* mheight, 70, cb, COL_MENUCONTENT);
					break;
/*  the goodguy hier ist der gui part
				case NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN:
					scaninfo * pommes;
					pommes = (scaninfo *) data;
					sprintf(cb1, "%d   %d    %d    =  %d",pommes->found_tv_chans,pommes->found_radio_chans,pommes->found_data_chans,pommes->found_tv_chans+pommes->found_radio_chans+pommes->found_data_chans);
					frameBuffer->paintBoxRel(x +8,ypos+8*mheight,60, mheight , COL_MENUCONTENT);   //ist nen bischen zu tief
					g_Fonts->menu->RenderString(x + 10, ypos+ 9* mheight, 260, cb1, COL_MENUCONTENT);
					sprintf(cb3, "%s" , pommes->ServiceName);
					//dprintf( DEBUG_NORMAL, "Also SN = %s buffer = %s \n",pommes->ServiceName,cb3 );

					frameBuffer->paintBoxRel(x+ 8, ypos+ 5* mheight+2, width-x-10, mheight, COL_MENUCONTENT);
					g_Fonts->menu->RenderString(x+ 10, ypos+ 6* mheight, width-x-10,cb3, COL_MENUCONTENT, 0, true); // UTF-8
					delete (unsigned char*) pommes;
					break;
*/
				case NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN:
					sprintf(cb, "%d", data);
					frameBuffer->paintBoxRel(x +68, ypos+8*mheight,60, mheight , COL_MENUCONTENT);   //ist nen bischen zu tief
					g_Fonts->menu->RenderString(x + 72, ypos+ 9* mheight, 40, cb, COL_MENUCONTENT);
					break;
				case NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN:
					sprintf(cb, "%d", data);
					frameBuffer->paintBoxRel(x +146, ypos+8*mheight,60, mheight , COL_MENUCONTENT);   //ist nen bischen zu tief
					g_Fonts->menu->RenderString(x + 148, ypos+ 9* mheight, 70, cb, COL_MENUCONTENT);
					break;
				case NeutrinoMessages::EVT_SCAN_COMPLETE:
				case NeutrinoMessages::EVT_SCAN_FAILED:
    					success  = (msg == NeutrinoMessages::EVT_SCAN_COMPLETE);
					istheend = true;
					msg      = CRCInput::RC_timeout;
					break;
				default:
					if ((msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) ) delete (unsigned char*) data;
					break;
			}
		}
	}

	hide();
	g_Sectionsd->setPauseScanning(false);
	ShowMsg("messagebox.info", success ? g_Locale->getText("scants.finished") : g_Locale->getText("scants.failed"), CMessageBox::mbBack, CMessageBox::mbBack, "info.raw", 450, -1, true); // UTF-8

	return menu_return::RETURN_REPAINT;
}

void CScanTs::hide()
{
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->paintBackgroundBoxRel(0,0, 720,576);
}


void CScanTs::paint()
{
	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("scants.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
}

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

#include "bouqueteditor_bouquets.h"
#include "messagebox.h"
#include "hintbox.h"
#include "bouqueteditor_channels.h"
#include "../global.h"

CBEBouquetWidget::CBEBouquetWidget()
{
	selected = 0;
	width = 500;
	height = 440;
	ButtonHeight = 25;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
	state = beDefault;
}

void CBEBouquetWidget::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);
	if ((liststart+pos==selected) && (state == beMoving))
	{
		g_FrameBuffer->paintIcon("gelb.raw", x + 8, ypos+4);
	}
	if(liststart+pos<Bouquets.size())
	{
		g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, Bouquets[liststart+pos].name, color);
	}
}

void CBEBouquetWidget::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;

	if(lastnum<10)
		numwidth = g_Fonts->channellist_number->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Fonts->channellist_number->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("00000");

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	g_FrameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((Bouquets.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}

void CBEBouquetWidget::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, "Bouquets" /*g_Locale->getText(name).c_str()*/, COL_MENUHEAD);
}

void CBEBouquetWidget::paintFoot()
{
	int ButtonWidth = width / 4;
	g_FrameBuffer->paintBoxRel(x,y+height, width,ButtonHeight, COL_MENUHEAD);
	g_FrameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW);

	g_FrameBuffer->paintIcon("rot.raw", x+width- 4* ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 4* ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.delete").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("gruen.raw", x+width- 3* ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.add").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("gelb.raw", x+width- 2* ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 2* ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.move").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("blau.raw", x+width- ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.rename").c_str(), COL_INFOBAR);

}

void CBEBouquetWidget::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height+ButtonHeight);
}

int CBEBouquetWidget::exec(CMenuTarget* parent, string actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

// getting all bouquets from zapit
	Bouquets.clear();
	g_Zapit->getBouquets(Bouquets, true);
	paintHead();
	paint();
	paintFoot();

	bouquetsChanged = false;
	int oldselected = selected;
	bool loop=true;
	while (loop)
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, g_settings.timing_epg );

		if ( (msg==CRCInput::RC_timeout) ||
			 (msg==g_settings.key_channelList_cancel))
		{
			if (state == beDefault)
			{
				if (bouquetsChanged)
				{
					CMessageBox* messageBox = new CMessageBox( "bouqueteditor.name", g_Locale->getText("bouqueteditor.savechanges?"), NULL, 480 );
					messageBox->exec( this, "");
					switch( messageBox->result)
					{
						case CMessageBox::mbrYes :
							loop=false;
							saveChanges();
						break;
						case CMessageBox::mbrNo :
							loop=false;
							discardChanges();
						break;
						case CMessageBox::mbrCancel :
							paintHead();
							paint();
							paintFoot();
						break;
					}
					delete messageBox;
				}
				else
				{
					loop = false;
				}
			}
			else if (state == beMoving)
			{
				cancelMoveBouquet();
			}
		}
		else if (msg==CRCInput::RC_up)
		{
			if (state == beDefault)
			{
				int prevselected=selected;
				if(selected==0)
				{
					selected = Bouquets.size()-1;
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
			else if (state == beMoving)
			{
				internalMoveBouquet(selected, selected - 1);
			}
		}
		else if (msg==CRCInput::RC_down)
		{
			if (state == beDefault)
			{
				int prevselected=selected;
				selected = (selected+1)%Bouquets.size();
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
			else if (state == beMoving)
			{
				internalMoveBouquet(selected, selected + 1);
			}
		}
		else if(msg==CRCInput::RC_red)
		{
			if (state == beDefault)
				deleteBouquet();
		}
		else if(msg==CRCInput::RC_green)
		{
			if (state == beDefault)
				addBouquet();
		}
		else if(msg==CRCInput::RC_yellow)
		{
			liststart = (selected/listmaxshow)*listmaxshow;
			if (state == beDefault)
				beginMoveBouquet();
			paintItem(selected - liststart);
		}
		else if(msg==CRCInput::RC_blue)
		{
			if (state == beDefault)
				renameBouquet();
		}
		else if(msg==CRCInput::RC_ok)
		{
			if (state == beDefault)
			{
				CBEChannelWidget* channelWidget = new CBEChannelWidget( Bouquets[ selected].name, selected + 1);
				channelWidget->exec( this, "");
				if (channelWidget->hasChanged())
					bouquetsChanged = true;
				delete channelWidget;
				paintHead();
				paint();
				paintFoot();
			}
			else if (state == beMoving)
			{
				finishMoveBouquet();
			}
		}
		else if( CRCInput::isNumeric(msg) )
		{
			if (state == beDefault)
			{
				//kein pushback - wenn man versehentlich wo draufkommt is die edit-arbeit umsonst
				//selected = oldselected;
				//g_RCInput->postMsg( msg, data );
				//loop=false;
			}
			else if (state == beMoving)
			{
				cancelMoveBouquet();
			}
		}
		else
		{
			neutrino->handleMsg( msg, data );
			// kein canceling...
		}
	}
	hide();
	return res;
}

void CBEBouquetWidget::deleteBouquet()
{
	g_Zapit->deleteBouquet( selected + 1);
	Bouquets.clear();
	g_Zapit->getBouquets(Bouquets, true);
	if (selected >= Bouquets.size())
		selected--;
	bouquetsChanged = true;
	paint();
}

void CBEBouquetWidget::addBouquet()
{
	string newName = inputName("", "bouqueteditor.bouquetname");
	if (newName != "")
	{
		g_Zapit->addBouquet( newName);
		Bouquets.clear();
		g_Zapit->getBouquets(Bouquets, true);
		selected = Bouquets.size() - 1;
		bouquetsChanged = true;
	}
	paintHead();
	paint();
	paintFoot();
}

void CBEBouquetWidget::beginMoveBouquet()
{
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEBouquetWidget::finishMoveBouquet()
{
	state = beDefault;
	if (newPosition != origPosition)
	{
		g_Zapit->moveBouquet( origPosition+1, newPosition+1);
		Bouquets.clear();
		g_Zapit->getBouquets(Bouquets, true);
		bouquetsChanged = true;
	}
	paint();
}

void CBEBouquetWidget::cancelMoveBouquet()
{
	state = beDefault;
	internalMoveBouquet( newPosition, origPosition);
}

void CBEBouquetWidget::renameBouquet()
{
	string newName = inputName( Bouquets[selected].name, "bouqueteditor.newbouquetname");
	if (newName != Bouquets[selected].name)
	{
		g_Zapit->renameBouquet( selected + 1, newName);
		Bouquets.clear();
		g_Zapit->getBouquets(Bouquets, true);
		bouquetsChanged = true;
	}
	paintHead();
	paint();
	paintFoot();
}

string CBEBouquetWidget::inputName( string defaultName, string caption)
{
	char Name[30] = "";
	if (defaultName != "")
	{
		strncpy( Name, defaultName.c_str(), 30);
	}

	CStringInputSMS* nameInput = new CStringInputSMS(caption, Name, 29,
												 "" /* hint 1*/, "" /*hint2*/,
													 "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
	nameInput->exec(this, "");
	return( Name);
}

void CBEBouquetWidget::internalMoveBouquet( unsigned int fromPosition, unsigned int toPosition)
{
	if ( toPosition == -1 ) return;
	if ( toPosition == Bouquets.size()) return;

	CZapitClient::responseGetBouquets Bouquet = Bouquets[fromPosition];
	if (fromPosition < toPosition)
	{
		for (unsigned int i=fromPosition; i<toPosition; i++)
			Bouquets[i] = Bouquets[i+1];
	}
	else if (fromPosition > toPosition)
	{
		for (unsigned int i=fromPosition; i>toPosition; i--)
			Bouquets[i] = Bouquets[i-1];
	}
	Bouquets[toPosition] = Bouquet;
	selected = toPosition;
	newPosition = toPosition;
	paint();
}

void CBEBouquetWidget::saveChanges()
{
	CHintBox* hintBox= new CHintBox(this, "bouqueteditor.name", g_Locale->getText("bouqueteditor.savingchanges"), 480 );
	hintBox->paint();
	g_Zapit->saveBouquets();
	g_Zapit->reinitChannels();
	hintBox->hide();
	delete hintBox;

	g_RCInput->postMsg( messages::EVT_BOUQUETSCHANGED, 0 );
}

void CBEBouquetWidget::discardChanges()
{
	CHintBox* hintBox= new CHintBox(this, "bouqueteditor.name", g_Locale->getText("bouqueteditor.discardingchanges"), 480 );
	hintBox->paint();
	g_Zapit->restoreBouquets();
	hintBox->hide();
	delete hintBox;
}


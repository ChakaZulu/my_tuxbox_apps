/*
  Neutrino-GUI  -   DBoxII-Project

  AudioPlayer by Dirch,Zwen
	
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/audioplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#define DBOX 1
#ifdef DBOX
#include <driver/aviaext.h>
#endif

#include <daemonc/remotecontrol.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>

#include <system/settings.h>

#include <algorithm>
#include <sys/time.h>
#include <fstream>
#include <iostream>

#if HAVE_DVB_API_VERSION >= 3
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"
#endif

#ifdef ConnectLineBox_Width
#undef ConnectLineBox_Width
#endif
#define ConnectLineBox_Width	15

//------------------------------------------------------------------------

CAudioPlayerGui::CAudioPlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;
	m_metainfo = "";

	filebrowser = new CFileBrowser();
	filebrowser->Multi_Select = true;
	filebrowser->Dirs_Selectable = true;
	audiofilefilter.addFilter("cdr");
	audiofilefilter.addFilter("mp3");
	audiofilefilter.addFilter("m2a");
	audiofilefilter.addFilter("mpa");
	audiofilefilter.addFilter("mp2");
	audiofilefilter.addFilter("m3u");
	audiofilefilter.addFilter("ogg");
	audiofilefilter.addFilter("url");
	audiofilefilter.addFilter("wav");
	filebrowser->Filter = &audiofilefilter;
	if(strlen(g_settings.network_nfs_mp3dir)!=0)
		Path = g_settings.network_nfs_mp3dir;
	else
		Path = "/";
}

//------------------------------------------------------------------------

CAudioPlayerGui::~CAudioPlayerGui()
{
	playlist.clear();
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);
	delete filebrowser;
}

//------------------------------------------------------------------------
int CAudioPlayerGui::exec(CMenuTarget* parent, const std::string & actionKey)
{
	CAudioPlayer::getInstance()->init();
	m_state=CAudioPlayerGui::STOP;
	current=-1;
	selected = 0;
	width = 710;
	if((g_settings.screen_EndX- g_settings.screen_StartX) < width+ConnectLineBox_Width)
		width=(g_settings.screen_EndX- g_settings.screen_StartX)-ConnectLineBox_Width;
	height = 570;
	if((g_settings.screen_EndY- g_settings.screen_StartY) < height)
		height=(g_settings.screen_EndY- g_settings.screen_StartY);
	sheight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	buttonHeight = std::min(25, sheight);
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	title_height=fheight*2+20+sheight+4;
	info_height=fheight*2;
	listmaxshow = (height-info_height-title_height-theight-2*buttonHeight)/(fheight);
	height = theight+info_height+title_height+2*buttonHeight+listmaxshow*fheight;	// recalc height

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-(width+ConnectLineBox_Width)) / 2) + g_settings.screen_StartX + ConnectLineBox_Width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height)/ 2) + g_settings.screen_StartY;
	m_idletime=time(NULL);
	m_screensaver=false;

	if(parent)
	{
		parent->hide();
	}

	if(g_settings.video_Format != CControldClient::VIDEOFORMAT_4_3)
		g_Controld->setVideoFormat(CControldClient::VIDEOFORMAT_4_3);

	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground)
		frameBuffer->saveBackgroundImage();
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("radiomode.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	// set zapit in standby mode
	g_Zapit->setStandby(true);
	if(g_settings.audio_avs_Control == CControld::TYPE_OST)
	{
		m_vol_ost = true;
		g_settings.audio_avs_Control = CControld::TYPE_AVS;
	}
	else
		m_vol_ost = false;

	// tell neutrino we're in audio mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_audio );
	// remember last mode
	m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true); 

#ifdef DBOX
	// disable iec aka digi out
	CAViAExt::getInstance()->iecOff();
#endif
	
	/*int ret =*/

	show();

	// Restore previous background
	if (usedBackground)
		frameBuffer->restoreBackgroundImage();
	frameBuffer->useBackground(usedBackground);
	frameBuffer->paintBackground();

	// Restore last mode
	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
	//g_Zapit->zapTo_serviceID(channel_id);
	g_Zapit->setStandby(false);
	if(m_vol_ost)
	{
		g_Controld->setVolume(100, CControld::TYPE_AVS);
		g_settings.audio_avs_Control = CControld::TYPE_OST;
	}

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);

#ifdef DBOX
	// enable iec aka digi out
	CAViAExt::getInstance()->iecOn();
#endif

	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CAudioPlayerGui::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
	paintLCD();

	bool loop=true;
	bool update=true;
	bool clear_before_update=false;
	key_level=0;
	while(loop)
	{
		if(!m_screensaver)
		{
			updateMetaData();
		}
		updateTimes();

		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_audio)
		{
			// stop if mode was changed in another thread
			loop=false;
		}
		if ((m_state != CAudioPlayerGui::STOP) && 
		    (CAudioPlayer::getInstance()->getState() == CBaseDec::STOP) && 
		    (!playlist.empty()))
		{
			int next = getNext();
			if (next >= 0)
				play(next);
			else
				stop();
		}

		if (update)
		{
			if(clear_before_update)
			{
				hide();
				clear_before_update=false;
			}
			update=false;
			paint();
		}
		g_RCInput->getMsg( &msg, &data, 10 ); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_timeout  || msg == NeutrinoMessages::EVT_TIMER)
		{
			int timeout = time(NULL) - m_idletime;
			int screensaver_timeout = atoi(g_settings.mp3player_screensaver);
			if(screensaver_timeout !=0 && timeout > screensaver_timeout*60 && !m_screensaver)
				screensaver(true);
		}
		else
		{
			m_idletime=time(NULL);
			if(m_screensaver)
			{
				screensaver(false);
			}
		}
		if( msg == CRCInput::RC_timeout)
		{
			// nothing
		}
		else if( msg == CRCInput::RC_home)
		{ 
			if (m_state != CAudioPlayerGui::STOP)
				stop();        
			else
				loop=false;
		}
		else if( msg == CRCInput::RC_left)
		{
			if(key_level==1)
			{
				if(current==-1)
					stop();
				else if(current-1 > 0)
					play(current-1);
				else
					play(0);
			}
			else
			{
				if(selected >0 )
				{
					if ((int(selected)-int(listmaxshow))<0)
						selected=playlist.size()-1;
					else
						selected -= listmaxshow;
					liststart = (selected/listmaxshow)*listmaxshow;
					update=true;
				}
			}

		}
		else if( msg == CRCInput::RC_right)
		{
			if(key_level==1)
			{
				int next = getNext();
				if(next>=0)
					play(next);
				else
					stop();
			}
			else
			{
				if(selected!=playlist.size()-1 && !playlist.empty())
				{
					selected+=listmaxshow;
					if (selected>playlist.size()-1)
						selected=0;
					liststart = (selected/listmaxshow)*listmaxshow;
					update=true;
				}
			}
		}
		else if( msg == CRCInput::RC_up && !playlist.empty())
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = playlist.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				update=true;
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if( msg == CRCInput::RC_down && !playlist.empty() > 0)
		{
			int prevselected=selected;
			selected = (selected+1)%playlist.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				update=true;
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if( msg == CRCInput::RC_ok && !playlist.empty())
		{
			// OK button
			play(selected);
		}
		else if(msg==CRCInput::RC_red )
		{
			if(key_level==0)
			{
				if (!playlist.empty())
				{
					CPlayList::iterator p = playlist.begin()+selected;
					playlist.erase(p);
					if((int)selected==current)
					{
						current--;
						//stop(); // Stop if song is deleted, next song will be startet automat.
					}
					if(selected > playlist.size()-1)
						selected = playlist.size()-1;
					update=true;
				}
			}
			else
			{
				stop();
			}
		}
		else if(msg==CRCInput::RC_green)
		{
			if(key_level==0)
			{
				hide();
				if(filebrowser->exec(Path.c_str()))
				{
					Path=filebrowser->getCurrentDir();
					CFileList::iterator files = filebrowser->getSelectedFiles()->begin();
					for(; files != filebrowser->getSelectedFiles()->end();files++)
					{
						if ((files->getType() == CFile::FILE_CDR) ||
						    (files->getType() == CFile::FILE_OGG) ||
						    (files->getType() == CFile::FILE_MP3) ||
						    (files->getType() == CFile::FILE_WAV))
						{
							CAudiofile audiofile;
							audiofile.Filename = files->Name;
							audiofile.FileType = files->getType();
							playlist.push_back(audiofile);
						}
						if(files->getType() == CFile::STREAM_MP3)
						{
							CAudiofile mp3;
							mp3.FileType = CFile::STREAM_MP3;
							mp3.Filename = files->Name;
							mp3.Artist = "Shoutcast";
							std::string tmp = mp3.Filename.substr(mp3.Filename.rfind('/')+1);
							tmp = tmp.substr(0,tmp.length()-4);	//remove .url
							mp3.Title = tmp;
							mp3.Duration = 0;
							char url[80];
							FILE* f=fopen(files->Name.c_str(), "r");
							if(f!=NULL)
							{
								fgets(url, 80, f);
								if(url[strlen(url)-1] == '\n') url[strlen(url)-1]=0;
								if(url[strlen(url)-1] == '\r') url[strlen(url)-1]=0;
								mp3.Album = url;
								playlist.push_back(mp3);
								fclose(f);
							}
						}
						else if(files->getType() == CFile::FILE_PLAYLIST)
						{
							std::string sPath = files->Name.substr(0, files->Name.rfind('/'));
							std::ifstream infile;
							char cLine[256];
							infile.open(files->Name.c_str(), std::ifstream::in);
							while (infile.good())
							{
								infile.getline(cLine, 255);
								// remove CR
								if(cLine[strlen(cLine)-1]=='\r')
									cLine[strlen(cLine)-1]=0;
								if(strlen(cLine) > 0 && cLine[0]!='#') 
								{
									std::string filename = sPath;
									filename += '/';
									filename += cLine;
                           
									unsigned int pos;
									while((pos=filename.find('\\'))!=std::string::npos)
										filename[pos]='/';

									std::ifstream testfile;
									testfile.open(filename.c_str(), std::ifstream::in);
									if(testfile.good())
									{
										// Check for duplicates and remove (playlist has higher prio)
										CPlayList::iterator p=playlist.begin();
										while(p!=playlist.end())
										{
											if(p->Filename == filename)
												playlist.erase(p);
											else
												p++;
										}
										CAudiofile mp3;
										mp3.FileType = CFile::FILE_MP3;
										mp3.Filename = filename;
										playlist.push_back(mp3);
									}
									testfile.close();
								}
							}
							infile.close();
						}
					}
				}
				CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
				paintLCD();
				update=true;
			}
			else
			{
				if(curr_audiofile.FileType != CFile::STREAM_MP3)
					rev();
			}
		}
		else if(msg==CRCInput::RC_yellow)
		{
			if(key_level==0)
			{
				//stop();
				playlist.clear();
				current=-1;
				selected=0;
				clear_before_update=true;
				update=true;
			}
			else
			{
				pause();
			}
		}
		else if(msg==CRCInput::RC_blue)
		{
			if (key_level == 0)
			{
				if (!(playlist.empty()))
				{
					if (current > 0)
					{
						std::swap(playlist[0], playlist[current]);
						current = 0;
					}
				
					std::random_shuffle((current != 0) ? playlist.begin() : playlist.begin() + 1, playlist.end());

					selected = 0;

					update = true;
				}
			}
			else
			{
				if(curr_audiofile.FileType != CFile::STREAM_MP3)
					ff();
			}

		}
		else if(msg==CRCInput::RC_help)
		{
			if(key_level==1)
			{
				key_level=0;
				paintFoot();
			}
			else
			{
				if(m_state!=CAudioPlayerGui::STOP)
				{
					key_level=1;
					paintFoot();
				} 
				else 
				{
					if (!playlist.empty()) 
					{
						savePlaylist();
						CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
						paintLCD();
						update=true;
					}
				}
			}
		}
		else if ((msg >= CRCInput::RC_1) && (msg <= CRCInput::RC_9) && !(playlist.empty()))
		{ //numeric zap
			int x1=(g_settings.screen_EndX- g_settings.screen_StartX)/2 + g_settings.screen_StartX-50;
			int y1=(g_settings.screen_EndY- g_settings.screen_StartY)/2 + g_settings.screen_StartY;
			int val=0;
			char str[11];
			do
			{
				val = val * 10 + CRCInput::getNumericValue(msg);
				sprintf(str,"%d",val);
				int w = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(str);
				int h = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight();
				frameBuffer->paintBoxRel(x1 - 7, y1 - h - 5, w + 14, h + 10, COL_MENUCONTENT_PLUS_6);
				frameBuffer->paintBoxRel(x1 - 4, y1 - h - 3, w +  8, h +  6, COL_MENUCONTENTSELECTED_PLUS_0);
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(x1,y1,w+1,str,COL_MENUCONTENTSELECTED,0);
				g_RCInput->getMsg( &msg, &data, 100 ); 
			} while (g_RCInput->isNumeric(msg) && val < 1000000);
			if (msg == CRCInput::RC_ok)
				selected = std::min((int)playlist.size(), val) - 1;
			update = true;
		}
		else if(msg == CRCInput::RC_0)
		{
			if(current>=0)
			{
				selected=current;
				update=true;
			}
		}
		else if(msg==CRCInput::RC_setup)
		{
			CNFSSmallMenu nfsMenu;
			nfsMenu.exec(this, "");
			CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
			paintLCD();
			update=true;
			//pushback key if...
			//g_RCInput->postMsg( msg, data );
			//loop=false;
		}
		else if(msg == NeutrinoMessages::CHANGEMODE)
		{
			if((data & NeutrinoMessages::mode_mask) !=NeutrinoMessages::mode_audio)
			{
				loop = false;
				m_LastMode=data;
			}
		}
		else if(msg == NeutrinoMessages::RECORD_START ||
			msg == NeutrinoMessages::ZAPTO ||
			msg == NeutrinoMessages::STANDBY_ON ||
			msg == NeutrinoMessages::SHUTDOWN ||
			msg == NeutrinoMessages::SLEEPTIMER)
		{
			// Exit for Record/Zapto Timers
			loop = false;
			g_RCInput->postMsg(msg, data);
		}
		else if(msg == NeutrinoMessages::EVT_TIMER)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
			// update mute icon
			paintHead();
			paintLCD();
		}
	}
	hide();

	if(m_state != CAudioPlayerGui::STOP)
		stop();

	return(res);
}

//------------------------------------------------------------------------

void CAudioPlayerGui::hide()
{
//	printf("hide(){\n");
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x-ConnectLineBox_Width-1, y+title_height-1, width+ConnectLineBox_Width+2, height+2-title_height);
		clearItemID3DetailsLine();
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
		visible = false;
	}
//	printf("hide()}\n");
}

//------------------------------------------------------------------------

void CAudioPlayerGui::paintItem(int pos)
{
	int ypos = y + title_height + theight + pos*fheight;
	uint8_t    color;
	fb_pixel_t bgcolor;

	if ((pos + liststart) == selected)
	{
		if ((pos + liststart) == (unsigned)current)
		{
			color   = COL_MENUCONTENTSELECTED + 2;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_2;
		}
		else
		{
			color   = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
	}
	else
		if (((pos + liststart) < playlist.size()) && (pos & 1))
		{
			if ((pos + liststart) == (unsigned)current)
			{
				color   = COL_MENUCONTENTDARK + 2;
				bgcolor = COL_MENUCONTENTDARK_PLUS_2;
			}
			else
			{
				color   = COL_MENUCONTENTDARK;
				bgcolor = COL_MENUCONTENTDARK_PLUS_0;
			}
		}
		else
		{
			if ((pos + liststart) == (unsigned)current)
			{
				color   = COL_MENUCONTENT + 2;
				bgcolor = COL_MENUCONTENT_PLUS_2;
			}
			else
			{
				color   = COL_MENUCONTENT;
				bgcolor = COL_MENUCONTENT_PLUS_0;
			}
		}

	frameBuffer->paintBoxRel(x, ypos, width - 15, fheight, bgcolor);

	if ((pos + liststart) < playlist.size())
	{
		if (playlist[pos + liststart].Title.empty())
		{
			// id3tag noch nicht geholt
			GetMetaData(&playlist[pos + liststart]);
			if(m_state!=CAudioPlayerGui::STOP && !g_settings.mp3player_highprio)
				usleep(100*1000);
		}
		char sNr[20];
		sprintf(sNr, "%2d : ", pos + liststart + 1);
		std::string tmp=sNr;
 		std::string artist="Artist?";
		std::string title="Title?";
		
		if (!playlist[pos + liststart].Artist.empty())
			artist = playlist[pos + liststart].Artist;
		if (!playlist[pos + liststart].Title.empty())
			title = playlist[pos + liststart].Title;
		if(g_settings.mp3player_display == TITLE_ARTIST)
		{
			tmp += title;
			tmp += ", ";
			tmp += artist;
		}
		else //if(g_settings.mp3player_display == ARTIST_TITLE)
		{
			tmp += artist;
			tmp += ", ";
			tmp += title;
		}

		if (!playlist[pos + liststart].Album.empty())
		{
			tmp += " (";
			tmp += playlist[pos + liststart].Album;
			tmp += ')';
		}
		
		char dura[9];
		snprintf(dura, 8, "%ld:%02ld", playlist[pos + liststart].Duration / 60, playlist[pos + liststart].Duration % 60);
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(dura)+5;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+fheight, width-30-w, tmp, color, fheight, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-15-w,ypos+fheight, w, dura, color, fheight);
		
		if ((pos + liststart) == selected)
		{
			paintItemID3DetailsLine(pos);
			if (m_state == CAudioPlayerGui::STOP)
				CLCD::getInstance()->showAudioTrack(playlist[pos + liststart].Artist, playlist[pos + liststart].Title, playlist[pos + liststart].Album);
		}
		
	}
}

//------------------------------------------------------------------------

void CAudioPlayerGui::paintHead()
{
//	printf("paintHead{\n");
	std::string strCaption = g_Locale->getText(LOCALE_AUDIOPLAYER_HEAD);
	frameBuffer->paintBoxRel(x,y+title_height, width,theight, COL_MENUHEAD_PLUS_0);
	frameBuffer->paintIcon("mp3.raw",x+7,y+title_height+10);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+35,y+theight+title_height+0, width- 45, strCaption, COL_MENUHEAD, 0, true); // UTF-8
	int ypos=y+title_height;
	if(theight > 26)
		ypos = (theight-26) / 2 + y + title_height;
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_DBOX, x+ width- 30, ypos );
	if( CNeutrinoApp::getInstance()->isMuted() )
	{
		int xpos=x+width-75;
		ypos=y+title_height;
		if(theight > 32)
			ypos = (theight-32) / 2 + y + title_height;
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, xpos, ypos);
	}
//	printf("paintHead}\n");
}

//------------------------------------------------------------------------
const struct button_label AudioPlayerButtons[2][4] =
{
	{
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_AUDIOPLAYER_STOP        },
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_REWIND      },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_PAUSE       },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_AUDIOPLAYER_FASTFORWARD },
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_AUDIOPLAYER_DELETE      },
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_ADD         },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_DELETEALL   },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_AUDIOPLAYER_SHUFFLE     }
	}
};

void CAudioPlayerGui::paintFoot()
{
//	printf("paintFoot{\n");
	if(m_state==CAudioPlayerGui::STOP) // insurance
		key_level=0;
	int ButtonWidth = (width-20) / 4;
	int ButtonWidth2 = (width-50) / 2;
	frameBuffer->paintBoxRel(x,y+(height-info_height-2*buttonHeight), width,2*buttonHeight, COL_MENUHEAD_PLUS_0);
	frameBuffer->paintHLine(x, x+width,  y+(height-info_height-2*buttonHeight), COL_INFOBAR_SHADOW_PLUS_0);

	if (!(playlist.empty()))
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + 1* ButtonWidth2 + 25, y+(height-info_height-buttonHeight)-3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + 1 * ButtonWidth2 + 53 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth2- 28, g_Locale->getText(LOCALE_AUDIOPLAYER_PLAY), COL_INFOBAR, 0, true); // UTF-8
		if (m_state==CAudioPlayerGui::STOP) {
		  // help will store a playlist file
		  frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ 0* ButtonWidth + 25, y+(height-info_height-buttonHeight)-3);
		  g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 0* ButtonWidth +53 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth2- 28, g_Locale->getText(LOCALE_AUDIOPLAYER_SAVE_PLAYLIST), COL_INFOBAR, 0, true); // UTF-8
		}
	}
	if(m_state!=CAudioPlayerGui::STOP)
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ 0* ButtonWidth + 25, y+(height-info_height-buttonHeight)-3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 0* ButtonWidth +53 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth2- 28, g_Locale->getText(LOCALE_AUDIOPLAYER_KEYLEVEL), COL_INFOBAR, 0, true); // UTF-8
	}

	if (key_level == 0)
	{
		if (playlist.empty())
			::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + ButtonWidth + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 1, &(AudioPlayerButtons[1][1]));
		else
			::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 4, AudioPlayerButtons[1]);
	}
	else
	{	
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 4, AudioPlayerButtons[0]);
	}
//	printf("paintFoot}\n");
}
//------------------------------------------------------------------------
void CAudioPlayerGui::paintInfo()
{
	if(m_state==CAudioPlayerGui::STOP)
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
	else
	{
		frameBuffer->paintBoxRel(x,         y, width, title_height-10, COL_MENUCONTENT_PLUS_6);
		frameBuffer->paintBoxRel(x+2, y +2 , width-4, title_height-14, COL_MENUCONTENTSELECTED_PLUS_0);
		char sNr[20];
		sprintf(sNr, ": %2d", current+1);
		std::string tmp = g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYING);
		tmp += sNr ;
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		int xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+xstart, y + 4 + 1*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		if (curr_audiofile.Title.empty())
			tmp = curr_audiofile.Artist;
		else if (curr_audiofile.Artist.empty())
			tmp = curr_audiofile.Title;
		else if (g_settings.mp3player_display == TITLE_ARTIST)
		{
			tmp = curr_audiofile.Title;
			tmp += " / ";
			tmp += curr_audiofile.Artist;
		}
		else //if(g_settings.mp3player_display == ARTIST_TITLE)
		{
			tmp = curr_audiofile.Artist;
			tmp += " / ";
			tmp += curr_audiofile.Title;
		}

		w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+xstart, y +4+ 2*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
#ifdef INCLUDE_UNUSED_STUFF
		tmp = curr_audiofile.Bitrate + " / " + curr_audiofile.Samplerate + " / " + curr_audiofile.ChannelMode + " / " + curr_audiofile.Layer;
#endif
		// reset so fields get painted always
		m_metainfo="";
		m_time_total=0;
		m_time_played=0;
		updateMetaData();
		updateTimes(true);
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	paintHead();
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+title_height+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((playlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3);

	paintFoot();
	paintInfo();
	visible = true;
	
}

//------------------------------------------------------------------------

void CAudioPlayerGui::clearItemID3DetailsLine ()
{
	paintItemID3DetailsLine (-1);
}

void CAudioPlayerGui::paintItemID3DetailsLine (int pos)
{
	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + title_height + theight+0 + pos*fheight;
	int ypos2 = y + (height-info_height);
	int ypos1a = ypos1 + (fheight/2)-2;
	int ypos2a = ypos2 + (info_height/2)-2;
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;


	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos - 1, y + title_height, ConnectLineBox_Width + 1, height - title_height);

	// paint Line if detail info (and not valid list pos)
	if (!playlist.empty() && (pos >= 0))
	{
		// 1. col thick line
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 4,fheight,     col1);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 4,info_height, col1);
		
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 4,ypos2a-ypos1a, col1);
		
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,4, col1);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos2a, 12,4, col1);
		
		// 2. col small line
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 1,fheight,     col2);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 1,info_height, col2);
		
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 1,ypos2a-ypos1a+4, col2);

		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,1, col2);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-12, ypos2a, 8,1, col2);
		
		// -- small Frame around infobox
		frameBuffer->paintBoxRel(x, ypos2, width, 2, col1);
		frameBuffer->paintBoxRel(x, ypos2 + 2, 2, info_height - 4, col1);
		frameBuffer->paintBoxRel(x + width - 2, ypos2 + 2, 2, info_height - 4, col1);
		frameBuffer->paintBoxRel(x, ypos2 + info_height - 2, width, 2, col1);
//		frameBuffer->paintBoxRel(x, ypos2, width, info_height, col1);

		// paint id3 infobox 
		frameBuffer->paintBoxRel(x+2, ypos2 +2 , width-4, info_height-4, COL_MENUCONTENTDARK_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, ypos2 + 2 + 1*fheight, width- 80, playlist[selected].Title, COL_MENUCONTENTDARK, 0, true); // UTF-8
		std::string tmp;
		if (playlist[selected].Genre.empty())
			tmp = playlist[selected].Year;
		else if (playlist[selected].Year.empty())
			tmp = playlist[selected].Genre;
		else
		{
			tmp = playlist[selected].Genre;
			tmp += " / ";
			tmp += playlist[selected].Year;
		}
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true) + 10; // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w-5, ypos2 + 2 + 1*fheight, w, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
		tmp = playlist[selected].Artist;
		if (!(playlist[selected].Album.empty()))
		{
			tmp += " (";
			tmp += playlist[selected].Album;
			tmp += ')';
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, ypos2 + 2*fheight-2, width- 20, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
	else
	{
		frameBuffer->paintBackgroundBoxRel(x, ypos2, width, info_height);
	}
}

void CAudioPlayerGui::stop()
{
	m_state=CAudioPlayerGui::STOP;
	current=-1;
	//LCD
	paintLCD();
	//Display
	paintInfo();
	key_level=0;
	paintFoot();
	
	if(CAudioPlayer::getInstance()->getState() != CBaseDec::STOP)
		CAudioPlayer::getInstance()->stop();
}

void CAudioPlayerGui::pause()
{
	if(m_state==CAudioPlayerGui::PLAY || m_state==CAudioPlayerGui::FF || m_state==CAudioPlayerGui::REV)
	{
		m_state=CAudioPlayerGui::PAUSE;
		CAudioPlayer::getInstance()->pause();
	}
	else if(m_state==CAudioPlayerGui::PAUSE)
	{
		m_state=CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->pause();
	}
	paintLCD();
}

void CAudioPlayerGui::ff()
{
	if(m_state==CAudioPlayerGui::FF)
	{
		m_state=CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->ff();
	}
	else if(m_state==CAudioPlayerGui::PLAY || m_state==CAudioPlayerGui::PAUSE || m_state==CAudioPlayerGui::REV)
	{
		m_state=CAudioPlayerGui::FF;
		CAudioPlayer::getInstance()->ff();
	}
	paintLCD();
}

void CAudioPlayerGui::rev()
{
	if(m_state==CAudioPlayerGui::REV)
	{
		m_state=CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->rev();
	}
	else if(m_state==CAudioPlayerGui::PLAY || m_state==CAudioPlayerGui::PAUSE || m_state==CAudioPlayerGui::FF)
	{
		m_state=CAudioPlayerGui::REV;
		CAudioPlayer::getInstance()->rev();
	}
	paintLCD();
}


void CAudioPlayerGui::play(int pos)
{
	//printf("AudioPlaylist: play %d/%d\n",pos,playlist.size());
	unsigned int old_current=current;
	unsigned int old_selected=selected;

	current=pos;
	if(g_settings.mp3player_follow)
		selected=pos;

	if(selected - liststart >= listmaxshow && g_settings.mp3player_follow)
	{
		liststart=selected;
		if(!m_screensaver)
			paint();
	}
	else if(liststart - selected < 0 && g_settings.mp3player_follow)
	{
		liststart=selected-listmaxshow+1;
		if(!m_screensaver)
			paint();
	}
	else
	{
		if(old_current - liststart >=0 && old_current - liststart < listmaxshow)
		{
			if(!m_screensaver)
				paintItem(old_current - liststart);
		}
		if(pos - liststart >=0 && pos - liststart < listmaxshow)
		{
			if(!m_screensaver)
				paintItem(pos - liststart);
		}
		if(g_settings.mp3player_follow)
		{
			if(old_selected - liststart >=0 && old_selected - liststart < listmaxshow)
				if(!m_screensaver)
					paintItem(old_selected - liststart);
		}
	}

	if (playlist[pos].Title.empty())
	{
		// id3tag noch nicht geholt
		GetMetaData(&playlist[pos]);
	}
	m_metainfo="";
	m_time_played=0;
	m_time_total=playlist[current].Duration;
	m_state=CAudioPlayerGui::PLAY;
	curr_audiofile = playlist[current];
	// Play
	CAudioPlayer::getInstance()->play(curr_audiofile.Filename.c_str(), g_settings.mp3player_highprio==1); 
	//LCD
	paintLCD();
	// Display
	if(!m_screensaver)
		paintInfo();
	key_level=1;
	if(!m_screensaver)
		paintFoot();
}

int CAudioPlayerGui::getNext()
{
	int ret=current+1;
	if(playlist.empty())
		return -1;
	if((unsigned)ret+1 > playlist.size())
		ret=0;
	return ret;
}
void CAudioPlayerGui::updateMetaData()
{
	bool updateMeta=false;
	bool updateLcd=false;
	bool updateScreen=false;

	if(m_state!=CAudioPlayerGui::STOP)
	{
		CAudioMetaData metaData = CAudioPlayer::getInstance()->getMetaData();
		if(metaData.changed || m_metainfo.empty())
		{
			std::string info = metaData.type_info;
			if(metaData.bitrate > 0)
			{
				info += " / " ;
				if(metaData.vbr)
					info += "VBR ";
				char rate[31];
				snprintf(rate, 30, "%ukbs", metaData.bitrate/1000);
				info += rate;
			}
			if(metaData.samplerate > 0)
			{
				info += " / " ;
				char rate[31];
				snprintf(rate, 30, "%.1fKHz", (float)metaData.samplerate/1000);
				info += rate;
			}
			if(m_metainfo!=info)
			{
				m_metainfo=info;
				updateMeta=true;
			}
			
			if (!metaData.artist.empty()  &&
				 metaData.artist != curr_audiofile.Artist)
			{
				curr_audiofile.Artist = metaData.artist;
				updateScreen=true;
				updateLcd=true;
			}
			if (!metaData.title.empty() &&
				 metaData.title != curr_audiofile.Title)
			{
				curr_audiofile.Title = metaData.title;
				updateScreen=true;
				updateLcd=true;
			}
			if (!metaData.sc_station.empty()  &&
				 metaData.sc_station != curr_audiofile.Album)
			{
				curr_audiofile.Album = metaData.sc_station;
				updateLcd=true;
			}
		}
		if (CAudioPlayer::getInstance()->getScBuffered()!=0)
		{
			updateLcd=true;
		}
		if(updateLcd)
			paintLCD();
		if(updateScreen)
			paintInfo();
		if(updateMeta || updateScreen)
		{
			frameBuffer->paintBoxRel(x + 10, y+ 4 + 2*fheight, width-20, sheight, COL_MENUCONTENTSELECTED_PLUS_0);
			int xstart = ((width - 20 - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth( m_metainfo ))/2)+10;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ xstart, y+4 + 2*fheight+sheight, width- 2*xstart, m_metainfo, COL_MENUCONTENTSELECTED);
		}
	}

}

void CAudioPlayerGui::updateTimes(const bool force)
{
	if (m_state != CAudioPlayerGui::STOP)
	{
		bool updateTotal = force;
		bool updatePlayed = force;

		if (m_time_total != CAudioPlayer::getInstance()->getTimeTotal())
		{
			m_time_total = CAudioPlayer::getInstance()->getTimeTotal();
			if (curr_audiofile.Duration != CAudioPlayer::getInstance()->getTimeTotal())
			{
				curr_audiofile.Duration = CAudioPlayer::getInstance()->getTimeTotal();
				if(current >=0)
					playlist[current].Duration = CAudioPlayer::getInstance()->getTimeTotal();
			}
			updateTotal = true;
		}
		if ((m_time_played != CAudioPlayer::getInstance()->getTimePlayed()))
		{
			m_time_played = CAudioPlayer::getInstance()->getTimePlayed();
			updatePlayed = true;
		}
		if(!m_screensaver)
		{
			char tot_time[11];
			snprintf(tot_time, 10, " / %ld:%02ld", m_time_total / 60, m_time_total % 60);
			char tmp_time[8];
			snprintf(tmp_time, 7, "%ld:00", m_time_total / 60);
			char play_time[8];
			snprintf(play_time, 7, "%ld:%02ld", m_time_played / 60, m_time_played % 60);
			
			int w1 = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tot_time);
			int w2 = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp_time);

			if (updateTotal)
			{
				frameBuffer->paintBoxRel(x+width-w1-10, y+4, w1+4, fheight, COL_MENUCONTENTSELECTED_PLUS_0);
				if(m_time_total > 0)
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w1-10, y+4 + fheight, w1, tot_time, COL_MENUCONTENTSELECTED);
			}
			if (updatePlayed || (m_state == CAudioPlayerGui::PAUSE))
			{
				frameBuffer->paintBoxRel(x+width-w1-w2-15, y+4, w2+4, fheight, COL_MENUCONTENTSELECTED_PLUS_0);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				if ((m_state != CAudioPlayerGui::PAUSE) || (tv.tv_sec & 1))
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w1-w2-11, y+4 + fheight, w2, play_time, COL_MENUCONTENTSELECTED);
				}
			}
		}
		if((updatePlayed || updateTotal) && m_time_total!=0)
		{
			CLCD::getInstance()->showAudioProgress((int)(100.0 * m_time_played / m_time_total), CNeutrinoApp::getInstance()->isMuted());
		}
	}
}

void CAudioPlayerGui::paintLCD()
{
	switch(m_state)
	{
	case CAudioPlayerGui::STOP:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_STOP);
		CLCD::getInstance()->showAudioProgress(0, CNeutrinoApp::getInstance()->isMuted());
		break;
	case CAudioPlayerGui::PLAY:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_PLAY);
		CLCD::getInstance()->showAudioTrack(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
		if(m_time_total!=0)
			CLCD::getInstance()->showAudioProgress((int)(100.0 * m_time_played / m_time_total), CNeutrinoApp::getInstance()->isMuted());
		else
			CLCD::getInstance()->showAudioProgress((int)(100.0 * CAudioPlayer::getInstance()->getScBuffered() / 65536), CNeutrinoApp::getInstance()->isMuted());
		break;
	case CAudioPlayerGui::PAUSE:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_PAUSE);
		CLCD::getInstance()->showAudioTrack(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
		break;
	case CAudioPlayerGui::FF:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_FF);
		CLCD::getInstance()->showAudioTrack(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
		break;
	case CAudioPlayerGui::REV:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_REV);
		CLCD::getInstance()->showAudioTrack(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
		break;
	}
}

void CAudioPlayerGui::screensaver(bool on)
{
	if(on)
	{
		m_screensaver=true;
		frameBuffer->ClearFrameBuffer();
	}
	else
	{
		m_screensaver=false;
		frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
		frameBuffer->loadBackground("radiomode.raw");
		frameBuffer->useBackground(true);
		frameBuffer->paintBackground();
		paint();
		m_idletime=time(NULL);
	}
}

void CAudioPlayerGui::GetMetaData(CAudiofile *File)
{
	CAudioMetaData m=CAudioPlayer::getInstance()->readMetaData(File->Filename.c_str(), 
																				  m_state!=CAudioPlayerGui::STOP && 
																				  !g_settings.mp3player_highprio);

	File->Title = m.title;
	File->Artist = m.artist;
	File->Album = m.album;
	File->Year = m.date;
	File->Duration = m.total_time;
	File->Genre = m.genre;
	
	if (File->Artist.empty() && File->Title.empty())
	{
		//Set from Filename
		std::string tmp = File->Filename.substr(File->Filename.rfind('/')+1);
		tmp = tmp.substr(0,tmp.length()-4);	//remove extension (.mp3)
		unsigned int i = tmp.rfind(" - ");
		if(i != std::string::npos)
		{ // Trennzeiche " - " gefunden
			File->Artist = tmp.substr(0, i);
			File->Title = tmp.substr(i+3);
		}
		else
		{
			i = tmp.rfind('-');
			if(i != std::string::npos)
			{ //Trennzeichen "-"
				File->Artist = tmp.substr(0, i);
				File->Title = tmp.substr(i+1);
			}
			else
				File->Title	= tmp;
		}
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
		File->Artist = Latin1_to_UTF8(File->Artist);
		File->Title = Latin1_to_UTF8(File->Title);
#endif
	}
}

void CAudioPlayerGui::savePlaylist()
{
	const char * path;
	
	// .m3u playlist
	// http://hanna.pyxidis.org/tech/m3u.html
	
	CFileBrowser browser;
	browser.Multi_Select = false;
	browser.Dir_Mode = true;
	CFileFilter dirFilter;
	dirFilter.addFilter("m3u");
	browser.Filter = &dirFilter;
	// select preferred directory if exists
	if (strlen(g_settings.network_nfs_mp3dir) != 0)
		path = g_settings.network_nfs_mp3dir;
	else
		path = "/";
	
	// let user select target directory
	this->hide();
	if (browser.exec(path)) {
		// refresh view
		this->paint();
		CFile *file = browser.getSelectedFile();
		std::string absPlaylistDir = file->getPath();
		
		// add a trailing slash if necessary
		if ((absPlaylistDir.empty()) || ((*(absPlaylistDir.rbegin()) != '/')))
		{
			absPlaylistDir += '/';
		}
		absPlaylistDir += file->getFileName();
		
		const int filenamesize = 30;
		char filename[filenamesize+1] = "";
		
		if (file->getType() == CFile::FILE_PLAYLIST) {
			// file is playlist so we should ask if we can overwrite it
			std::string name = file->getPath();
			name += '/';
			name += file->getFileName();
			bool overwrite = askToOverwriteFile(name);
			if (!overwrite) {
				return;
			}
			snprintf(filename, name.size(), "%s", name.c_str());
		} else if (file->getType() == CFile::FILE_DIR) {
			// query for filename
			this->hide();
			CStringInputSMS filenameInput(LOCALE_AUDIOPLAYER_PLAYLIST_NAME,
						      filename,
						      filenamesize-1,
						      LOCALE_AUDIOPLAYER_PLAYLIST_NAME_HINT1,
						      LOCALE_AUDIOPLAYER_PLAYLIST_NAME_HINT2,
						      "abcdefghijklmnopqrstuvwxyz0123456789-.,:!?/ ");
			filenameInput.exec(NULL, "");
			// refresh view
			this->paint();
			std::string name = absPlaylistDir;
			name += '/';
			name += filename;
			name += ".m3u";
			std::ifstream input(name.c_str());
			
			// test if file exists and query for overwriting it or not
			if (input.is_open()) {
				bool overwrite = askToOverwriteFile(name);
				if (!overwrite) {
					return;
				}
			}
			input.close();
		} else {
			std::cout << "neither .m3u nor directory selected, abort" << std::endl;
			return;
		}
		std::string absPlaylistFilename = absPlaylistDir;
		absPlaylistFilename += '/';
		absPlaylistFilename += filename;
		absPlaylistFilename += ".m3u";		
		std::ofstream playlistFile(absPlaylistFilename.c_str());
		std::cout << "Audioplayer: writing playlist to " << absPlaylistFilename << std::endl;
		if (!playlistFile) {
			// an error occured
			const int msgsize = 255;
			char msg[msgsize] = "";
			snprintf(msg,
				 msgsize,
				 "%s\n%s",
				 g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYLIST_FILEERROR_MSG),
				 absPlaylistFilename.c_str());
			
			DisplayErrorMessage(msg);
			// refresh view
			this->paint();
			std::cout << "could not create play list file " 
				  << absPlaylistFilename << std::endl;
			return;
		}
		// writing .m3u file
		playlistFile << "#EXTM3U" << std::endl;
		
		CPlayList::const_iterator it;
		for (it = playlist.begin();it!=playlist.end();it++) {
			playlistFile << "#EXTINF:" << it->Duration << ","
				     << it->Artist << " - " << it->Title << std::endl;
			playlistFile << absPath2Rel(absPlaylistDir, it->Filename) << std::endl;
		}
		playlistFile.close();
	}  
	this->paint();
}

bool CAudioPlayerGui::askToOverwriteFile(const std::string& filename) {
	
	char msg[filename.length()+127];
	snprintf(msg,
		 filename.length()+126,
		 "%s\n%s",
		 g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYLIST_FILEOVERWRITE_MSG),
		 filename.c_str());
	bool res = (ShowMsgUTF(LOCALE_AUDIOPLAYER_PLAYLIST_FILEOVERWRITE_TITLE,
			       msg,CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo)
		    == CMessageBox::mbrYes);
	this->paint();
	return res;
}

std::string CAudioPlayerGui::absPath2Rel(const std::string& fromDir,
					 const std::string& absFilename) {
	std::string res = "";

	int length = fromDir.length() < absFilename.length() ? fromDir.length() : absFilename.length();
	int lastSlash = 0;
	// find common prefix for both paths
	// fromDir:     /foo/bar/angle/1          (length: 16)
	// absFilename: /foo/bar/devil/2/fire.mp3 (length: 19)
	// -> /foo/bar/ is prefix, lastSlash will be 8
	for (int i=0;i<length;i++) {
		if (fromDir[i] == absFilename[i]) {
			if (fromDir[i] == '/') {
				lastSlash = i;
			}
		} else {
			break;
		}
	}
	// cut common prefix
	std::string relFilepath = absFilename.substr(lastSlash+1,absFilename.length()-lastSlash+1);
	// relFilepath is now devil/2/fire.mp3
	
	// First slash is not removed because we have to go up each directory.
	// Since the slashes are counted later we make sure for each directory one slash is present
	std::string relFromDir = fromDir.substr(lastSlash,fromDir.length()-lastSlash);
	// relFromDir is now /angle/1
	
	// go up as many directories as neccessary
	for (unsigned int i=0;i<relFromDir.size();i++) {
		if (relFromDir[i] == '/') {
			res = res + "../";
		}
	}
	
	res = res + relFilepath;
	return res;
}


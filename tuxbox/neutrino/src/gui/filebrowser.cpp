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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "filebrowser.h"

#include <driver/encoding.h>

#include <gui/widget/icons.h>
#include "widget/messagebox.h"

#include <algorithm>
#include <iostream>
#include <global.h>
#include <neutrino.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <sys/stat.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#ifdef __USE_FILE_OFFSET64
typedef struct dirent64 dirent_struct;
#define my_alphasort alphasort64
#define my_scandir scandir64
#else
typedef struct dirent dirent_struct;
#define my_alphasort alphasort
#define my_scandir scandir64
#endif

//------------------------------------------------------------------------
size_t CurlWriteToString(void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	return size*nmemb;
}
//------------------------------------------------------------------------
int CFile::getType()
{
	if(S_ISDIR(Mode))
		return FILE_DIR;

	int ext_pos = Name.rfind(".");
	if( ext_pos > 0)
	{
		std::string extension;
		extension = Name.substr(ext_pos + 1, Name.length() - ext_pos);
		if((strcasecmp(extension.c_str(),"mp3") == 0) || (strcasecmp(extension.c_str(),"m2a") == 0) ||
			(strcasecmp(extension.c_str(),"mpa") == 0) )
			return FILE_MP3;
      if((strcasecmp(extension.c_str(),"m3u") == 0))
         return FILE_MP3_PLAYLIST;
		if((strcasecmp(extension.c_str(),"txt") == 0) || (strcasecmp(extension.c_str(),"sh") == 0))
			return FILE_TEXT;
		if((strcasecmp(extension.c_str(),"jpg") == 0) || (strcasecmp(extension.c_str(),"png") == 0) || 
			(strcasecmp(extension.c_str(),"bmp") == 0) || (strcasecmp(extension.c_str(),"gif") == 0))
			return FILE_PICTURE;
	}
	return FILE_UNKNOWN;
}

//------------------------------------------------------------------------

std::string CFile::getFileName()		// return name.extension or folder name without trailing /
{
	int namepos = Name.rfind("/");
	if( namepos >= 0)
	{
		return Name.substr(namepos + 1);
	}
	else
		return Name;
}

//------------------------------------------------------------------------

std::string CFile::getPath()			// return complete path including trailing /
{
	int pos = 0;
	std::string tpath;

	if((pos = Name.rfind("/")) > 1)
	{
		tpath = Name.substr(0,pos+1);
	}
	else
		tpath = "/";
	return (tpath);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

// sort operators
bool sortByName (const CFile& a, const CFile& b)
{
	if(a.Name == b.Name)
		return a.Mode < b.Mode;
	else
		return a.Name < b.Name ;
}

bool sortByType (const CFile& a, const CFile& b)
{
	if(a.Mode == b.Mode)
		return a.Name > b.Name;
	else
		return a.Mode < b.Mode ;
}

bool sortByDate (const CFile& a, const CFile& b)
{
	return a.Time < b.Time ;
}

//------------------------------------------------------------------------

CFileBrowser::CFileBrowser()
{
	frameBuffer = CFrameBuffer::getInstance();

	Filter = NULL;
	use_filter = true;
	Multi_Select = false;
	Dirs_Selectable = false;
	Dir_Mode = false;
	selected = 0;
	smode = 0;

	width  = 500;
	height = 380;
   if((g_settings.screen_EndX- g_settings.screen_StartX) < width)
      width=(g_settings.screen_EndX- g_settings.screen_StartX);
	if((g_settings.screen_EndY- g_settings.screen_StartY) < height)
      height=(g_settings.screen_EndY- g_settings.screen_StartY);
	
	foheight = 30;

	theight  = g_Fonts->eventlist_title->getHeight();
	fheight = g_Fonts->filebrowser_item->getHeight();


	listmaxshow = max(1,(int)(height - theight - foheight)/fheight);

	height = theight+foheight+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	if(x+width > 720)
		x=720-width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	if(y+height > 576)
		x=576-width;
	
	liststart = 0;
}

//------------------------------------------------------------------------


CFileBrowser::~CFileBrowser()
{
}

//------------------------------------------------------------------------

CFile *CFileBrowser::getSelectedFile()
{
	if ((filelist.size() > 0) && (filelist[selected].Name.length() > 0))
		return &filelist[selected];
	else
		return NULL;
}

//------------------------------------------------------------------------

CFileList *CFileBrowser::getSelectedFiles()
{
	return &selected_filelist;
}

//------------------------------------------------------------------------

void CFileBrowser::ChangeDir(std::string filename)
{
	std::string newpath;
	if(filename == "..")
	{
		unsigned int pos;
		if(Path.find(VLC_URI)==0)
		{
			pos = Path.substr(strlen(VLC_URI), Path.length()-strlen(VLC_URI)-1).rfind("/");
			if (pos != std::string::npos)
				pos += strlen(VLC_URI);
		}
		else
		{
			pos = Path.substr(0,Path.length()-1).rfind("/");
		}
		if(pos == std::string::npos)
			pos = Path.length();
		newpath = Path.substr(0,pos);
//		printf("path: %s filename: %s newpath: %s\n",Path.c_str(),filename.c_str(),newpath.c_str());
	}
	else
	{
		newpath=filename;
	}
	if(newpath.rfind("/") != newpath.length()-1 ||
      newpath.length() == 0 ||
		newpath == VLC_URI)
	{
		newpath = newpath + "/";
	}
	filelist.clear();
	Path = newpath;
	name = newpath;
	CFileList allfiles;
	readDir(newpath, &allfiles);
	// filter
	CFileList::iterator file = allfiles.begin();
	for(; file != allfiles.end() ; file++)
	{
		if(Filter != NULL && (!S_ISDIR(file->Mode)) && use_filter)
		{
			if(!Filter->matchFilter(file->Name))
			{
				continue;
			}
		}
		if(Dir_Mode && (!S_ISDIR(file->Mode)))
		{
			continue;
		}
		filelist.push_back(*file);
	}
	// sort result
	if( smode == 0 )
		sort(filelist.begin(), filelist.end(), sortByName);
	else
		sort(filelist.begin(), filelist.end(), sortByDate);

	selected = 0;
	paintHead();
	paint();
}

//------------------------------------------------------------------------
bool CFileBrowser::readDir(std::string dirname, CFileList* flist)
{
	bool ret;
		
	if (dirname.find(VLC_URI)==0)
	{
		ret = readDir_vlc(dirname, flist);
	}
	else
	{
		ret = readDir_std(dirname, flist);
	}
	return ret;
}

bool CFileBrowser::readDir_vlc(std::string dirname, CFileList* flist)
{
	printf("readDir_vlc %s\n",dirname.c_str());
	std::string answer="";
	char *dir_escaped = curl_escape(dirname.substr(strlen(VLC_URI)).c_str(), 0);
	std::string url = m_baseurl + dir_escaped;
	curl_free(dir_escaped);
	cout << "[FileBrowser] vlc URL: " << url << endl;
	CURL *curl_handle;
	CURLcode httpres;
	/* init the curl session */
	curl_handle = curl_easy_init();
	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
						  CurlWriteToString);
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&answer);
	/* error handling */
	char error[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error);
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD, "admin:admin"); /* !!! make me customizable */
	/* get it! */
	httpres = curl_easy_perform(curl_handle);
	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
	/* Convert \ to / */
	for( unsigned int pos=answer.find("\\"); pos!=std::string::npos ; pos=answer.find("\\"))
		answer[pos]='/';
	// cout << "Answer:" << endl << "----------------" << endl << answer << endl;
	/*!!! TODO check httpres and display error */
	if (!answer.empty() && !httpres)
	{
		unsigned int start=0;
		for (unsigned int pos=answer.find("\n",0) ; pos != std::string::npos ; pos=answer.find("\n",start))
		{
			CFile file;
			std::string entry = answer.substr(start, pos-start);
			//cout << "Entry" << entry << endl;
			if (entry.find("DIR:")==0) 
				file.Mode = S_IFDIR + 0777 ;
			else
				file.Mode = S_IFREG + 0777 ;
			unsigned int spos = entry.rfind("/");
			if(spos!=std::string::npos)
			{
				file.Name = dirname + entry.substr(spos+1);
				file.Size = 0;
				file.Time = 0;
				flist->push_back(file);
			}
			else
				cout << "Error misformed path " << entry << endl;
			start=pos+1;
		}
		return true;
	}
	else
	{
		cout << "Error reading vlc dir" << endl;
		/* since all CURL error messages use only US-ASCII characters, when can safely print them as if they were UTF-8 encoded */
		DisplayErrorMessage(error); // UTF-8
		CFile file;
		file.Name = dirname + "..";
		file.Mode = S_IFDIR + 0777;
		file.Size = 0;
		file.Time = 0;
		flist->push_back(file);
	}
	return false;
}

bool CFileBrowser::readDir_std(std::string dirname, CFileList* flist)
{
	printf("readDir_std %s\n",dirname.c_str());
	struct stat statbuf;
	dirent_struct **namelist;
	int n;

	n = my_scandir(dirname.c_str(), &namelist, 0, my_alphasort);
	if (n < 0)
	{
		perror(("Filebrowser scandir: "+dirname).c_str());
		return false;
	}
	for(int i = 0; i < n;i++)
	{
		CFile file;
		if(strcmp(namelist[i]->d_name,".") != 0)
		{
			file.Name = dirname + namelist[i]->d_name;
//			printf("file.Name: '%s', getFileName: '%s' getPath: '%s'\n",file.Name.c_str(),file.getFileName().c_str(),file.getPath().c_str());
			if(stat((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				file.Time = statbuf.st_mtime;
				flist->push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);
	
	return true;
}

//------------------------------------------------------------------------

bool CFileBrowser::exec(std::string Dirname)
{
	bool res = false;

	m_baseurl = "http://" + g_settings.streaming_server_ip +
	            ":" + g_settings.streaming_server_port + "/admin/dboxfiles.html?dir=";
	for( unsigned int pos=Dirname.find("\\"); pos!=std::string::npos ; pos=Dirname.find("\\"))
		Dirname[pos]='/';
	name = Dirname;
	paintHead();
	ChangeDir(Dirname);
	paint();
	paintFoot();

	int oldselected = selected;

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "Filebrowser: \"%s\"", dirname.c_str() );
		g_ActionLog->println(buf);
	#endif

	uint msg; uint data;
	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_filebrowser );

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_filebrowser );

		if ( msg == CRCInput::RC_yellow )
		{
			if(Multi_Select)
			{
				if(filelist[selected].getFileName() != "..")
				{
					if( (S_ISDIR(filelist[selected].Mode) && Dirs_Selectable) || !S_ISDIR(filelist[selected].Mode) )
					{
						filelist[selected].Marked = !filelist[selected].Marked;
						paintItem(selected - liststart);
					}
				}
				msg = CRCInput::RC_down;	// jump to next item
			}
		}

		if ( msg == CRCInput::RC_red )
		{
			selected+=listmaxshow;
			if (selected>filelist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_green )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=filelist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_up )
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = filelist.size()-1;
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
			if (filelist.size() != 0)
			{
				int prevselected=selected;
				selected = (selected + 1) % filelist.size();
				paintItem(prevselected - liststart);
				unsigned int oldliststart = liststart;
				liststart = (selected/listmaxshow)*listmaxshow;
				if(oldliststart!=liststart)
					paint();
				else
					paintItem(selected - liststart);
			}
		}
		else if ( ( msg == CRCInput::RC_timeout ) )
		{
			selected = oldselected;
			loop=false;
		}
		else if ( msg == CRCInput::RC_right )
		{
			if (filelist.size() != 0)
			{
				if (S_ISDIR(filelist[selected].Mode) && (filelist[selected].getFileName() != ".."))
					ChangeDir(filelist[selected].Name);
			}
		}
		else if ( msg == CRCInput::RC_left )
		{
			ChangeDir("..");
		}
		else if ( msg == CRCInput::RC_blue )
		{
			if(Filter != NULL)
			{
				use_filter = !use_filter;
				ChangeDir(Path);
			}
		}
		else if ( msg == CRCInput::RC_home )
		{
			loop = false;
		}
		else if (msg == CRCInput::RC_ok)
		{
			
			if (filelist.size() != 0)
			{
				if (filelist[selected].getFileName() == "..")
					ChangeDir("..");
				else
				{
					std::string filename = filelist[selected].Name;
					if ( filename.length() > 1 )
					{
						if((!Multi_Select) && S_ISDIR(filelist[selected].Mode) && !Dir_Mode)
						{
							ChangeDir(filelist[selected].Name);
						}
						else
						{
							filelist[selected].Marked = true;
							loop = false;
							res = true;
						}
					}
				}
			}
		}
		else if (msg==CRCInput::RC_help)
		{
			smode++;
			if( smode > 1 )
				smode = 0;
			if( smode == 0 )
				sort(filelist.begin(), filelist.end(), sortByName);
			else
				sort(filelist.begin(), filelist.end(), sortByDate);
			paint();
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
		}
	}

	hide();

	selected_filelist.clear();

	if(res && Multi_Select)
	{
		CProgressWindow * progress = new CProgressWindow();
		progress->setTitle(g_Locale->getText("filebrowser.scan"));
		progress->exec(NULL,"");
		for(unsigned int i = 0; i < filelist.size();i++)
			if(filelist[i].Marked)
			{
				if(S_ISDIR(filelist[i].Mode))
					addRecursiveDir(&selected_filelist,filelist[i].Name, true, progress);
				else
					selected_filelist.push_back(filelist[i]);
			}
		progress->hide();
		delete progress;
	}

	#ifdef USEACTIONLOG
		g_ActionLog->println("Filebrowser: closed");
	#endif

	return res;
}

//------------------------------------------------------------------------

void CFileBrowser::addRecursiveDir(CFileList * re_filelist, std::string rpath, bool bRootCall, CProgressWindow * progress)
{
	printf("addRecursiveDir %s\n",rpath.c_str());
	int n;
	uint msg, data;
	if (bRootCall) bCancel=false;
	
	g_RCInput->getMsg_us(&msg, &data, 1);
	if (msg==CRCInput::RC_home)
	{
		// home key cancel scan
		bCancel=true;
	}
	else if (msg!=CRCInput::RC_timeout)
	{
		// other event, save to low priority queue
		g_RCInput->postMsg( msg, data, false );
	}
	if(bCancel)
		return;
		
	if(rpath[rpath.length()-1]!='/')
	{
		rpath = rpath + "/";
	}

	CFileList tmplist;
	if(!readDir(rpath, &tmplist))
	{
		perror(("Recursive scandir: "+rpath).c_str());
	}
	else 
	{
		n = tmplist.size();
		if(progress)
		{
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
			progress->showStatusMessageUTF(Latin1_to_UTF8(rpath)); // ISO-8859-1
#else
			progress->showStatusMessageUTF(rpath); // UTF-8
#endif
		}
		for(int i = 0; i < n;i++)
		{
			if(progress)
			{
				progress->showGlobalStatus(100/n*i);
			}
			std::string basename = tmplist[i].Name.substr(tmplist[i].Name.rfind("/")+1);
			if( basename != ".." )
			{
				if(Filter != NULL && (!S_ISDIR(tmplist[i].Mode)) && use_filter)
				{
					if(!Filter->matchFilter(tmplist[i].Name))
					{
						continue;
					}
				}
				if(!S_ISDIR(tmplist[i].Mode))
					re_filelist->push_back(tmplist[i]);
				else
					addRecursiveDir(re_filelist,tmplist[i].Name, false, progress);
			}
		}
	}
}


//------------------------------------------------------------------------

void CFileBrowser::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

//------------------------------------------------------------------------

void CFileBrowser::paintItem(unsigned int pos, unsigned int spalte)
{
	int color;
	int ypos = y+ theight+0 + pos*fheight;
	CFile * actual_file = NULL;
	std::string fileicon;


	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
		paintFoot();
	}
	else
	{
		color = COL_MENUCONTENT;//DARK;
	}
	
	if( (liststart + pos) <filelist.size() )
	{
		actual_file = &filelist[liststart+pos];
		if(actual_file->Marked)
			color = color+2;

		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

		if ( actual_file->Name.length() > 0 )
		{
			if (liststart+pos==selected)
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
				CLCD::getInstance()->showMenuText(0, actual_file->getFileName()); // ISO-8859-1
#else
				CLCD::getInstance()->showMenuText(0, actual_file->getFileName(), -1, true); // UTF-8
#endif
			switch(actual_file->getType())
			{
				case CFile::FILE_MP3 : 
						fileicon = "mp3.raw";
//						color = COL_MENUCONTENT;
					break;
				case CFile::FILE_DIR : 
						fileicon = "folder.raw";
					break;
				case CFile::FILE_PICTURE:
				case CFile::FILE_TEXT:
				default:
						fileicon = "file.raw";
			}
			frameBuffer->paintIcon(fileicon, x+5 , ypos + (fheight-16) / 2 );
			
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
			g_Fonts->filebrowser_item->RenderString(x+35, ypos+ fheight, width -(35+170) , actual_file->getFileName(), color); // ISO-8859-1
#else
			g_Fonts->filebrowser_item->RenderString(x+35, ypos+ fheight, width -(35+170) , actual_file->getFileName(), color, 0, true); // UTF-8
#endif

			if( S_ISREG(actual_file->Mode) )
			{
				std::string modestring;
				for(int m = 2; m >=0;m--)
				{
					modestring += std::string((actual_file->Mode & (4 << (m*3)))?"r":"-");
					modestring += std::string((actual_file->Mode & (2 << (m*3)))?"w":"-");
					modestring += std::string((actual_file->Mode & (1 << (m*3)))?"x":"-");
				}
				g_Fonts->filebrowser_item->RenderString(x + width - 180 , ypos+ fheight, 80, modestring.c_str(), color);

				char tmpstr[256];
            if((double)actual_file->Size / (1024. * 1024 * 1024) > 1)
            {
               snprintf(tmpstr,sizeof(tmpstr),"%.4gG", 
                        (double)actual_file->Size / (1024. * 1024 * 1024));
            }
            else if((double)actual_file->Size / (1024. * 1024) > 1)
            {
               snprintf(tmpstr,sizeof(tmpstr),"%.4gM", 
                        (double)actual_file->Size / (1024. * 1024));
            }
            else if((double)actual_file->Size / (1024.) > 1)
            {
               snprintf(tmpstr,sizeof(tmpstr),"%.4gK", 
                        (double)actual_file->Size / (1024.));
            }
            else
               snprintf(tmpstr,sizeof(tmpstr),"%d", (int)actual_file->Size);

				int breite = g_Fonts->filebrowser_item->getRenderWidth(tmpstr)< 80?g_Fonts->filebrowser_item->getRenderWidth(tmpstr):70;
				g_Fonts->filebrowser_item->RenderString(x + width - 90 + (70 - breite), ypos+ fheight, breite, tmpstr, color);
			}
			if( S_ISDIR(actual_file->Mode) )
			{
				char timestring[18];
				time_t rawtime;

				rawtime = actual_file->Time;
				strftime(timestring, 18, "%d-%m-%Y %H:%M", gmtime(&rawtime));
				int breite = g_Fonts->filebrowser_item->getRenderWidth(timestring);
				g_Fonts->filebrowser_item->RenderString(x + width - 20 - breite , ypos+ fheight, breite+1, timestring, color);
			}
		}
	}
	else
		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, COL_MENUCONTENT/*DARK*/);
}

//------------------------------------------------------------------------

void CFileBrowser::paintHead()
{
	char l_name[100];

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);

#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
	snprintf(l_name, sizeof(l_name), "%s %s", CZapitClient::Utf8_to_Latin1(g_Locale->getText("filebrowser.head")).c_str(), name.c_str()); // ISO-8859-1
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width-11, l_name, COL_MENUHEAD); // ISO-8859-1
#else
	snprintf(l_name, sizeof(l_name), "%s %s", (g_Locale->getText("filebrowser.head")), name.c_str()); // UTF-8
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width-11, l_name, COL_MENUHEAD, 0, true); // UTF-8
#endif
}

//------------------------------------------------------------------------

void CFileBrowser::paintFoot()
{
//	int ButtonWidth = 25;
	int dx = width / 4;
	int type;
	int by = y + height - (foheight -4);
	int ty = by + g_Fonts->infobar_small->getHeight();

	frameBuffer->paintBoxRel(x, y+height- (foheight ), width, (foheight ), COL_MENUHEAD);

	if(filelist.size()>0)
	{
		std::string nextsort;
		type = filelist[selected].getType();

		if( (type != CFile::FILE_UNKNOWN) || (S_ISDIR(filelist[selected].Mode)) )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x +3 , by -3);
			g_Fonts->infobar_small->RenderString(x + 35, ty, dx - 35, g_Locale->getText("filebrowser.select"), COL_INFOBAR, 0, true); // UTF-8
		}

		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x + (1 * dx), by -3);
		if( smode == 1 )
			nextsort = g_Locale->getText("filebrowser.sort.name");
		else
			nextsort = g_Locale->getText("filebrowser.sort.date");
		g_Fonts->infobar_small->RenderString(x + 35 + (1 * dx), ty, dx - 35, nextsort.c_str(), COL_INFOBAR, 0, true); // UTF-8

		if(Multi_Select)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x + (2 * dx), by);
			g_Fonts->infobar_small->RenderString(x + 25 + (2 * dx), ty, dx - 25, g_Locale->getText("filebrowser.mark"), COL_INFOBAR, 0, true); // UTF-8
			
		}

		if(Filter != NULL)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, x + (3 * dx), by);
			g_Fonts->infobar_small->RenderString(x + 25 + (3 * dx), ty, dx - 25, use_filter?g_Locale->getText("filebrowser.filter.inactive"):g_Locale->getText("filebrowser.filter.active"), COL_INFOBAR, 0, true); // UTF-8
		}
	}
}

//------------------------------------------------------------------------

void CFileBrowser::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

//	if (filelist[0].Name.length() != 0)
//		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ width- 30, y+ 5 );
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText("filebrowser.head") );

	for(unsigned int count=0;count<listmaxshow;count++)
		paintItem(count);

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((filelist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);
}

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

#include "update.h"
#include "../global.h"

#define gTmpPath "/var/tmp/"
#define gUserAgent "neutrino/softupdater 1.0"

CFlashTool::CFlashTool()
{
	statusViewer = NULL;
	mtdDevice = "";
	ErrorMessage = "";
}

string CFlashTool::getErrorMessage()
{
	return ErrorMessage;
}

void CFlashTool::setMTDDevice( string mtddevice )
{
	mtdDevice = mtddevice;
}

void CFlashTool::setStatusViewer( CFlashTool_StatusViewer* statusview )
{
	statusViewer = statusview;
}

bool CFlashTool::program( string filename )
{
	int	fd1, fd2;
	long	filesize;

	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage("");
	}

	if(mtdDevice=="")
	{
		ErrorMessage = "mtd-device not set";
		return false;
	}

	if(mtdDevice=="")
	{
		ErrorMessage = "filename not set";
		return false;
	}

	if( (fd1 = open( filename.c_str(), O_RDONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenfile");
		return false;
	}

	filesize = lseek( fd1, 0, SEEK_END);
	lseek( fd1, 0, SEEK_SET);

	if(filesize==0)
	{
		ErrorMessage = g_Locale->getText("flashupdate.fileis0bytes");
		return false;
	}

	printf("filesize: %ld\n", filesize);


	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.eraseingflash"));
	}

	if(!erase())
	{
		return false;
	}

	if(statusViewer)
	{
		statusViewer->showGlobalStatus(75);
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.programmingflash"));
	}

	if( (fd2 = open( mtdDevice.c_str(), O_WRONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenmtd");
		close(fd1);
		return false;
	}

	char buf[1024];
	long fsize = filesize;
	while(fsize>0)
	{
		long block = fsize;
		if(block>(long)sizeof(buf))
		{
			block = sizeof(buf);
		}
		read( fd1, &buf, block);
		write( fd2, &buf, block);
		fsize -= block;
		char prog = char(100-(100./filesize*fsize));
		if(statusViewer)
		{
			statusViewer->showLocalStatus(prog);
		}
	}

	if(statusViewer)
	{
		statusViewer->showGlobalStatus(100);
		statusViewer->showLocalStatus(100);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.ready"));
	}

	close(fd1);
	close(fd2);
	return true;
}

bool CFlashTool::erase()
{
	int		fd;
	mtd_info_t	meminfo;
	erase_info_t	erase;

	if( (fd = open( mtdDevice.c_str(), O_RDWR )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenmtd");
		return false;
	}

	if( ioctl( fd, MEMGETINFO, &meminfo ) != 0 )
	{
		ErrorMessage = "can't get mtd-info";
		return false;
	}

	erase.length = meminfo.erasesize;
	for (erase.start = 0; erase.start < meminfo.size;erase.start += meminfo.erasesize)
	{
		/*
		printf( "\rErasing %u Kibyte @ %x -- %2u %% complete.",
		                 meminfo.erasesize/1024, erase.start,
		                 erase.start*100/meminfo.size );
		*/
		if(statusViewer)
		{
			statusViewer->showLocalStatus( char(erase.start*100./meminfo.size));
		}

		if(ioctl( fd, MEMERASE, &erase) != 0)
		{
			ErrorMessage = "erase error";
			close(fd);
			return false;
		}
	}

	close(fd);
	return true;
}


CHTTPUpdater::CHTTPUpdater()
{
	BasePath = "http://dboxupdate.berlios.de/update/";
	ImageFile = "cramfs.img";
	VersionFile = "version";
	//use other path?
	FILE* fd = fopen("/var/etc/update.conf", "r");
	if(fd)
	{
		char buf[1000];
		char buf2[1000];
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf(buf, "basepath: %s\n", &buf2);
		}
		BasePath = buf2;
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf(buf, "imagefile: %s\n", &buf2);
			if (strlen(buf2)> 0)
			{
				ImageFile = buf2;
			}

			if(fgets(buf,sizeof(buf),fd)!=NULL)
			{
				sscanf(buf, "versionfile: %s\n", &buf2);
				if (strlen(buf2)> 0)
				{
					VersionFile = buf2;
				}
			}
		}
		fclose(fd);

		printf("[CHTTPUpdater] HTTP-Basepath: %s\n", BasePath.c_str() );
		printf("[CHTTPUpdater] Image-Filename: %s\n", ImageFile.c_str() );
		printf("[CHTTPUpdater] Version-Filename: %s\n", VersionFile.c_str() );
	}
	statusViewer = NULL;
}

void CHTTPUpdater::setStatusViewer( CFlashTool_StatusViewer* statusview )
{
	statusViewer = statusview;
}


int CHTTPUpdater::show_progress( void *clientp, size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow)
{

	((CFlashTool_StatusViewer*)clientp)->showLocalStatus( int( dlnow*100.0/dltotal ) );
	return 0;
}

bool CHTTPUpdater::getInfo()
{
	statusViewer->showStatusMessage( g_Locale->getText("flashupdate.getinfofile") );
	CURL *curl;
	CURLcode res;
	FILE *headerfile;
	string	sFileName = gTmpPath+ VersionFile;
	headerfile = fopen(sFileName.c_str(), "w");
	res = (CURLcode) 1;
	curl = curl_easy_init();
	if(curl)
	{
		string gURL = BasePath + VersionFile;
		curl_easy_setopt(curl, CURLOPT_URL, gURL.c_str() );
		curl_easy_setopt(curl, CURLOPT_FILE, headerfile);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, show_progress);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, statusViewer);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, gUserAgent);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

		if(strcmp(g_settings.softupdate_proxyserver,"")!=0)
		{//use proxyserver
			printf("use proxyserver\n");
			curl_easy_setopt(curl, CURLOPT_PROXY, g_settings.softupdate_proxyserver);

			if(strcmp(g_settings.softupdate_proxyusername,"")!=0)
			{//use auth
				printf("use proxyauth\n");
				char tmp[200];
				strcpy(tmp, g_settings.softupdate_proxyusername);
				strcat(tmp, ":");
				strcat(tmp, g_settings.softupdate_proxypassword);
				curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, tmp);
			}
		}

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	//	printf("do flush\n");
	fflush(headerfile);
	//	printf("do close\n");
	fclose(headerfile);
	//	printf("ready\n");
	return res==0;
}

bool CHTTPUpdater::getFile( string version )
{
	statusViewer->showStatusMessage( g_Locale->getText("flashupdate.getupdatefile")+ " v"+ version );
	CURL *curl;
	CURLcode res;
	FILE *headerfile;
	string	sFileName = gTmpPath+ ImageFile;
	headerfile = fopen(sFileName.c_str(), "w");
	res = (CURLcode) 1;
	curl = curl_easy_init();
	if(curl)
	{
		string gURL = BasePath + ImageFile;
		curl_easy_setopt(curl, CURLOPT_URL, gURL.c_str() );
		curl_easy_setopt(curl, CURLOPT_FILE, headerfile);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, show_progress);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, statusViewer);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, gUserAgent);

		if(strcmp(g_settings.softupdate_proxyserver,"")!=0)
		{//use proxyserver
			printf("use proxyserver\n");
			curl_easy_setopt(curl, CURLOPT_PROXY, g_settings.softupdate_proxyserver);

			if(strcmp(g_settings.softupdate_proxyusername,"")!=0)
			{//use auth
				printf("use proxyauth\n");
				char tmp[200];
				strcpy(tmp, g_settings.softupdate_proxyusername);
				strcat(tmp, g_settings.softupdate_proxypassword);
				curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, tmp);
			}
		}
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	//	printf("do flush\n");
	fflush(headerfile);
	//	printf("do close\n");
	fclose(headerfile);
	//	printf("ready\n");
	return res==0;
}


CFlashUpdate::CFlashUpdate()
{
	width = 550;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight+20;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

void CFlashUpdate::stopEPGScanning( bool off )
{
  char rip[]="127.0.0.1";

  int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SAI servaddr;
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(sectionsd::portNumber);
  inet_pton(AF_INET, rip, &servaddr.sin_addr);

  if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1) {
    perror("Couldn't connect to sectionsd!");
    return;
  }

  sectionsd::msgRequestHeader req;
    req.version = 2;
    req.command = sectionsd::pauseScanning;
    req.dataLength = 4;
    write(sock_fd, &req, sizeof(req));
	int stopit = 0;
	if(off)
	{
		stopit=1;
	}
    write(sock_fd, &stopit, req.dataLength);

    close(sock_fd);
}

int CFlashUpdate::exec(CMenuTarget* parent, string)
{
	int res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}
	stopEPGScanning( true );

	paint();

	stopEPGScanning( false );

	bool doLoop = true;

	uint msg; uint data;
	while ( doLoop )
	{
		g_RCInput->getMsg( &msg, &data, 190);

		if ( msg == CRCInput::RC_timeout )
			doLoop = false;
		else
		{
			switch ( neutrino->handleMsg( msg, data ) )
			{
				case messages_return::cancel_all:
					res = menu_return::RETURN_EXIT_ALL;
				case messages_return::unhandled:
					doLoop = false;
			}
		}

	}

	hide();
	return res;
}

void CFlashUpdate::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CFlashUpdate::showGlobalStatus(int prog)
{
	static int lastprog = -1;

	if(prog>100)
	{
		prog = 100;
	}
	if(prog<0)
	{
		prog=0;
	}
	if(lastprog==prog)
	{
		return;
	}
	lastprog = prog;
	g_FrameBuffer->paintBox(x+10, globalstatusY, x+width-10, globalstatusY+10, COL_MENUCONTENT +2);
	if(prog!=0)
	{
		int pos = x+10+(prog*3);
		g_FrameBuffer->paintBox(x+10, globalstatusY,pos, globalstatusY+10, COL_MENUCONTENT +7);
	}
}

void CFlashUpdate::showLocalStatus(int prog)
{
	static int lastprog = -1;

	if(prog>100)
	{
		prog = 100;
	}
	if(prog<0)
	{
		prog=0;
	}

	if(lastprog==prog)
	{
		return;
	}
	lastprog = prog;

	g_FrameBuffer->paintBox(x+10, localstatusY, x+width-10, localstatusY+10, COL_MENUCONTENT +2);
	if(prog!=0)
	{
		int pos = x+10+(prog*3);
		g_FrameBuffer->paintBox(x+10, localstatusY, pos, localstatusY+10, COL_MENUCONTENT +7);
	}
}

void CFlashUpdate::showStatusMessage(string text)
{
	g_FrameBuffer->paintBox(statusTextX-5, statusTextY-mheight, x+width, statusTextY,  COL_MENUCONTENT);
	g_Fonts->menu->RenderString(statusTextX, statusTextY, width -(statusTextX- x), text.c_str(), COL_MENUCONTENT);
}


void CFlashUpdate::paint()
{
	int fp_fd = 0;
	if ((fp_fd = open("/dev/dbox/fp0",O_RDWR)) <= 0)
	{
		perror("[neutrino] open fp0");
		return;
	}

	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width- 10, g_Locale->getText("flashupdate.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10,  g_Locale->getText("flashupdate.action").c_str(), COL_MENUCONTENT);
	statusTextY = ypos+mheight;
	statusTextX = x+10 + g_Fonts->menu->getRenderWidth(g_Locale->getText("flashupdate.action").c_str())+10;
	ypos+= mheight;

	localstatusY = ypos+ mheight-20;
	showLocalStatus(0);
	ypos+= mheight+10;

	CHTTPUpdater http;
	http.setStatusViewer(this);

	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!http.getInfo())
		{
			showStatusMessage( g_Locale->getText("flashupdate.getinfofileerror") );
			close(fp_fd);
			return;
		}
	}
	//versionspr�fung.....
	showLocalStatus(100);
	showStatusMessage(g_Locale->getText("flashupdate.versioncheck").c_str());

	//installierte version...
	installed_major = installed_provider = 0;
	strcpy(installed_minor, "0");
	sscanf(g_settings.softupdate_currentversion, "%d.%d.%s", &installed_major, &installed_provider, (char*) &installed_minor);

	//neue version?
	new_major = new_provider = 0;
	strcpy(new_minor, "0");
	strcpy(new_md5sum, "");

	string	sFileName = gTmpPath+ http.VersionFile;
	FILE* fd = fopen(sFileName.c_str(), "r");
	if(!fd)
	{
		sFileName= sFileName+ ".txt";
		fd = fopen(sFileName.c_str(), "r");
	}

	if(!fd)
	{
		showStatusMessage( g_Locale->getText("flashupdate.getinfofileerror") );
		close(fp_fd);
		return;
	}
	char buf[100];
	if(fgets(buf,sizeof(buf),fd)!=NULL)
	{
		//printf("vstr: %s\n", buf);
		buf[28]= 0;
		sscanf(buf, "version: %d.%d.%s\n", &new_major, &new_provider, (char*) &new_minor);
	}
	else
	{
		showStatusMessage( g_Locale->getText("flashupdate.getinfofileerror") );
		close(fp_fd);
		return;
	}
	if(fgets(buf,sizeof(buf),fd)!=NULL)
	{
		sscanf(buf, "md5sum: %s\n", (char*) &new_md5sum);
	}
	fclose(fd);

	//printf("installed - %d : %d : %s\n", installed_major, installed_provider, installed_minor);
	//printf("new - %d : %d : %s\n", new_major, new_provider, new_minor);
	if(installed_major!=new_major)
	{
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.majorversiondiffer1").c_str() , COL_MENUCONTENT);
		ypos+= mheight;
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.majorversiondiffer2").c_str() , COL_MENUCONTENT);
		close(fp_fd);
		return;
	}
	if(installed_provider!=new_provider)
	{
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.providerversiondiffer1").c_str() , COL_MENUCONTENT);
		ypos+= mheight;
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.providerversiondiffer2").c_str() , COL_MENUCONTENT);
		close(fp_fd);
		return;
	}
	if(strcmp(installed_minor,new_minor)== 0)
	{
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.nonewversion1").c_str() , COL_MENUCONTENT);
		ypos+= mheight;
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.nonewversion2").c_str() , COL_MENUCONTENT);
		close(fp_fd);
		return;
	}


	//start update
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.globalprogress").c_str() , COL_MENUCONTENT);
	ypos+= mheight;

	globalstatusY = ypos+ mheight-20;
	ypos+= mheight >>1;
	showGlobalStatus(0);

	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!http.getFile( new_minor ))
		{
			showStatusMessage(g_Locale->getText("flashupdate.getupdatefileerror") );
			close(fp_fd);
			return;
		}
	}

	showGlobalStatus(25);

	//md5check...
	unsigned char   md5buffer[16];
	char            md5string[40]="";

	sFileName = gTmpPath+ http.ImageFile;
	showStatusMessage(g_Locale->getText("flashupdate.md5check") );
	if( md5_file(sFileName.c_str(), 1, (unsigned char*) &md5buffer))
	{
		showStatusMessage(g_Locale->getText("flashupdate.cantopenfile") );
		close(fp_fd);
		return;
	}
	for(int count=0;count<16;count++)
	{
		char tmp[6];
		sprintf((char*) &tmp, "%02x", md5buffer[count] );
		strcat(md5string, tmp);
	}
	printf("%s\n%s\n\n", new_md5sum, md5string);
	if(strcmp(md5string, new_md5sum)!=0)
	{
		showStatusMessage(g_Locale->getText("flashupdate.md5sumerror") );
		close(fp_fd);
		return;
	}

	showGlobalStatus(50);
	//flash it...
	CFlashTool ft;
	ft.setMTDDevice("/dev/mtd/3");
	ft.setStatusViewer(this);
	if(!ft.program(sFileName))
	{
		showStatusMessage( ft.getErrorMessage() );
		close(fp_fd);
		return;
	}

	//versionsinfo schreiben
	FILE* fd2 = fopen("/var/etc/version", "w");
	fprintf(fd2, "%d.%d.%s\n", new_major, new_provider, new_minor);
	fflush(fd2);
	fclose(fd2);

	showStatusMessage( g_Locale->getText("flashupdate.ready") );

	neutrino->exec(NULL, "savesettings");

	sleep(2);

	g_FrameBuffer->paintBoxRel(x, y+ hheight, width, height- hheight, COL_MENUCONTENT);
	g_Fonts->menu->RenderString(x+ 10, y+ mheight*3, width- 10, g_Locale->getText("flashupdate.reboot").c_str() , COL_MENUCONTENT);

	sleep(2);
	if (ioctl(fp_fd,FP_IOCTL_REBOOT)< 0)
	{
		perror("FP_IOCTL_REBOOT:");
	}
	close(fp_fd);
	sleep(20000);
}




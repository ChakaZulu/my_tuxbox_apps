#include "scan.h"
#include "../global.h"

CScanTs::CScanTs()
{
	width = 400;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

void CScanTs::sectionsdPauseScanning(int PauseIt)
{
    char rip[]="127.0.0.1";

    int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SAI servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(sectionsd::portNumber);
    inet_pton(AF_INET, rip, &servaddr.sin_addr);

    if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
    {
        perror("CScanTs - PauseScanning - couldn't connect to sectionsd!\n");
    }
    else
    {
        sectionsd::msgRequestHeader req;
        req.version = 2;
        req.command = sectionsd::pauseScanning;
        req.dataLength = 4;
        write(sock_fd, &req, sizeof(req));
        write(sock_fd, &PauseIt, req.dataLength);
        sectionsd::msgResponseHeader resp;
        memset(&resp, 0, sizeof(resp));
        if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
        {
            close(sock_fd);
            return;
        }
        close(sock_fd);
    }
}

bool CScanTs::scanReady(int *ts, int *services)
{
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		char *return_buf;
		st_rmsg		sendmessage;

		sendmessage.version=1;
		sendmessage.cmd = 'h';

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1505);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		#ifdef HAS_SIN_LEN
 			servaddr.sin_len = sizeof(servaddr); // needed ???
		#endif


		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
 	 		perror("neutrino: connect(zapit)");
			exit(-1);
		}

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		return_buf = (char*) malloc(4);
		memset(return_buf, 0, 4);
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
	
		//printf("scan: %s", return_buf);
		if (return_buf[0] == '-')
		{
			free(return_buf);
			close(sock_fd);
			return true;
		}
		else
		{
			if (recv(sock_fd, ts, sizeof(int),0) <= 0 ) {
				perror("recv(zapit)");
				exit(-1);
			}
			if (recv(sock_fd, services, sizeof(int),0) <= 0 ) {
				perror("recv(zapit)");
				exit(-1);
			}
			//printf("Found transponders: %d\n", *ts);
			//printf("Found channels: %d\n", *services);
			free(return_buf);
			close(sock_fd);
			return false;
		}

}

void CScanTs::startScan()
{
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		char *return_buf;
		st_rmsg		sendmessage;

		sendmessage.version=1;
		sendmessage.cmd = 'g';

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1505);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		#ifdef HAS_SIN_LEN
 			servaddr.sin_len = sizeof(servaddr); // needed ???
		#endif


		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
 	 		perror("neutrino: connect(zapit)");
			exit(-1);
		}

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		return_buf = (char*) malloc(4);
		memset(return_buf, 0, 4);
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
	
		printf("startscan: %s", return_buf);
		free(return_buf);
		close(sock_fd);
}


int CScanTs::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int key = g_RCInput->getKey(190);
	if(key != CRCInput::RC_ok)
	{
		hide();
		return CMenuTarget::RETURN_REPAINT;
	}
    sectionsdPauseScanning(1);
	startScan();
	
	char buf[100];
	int ts = 0;
	int services = 0;
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
	ypos= y+ hheight + (mheight >>1);
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.transponders").c_str(), COL_MENUCONTENT);
	ypos+= mheight;
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.services").c_str(), COL_MENUCONTENT);
	

	int xpos1 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.transponders").c_str());
	int xpos2 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.services").c_str());
	g_FrameBuffer->loadPal("radar.pal", 20, 100);
	int pos = 0;
	bool finish = false;
	while (!finish)
	{
		if(pos==0)
		{	//query zapit every xth loop
			finish = scanReady(&ts, &services);
		}

		ypos= y+ hheight + (mheight >>1);
	
		char filename[30];
		sprintf(filename, "radar%d.raw", pos);
		pos = (pos+1)%10;
		g_FrameBuffer->paintIcon8(filename, x+300,ypos+15, 20);

		sprintf(buf, "%d", ts);
		g_FrameBuffer->paintBoxRel(xpos1, ypos, 80, mheight, COL_MENUCONTENT);
		g_Fonts->menu->RenderString(xpos1, ypos+ mheight, width, buf, COL_MENUCONTENT); 
		ypos+= mheight;

		sprintf(buf, "%d", services);
		g_FrameBuffer->paintBoxRel(xpos2, ypos, 80, mheight, COL_MENUCONTENT);
		g_Fonts->menu->RenderString(xpos2, ypos+ mheight, width, buf, COL_MENUCONTENT);
		
		//g_RCInput->getKey(190);
		usleep(100000);
	}


	hide();
    neutrino->channelsInit();
    sectionsdPauseScanning(0);
	return CMenuTarget::RETURN_REPAINT;
}

void CScanTs::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


void CScanTs::paint()
{
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("scants.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);
	
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.info1").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.info2").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

}

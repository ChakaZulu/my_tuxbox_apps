/*
  Neutrino-GUI  -   DBoxII-Project

  Movieplayer (c) 2003 by gagga
  Based on code by Dirch, obi and the Metzler Bros. Thanks.

  $Id: movieplayer.cpp,v 1.62 2004/01/17 23:40:54 zwen Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

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

/* KNOWN ISSUES:
   - AC3 handling does not work
   - TS which are played back from CIFS drives may not work in a good quality.
*/


/* TODOs / Release Plan:
   - always: fix bugs
   (currently planned order)
   - Nicer UI
   - Chapter support for DVD and (S)VCD
   - Playing from Bookmarks
   - MP3 HTTP streaming
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_DVB_API_VERSION >= 3

#include <gui/movieplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>

#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>

#include <algorithm>
#include <fstream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <transform.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <poll.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

#define AVIA_AV_STREAM_TYPE_0           0x00
#define AVIA_AV_STREAM_TYPE_SPTS        0x01
#define AVIA_AV_STREAM_TYPE_PES         0x02
#define AVIA_AV_STREAM_TYPE_ES          0x03

#define STREAMTYPE_DVD	1
#define STREAMTYPE_SVCD	2
#define STREAMTYPE_FILE	3

#define MOVIEPLAYER_ConnectLineBox_Width	15

#define RINGBUFFERSIZE 348*188*10
#define MAXREADSIZE 348*188
#define MINREADSIZE 348*188


static CMoviePlayerGui::state playstate;
static bool isTS;
int speed = 1;
static long fileposition;
ringbuffer_t *ringbuf;
bool bufferfilled;
int streamingrunning;
unsigned short pida, pidv, ac3;
CHintBox *hintBox;
CHintBox *bufferingBox;
bool avpids_found;

//------------------------------------------------------------------------
size_t
CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	return size * nmemb;
}

//------------------------------------------------------------------------

CMoviePlayerGui::CMoviePlayerGui() : bookmarkfile('\t')
{
	frameBuffer = CFrameBuffer::getInstance ();

	visible = false;
	selected = 0;

	filebrowser = new CFileBrowser ();
	filebrowser->Multi_Select = false;
	filebrowser->Dirs_Selectable = false;
	videofilefilter.addFilter ("ts");
	videofilefilter.addFilter ("ps");
	videofilefilter.addFilter ("mpg");
	videofilefilter.addFilter ("mpeg");
	videofilefilter.addFilter ("m2p");
	videofilefilter.addFilter ("avi");
	videofilefilter.addFilter ("vob");
	filebrowser->Filter = &videofilefilter;
	if (strlen (g_settings.network_nfs_moviedir) != 0)
		Path = g_settings.network_nfs_moviedir;
	else
		Path = "/";
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui ()
{
	delete filebrowser;
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);

}

//------------------------------------------------------------------------
int
CMoviePlayerGui::exec (CMenuTarget * parent, const std::string & actionKey)
{
	// read Bookmarkfile
	int bookmarkCount=0;
	if(bookmarkfile.loadConfig("/var/tuxbox/config/bookmarks")) {
        bookmarkCount = bookmarkfile.getInt32( "bookmarkcount", 0 );
        printf("bookmarkcount:%d\n",bookmarkCount);
        for (int i=0;i<bookmarkCount;i++) {
            char counterstring[4];
            sprintf(counterstring, "%d",(i+1));
            std::string bookmarkstring = "bookmark";
            bookmarkstring += counterstring;
            std::string bookmarknamestring = bookmarkstring + ".name";
            std::string bookmarkurlstring = bookmarkstring + ".url";
            std::string bookmarktimestring = bookmarkstring + ".time";
            bookmarkname[i] = bookmarkfile.getString(bookmarknamestring,"name");
            printf("bookmarkname: %s\n",bookmarkname[i].c_str());
            bookmarkurl[i] = bookmarkfile.getString(bookmarkurlstring,"url");
            printf("bookmarkurl: %s\n",bookmarkurl[i].c_str());
            bookmarktime[i] = bookmarkfile.getString(bookmarktimestring,"time");
            printf("bookmarktime: %s\n",bookmarktime[i].c_str());
        }
    }
    
    current = -1;
	selected = 0;

	printf("[movieplayer.cpp] actionKey=%s\n",actionKey.c_str());
	
	//define screen width
	width = 710;
	if ((g_settings.screen_EndX - g_settings.screen_StartX) <
	    width + MOVIEPLAYER_ConnectLineBox_Width)
		width =
			(g_settings.screen_EndX - g_settings.screen_StartX) -
			MOVIEPLAYER_ConnectLineBox_Width;

	//define screen height
	height = 570;
	if ((g_settings.screen_EndY - g_settings.screen_StartY) < height)
		height = (g_settings.screen_EndY - g_settings.screen_StartY);
	sheight      = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	buttonHeight = std::min(25, sheight);
	theight      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	title_height = fheight * 2 + 20 + sheight + 4;
	info_height = fheight * 2;
	listmaxshow =
		(height - info_height - title_height - theight -
		 2 * buttonHeight) / (fheight);
	height = theight + info_height + title_height + 2 * buttonHeight + listmaxshow * fheight;	// recalc height

	x =
		(((g_settings.screen_EndX - g_settings.screen_StartX) -
		  (width + MOVIEPLAYER_ConnectLineBox_Width)) / 2) + g_settings.screen_StartX +
		MOVIEPLAYER_ConnectLineBox_Width;
	y =
		(((g_settings.screen_EndY - g_settings.screen_StartY) - height) / 2) +
		g_settings.screen_StartY;

	if (parent)
	{
		parent->hide ();
	}

	// set zapit in standby mode
	g_Zapit->setStandby (true);

	// tell neutrino we're in ts_mode
	CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
						 NeutrinoMessages::mode_ts);
	// remember last mode
	m_LastMode =
		(CNeutrinoApp::getInstance ()->
		 getLastMode () /*| NeutrinoMessages::norezap */ );

	// Stop sectionsd
	g_Sectionsd->setPauseScanning (true);


	if (actionKey=="fileplayback") {
        PlayStream (STREAMTYPE_FILE);	
	}
	else if (actionKey=="dvdplayback") {
        PlayStream (STREAMTYPE_DVD);
	}
	else if (actionKey=="vcdplayback") {
        PlayStream (STREAMTYPE_SVCD);
	}
	else if (actionKey=="tsplayback") {
        isTS=true;
        PlayFile();
	}
	

	//stop();
	hide ();

	g_Zapit->setStandby (false);

	// Start Sectionsd
	g_Sectionsd->setPauseScanning (false);

	// Restore last mode
	CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
						 m_LastMode);

	// always exit all
	return menu_return::RETURN_REPAINT;
}

//------------------------------------------------------------------------
CURLcode sendGetRequest (const std::string & url) {
	CURL *curl;
	CURLcode httpres;

	std::string response = "";
  
	curl = curl_easy_init ();
	curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt (curl, CURLOPT_FILE, (void *) &response);
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true); 
	httpres = curl_easy_perform (curl);
	//printf ("[movieplayer.cpp] HTTP Result: %d\n", httpres);
	curl_easy_cleanup (curl);
	return httpres;
}

//------------------------------------------------------------------------
bool VlcSendPlaylist(char* mrl)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	
	// empty playlist
	std::string emptyurl = baseurl + "?control=empty";
	httpres = sendGetRequest(emptyurl);
	printf ("[movieplayer.cpp] HTTP Result (emptyurl): %d\n", httpres);
	if (httpres != 0)
	{
		DisplayErrorMessage(g_Locale->getText("movieplayer.nostreamingserver")); // UTF-8
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}

	// add MRL
	/* demo MRLs:
	   - DVD: dvdsimple:D:@1:1
	   - DemoMovie: c:\\TestMovies\\dolby.mpg
	   - SVCD: vcd:D:@1:1
	*/
	std::string addurl = baseurl + "?control=add&mrl=" + (char*) mrl;
	httpres = sendGetRequest(addurl);
	return (httpres==0);
}
#define TRANSCODE_VIDEO_OFF 0
#define TRANSCODE_VIDEO_MPEG1 1
#define TRANSCODE_VIDEO_MPEG2 2
//------------------------------------------------------------------------
bool VlcRequestStream(int  transcodeVideo, int transcodeAudio)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	
	// add sout (URL encoded)
	// Example(mit transcode zu mpeg1): ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	// Example(ohne transcode zu mpeg1): ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	//TODO make this nicer :-)
	std::string souturl;

	//Resolve Resolution from Settings...
	char* res_horiz = "";
	char* res_vert = "";
	switch (g_settings.streaming_resolution)
	{
		case 0:
			res_horiz = "352";
			res_vert = "288";
			break;
		case 1:
			res_horiz = "352";
			res_vert = "576";
			break;
		case 2:
			res_horiz = "480";
			res_vert = "576";
			break;
		case 3:
			res_horiz = "704";
			res_vert = "576";
			break;
		default:
			res_horiz = "352";
			res_vert = "288";
	} //switch
	souturl = "#";
	if(transcodeVideo!=TRANSCODE_VIDEO_OFF || transcodeAudio!=0)
	{
		souturl += "transcode{";
		if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
		{
			const char* codec = (transcodeVideo == TRANSCODE_VIDEO_MPEG1) ? "mpgv" : "mp2v";
			souturl += std::string("vcodec=") + codec + ",vb=" + g_settings.streaming_videorate;
			souturl += std::string(",width=") + res_horiz + ",height=" + res_vert;
		}
		if(transcodeAudio!=0)
		{
			if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
				souturl += ",";
			souturl += std::string("acodec=mpga,ab=") + g_settings.streaming_audiorate + ",channels=2";
		}
		souturl += "}:";
	}
	souturl += "duplicate{dst=std{access=http,mux=ts,url=:";
	souturl += g_settings.streaming_server_port;
	souturl += "/dboxstream}}";
	
	char *tmp = curl_escape (souturl.c_str (), 0);
	printf("[movieplayer.cpp] URL      : %s?sout=%s\n",baseurl.c_str(), souturl.c_str());
	printf("[movieplayer.cpp] URL(enc) : %s?sout=%s\n",baseurl.c_str(), tmp);
	std::string url = baseurl + "?sout=" + tmp;
	curl_free(tmp);
	httpres = sendGetRequest(url);

	// play MRL
	std::string playurl = baseurl + "?control=play&item=0";
	httpres = sendGetRequest(playurl);

	return true; // TODO error checking
}
//------------------------------------------------------------------------
void *
ReceiveStreamThread (void *mrl)
{
	printf ("[movieplayer.cpp] ReceiveStreamThread started\n");
	int skt;

	int nothingreceived=0;
	
	// Get Server and Port from Config

	if (!VlcSendPlaylist((char*)mrl))
	{
		DisplayErrorMessage(g_Locale->getText("movieplayer.nostreamingserver")); // UTF-8
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}
	

	int transcodeVideo, transcodeAudio;
	std::string sMRL=(char*)mrl;
	//Menu Option Force Transcode: Transcode all Files, including mpegs.
	if ((!memcmp((char*)mrl, "vcd:", 4) ||
		  !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "mpg") || 
		  !strcasecmp(sMRL.substr(sMRL.length()-4).c_str(), "mpeg") ||
		  !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "m2p")))
	{
		if (g_settings.streaming_force_transcode_video)
			transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		else
			transcodeVideo=0;
		transcodeAudio=g_settings.streaming_transcode_audio;
	}
	else
	{
		transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		if((!memcmp((char*)mrl, "dvd", 3) && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
			g_settings.streaming_force_avi_rawaudio)
			transcodeAudio=0;
		else
			transcodeAudio=1;
	}
	VlcRequestStream(transcodeVideo, transcodeAudio);

// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
// Unfortunately HTTP HEAD requests are not supported by VLC :(
// vlc 0.6.3 and up may support HTTP HEAD requests.

// Open HTTP connection to VLC

	const char *server = g_settings.streaming_server_ip.c_str ();
	int port;
	sscanf (g_settings.streaming_server_port, "%d", &port);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (port);
	servAddr.sin_addr.s_addr = inet_addr (server);

	printf ("[movieplayer.cpp] Server: %s\n", server);
	printf ("[movieplayer.cpp] Port: %d\n", port);
	char buf[RINGBUFFERSIZE];
	int len;

	while (true)
	{

		//printf ("[movieplayer.cpp] Trying to call socket\n");
		skt = socket (AF_INET, SOCK_STREAM, 0);

		printf ("[movieplayer.cpp] Trying to connect socket\n");
		if (connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
		{
			perror ("SOCKET");
			playstate = CMoviePlayerGui::STOPPED;
			pthread_exit (NULL);
		}
		fcntl (skt, O_NONBLOCK);
		printf ("[movieplayer.cpp] Socket OK\n");

		// Skip HTTP header
		const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
		int msglen = strlen (msg);
		if (send (skt, msg, msglen, 0) == -1)
		{
			perror ("send()");
			playstate = CMoviePlayerGui::STOPPED;
			pthread_exit (NULL);
		}

		printf ("[movieplayer.cpp] GET Sent\n");

		// Skip HTTP Header
		int found = 0;
		char line[200];
		strcpy (line, "");
		while (true)
		{
			len = recv (skt, buf, 1, 0);
			strncat (line, buf, 1);
			if (strcmp (line, "HTTP/1.0 404") == 0)
			{
				printf ("[movieplayer.cpp] VLC still does not send. Retrying...\n");
				close (skt);
				break;
			}
			if ((((found & (~2)) == 0) && (buf[0] == '\r')) || /* found == 0 || found == 2 */
			    (((found & (~2)) == 1) && (buf[0] == '\n')))   /* found == 1 || found == 3 */
			{
				if (found == 3)
					goto vlc_is_sending;
				else
					found++;
			}
			else
			{
				found = 0;
			}
		}
	}
 vlc_is_sending:
	printf ("[movieplayer.cpp] Now VLC is sending. Read sockets created\n");
	hintBox->hide ();
	bufferingBox->paint ();
	printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");

	int size;
	streamingrunning = 1;
	int fd = open ("/tmp/tmpts", O_CREAT | O_WRONLY);

	struct pollfd poller[1];
	poller[0].fd = skt;
	poller[0].events = POLLIN | POLLPRI;
	int pollret;

	while (streamingrunning == 1)
	{
		while ((size = ringbuffer_write_space (ringbuf)) == 0)
		{
			if (playstate == CMoviePlayerGui::STOPPED)
			{
				close(skt);
				pthread_exit (NULL);
			}
			if (!avpids_found)
			{
				// find apid and vpid. Easiest way to do that is to write the TS to a file 
				// and use the usual find_avpids function. This is not even overhead as the
				// buffer needs to be prefilled anyway
				close (fd);
				fd = open ("/tmp/tmpts", O_RDONLY);
				//Use global pida, pidv
				//unsigned short pidv = 0, pida = 0;
				find_avpids (fd, &pidv, &pida);
				lseek(fd, 0, SEEK_SET);
				ac3 = (is_audio_ac3(fd) > 0);
				close (fd);
				printf ("[movieplayer.cpp] ReceiveStreamThread: while streaming found pida: 0x%04X ; pidv: 0x%04X ; ac3: %d\n",
					pida, pidv, ac3);
				avpids_found = true;
			}
			if (!bufferfilled) {
				bufferingBox->hide ();
				//TODO reset drivers?
				bufferfilled = true;
			}
		}
		//printf("[movieplayer.cpp] ringbuf write space:%d\n",size);

		if (playstate == CMoviePlayerGui::STOPPED)
		{
			close(skt);
			pthread_exit (NULL);
		}

		pollret = poll (poller, (unsigned long) 1, -1);

		if ((pollret < 0) ||
		    ((poller[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0))
		{
			perror ("Error while polling()");
			playstate = CMoviePlayerGui::STOPPED;
			close(skt);
			pthread_exit (NULL);
		}


		if ((poller[0].revents & (POLLIN | POLLPRI)) != 0)
			len = recv (poller[0].fd, buf, size, 0);
		else
			len = 0;

		if (len > 0)
		{
			nothingreceived = 0;
			//printf ("[movieplayer.cpp] bytes received:%d\n", len);
			if (!avpids_found)
			{
				write (fd, buf, len);
			}
		}
		else {
			if (playstate == CMoviePlayerGui::PLAY) {
				nothingreceived++;
				if (nothingreceived > 200) {
					printf ("[movieplayer.cpp] ReceiveStreamthread: Didn't receive for a while. Stopping.\n");
					playstate = CMoviePlayerGui::STOPPED;	
				}	
			}
		}
      
		while (len > 0)
		{
			len -= ringbuffer_write (ringbuf, buf, len);
		}

	}
	close(skt);
	pthread_exit (NULL);
}


//------------------------------------------------------------------------
void *
PlayStreamThread (void *mrl)
{
	char buf[348 * 188];
	bool failed = false;
	// use global pida and pidv
	pida = 0, pidv = 0, ac3 = 0;
	int done, dmxa = 0, dmxv = 0, dvr = 0, adec = 0, vdec = 0;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	ringbuf = ringbuffer_create (RINGBUFFERSIZE);
	printf ("[movieplayer.cpp] ringbuffer created\n");

	bufferingBox = new CHintBox("messagebox.info", g_Locale->getText("movieplayer.buffering")); // UTF-8

	CURLcode httpres;

	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	printf ("[movieplayer.cpp] mrl:%s\n", (char *) mrl);
	pthread_t rcst;
	pthread_create (&rcst, 0, ReceiveStreamThread, mrl);
	//printf ("[movieplayer.cpp] ReceiveStreamThread created\n");
	if ((dmxa =
	     open (DMX, O_RDWR | O_NONBLOCK)) < 0
	    || (dmxv =
		open (DMX,
		      O_RDWR | O_NONBLOCK)) < 0
	    || (dvr =
		open (DVR,
		      O_WRONLY | O_NONBLOCK)) < 0
	    || (adec =
		open (ADEC,
		      O_RDWR | O_NONBLOCK)) < 0
	    || (vdec = open (VDEC, O_RDWR | O_NONBLOCK)) < 0)
	{
		failed = true;
	}

	playstate = CMoviePlayerGui::SOFTRESET;
	printf ("[movieplayer.cpp] read starting\n");
	size_t readsize, len;
	len = 0;
	bool driverready = false;
	std::string pauseurl   = baseurl + "?control=pause";
	std::string unpauseurl = baseurl + "?control=pause";
	while (playstate > CMoviePlayerGui::STOPPED)
	{
		readsize = ringbuffer_read_space (ringbuf);
		if (readsize > MAXREADSIZE)
		{
			readsize = MAXREADSIZE;
		}
		//printf("[movieplayer.cpp] readsize=%d\n",readsize);
		if (bufferfilled)
		{
			if (!driverready)
			{
				driverready = true;
				// pida and pidv should have been set by ReceiveStreamThread now
				printf ("[movieplayer.cpp] PlayStreamthread: while streaming found pida: 0x%04X ; pidv: 0x%04X\n",
					pida, pidv);

				p.input = DMX_IN_DVR;
				p.output = DMX_OUT_DECODER;
				p.flags = DMX_IMMEDIATE_START;
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				if (ioctl (dmxa, DMX_SET_PES_FILTER, &p) < 0)
					failed = true;
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				if (ioctl (dmxv, DMX_SET_PES_FILTER, &p) < 0)
					failed = true;
				if (ac3 == 1) {
					if (ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL)<0)
					{
						perror("AUDIO_SET_BYPASS_MODE");
						failed=true;
					}
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				if (ioctl (adec, AUDIO_PLAY) < 0)
				{
					perror ("AUDIO_PLAY");
					failed = true;
				}

				if (ioctl (vdec, VIDEO_PLAY) < 0)
				{
					perror ("VIDEO_PLAY");
					failed = true;
				}

				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				printf ("[movieplayer.cpp] PlayStreamthread: Driver successfully set up\n");
				bufferingBox->hide ();
			}

			len = ringbuffer_read (ringbuf, buf, (readsize / 188) * 188);

			switch (playstate)
			{
			case CMoviePlayerGui::PAUSE:
				//ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);

				// pause VLC
				httpres = sendGetRequest(pauseurl);

				while (playstate == CMoviePlayerGui::PAUSE)
				{
					//ioctl (dmxv, DMX_STOP);	
					//ioctl (dmxa, DMX_STOP);
					usleep(100000); // no busy wait
				}
				// unpause VLC
				httpres = sendGetRequest(unpauseurl);
				speed = 1;
				break;
			case CMoviePlayerGui::RESYNC:
			    printf ("[movieplayer.cpp] Resyncing\n");
				ioctl (dmxa, DMX_STOP);
				ioctl (dmxa, DMX_START);
				playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::PLAY:
				if (len < MINREADSIZE)
				{
					bufferingBox->paint ();
					printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");
					bufferfilled = false;
			
				}
				//printf ("[movieplayer.cpp] [%d bytes read from ringbuf]\n", len);
				done = 0;
				while (len > 0)
				{
					wr = write (dvr, &buf[done], len);
					//printf ("[movieplayer.cpp] [%d bytes written]\n", wr);
					len -= wr;
					done += wr;
				}
				break;
			case CMoviePlayerGui::SOFTRESET:
				ioctl (vdec, VIDEO_STOP);
				ioctl (adec, AUDIO_STOP);
				ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);
				ioctl (vdec, VIDEO_PLAY);
				if (g_settings.streaming_ac3_enabled == 1) {
					ioctl (adec, AUDIO_SET_BYPASS_MODE, 0UL );
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				ioctl (adec, AUDIO_PLAY);
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				ioctl (dmxa, DMX_SET_PES_FILTER, &p);
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				ioctl (dmxv, DMX_SET_PES_FILTER, &p);
				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				speed = 1;
				playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
				break;
			}
		}
	}

	ioctl (vdec, VIDEO_STOP);
	ioctl (adec, AUDIO_STOP);
	ioctl (dmxv, DMX_STOP);
	ioctl (dmxa, DMX_STOP);
	close (dmxa);
	close (dmxv);
	close (dvr);
	close (adec);
	close (vdec);

	// stop VLC
	std::string stopurl = baseurl + "?control=stop";
	httpres = sendGetRequest(stopurl);

	printf ("[movieplayer.cpp] Waiting for RCST to stop\n");
	pthread_join (rcst, NULL);
	printf ("[movieplayer.cpp] Seems that RCST was stopped succesfully\n");
  
	// Some memory clean up
	ringbuffer_free(ringbuf);
	delete bufferingBox;
	delete hintBox;
	
	pthread_exit (NULL);
}

//------------------------------------------------------------------------
void *
PlayFileThread (void *filename)
{
	bool failed = false;
	unsigned char buf[384 * 188 * 2];
	unsigned short pida = 0, pidv = 0, ac3=0;
	int done, fd = 0, dmxa = 0, dmxv = 0, dvr = 0, adec = 0, vdec = 0;
	struct dmx_pes_filter_params p;
	ssize_t wr = 0;
	ssize_t cache = sizeof (buf);
	size_t r = 0;
	if ((char *) filename == NULL)
	{
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if ((fd = open ((char *) filename, O_RDONLY | O_LARGEFILE)) < 0)
	{
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	// todo: check if file is valid ts or pes
	if (isTS)
	{
		find_avpids (fd, &pidv, &pida);
		lseek(fd, 0, SEEK_SET);
		ac3 = is_audio_ac3 (fd);
		printf ("[movieplayer.cpp] found pida: 0x%04X ; pidv: 0x%04X ; ac3: %d\n",
			pida, pidv, ac3);
	}
	else
	{				// Play PES
		pida = 0x900;
		pidv = 0x8ff;
	}

	lseek (fd, 0L, SEEK_SET);
	if ((dmxa = open (DMX, O_RDWR)) < 0
	    || (dmxv = open (DMX, O_RDWR)) < 0
	    || (dvr = open (DVR, O_WRONLY)) < 0
	    || (adec = open (ADEC, O_RDWR)) < 0 || (vdec = open (VDEC, O_RDWR)) < 0)
	{
		failed = true;
	}

	p.input = DMX_IN_DVR;
	p.output = DMX_OUT_DECODER;
	p.flags = DMX_IMMEDIATE_START;
	p.pid = pida;
	p.pes_type = DMX_PES_AUDIO;
	if (ioctl (dmxa, DMX_SET_PES_FILTER, &p) < 0)
		failed = true;
	p.pid = pidv;
	p.pes_type = DMX_PES_VIDEO;
	if (ioctl (dmxv, DMX_SET_PES_FILTER, &p) < 0)
		failed = true;
	fileposition = 0;
	if (isTS && !failed)
	{
		while ((r = read (fd, buf, cache)) > 0 && playstate >= CMoviePlayerGui::PLAY)
		{
			done = 0;
			wr = 0;
			fileposition += r;
			switch (playstate)
			{
			case CMoviePlayerGui::PAUSE:
				while (playstate == CMoviePlayerGui::PAUSE)
				{
					ioctl (dmxa, DMX_STOP);
				}
				break;
			case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
				ioctl (dmxa, DMX_STOP);
				lseek (fd, cache * speed, SEEK_CUR);
				fileposition += cache * speed;
				break;
			case CMoviePlayerGui::SOFTRESET:
				ioctl (vdec, VIDEO_STOP);
				ioctl (adec, AUDIO_STOP);
				ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);
				ioctl (vdec, VIDEO_PLAY);
				if (ac3 == 1) {
					ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL);
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				ioctl (adec, AUDIO_PLAY);
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				ioctl (dmxa, DMX_SET_PES_FILTER, &p);
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				ioctl (dmxv, DMX_SET_PES_FILTER, &p);
				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				speed = 1;
				playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::PLAY:
			case CMoviePlayerGui::RESYNC:
				break;
			}

			do
			{
				wr = write (dvr, &buf[done], r);
				if (!done)
					cache = wr;
				done += wr;
				r -= wr;
			}
			while (r);
		}
	}
	else if (!failed)
	{
		ioctl (vdec, VIDEO_PLAY);
		if (g_settings.streaming_ac3_enabled == 1) {
			ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL);
		}
		else
		{
			ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
		}
		ioctl (adec, AUDIO_PLAY);
		ioctl (dmxv, DMX_START);
		ioctl (dmxa, DMX_START);
		pes_to_ts2 (fd, dvr, pida, pidv, (const int *)&playstate);	// VERY bad performance!!!
	}

	ioctl (vdec, VIDEO_STOP);
	ioctl (adec, AUDIO_STOP);
	ioctl (dmxv, DMX_STOP);
	ioctl (dmxa, DMX_STOP);
	close (fd);
	close (dmxa);
	close (dmxv);
	close (dvr);
	close (adec);
	close (vdec);
	if (playstate != CMoviePlayerGui::STOPPED)
	{
		playstate = CMoviePlayerGui::STOPPED;
		g_RCInput->postMsg (CRCInput::RC_red, 0);	// for faster exit in PlayStream(); do NOT remove!
	}

	pthread_exit (NULL);
}


void updateLcd(const std::string & sel_filename)
{
	char tmp[20];
	std::string lcd;
	
	switch(playstate)
	{
	case CMoviePlayerGui::PAUSE:
		lcd = "|| (";
		lcd += sel_filename;
		lcd += ')';
		break;
	case CMoviePlayerGui::REW:
		sprintf(tmp, "%dx<< ", speed);
		lcd = tmp;
		lcd += sel_filename;
		break;
	case CMoviePlayerGui::FF:
		sprintf(tmp, "%dx>> ", speed);
		lcd = tmp;
		lcd += sel_filename;
		break;
	default:
		lcd = "> ";
		lcd += sel_filename;
		break;
	}
	
	CLCD::getInstance()->showServicename(lcd);
}

//------------------------------------------------------------------------
void
CMoviePlayerGui::PlayStream (int streamtype)
{
	uint msg, data;
	std::string sel_filename;
	bool update_info = true, start_play = false, exit =
		false, open_filebrowser = true;
	char mrl[200];
	if (streamtype == STREAMTYPE_DVD)
	{
		strcpy (mrl, "dvdsimple:");
		strcat (mrl, g_settings.streaming_server_cddrive);
		strcat (mrl, "@1:1");
		printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
		sel_filename = "DVD";
		open_filebrowser = false;
		start_play = true;
	}
	else if (streamtype == STREAMTYPE_SVCD)
	{
		strcpy (mrl, "vcd:");
		strcat (mrl, g_settings.streaming_server_cddrive);
		strcat (mrl, "@1:1");
		printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
		sel_filename = "(S)VCD";
		open_filebrowser = false;
		start_play = true;

	}

	playstate = CMoviePlayerGui::STOPPED;
	/* playstate == CMoviePlayerGui::STOPPED         : stopped
	 * playstate == CMoviePlayerGui::PREPARING       : preparing stream from server
	 * playstate == CMoviePlayerGui::ERROR           : error setting up server
	 * playstate == CMoviePlayerGui::PLAY            : playing
	 * playstate == CMoviePlayerGui::PAUSE           : pause-mode
	 * playstate == CMoviePlayerGui::FF              : fast-forward
	 * playstate == CMoviePlayerGui::REW             : rewind
	 * playstate == CMoviePlayerGui::SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
	 */
	do
	{
		if (exit)
		{
			exit = false;
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				break;
			}
		}

		if (open_filebrowser)
		{
			open_filebrowser = false;
			filename = NULL;
			char startDir[40 + 6];
			strcpy (startDir, "vlc://");
			strcat (startDir, g_settings.streaming_server_startdir);
			printf ("[movieplayer.cpp] Startdir: %s\n", startDir);
			if (filebrowser->exec (startDir))
			{
				Path = filebrowser->getCurrentDir ();
				CFile * file;
				if ((file = filebrowser->getSelectedFile()) != NULL)
				{
					filename = file->Name.c_str();
					sel_filename = file->getFileName();
					//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
					int namepos = file->Name.rfind("vlc://");
					std::string mrl_str = file->Name.substr(namepos + 6);
					char *tmp = curl_escape (mrl_str.c_str (), 0);
					strncpy (mrl, tmp, sizeof (mrl) - 1);
					curl_free (tmp);
					printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);

					update_info = true;
					start_play = true;
				}
			}
			else
			{
				if (playstate == CMoviePlayerGui::STOPPED)
					break;
			}

			CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
		}

		if (update_info)
		{
			update_info = false;
			updateLcd(sel_filename);
		}

		if (start_play)
		{
			start_play = false;
			bufferfilled = false;
			avpids_found=false;
	  
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				pthread_join (rct, NULL);
			}
			//TODO: Add Dialog (Remove Dialog later)
			hintBox = new CHintBox("messagebox.info", g_Locale->getText("movieplayer.pleasewait")); // UTF-8
			hintBox->paint();
			if (pthread_create (&rct, 0, PlayStreamThread, (void *) mrl) != 0)
			{
				break;
			}
			playstate = CMoviePlayerGui::SOFTRESET;
		}

		g_RCInput->getMsg (&msg, &data, 100);	// 10 secs..
		if (msg == CRCInput::RC_home || msg == CRCInput::RC_red)
		{
			//exit play
			exit = true;
		}
		else if (msg == CRCInput::RC_yellow)
		{
			update_info = true;
			playstate = (playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
		}
		else if (msg == CRCInput::RC_green)
		{
			if (playstate == CMoviePlayerGui::PLAY) playstate = CMoviePlayerGui::RESYNC;
		}
		else if (msg == CRCInput::RC_help)
 		{
     		std::string helptext = g_Locale->getText("movieplayer.help");
     		std::string fullhelptext = helptext + "\nVersion: $Revision: 1.62 $\n\nMovieplayer (c) 2003 by gagga";
     		ShowMsgUTF("messagebox.info", fullhelptext.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
 		}
		else
			if (msg == NeutrinoMessages::RECORD_START
			    || msg == NeutrinoMessages::ZAPTO
			    || msg == NeutrinoMessages::STANDBY_ON
			    || msg == NeutrinoMessages::SHUTDOWN
			    || msg == NeutrinoMessages::SLEEPTIMER)
			{
				// Exit for Record/Zapto Timers
				exit = true;
				g_RCInput->postMsg (msg, data);
			}
			else
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					exit = true;
				}
	}
	while (playstate >= CMoviePlayerGui::PLAY);
	pthread_join (rct, NULL);
}

void
CMoviePlayerGui::PlayFile (void)
{
	uint msg, data;
	std::string sel_filename;
	bool update_lcd = true, open_filebrowser =
		true, start_play = false, exit = false;
	playstate = CMoviePlayerGui::STOPPED;
	/* playstate == CMoviePlayerGui::STOPPED         : stopped
	 * playstate == CMoviePlayerGui::PLAY            : playing
	 * playstate == CMoviePlayerGui::PAUSE           : pause-mode
	 * playstate == CMoviePlayerGui::FF              : fast-forward
	 * playstate == CMoviePlayerGui::REW             : rewind
	 * playstate == CMoviePlayerGui::SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
	 */
	do
	{
		if (exit)
		{
			exit = false;
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				break;
			}
		}

		if (open_filebrowser)
		{
			open_filebrowser = false;
			filename = NULL;
			if (filebrowser->exec(g_settings.network_nfs_moviedir))
			{
				Path = filebrowser->getCurrentDir();
				CFile * file;
				if ((file = filebrowser->getSelectedFile()) != NULL)
				{
					filename = file->Name.c_str();
					update_lcd = true;
					start_play = true;
					sel_filename = filebrowser->getSelectedFile()->getFileName();
				}
			}
			else
			{
				if (playstate == CMoviePlayerGui::STOPPED)
					break;
			}

			CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
		}

		if (update_lcd)
		{
			update_lcd = false;
			updateLcd(sel_filename);
		}

		if (start_play)
		{
			start_play = false;
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				pthread_join (rct, NULL);
			}

			if (pthread_create
			    (&rct, 0, PlayFileThread, (void *) filename) != 0)
			{
				break;
			}
			playstate = CMoviePlayerGui::SOFTRESET;
		}

		g_RCInput->getMsg (&msg, &data, 100);	// 10 secs..
		if (msg == CRCInput::RC_red || msg == CRCInput::RC_home)
		{
			//exit play
			exit = true;
		}
		else if (msg == CRCInput::RC_yellow)
		{
			update_lcd = true;
			playstate = (playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
		}
		else if (msg == CRCInput::RC_blue)
		{
			FILE *bookmarkfile;
			char bookmarkfilename[] =
				"/var/tuxbox/config/movieplayer.bookmarks";
			bookmarkfile = fopen (bookmarkfilename, "a");
			fprintf (bookmarkfile, "%s\n", filename);
			fprintf (bookmarkfile, "%ld\n", fileposition);
			fclose (bookmarkfile);
		}
 		else if (msg == CRCInput::RC_help)
 		{
     		std::string helptext = g_Locale->getText("movieplayer.help");
     		std::string fullhelptext = helptext + "\nVersion: $Revision: 1.62 $\n\nMovieplayer (c) 2003 by gagga";
     		ShowMsgUTF("messagebox.info", fullhelptext.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
 		}
        else if (msg == CRCInput::RC_left)
		{
			// rewind
			if (speed > 1)
				speed = 1;
			speed *= -2;
			speed *= (speed > 1 ? -1 : 1);
			playstate = CMoviePlayerGui::REW;
			update_lcd = true;
		}
		else if (msg == CRCInput::RC_right)
		{
			// fast-forward
			if (speed < 1)
				speed = 1;
			speed *= 2;
			playstate = CMoviePlayerGui::FF;
			update_lcd = true;
		}
		else if (msg == CRCInput::RC_up || msg == CRCInput::RC_down)
		{
			// todo: next/prev file
		}
		else if (msg == CRCInput::RC_help)
		{
			// todo: infobar
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (playstate > CMoviePlayerGui::PLAY)
			{
				update_lcd = true;
				playstate = CMoviePlayerGui::SOFTRESET;
			}
			else
				open_filebrowser = true;
		}
		else
			if (msg == NeutrinoMessages::RECORD_START
			    || msg == NeutrinoMessages::ZAPTO
			    || msg == NeutrinoMessages::STANDBY_ON
			    || msg == NeutrinoMessages::SHUTDOWN
			    || msg == NeutrinoMessages::SLEEPTIMER)
			{
				// Exit for Record/Zapto Timers
				isTS = true;		// also exit in PES Mode
				exit = true;
				g_RCInput->postMsg (msg, data);
			}
			else
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					isTS = true;		// also exit in PES Mode
					exit = true;
				}
	}
	while (playstate >= CMoviePlayerGui::PLAY);
	pthread_join (rct, NULL);
}

/* Gui not used at the moment !!! See neutrino.cpp for current GUI (std. menu class) */
int
CMoviePlayerGui::show ()
{
	uint msg, data;
	bool loop = true, update = true;
	while (loop)
	{
		if (CNeutrinoApp::getInstance ()->
		    getMode () != NeutrinoMessages::mode_ts)
		{
			// stop if mode was changed in another thread
			loop = false;
		}

		if (update)
		{
			hide ();
			update = false;
			paint ();
		}

		// Check Remote Control

		g_RCInput->getMsg (&msg, &data, 10);	// 1 sec timeout to update play/stop state display
		if (msg == CRCInput::RC_home)
		{			//Exit after cancel key
    
			loop = false;
		}
		else if (msg == CRCInput::RC_timeout)
		{
			// do nothing
		}
//------------ RED --------------------
		else if (msg == CRCInput::RC_red)
		{
			hide ();
			PlayStream (STREAMTYPE_FILE);
			paint ();
		}
//------------ GREEN --------------------
		else if (msg == CRCInput::RC_green)
		{
			hide ();
			isTS = true;
			PlayFile ();
			paint ();
		}
/*//------------ YELLOW --------------------
  else if (msg == CRCInput::RC_yellow)
  {
  hide ();
  isTS = false;
  PlayFile ();
  paint ();
  }
*/
//------------ YELLOW --------------------
		else if (msg == CRCInput::RC_yellow)
		{
			hide ();
			PlayStream (STREAMTYPE_DVD);
			paint ();
		}
//------------ BLUE --------------------
		else if (msg == CRCInput::RC_blue)
		{
			hide ();
			PlayStream (STREAMTYPE_SVCD);
			paint ();
		}
		else if (msg == NeutrinoMessages::CHANGEMODE)
		{
			if ((data & NeutrinoMessages::mode_mask) != NeutrinoMessages::mode_ts)
			{
				loop = false;
				m_LastMode = data;
			}
		}
		else
			if (msg == NeutrinoMessages::RECORD_START
			    || msg == NeutrinoMessages::ZAPTO
			    || msg == NeutrinoMessages::STANDBY_ON
			    || msg == NeutrinoMessages::SHUTDOWN
			    || msg == NeutrinoMessages::SLEEPTIMER)
			{
				// Exit for Record/Zapto Timers
				// Add bookmark
				loop = false;
				g_RCInput->postMsg (msg, data);
			}
			else
			{
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					loop = false;
				}
				// update mute icon
				paintHead ();
			}
	}
	hide ();

	return -1;
}

//------------------------------------------------------------------------

void
CMoviePlayerGui::hide ()
{
	if (visible)
	{
/* the following 2 paintBackgroundBoxRel calls are superseeded by the ClearFrameBuffer call */
/*
		frameBuffer->paintBackgroundBoxRel (x -
						    MOVIEPLAYER_ConnectLineBox_Width
						    - 1,
						    y +
						    title_height
						    - 1,
						    width
						    +
						    MOVIEPLAYER_ConnectLineBox_Width
						    + 2, height + 2 - title_height);
		frameBuffer->paintBackgroundBoxRel (x, y, width, title_height);
*/
		frameBuffer->ClearFrameBuffer();
		visible = false;
	}
}

//------------------------------------------------------------------------

void
CMoviePlayerGui::paintHead ()
{
	frameBuffer->paintBoxRel (x, y + title_height, width, theight, COL_MENUHEAD);
	frameBuffer->paintIcon ("movie.raw", x + 7, y + title_height + 10);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString (x + 35, y + theight + title_height + 0, width - 45, g_Locale->getText("movieplayer.head"), COL_MENUHEAD, 0, true); // UTF-8
	int ypos = y + title_height;
	if (theight > 26)
		ypos = (theight - 26) / 2 + y + title_height;
	frameBuffer->paintIcon (NEUTRINO_ICON_BUTTON_DBOX, x + width - 30, ypos);
	if (CNeutrinoApp::getInstance ()->isMuted ())
	{
		int xpos = x + width - 75;
		ypos = y + title_height;
		if (theight > 32)
			ypos = (theight - 32) / 2 + y + title_height;
		frameBuffer->paintIcon (NEUTRINO_ICON_BUTTON_MUTE, xpos, ypos);
	}
	visible = true;
}

//------------------------------------------------------------------------

void
CMoviePlayerGui::paintImg ()
{
	// TODO: find better image
	frameBuffer->paintBoxRel (x,
				  y +
				  title_height +
				  theight, width,
				  height -
				  info_height -
				  2 *
				  buttonHeight -
				  title_height - theight, COL_BACKGROUND);
	frameBuffer->paintIcon ("movieplayer.raw",
				x + 25, y + 15 + title_height + theight);
}

//------------------------------------------------------------------------
const struct button_label MoviePlayerButtons[4] =
{
/*
	{ .button = NEUTRINO_ICON_BUTTON_RED   , .locale = "movieplayer.choosestreamfile" },
	{ .button = NEUTRINO_ICON_BUTTON_GREEN , .locale = "movieplayer.choosets"         },
	{ .button = NEUTRINO_ICON_BUTTON_YELLOW, .locale = "movieplayer.choosestreamdvd"  },
	{ .button = NEUTRINO_ICON_BUTTON_BLUE  , .locale = "movieplayer.choosestreamsvcd" }
*/
	{ NEUTRINO_ICON_BUTTON_RED   , "movieplayer.choosestreamfile" },
	{ NEUTRINO_ICON_BUTTON_GREEN , "movieplayer.choosets"         },
	{ NEUTRINO_ICON_BUTTON_YELLOW, "movieplayer.choosestreamdvd"  },
	{ NEUTRINO_ICON_BUTTON_BLUE  , "movieplayer.choosestreamsvcd" }
};

void
CMoviePlayerGui::paintFoot ()
{
	int ButtonWidth = (width - 20) / 4;

	frameBuffer->paintBoxRel (x,
				  y + (height -
				       info_height
				       -
				       2 *
				       buttonHeight),
				  width, 2 * buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine (x, x + width - x,
				 y + (height -
				      info_height
				      - 2 * buttonHeight), COL_INFOBAR_SHADOW);

	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 4, MoviePlayerButtons);

/*  frameBuffer->paintIcon (NEUTRINO_ICON_BUTTON_RED, x + 0 * ButtonWidth + 10,
    y + (height - info_height - 2 * buttonHeight) + 4);
    g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString (x + 0 * ButtonWidth + 30, y + (height - info_height - 2 * buttonHeight) + 24 - 1, ButtonWidth - 20, g_Locale->getText("movieplayer.bookmark"), COL_INFOBAR, 0, true); // UTF-8
*/
}

void
CMoviePlayerGui::paint ()
{
	CLCD * lcd = CLCD::getInstance();
	lcd->setMode(CLCD::MODE_TVRADIO);
	lcd->showServicename(g_Locale->getText("mainmenu.movieplayer"));

	frameBuffer->loadPal ("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground ("radiomode.raw");
	frameBuffer->useBackground (true);
	frameBuffer->paintBackground ();
	paintHead ();
	paintImg ();
	paintFoot ();
	visible = true;
}

#endif

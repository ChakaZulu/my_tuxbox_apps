#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <lib/base/buffer.h>
#include <lib/system/econfig.h>
#include <lib/dvb/decoder.h>
#include <lib/movieplayer/movieplayer.h>

#define BLOCKSIZE 4*65424
#define PVRDEV "/dev/pvr"
#define INITIALBUFFERING BLOCKSIZE*6

eIOBuffer tsBuffer(BLOCKSIZE);
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

extern void find_avpids(eIOBuffer *tsBuffer, int *vpid, int *apid, int *ac3);
extern int tcpOpen(eString, int);
extern eString httpEscape(eString);
extern CURLcode sendGetRequest (const eString& url, eString& response, bool useAuthorization);
extern bool playService(const eServiceReference &ref);

pthread_t dvr;
void *dvrThread(void *);
pthread_t receiver;
void *receiverThread(void *);

int skt = -1;
int play = -1;

eMoviePlayer *eMoviePlayer::instance;

eMoviePlayer::eMoviePlayer(): messages(this,1)
{
	if (!instance)
		instance = this;
		
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	eDebug("[MOVIEPLAYER] starting...");
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	play = -1; skt = -1; // terminate receiver and dvr thread if they are still running
	messages.send(Message::quit);
	if ( thread_running() )
		kill();
	if (instance == this)
		instance = 0;
}

void eMoviePlayer::thread()
{
	nice(5);
	exec();
}

void eMoviePlayer::start(const char *filename)
{
	messages.send(Message(Message::start, filename ? strdup(filename) : 0));
}


int eMoviePlayer::sendRequest2VLC(eString command, bool authenticate)
{
	CURLcode httpres;
	eString baseURL = "http://" + serverIP + ':' + eString().sprintf("%d", serverPort) + '/';
	eDebug("[MOVIEPLAYER] sendRequest2VLC: %s", eString(baseURL + command).c_str());
	eString response;
	httpres = sendGetRequest(baseURL + command, response, authenticate);
	eDebug("[MOVIEPLAYER] HTTP result for vlc command %s: %d", command.c_str(), httpres);
	
	return httpres;
}

bool AVPids(int skt, int *apid, int *vpid, int *ac3)
{
	int len = 0;
	int error = 0;
	char tempBuffer[BLOCKSIZE];
	
	eDebug("[MOVIEPLAYER] buffering data...");
	
	// fill buffer and temp file
	do
	{
		len = recv(skt, tempBuffer, BLOCKSIZE, 0);
		if (len > 0)
		{
//			eDebug("[MOVIEPLAYER] writing %d bytes to buffer, total: %d", len, tsBuffer.size());
			tsBuffer.write(tempBuffer, len);
		}
		else 
			error++;
	}
	while (tsBuffer.size() < INITIALBUFFERING && error < 100);
	
	if (error == 0)
	{
		find_avpids(&tsBuffer, vpid, apid, ac3);
		eDebug("[MOVIEPLAYER] found apid: 0x%04X, vpid: 0x%04X, ac3: %d", *apid, *vpid, *ac3);
	}
	
	return (*apid != -1 && *vpid != -1);
}

int eMoviePlayer::playStream(eString mrl)
{
	int apid = -1, vpid = -1, ac3 = -1;
	
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	skt = tcpOpen(serverIP, serverPort);
	if (skt == -1)
	{
		eDebug("[MOVIEPLAYER] couldn't connect socket");
		return -1;
	}
	
	eDebug("[MOVIEPLAYER] socket connected: skt = %d", skt);
	
	const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
	if (send(skt, msg, strlen (msg), 0) == -1)
	{
		eDebug("[MOVIEPLAYER] sending GET failed.");
		close(skt);
		return -2;
	}
		
	eDebug("[MOVIEPLAYER] GET request sent.");
		
	// Skip HTTP Header
	bool header = false;
	char line[256];
	memset(line, '\0', sizeof(line));
	char *bp = line;
	while ((unsigned int)(bp - line) < sizeof(line) - 1)
	{
//		eDebug("[MOVIEPLAYER] reading: %s", line);
		recv(skt, bp, 1, 0);
		if (strstr(line, "\r\n\r\n") != 0)
		{
			if (strstr(line, "HTTP/1.0 404") != 0)
			{
				eDebug("[MOVIEPLAYER] VLC header not received...");
				close(skt);
				return -3;
			}
			else
			{
				eDebug("[MOVIEPLAYER] VLC header received.");
				header = true;
				break;
			}
		}
		bp++;
	}
	
	if (!header)
	{
		eDebug("[MOVIEPLAYER] something received... but not a header\n%s", line);
		close(skt);
		return -4;
	}
	
	fcntl(skt, O_NONBLOCK);
	
	tsBuffer.clear();

	if (!(AVPids(skt, &apid, &vpid, &ac3)))
	{
		eDebug("[MOVIEPLAYER] could not find AV pids.");
		close(skt);
		return -5;
	}
	
	eDebug("[MOVIEPLAYER] AV pids found.");
	
	// save current dvb service for later
	eDVBServiceController *sapi;
	if (sapi = eDVB::getInstance()->getServiceAPI())
		suspendedServiceReference = sapi->service;
	// stop dvb service
	eServiceInterface::getInstance()->stop();
	
	// set pids
	Decoder::parms.vpid = vpid;
	Decoder::parms.apid = apid;
	Decoder::parms.audio_type = DECODE_AUDIO_MPEG;
	if (ac3)
	{
		if (mrl.right(3) == "vob" || mrl.left(3) == "dvd")
			Decoder::parms.audio_type = DECODE_AUDIO_AC3_VOB;
		else
			Decoder::parms.audio_type = DECODE_AUDIO_AC3;
	}
	
	Decoder::Set();
			
	// create receiver thread
	pthread_create(&receiver, 0, receiverThread, (void *)&skt);
	// create dvr thread
	play = 1;
	pthread_create(&dvr, 0, dvrThread, (void *)&play);
	pthread_join(receiver, 0);
	play = 0; // request termination of dvr thread
	pthread_join(dvr, 0);

	Decoder::Flush();
	
	tsBuffer.clear();

	// restore suspended dvb service
	playService(suspendedServiceReference);
	
	return 0;
}

void eMoviePlayer::gotMessage(const Message &msg )
{
	eString mrl;
	switch (msg.type)
	{
		case Message::start:
		{
			if (msg.filename)
			{
				mrl = msg.filename;
				eDebug("[MOVIEPLAYER] mrl = %s", mrl.c_str());
				free((char*)msg.filename);
				
				serverPort = 8080;
				eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
				char *serverip;
				if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
					serverip = strdup("");
				serverIP = eString(serverip);
				free(serverip);
				eDebug("[MOVIEPLAYER] Server IP: %s", serverIP.c_str());
				eDebug("[MOVIEPLAYER] Server Port: %d", serverPort);
				
				int retry = 10;
				do
				{
					// vlc: empty playlist
					if (sendRequest2VLC("?control=empty", false) > 0)
					{
						eDebug("[MOVIEPLAYER] couldn't communicate with vlc, streaming server ip address may be wrong in settings.");
						break;
					}
					// vlc: add mrl to playlist
					if (sendRequest2VLC("?control=add&mrl=" + httpEscape(mrl), false))
						break;
					// vlc: set sout...
					if (sendRequest2VLC("?sout=" + httpEscape(sout(mrl)), false))
						break;
					// vlc: start playback of first item in playlist
					if (sendRequest2VLC("?control=play&item=0", false))
						break;
//					usleep(100000); // wait a little bit for vlc to start sending the stream
					// receive and play ts stream
				} while (playStream(mrl) < 0 && retry-- > 0);

				// shutdown vlc
				sendRequest2VLC("admin/?control=shutdown", true);
			}
			break;
		}
		case Message::quit:
		{
			quit(0);
			break;
		}
		default:
			eDebug("unhandled thread message");
	}
}

eString eMoviePlayer::sout(eString mrl)
{
	eString soutURL = "#";
	eString serverIP, DVDDrive;
	int transcodeAudio = 0, transcodeVideo = 0;
	int serverPort;
	int settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingForceTranscodeVideo, settingAudioRate, settingForceTranscodeAudio;
	
	readStreamingServerSettings(serverIP, serverPort, DVDDrive, settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingForceTranscodeVideo, settingAudioRate, settingForceTranscodeAudio);
	
	eDebug("[MOVIEPLAYER] determine ?sout for mrl: %s", mrl.c_str());
	
	if (settingForceTranscodeVideo)
		transcodeVideo = settingTranscodeVideoCodec;
	transcodeAudio = settingForceTranscodeAudio;
	
	eDebug("[MOVIEPLAYER] transcoding audio: %d, video: %d", transcodeAudio, transcodeVideo);

	// add sout (URL encoded)
	// example (with transcode to mpeg1):
	//  ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	// example (without transcode to mpeg1): 
	// ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}

	eString res_horiz;
	eString res_vert;
	switch (settingResolution)
	{
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
	}
	
	if (transcodeVideo || transcodeAudio)
	{
		soutURL += "transcode{";
		if (transcodeVideo)
		{
			eString videoCodec = (transcodeVideo == 1) ? "mpgv" : "mp2v";
			soutURL += "vcodec=" + videoCodec;
			soutURL += ",vb=" + eString().sprintf("%d", settingVideoRate);
			soutURL += ",width=" + res_horiz;
			soutURL += ",height=" + res_vert;
		}
		if (transcodeAudio)
		{
			if (transcodeVideo)
				soutURL += ",";
			soutURL += "acodec=mpga,ab=" + eString().sprintf("%d", settingAudioRate) + ",channels=2";
		}
		soutURL += "}:";
	}
	
	soutURL += "duplicate{dst=std{access=http,mux=ts,url=:" + eString().sprintf("%d", serverPort) + "/dboxstream}}";
	eDebug("[MOVIEPLAYER] sout = %s", soutURL.c_str());
	return soutURL;
}

void eMoviePlayer::readStreamingServerSettings(eString& serverIP, int& serverPort, eString& 
DVDDrive, int& settingVideoRate, int& settingResolution, int& settingTranscodeVideoCodec, int& settingForceTranscodeVideo, int& settingAudioRate, int& settingForceTranscodeAudio)
{
	char *serverip;
	if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
		serverip = strdup("");
	serverIP = eString(serverip);
	free(serverip);
	serverPort = 8080;
	eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
	char *dvddrive;
	if (eConfig::getInstance()->getKey("/movieplayer/dvddrive", dvddrive))
		dvddrive = strdup("D");
	DVDDrive = eString(dvddrive);
	free(dvddrive);
	settingResolution = 3;
	eConfig::getInstance()->getKey("/movieplayer/resolution", settingResolution);
	settingAudioRate = 192;
	eConfig::getInstance()->getKey("/movieplayer/audiorate", settingAudioRate);
	settingVideoRate = 2048;
	eConfig::getInstance()->getKey("/movieplayer/videorate", settingVideoRate);
	settingTranscodeVideoCodec = 2;
	eConfig::getInstance()->getKey("/movieplayer/transcodevideocodec", settingTranscodeVideoCodec);
	settingForceTranscodeVideo = 0;
	eConfig::getInstance()->getKey("/movieplayer/forcetranscodevideo", settingForceTranscodeVideo);
	settingForceTranscodeAudio = 0;
	eConfig::getInstance()->getKey("/movieplayer/forcetranscodeaudio", settingForceTranscodeAudio);
}

void eMoviePlayer::writeStreamingServerSettings(eString serverIP, int serverPort, eString DVDDrive, int settingVideoRate, int settingResolution, int settingTranscodeVideoCodec, int settingForceTranscodeVideo, int settingAudioRate, int settingForceTranscodeAudio)
{
	eConfig::getInstance()->setKey("/movieplayer/serverip", serverIP.c_str());
	eConfig::getInstance()->setKey("/movieplayer/serverport", serverPort);
	eConfig::getInstance()->setKey("/movieplayer/dvddrive", DVDDrive.c_str());
	eConfig::getInstance()->setKey("/movieplayer/resolution", settingResolution);
	eConfig::getInstance()->setKey("/movieplayer/audiorate", settingAudioRate);
	eConfig::getInstance()->setKey("/movieplayer/videorate", settingVideoRate);
	eConfig::getInstance()->setKey("/movieplayer/transcodevideocodec", settingTranscodeVideoCodec);
	eConfig::getInstance()->setKey("/movieplayer/forcetranscodevideo", settingForceTranscodeVideo);
	eConfig::getInstance()->setKey("/movieplayer/forcetranscodeaudio", settingForceTranscodeAudio);
}

void *dvrThread(void *ctrl)
{
	char tempBuffer[BLOCKSIZE];
	int rd = 0;
	eDebug("[MOVIEPLAYER] dvrThread starting...");
	int pvrfd = 0;
	pvrfd = open(PVRDEV, O_RDWR);
	eDebug("[MOVIEPLAYER] pvr device opened: %d", pvrfd);
	nice(-15);
	while (*((int *)ctrl) > 0)
	{
		if (tsBuffer.size() > 0)
		{
			pthread_mutex_lock(&mutex);
			rd = tsBuffer.read(tempBuffer, BLOCKSIZE);
			pthread_mutex_unlock(&mutex);
			write(pvrfd, tempBuffer, rd);
//			eDebug("%d [MOVIEPLAYER]     >>> writing %d bytes to dvr...", tsBuffer.size(), rd);
		}
		else 
			usleep(100);
	}
	close(pvrfd);
	*((int *)ctrl) = -1;
	eDebug("[MOVIEPLAYER] dvrThread stopping...");
	pthread_exit(NULL);
}

void *receiverThread(void *skt)
{
	char tempBuffer[BLOCKSIZE];
	int len = 0;
	eDebug("[MOVIEPLAYER] receiverThread starting: skt = %d", *((int *)skt));
	nice(-15);
	// fill buffer
	do
	{
		if (tsBuffer.size() < BLOCKSIZE * 6)
		{
			len = recv(*((int *)skt), tempBuffer, BLOCKSIZE, 0);
//			eDebug("%d [MOVIEPLAYER] <<< writing %d bytes to buffer...", tsBuffer.size(), len);
			if (len >= 0)
			{
				pthread_mutex_lock(&mutex);
				tsBuffer.write(tempBuffer, len);
				pthread_mutex_unlock(&mutex);
			}
		}
		else
			len = 1; // to prevent loop from ending
	}
	while (len > 0);
	close(*((int *)skt));
	*((int *)skt) = -1;
	eDebug("[MOVIEPLAYER] receiverThread stopping...");
	pthread_exit(NULL);
}


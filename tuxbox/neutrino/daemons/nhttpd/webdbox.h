#ifndef __webdbox__
#define __webdbox__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include <vector>
#include <map>

#include "request.h"
//#include "controldclient.h"

#include "../controld/clientlib/controldclient.h"
#include "../../sections/clientlib/sectionsdclient.h"
#include "../../zapit/clientlib/zapitclient.h"
#include "../timerd/clientlib/timerdclient.h"


using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

extern string b64decode(char *s);
extern string itoh(unsigned int conv);
extern string itoa(unsigned int conv);


class CWebserver;
class CWebserverRequest;
class CWebserverCGI;

//-------------------------------------------------------------------------


class TWebDbox
{
	CWebserver * Parent;
	CControldClient controld;
	CSectionsdClient sectionsd;
	CZapitClient zapit;
	CTimerdClient timerd;
	CZapitClient::BouquetChannelList ChannelList;
	map<unsigned, CChannelEvent *> ChannelListEvents;
	map<int, CZapitClient::BouquetChannelList> BouquetsList;
	CZapitClient::BouquetList BouquetList;
	bool standby_mode;
	string Dbox_Hersteller[4];
	string videooutput_names[3];
	string videoformat_names[3];
	string audiotype_names[5];



public:
	TWebDbox(CWebserver * server);
	~TWebDbox();

	CChannelEventList eList;


// get functions to collect data
	void GetChannelEvents();
	bool GetStreamInfo(int bitinfo[10]);

	string GetServiceName(int onid_sid);

	bool GetBouquets(void)
	{
		BouquetList.clear();
		zapit.getBouquets(BouquetList); 
		return true;
	};

	bool GetBouquet(unsigned int BouquetNr)
	{
		BouquetsList[BouquetNr].clear();
		zapit.getBouquetChannels(BouquetNr,BouquetsList[BouquetNr]);
		return true;
	};

	bool GetChannelList(void)
	{
		ChannelList.clear();
		zapit.getChannels(ChannelList);
		return true;
	};

	bool ExecuteCGI(CWebserverRequest* request);
// send functions for ExecuteCGI (controld api)
	void SendEventList(CWebserverRequest *request,unsigned onidSid);
	void SendcurrentVAPid(CWebserverRequest* request);
	void SendSettings(CWebserverRequest* request);
	void SendStreamInfo(CWebserverRequest* request);
	void SendBouquets(CWebserverRequest *request);
	void SendBouquet(CWebserverRequest *request,int BouquetNr);
	void SendChannelList(CWebserverRequest *request);


	bool Execute(CWebserverRequest* request);
// show functions for Execute (web api)
	void ShowEventList(CWebserverRequest* request, unsigned onidSid);
	void ShowBouquet(CWebserverRequest *request,int BouquetNr = -1);
	void ShowBouquets(CWebserverRequest *request, int BouquetNr = 0);
	bool ShowControlpanel(CWebserverRequest* request);
	void ShowSettings(CWebserverRequest *request);
	void ShowCurrentStreamInfo(CWebserverRequest* request);
	bool ShowEpg(CWebserverRequest* request,string EpgID,string Startzeit = "");

// support functions
	void ZapTo(string target);
	void UpdateBouquets(void);


// alt
	void GetEPG(CWebserverRequest *request,unsigned long long epgid, time_t *,bool cgi=false);
};

#endif

/*
 * $Id: bouquets.h,v 1.21 2002/08/29 09:27:52 thegoodguy Exp $
 */

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <functional>
#include <vector>
#include <stdio.h>

#include "channel.h"
#include "xml/xmltree.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define CONFIGDIR "/var/tuxbox/config/zapit"
#endif

using namespace std;

typedef vector<CZapitChannel*> ChannelList;

/* struct for comparing channels by channel number*/
struct CmpChannelByChNr: public binary_function <CZapitChannel* , CZapitChannel* , bool>
{
	bool operator() (CZapitChannel* c1, CZapitChannel* c2)
	{
		return (c1->getChannelNumber() < c2->getChannelNumber());
	};
};

/* struct for comparing channels by channel name*/
struct CmpChannelByChName: public binary_function <CZapitChannel* , CZapitChannel* , bool>
{
	bool operator() (CZapitChannel*  c1, CZapitChannel*  c2)
	{
		return (c1->getName() < c2->getName());
	};
};


class CBouquet
{
	private:
		CZapitChannel* getChannelByName(char* serviceName, unsigned char serviceType = 0);

	public:
		string Name;
		bool   bHidden;
		bool   bLocked;

		ChannelList radioChannels;
		ChannelList tvChannels;

		CBouquet(string name) { Name=name; bHidden = false; bLocked = false; }
		CBouquet(const CBouquet& bouquet);

		~CBouquet();

		void addService (CZapitChannel* newChannel);

		void removeService (CZapitChannel* oldChannel);
		void removeService (char* serviceName, unsigned char serviceType = 0)	{removeService( getChannelByName( serviceName, serviceType));}
		void removeService (unsigned int onidSid, unsigned char serviceType = 0)			{removeService( getChannelByOnidSid( onidSid, serviceType));}

		void moveService (char* serviceName, unsigned int newPosition, unsigned char serviceType);
//		void moveService (unsigned int onidSid, unsigned int newPosition);
		void moveService (unsigned int oldPosition, unsigned int newPosition, unsigned char serviceType);

		int recModeRadioSize( unsigned int);
		int recModeTVSize( unsigned int);
		CZapitChannel* getChannelByOnidSid(unsigned int onidSid, unsigned char serviceType = 0);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		CBouquet* remainChannels;
		void makeRemainingChannelsBouquet(unsigned int tvChanNr, unsigned int radioChanNr, string strTitle);
		void parseBouquetsXml(XMLTreeNode *root);
		string convertForXML( string s);
		void storeBouquets();
	public:
		class ChannelIterator
		{
			private:
				CBouquetManager* Owner;
				unsigned int b;
				int c;
				bool tv;           // true -> tvChannelIterator, false -> radioChannelIterator
				ChannelList* getBouquet() { return (tv ? &(Owner->Bouquets[b]->tvChannels) : &(Owner->Bouquets[b]->radioChannels)); };
			public:
				ChannelIterator(CBouquetManager* owner, unsigned int B=0, int C=0, bool TV=true) { Owner = owner; b=B; c=C; tv = TV; };
				bool operator != (const ChannelIterator& it) const;
				bool operator == (const ChannelIterator& it) const;
				ChannelIterator operator ++(int);
				CZapitChannel* operator *();
		};

		ChannelIterator tvChannelsBegin() { return ChannelIterator(this, 0, -1, true)++; };
		ChannelIterator tvChannelsEnd()   { return ChannelIterator(this, 0, -2, true);   };
		ChannelIterator tvChannelsFind(unsigned int onid_sid);

		ChannelIterator radioChannelsBegin() { return ChannelIterator(this, 0, -1, false)++; };
		ChannelIterator radioChannelsEnd()   { return ChannelIterator(this, 0, -2, false);   };
		ChannelIterator radioChannelsFind(unsigned int onid_sid);

		BouquetList Bouquets;
		BouquetList storedBouquets;

		void saveBouquets();
		void loadBouquets( bool ignoreBouquetFile = false);
		void restoreBouquets();
		void cleanUp();
		void renumServices();

		CBouquet* addBouquet( string name);
		void deleteBouquet( unsigned int id);
		void deleteBouquet( string name);
		int  existsBouquet( string name);
		void moveBouquet( unsigned int oldId, unsigned int newId);
		bool existsChannelInBouquet( unsigned int bq_id, unsigned int onid_sid);

		void saveAsLast( unsigned int BouquetId, unsigned int channelNr);
		void getLast( unsigned int* BouquetId, unsigned int* channelNr);
		void clearAll();
		void onTermination();
		void onStart();

		CZapitChannel* copyChannelByOnidSid(unsigned int onid_sid);

};

//extern CBouquetManager* BouquetManager;

#endif /* __bouquets_h__ */

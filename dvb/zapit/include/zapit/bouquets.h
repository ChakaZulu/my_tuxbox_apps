/*
 * $Id: bouquets.h,v 1.37 2002/09/11 14:41:20 thegoodguy Exp $
 */

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <functional>
#include <vector>
#include <ext/hash_set>
#include <ext/hash_map>
#include <stdint.h>
#include <stdio.h>

#include "channel.h"
#include "xml/xmltree.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define CONFIGDIR "/var/tuxbox/config/zapit"
#endif

using namespace std;

typedef uint32_t t_channel_id;             // channel_id: (original_network_id << 16) | service_id

typedef __gnu_cxx::hash_map<t_channel_id, CZapitChannel> tallchans;
typedef __gnu_cxx::hash_map<t_channel_id, CZapitChannel>::iterator tallchans_iterator;

typedef vector<CZapitChannel*> ChannelList;

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
	public:
		string Name;
		bool   bHidden;
		bool   bLocked;

		ChannelList radioChannels;
		ChannelList tvChannels;

		CBouquet(const string name) { Name = name; bHidden = false; bLocked = false; }

		void addService(CZapitChannel* newChannel);

		void removeService(CZapitChannel* oldChannel);
		void removeService(const t_channel_id channel_id, unsigned char serviceType = 0) { removeService(getChannelByChannelID(channel_id, serviceType)); }

		void moveService (const unsigned int oldPosition, const unsigned int newPosition, const unsigned char serviceType);

		int recModeRadioSize( unsigned int);
		int recModeTVSize( unsigned int);
		CZapitChannel* getChannelByChannelID(const t_channel_id channel_id, const unsigned char serviceType = 0);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		CBouquet* remainChannels;
		void makeRemainingChannelsBouquet();
		void parseBouquetsXml(const XMLTreeNode *root);
		string convertForXML( string s);
		void storeBouquets();
	public:
		CBouquetManager() { remainChannels = NULL; };
		class ChannelIterator
		{
			private:
				CBouquetManager* Owner;
				bool tv;           // true -> tvChannelIterator, false -> radioChannelIterator
				unsigned int b;
				int c;
				ChannelList* getBouquet() { return (tv ? &(Owner->Bouquets[b]->tvChannels) : &(Owner->Bouquets[b]->radioChannels)); };
			public:
				ChannelIterator(CBouquetManager* owner, const bool TV = true);
				ChannelIterator operator ++(int);
				CZapitChannel* operator *();
				ChannelIterator FindChannelNr(const unsigned int channel);
				int getLowestChannelNumberWithChannelID(const t_channel_id channel_id);
				int getNrofFirstChannelofBouquet(const unsigned int bouquet_nr);
				bool EndOfChannels() { return (c == -2); };
		};

		ChannelIterator tvChannelsBegin() { return ChannelIterator(this, true); };
		ChannelIterator radioChannelsBegin() { return ChannelIterator(this, false); };

		BouquetList Bouquets;
		BouquetList storedBouquets;

		void saveBouquets();
		void loadBouquets( bool ignoreBouquetFile = false);
		void restoreBouquets();
		void renumServices();

		CBouquet* addBouquet( string name);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CBouquet* bouquet);
		int  existsBouquet( string name);
		void moveBouquet(const unsigned int oldId, const unsigned int newId);
		bool existsChannelInBouquet( unsigned int bq_id, const t_channel_id channel_id);

		void clearAll();

		CZapitChannel* findChannelByChannelID(const t_channel_id channel_id);

};

//extern CBouquetManager* BouquetManager;

#endif /* __bouquets_h__ */

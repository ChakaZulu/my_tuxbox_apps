/*
 * $Id: bouquets.h,v 1.49 2003/09/18 15:57:19 thegoodguy Exp $
 */

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>

#include <inttypes.h>
#include <zapit/client/zapitclient.h>

#include "channel.h"
#include "xmlinterface.h"

using namespace std;

typedef map<t_channel_id, CZapitChannel> tallchans;
typedef tallchans::iterator tallchans_iterator;

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
		void removeService(const t_channel_id channel_id, unsigned char serviceType = ST_RESERVED) { removeService(getChannelByChannelID(channel_id, serviceType)); }

		void moveService (const unsigned int oldPosition, const unsigned int newPosition, const unsigned char serviceType);

		int recModeRadioSize(unsigned int);
		int recModeTVSize(unsigned int);
		CZapitChannel* getChannelByChannelID(const t_channel_id channel_id, const unsigned char serviceType = ST_RESERVED);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		CBouquet* remainChannels;
		void makeRemainingChannelsBouquet();
		void parseBouquetsXml(const xmlNodePtr root);
		void writeBouquetHeader(FILE * bouq_fd, uint i, const char * bouquetName);
		void writeBouquetFooter(FILE * bouq_fd);
		void writeBouquetChannels(FILE * bouq_fd, uint i);
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

		void saveBouquets(void);
		void saveBouquets(const CZapitClient::bouquetMode bouquetMode, const char * const providerName);
		void loadBouquets(bool ignoreBouquetFile = false);
		void storeBouquets();
		void restoreBouquets();
		void renumServices();

		CBouquet* addBouquet(string name);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CBouquet* bouquet);
		int  existsBouquet(string name);
		void moveBouquet(const unsigned int oldId, const unsigned int newId);
		bool existsChannelInBouquet(unsigned int bq_id, const t_channel_id channel_id);

		void clearAll();

		CZapitChannel* findChannelByChannelID(const t_channel_id channel_id);

};

#endif /* __bouquets_h__ */

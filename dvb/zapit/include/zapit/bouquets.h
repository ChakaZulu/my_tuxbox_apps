#ifndef __bouquets__
#define __bouquets__

#ifndef BOUQUETS_CPP
#define BOUQUETS_CPP extern
#endif

#include <vector>
#include "getservices.h"
#include "xml/xmltree.h"

using namespace std;

typedef vector<channel*> ChannelList;

class CBouquet
{
	private:
		channel* getChannelByName(char* serviceName, uint serviceType = 0);
		channel* getChannelByOnidSid(uint onidSid, uint serviceType = 0);

	public:
		string Name;

		ChannelList radioChannels;
		ChannelList tvChannels;

		CBouquet( string name) { Name=name; }
		~CBouquet();

		void addService( channel* newChannel);

		void removeService( channel* oldChannel);
		void removeService( char* serviceName, uint serviceType = 0)	{removeService( getChannelByName( serviceName, serviceType));}
		void removeService( uint onidSid, uint serviceType = 0)			{removeService( getChannelByOnidSid( onidSid, serviceType));}

		void moveService(  char* serviceName, uint newPosition, uint serviceType);
//		void moveService(  uint onidSid, uint newPosition);
		void moveService(  uint oldPosition, uint newPosition, uint serviceType);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		CBouquet* remainChannels;
		void makeRemainingChannelsBouquet(unsigned int tvChanNr, unsigned int radioChanNr);
		void parseBouquetsXml(XMLTreeNode *root);
	public:
		class tvChannelIterator
		{
			private:
				CBouquetManager* Owner;
				int b;
				int c;
			public:
				tvChannelIterator(CBouquetManager* owner, int B=0, int C=0) { Owner = owner; b=B;c=C;};
				tvChannelIterator operator ++(int);
				bool operator != (const tvChannelIterator& it) const;
				bool operator == (const tvChannelIterator& it) const;
				channel* operator *();
			friend class CBouquetManager;
		};

		tvChannelIterator tvChannelsBegin();
		tvChannelIterator tvChannelsEnd(){ return tvChannelIterator(this, -1, -1);};
		tvChannelIterator tvChannelsFind( unsigned int onid_sid);

		class radioChannelIterator
		{
			private:
				CBouquetManager* Owner;
				int b;
				int c;
			public:
				radioChannelIterator(CBouquetManager* owner, int B=0, int C=0) { Owner = owner; b=B;c=C;};
				radioChannelIterator operator ++(int);
				bool operator != (const radioChannelIterator& it) const;
				bool operator == (const radioChannelIterator& it) const;
				channel* operator *();
			friend class CBouquetManager;
		};

		radioChannelIterator radioChannelsBegin();
		radioChannelIterator radioChannelsEnd(){ return radioChannelIterator(this, -1, -1);};
		radioChannelIterator radioChannelsFind( unsigned int onid_sid);

		BouquetList Bouquets;

		void saveBouquets();
		void loadBouquets();
		void renumServices();

		CBouquet* addBouquet( string name);
		void deleteBouquet( uint id);
		void deleteBouquet( string name);
		void moveBouquet( uint oldId, uint newId);

		void saveAsLast( uint BouquetId, uint channelNr);
		void getLast( uint* BouquetId, uint* channelNr);
		void clearAll();
		void onTermination();
		void onStart();

		channel* copyChannelByOnidSid( unsigned int onid_sid);

};

BOUQUETS_CPP CBouquetManager* BouquetManager;

#endif

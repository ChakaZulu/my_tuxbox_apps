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
		channel* getChannelByName(char* serviceName, uint serviceType);
		channel* getChannelByOnidSid(uint onidSid, uint serviceType);

	public:
		string Name;

		ChannelList radioChannels;
		ChannelList tvChannels;

		CBouquet( string name) { Name=name; }

		void addService( channel* newChannel);

		void removeService( channel* oldChannel);
		void removeService( char* serviceName, uint serviceType)	{removeService( getChannelByName( serviceName, serviceType));}
		void removeService( uint onidSid, uint serviceType)			{removeService( getChannelByOnidSid( onidSid, serviceType));}

		void moveService(  char* serviceName, uint newPosition, uint serviceType);
//		void moveService(  uint onidSid, uint newPosition);
		void moveService(  uint oldPosition, uint newPosition, uint serviceType);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		void parseBouquetsXml(XMLTreeNode *root);
	public:
		BouquetList Bouquets;

		void saveBouquets();
		void loadBouquets();

		CBouquet* addBouquet( string name);
		void removeBouquet( uint id);
		void removeBouquet( string name);
		void moveBouquet( uint oldId, uint newId);

		void saveAsLast( uint BouquetId, uint channelNr);
		void getLast( uint* BouquetId, uint* channelNr);
		void clearAll();
		void onTermination();
		void onStart();
};

BOUQUETS_CPP CBouquetManager* BouquetManager;

#endif

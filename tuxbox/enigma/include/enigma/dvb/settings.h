#ifndef __src_core_dvb_settings_h
#define __src_core_dvb_settings_h

#include <core/dvb/dvb.h>

class eDVB;

class eDVBSettings
{
	eDVB &dvb;
public:
	friend class sortinChannel;
		/** the main transponder/servicelist */
	eTransponderList *transponderlist;

	ePtrList<eBouquet> bouquets;
	void removeDVBBouquets();
	void addDVBBouquet(const BAT *bat);
	eBouquet *getBouquet(int bouquet_id);
	eBouquet *getBouquet(eString bouquet_name);
	eBouquet *createBouquet(const eBouquet *parent, int bouquet_id, eString bouquet_name);
	eBouquet *createBouquet(const eBouquet *parent, eString bouquet_name);
	int getUnusedBouquetID(int range);
	
	void revalidateBouquets();
	eTransponderList *getTransponders();
	ePtrList<eBouquet> *getBouquets();
	void setTransponders(eTransponderList *tlist);
	void sortInChannels();

	void saveServices();
	void loadServices();

	void saveBouquets();
	void loadBouquets();
	
	eDVBSettings(eDVB &dvb);
	~eDVBSettings();
};

#endif

#ifndef __src_lib_dvb_settings_h
#define __src_lib_dvb_settings_h

#include <lib/dvb/dvb.h>

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
	void addDVBBouquet(eDVBNamespace origin, const BAT *bat);
	eBouquet *getBouquet(int bouquet_id);
	eBouquet *getBouquet(eString bouquet_name);
	eBouquet *createBouquet(int bouquet_id, eString bouquet_name);
	eBouquet *createBouquet(eString bouquet_name);
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

	void clearList();
	void removeOrbitalPosition(int orbital_position);
	int importSatcoDX(eString line);
	
	eDVBSettings(eDVB &dvb);
	~eDVBSettings();
};

#endif

#ifndef __bselect_h
#define __bselect_h

#include <core/gui/listbox.h>
#include <core/dvb/dvb.h>

class eListBoxEntryBouquet: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryBouquet>;
	friend class eBouquetSelector;
	friend struct moveTo_bouquet_id;
	eBouquet* bouquet;
public:
	eListBoxEntryBouquet(eListBox<eListBoxEntryBouquet>* lb, eBouquet* b)
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, b->bouquet_name), bouquet(b)
	{
	}
};


class eBouquetSelector: public eWindow
{
	eBouquet *result;
	eListBoxEntryBouquet* allServices;
	eString allServicesName;
private:
	void entrySelected(eListBoxEntryBouquet *entry);
	eListBox<eListBoxEntryBouquet> *bouquets;
public:
	const eBouquet *getAllServicesBouquet()	{	return allServices->bouquet; }
	Signal0<void> cancel;
	int fillBouquetList();
	eBouquetSelector();
	~eBouquetSelector();
	bool moveTo(int bouquet_id);
	eBouquet *choose(int irc=-1);
	eBouquet *current();
	eBouquet *next();
	eBouquet *prev();
};

#endif

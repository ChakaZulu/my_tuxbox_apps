#ifndef __src_core_dvb_dvbwidgets_h
#define __src_core_dvb_dvbwidgets_h

#include <core/gui/ewidget.h>
#include <core/gui/eListBox.h>

class eNumber;
class eTransponder;

class eTransponderWidget: public eWidget
{
	eNumber *frequency, *symbolrate;
	int type, edit;
	eListBoxEntryText *fecEntry[6], *polarityEntry[4];
	
	eListBox<eListBoxEntryText> *fec, *polarity;
public:
	enum type
	{
		deliveryCable, deliverySatellite
	};
	eTransponderWidget(eWidget *parent, int edit, int type);
	int load();
	int setTransponder(eTransponder *transponder);
};

#endif

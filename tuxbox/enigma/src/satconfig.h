#ifndef __satconfig_h
#define __satconfig_h

#include <core/gui/ewindow.h>

class eListbox;
class eButton;

class eSatelliteConfiguration: public eWindow
{
public:
	eSatelliteConfiguration(int sat);
};

class eSatelliteConfigurationManager: public eWindow
{
	eListbox *list;
	eButton *button_close, *sat_new, *sat_delete;
	int eventFilter(const eWidgetEvent &event);
public:
	void okPressed();
	void newSatellite();
	void deleteSatellite();
public:
	eSatelliteConfigurationManager();
	~eSatelliteConfigurationManager();
};

#endif

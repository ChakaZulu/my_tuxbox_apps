#ifndef __setup_bouquet_h
#define __setup_bouquet_h

#include <setup_window.h>

class eZapBouquetSetup: public eSetupWindow
{
	void editSelected();
	void createNewEmptyBouquet();
	void editModeSelected();
	void lockUnlockServices();
public:
	eZapBouquetSetup();
};

#endif //__setup_bouquet_h

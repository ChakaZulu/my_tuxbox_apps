#ifndef __enigma_mainmenu_h
#define __enigma_mainmenu_h

#include <libsig_comp.h>
#include <lib/gui/ewidget.h>

class gPixmap;
class eLabel;

#ifndef DISABLE_FILE
#define MENU_ENTRIES 9
#else
#define MENU_ENTRIES 6
#endif

class eMainMenu: public eWidget
{
	gPixmap *pixmaps[MENU_ENTRIES][2];
	eLabel *label[7], *description;
	int active;
	void setActive(int i);
	void sel_tv();
	void sel_radio();
#ifndef DISABLE_FILE
	void sel_file();
#endif
	void sel_vcr();
	void sel_setup();
	void sel_info();	
	void sel_quit();
	void sel_plugins();
	void sel_timer();	
	void eraseBackground(gPainter *, const eRect &where);
	void selected(int num);
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eMainMenu();
};

#endif

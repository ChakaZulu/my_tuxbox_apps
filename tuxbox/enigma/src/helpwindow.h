#ifndef __helpwindow_h
#define __helpwindow_h

#include <lib/gui/ewindow.h>

class eLabel;
class eProgress;

class eHelpWindow: public eWindow
{
	std::list<int> entryBeg;
	std::list<int>::iterator cur;
	eLabel *label;
	eWidget *scrollbox, *visible;
	eProgress *scrollbar;
	int curPage;
	bool doscroll;
	eString loadHelpText(int helpIDtoLoad);
	int eventHandler(const eWidgetEvent &event);
	void updateScrollbar();
public:
	eHelpWindow(ePtrList<eAction> &parseActionHelpList, int helpID=0);
	~eHelpWindow();
};

#endif



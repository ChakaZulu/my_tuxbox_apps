#ifndef __enumber_h
#define __enumber_h

#include <core/gui/ewidget.h>

class eLabel;
class gPainter;

/**
 * \brief A widget to enter a number.
 */
class eNumber: public eWidget
{
//	Q_OBJECT
private:
	void redrawNumber(gPainter *, int n, const eRect &rect);
	void redrawWidget(gPainter *, const eRect &rect);
	eRect getNumberRect(int n);
	int eventFilter(const eWidgetEvent &event);
	int number[16];
	int len, space, active;
	gColor cursor, normal;
	int have_focus;
	int min, max, digit, maxdigits, isactive;
	int flags;
	int base;
	eString descr;
	eLabel* tmpDescr; // used for description Label in LCD
protected:
	int keyUp(int key);
	int keyDown(int key);
	void gotFocus();
	void lostFocus();
public:
	Signal1<void, int*> selected;
	eNumber(eWidget *parent, int len, int min, int max, int maxdigits, int *init, int isactive=0, eLabel* descr=0, int grabfocus=1);
	~eNumber();
	int getNumber(int f=0) { if ((f>=0) && (f<len)) return number[f]; return -1; }
	void setNumber(int f, int n);

	void setLimits(int min, int max);
	void setNumberOfFields(int n);
	void setMaximumDigits(int n);
	enum
	{
		flagDrawPoints=1,
		flagDrawBoxes=2
	};
	void setFlags(int flags);
	void setBase(int base);
	
	void setNumber(int n);
	int getNumber();
};

#endif

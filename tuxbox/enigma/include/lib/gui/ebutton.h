#ifndef __ebutton_h
#define __ebutton_h

#include "elabel.h"
#include "grc.h"

class eButton: public eLabel
{
	gColor focus, normal;
//	Q_OBJECT
	eLabel*	tmpDescr; // used for LCD with description
protected:
	QString descr;
	void keyUp(int key);
	void gotFocus();
	void lostFocus();
/*
signals:
	void selected();*/
public:
	Signal0<void> selected;
	eButton(eWidget *parent, eLabel* descr=0, int takefocus=1);
};

#endif

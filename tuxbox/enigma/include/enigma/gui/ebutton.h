#ifndef __ebutton_h
#define __ebutton_h

#include "elabel.h"
#include "grc.h"

class eButton: public eLabel
{
	gColor focus, normal;
	Q_OBJECT
protected:
	QString descr;
	void keyUp(int key);
	void gotFocus();
	void lostFocus();
signals:
	void selected();
public:
	eButton(eWidget *parent, eLabel* descr=0);
};

#endif

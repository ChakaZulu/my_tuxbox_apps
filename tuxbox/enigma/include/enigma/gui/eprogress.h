#ifndef __eprogress_h
#define __eprogress_h

#include "ewidget.h"
#include "grc.h"

class eProgress: public eWidget
{
	Q_OBJECT
	int perc;
	gColor left, right;
public:
	eProgress(eWidget *parent);
	~eProgress();
	
	void setPerc(int perc);
	void redrawWidget(gPainter *target, const QRect &area);
};

#endif

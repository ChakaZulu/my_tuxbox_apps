#include "decoration.h"
#include <core/gdi/gpixmap.h>
#include <core/gdi/grc.h>
#include <core/gui/eskin.h>

/*

	+-------+-----------------+--------+
	|TopLeft|        Top      |TopRight|
	+------++-----------------+--+-----+
	|  Left|     client          |Right|
	+------+---+-----------+-----+-----+
	|BottomLeft|   Bottom  |BottomRight|
	+----------+-----------+-----------+

*/

eDecoration::eDecoration()
{
	iTopLeft=iTop=iTopRight=iLeft=iRight=iBottomLeft=iBottom=iBottomRight=0;
	borderLeft=borderTop=borderRight=borderBottom=0;
}

void eDecoration::load(const char *base)
{
	eString basename=base;	// all your
	iTopLeft=eSkin::getActive()->queryImage(basename + ".topLeft");
	iTop=eSkin::getActive()->queryImage(basename + ".top");
	iTopRight=eSkin::getActive()->queryImage(basename + ".topRight");
	iLeft=eSkin::getActive()->queryImage(basename + ".left");
	iRight=eSkin::getActive()->queryImage(basename + ".right");
	iBottomLeft=eSkin::getActive()->queryImage(basename + ".bottomLeft");
	iBottom=eSkin::getActive()->queryImage(basename + ".bottom");
	iBottomRight=eSkin::getActive()->queryImage(basename + ".bottomRight");

	borderLeft=borderTop=borderRight=borderBottom=0;
	
	if (iTop)
		borderTop = iTop->y;
	if (iLeft)
		borderLeft = iLeft->x;
	if (iRight)
		borderRight = iRight->x;
	if (iBottom)
		borderBottom = iBottom->y;
}

void eDecoration::drawDecoration(gPainter *target, ePoint size)
{
	int x=0, xm=size.x(), y, ym;
	
	if (iTopLeft)
	{
		target->blit(*iTopLeft, ePoint(0, 0));
		target->flush();
		x+=iTopLeft->x;
	}

	if (iTopRight)
	{
		xm-=iTopRight->x;
		target->blit(*iTopRight, ePoint(xm, 0), eRect(x, 0, size.x()-x, size.y()));
		target->flush();
	}
	
	if (iTop)
	{
		while (x<xm)
		{
			target->blit(*iTop, ePoint(x, 0), eRect(x, 0, xm-x, size.y()));
			x+=iTop->x;
		}
		target->flush();
	} else
	{
		target->fill(eRect(0, 0, size.x(), borderTop));
		target->flush();
	}

	x=0;
	xm=size.x();

	if (iBottomLeft)
	{
		target->blit(*iBottomLeft, ePoint(0, size.y()-iBottomLeft->y));
		target->flush();
		x+=iBottomLeft->x;
	}

	if (iBottomRight)
	{
		xm-=iBottomRight->x;
		target->blit(*iBottomRight, ePoint(xm, size.y()-iBottomRight->y), eRect(x, size.y()-iBottomRight->y, size.x()-x, iBottomRight->y));
		target->flush();
	}
	
	if (iBottom)
	{
		while (x<xm)
		{
			target->blit(*iBottom, ePoint(x, size.y()-iBottom->y), eRect(x, size.y()-iBottom->y, xm-x, iBottom->y));
			x+=iBottom->x;
		}
		target->flush();
	}
	
	y=0; ym=size.y();
	
	if (iTopLeft)
		y=iTopLeft->y;
	if (iBottomLeft)
		ym=size.y()-iBottomLeft->y;
	if (iLeft)
	{
		while (y<ym)
		{
			target->blit(*iLeft, ePoint(0, y), eRect(0, y, iLeft->x, ym-y));
			y+=iLeft->y;
		}
	}

	if (iTopRight)
		y=iTopRight->y;
	if (iBottomRight)
		ym=size.y()-iBottomRight->y;
	if (iRight)
	{
		while (y<ym)
		{
			target->blit(*iRight, ePoint(size.x()-iRight->x, y), eRect(size.x()-iRight->x, y, iRight->x, ym-y));
			y+=iRight->y;
		}
	}
	
	target->flush();
}

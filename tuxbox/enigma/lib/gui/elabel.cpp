#include "elabel.h"
#include "fb.h"
#include "font.h"
#include "lcd.h"
#include "eskin.h"
#include <qrect.h>

eLabel::eLabel(eWidget *parent, int flags, int takefocus):
	eWidget(parent, takefocus), flags(flags)
{
	para=0;
//	setBackgroundColor(eSkin::getActive()->queryScheme("fgColor"));
}

eLabel::~eLabel()
{
	invalidate();
}

void eLabel::invalidate()
{
	if (para)
		para->destroy();
	para=0;
}

void eLabel::validate()
{
	if (!para)
	{
		para=new eTextPara(QRect(0, 0, size.width(), size.height()));
		para->setFont(font);
		para->renderString(text, flags);
	}
}

void eLabel::willHide()
{
	invalidate(); // ob das sinn macht m�sste mal �berlegt werden
}

void eLabel::setFlags(int flag)
{
	flags|=flag;
	if (flag)
		invalidate();
}

void eLabel::redrawWidget(gPainter *target, const QRect &area)
{
	validate();
	target->setFont(font);
	target->renderPara(*para);
}

int eLabel::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedFont:
		invalidate();
		break;
	case eWidgetEvent::changedText:
		invalidate();
		break;
	}
	return 0;
}

QSize eLabel::getExtend()
{
	validate();
	qDebug("extends: %d %d", para->getExtend().width(), para->getExtend().height());
	return para->getExtend();
}

int eLabel::setProperty(const QString &prop, const QString &value)
{
	if (prop=="wrap")
		setFlags(RS_WRAP);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

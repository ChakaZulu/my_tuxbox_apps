#include "enigma_lcd.h"

#include <time.h>

#include <core/dvb/edvb.h>
#include <core/dvb/dvb.h>
#include <core/gdi/lcd.h>
#include <core/gdi/glcddc.h>
#include <core/gdi/font.h>
#include <core/gui/emessage.h>
#include <core/gui/eskin.h>

eZapLCD* eZapLCD::instance;

eZapLCD::eZapLCD(): eWidget()
{
	instance = this;
	setTarget(gLCDDC::getInstance());
	move(ePoint(0, 0));
	resize(eSize(120, 64));

	lcdMain = new eZapLCDMain(this);
	lcdMenu = new eZapLCDMenu(this);
	lcdScart = new eZapLCDScart(this);
	lcdMenu->hide();
	lcdScart->hide();
}

eZapLCD::~eZapLCD()
{
	delete lcdMain;
	delete lcdMenu;
	delete lcdScart;
}

eZapLCDMain::eZapLCDMain(eWidget *parent): eWidget(parent, 0), clocktimer(eApp)
{
	Volume = new eProgress(this);
	Volume->setName("volume_bar");
	
	Progress = new eProgress(this);
	Progress->setName("progress_bar");
	
	ServiceName = new eLabel(this);
	ServiceName->setName("service_name");
	
	Clock = new eLabel(this);
	Clock->setName("clock");
	
	if (eSkin::getActive()->build(this, "enigma_lcd_main"))
		eFatal("skin load of \"enigma_lcd\" failed");

	cur_start = cur_duration = -1;
	
	Volume->show();
	Progress->hide();
	
	CONNECT(clocktimer.timeout, eZapLCDMain::clockUpdate);
	CONNECT(eDVB::getInstance()->timeUpdated, eZapLCDMain::clockUpdate);
	CONNECT(eDVB::getInstance()->volumeChanged, eZapLCDMain::volumeUpdate);
	CONNECT(eDVB::getInstance()->switchedService, eZapLCDMain::serviceChanged);
	CONNECT(eDVB::getInstance()->leaveService, eZapLCDMain::leaveService);
	clockUpdate();
}

void eZapLCDMain::clockUpdate()
{
	time_t c=time(0)+eDVB::getInstance()->time_difference;
	tm *t=localtime(&c);
	if (t)
	{
		eString s;
		s.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
		clocktimer.start((70-t->tm_sec)*1000);
		Clock->setText(s);

		if ((cur_start <= c) && (c < cur_start+cur_duration))
		{
			Progress->setPerc((c-cur_start)*100/cur_duration);
			if (!Progress->isVisible())
				Progress->show();
		} else
		{
			if (Progress->isVisible())
				Progress->hide();
		}
	}	else
	{
		if (Progress->isVisible())
			Progress->hide();
		Clock->setText("--:--");
		clocktimer.start(60000);
	}
}

void eZapLCDMain::volumeUpdate(int vol)
{
	Volume->setPerc((63-vol)*100/63);
}

void eZapLCDMain::updateProgress(int start,int duration)
{
	cur_start = start;
	cur_duration = duration;
	clockUpdate();
}

void eZapLCDMain::serviceChanged(const eServiceReference &sref, int)
{
  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(sref);

	if (service)
		ServiceName->setText(service->service_name.c_str());
	else
		ServiceName->setText("unknown");
}

void eZapLCDMain::leaveService(const eServiceReference &service)
{
	if (Progress->isVisible())
		Progress->hide();
}

eZapLCDMenu::eZapLCDMenu(eWidget *parent): eWidget(parent, 0)
{	
	if (eSkin::getActive()->build(this, "enigma_lcd_menu"))
		eFatal("skin load of \"enigma_lcd_menu\" failed");

	ASSIGN(Title, eLabel, "title");
	ASSIGN(Element, eLabel, "element");
}

eZapLCDScart::eZapLCDScart(eWidget *parent): eWidget(parent, 0)
{	
	if (eSkin::getActive()->build(this, "enigma_lcd_scart"))
		eFatal("skin load of \"enigma_lcd_scart\" failed");

	ASSIGN(Title, eLabel, "enigma_logo");
	ASSIGN(Scart, eLabel, "lcd_scart");
}

#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "qrect.h"
#include "ezap.h"
#include "elbwindow.h"
#include "elistbox.h"
#include "elabel.h"
#include "scan.h"
#include "sselect.h"
#include "dvb.h"
#include "edvb.h"
#include "eprogress.h"
#include "streaminfo.h"
#include "httpd.h"
#include "http_file.h"
#include "http_dyn.h"
#include "ezap_dyn.h"
#include "showbnversion.h"
#include "decoder.h"
#include "ezap_xmlrpc.h"
#include "ezap_main.h"
#include "ezap_setup.h"
#include "ezap_plugins.h"
#include "emessage.h"
#include "grc.h"
#include "gfbdc.h"
#include "glcddc.h"
#include "eskin.h"
#include "eskin_register.h"
#include "epng.h"
#include "eavswitch.h"

#include <config.h>

eZap *eZap::instance;

int eZap::FontSize=0;

static char copyright[]="enigma, Copyright (C) dbox-Project\n"
"enigma comes with ABSOLUTELY NO WARRANTY\n"
"This is free software, and you are welcome\n"
"to redistribute it under certain conditions.\n"
"It is licensed under the GNU General Public License,\n"
"Version 1\n";

eZap *eZap::getInstance()
{
	return instance;
}

void eZap::keyDown(int code)
{
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::keyDown, code));
	else
		main->event(eWidgetEvent(eWidgetEvent::keyDown, code));
}

void eZap::keyUp(int code)
{
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::keyUp, code));
	else
		main->event(eWidgetEvent(eWidgetEvent::keyUp, code));
}

void eZap::status()
{
}

void eZap::switchFontSize()
{
	emit fontSizeChanged(FontSize=useBigFonts?28:24);
}

QString eZap::getVersion()
{
	return "enigma 0.1, compiled " __DATE__;
}

eZap::eZap(int argc, char **argv): QApplication(argc, argv, 0)
{
	instance=this;

	grc=new gRC();
	font=new fontRenderClass();


	lcd=new eDBoxLCD;
	glcddc=new gLCDDC(lcd);
	gfbdc=new gFBDC();

	focus=0;
	
	rc=new eRCInput;
	connect(rc, SIGNAL(keyDown(int)), SLOT(keyDown(int)));
	connect(rc, SIGNAL(keyUp(int)), SLOT(keyUp(int)));

	new eDVB();
	
	if (eDVB::getInstance()->config.getKey("/ezap/rc/repeatDelay", rc->rdelay))
		rc->rdelay=500;
	if (eDVB::getInstance()->config.getKey("/ezap/rc/repeatRate", rc->rrate))
		rc->rrate=30;
	if (eDVB::getInstance()->config.getKey("/ezap/osd/useBigOSD", useBigOSD))
		useBigOSD=0;
	if (eDVB::getInstance()->config.getKey("/ezap/osd/useBigFonts", useBigFonts))
		useBigFonts=0;

	eSkin_init();

	eSkin *skin=new eSkin;
	if (skin->load( DATADIR "/enigma/skins/default.esml"))
		qFatal("skin load failed (" DATADIR "/enigma/skins/default.esml)");
	skin->setPalette(gfbdc);
	skin->makeActive();

	qDebug("...");
	switchFontSize();
	qDebug("<-- switch font size");
	
	serviceSelector=new eServiceSelector;
	
	qDebug("<-- service selector");

	eDVB::getInstance()->configureNetwork();
	
	main=0;
	
	qDebug("<-- network");
#if 0
	while (1)
	{

		eWindow test;
		if (skin->build(&test, "test"))
			qFatal("skin load of \"test\" failed");

		eWidget *okButton=test.search("okButton");
		if (!okButton)
			qFatal("sorry no okButton");

		connect(okButton, SIGNAL(selected()), &test, SLOT(accept()));

		test.show();
		{
			gPixmap *logo=loadPNG("/usr/lib/edvb/osd.png");
			{
				gPixmapDC dc(logo);
				gPainter p(dc);
				p.mergePalette(gfbdc->getPixmap()); 
			}

			gPainter p(*gfbdc);

			for (int y=0; y<16; y++)
				for (int x=0; x<16; x++)
				{
					p.setForegroundColor(y*16+x);
					p.fill(QRect(x*16+100, y*16+100, 16, 16));
				}
			p.blit(*logo, QPoint(100, 100));
			p.flush();
			qDebug("done!!");
			delete logo;
		}
		test.exec();
		test.hide();
	}
	
	qFatal("das wars");

#endif
	main=new eZapMain();
	
	qDebug("[EZAP] starting httpd");
	eHTTPD *httpd=new eHTTPD(7575);
	eHTTPDynPathResolver *dyn_resolver=new eHTTPDynPathResolver();
	ezapInitializeDyn(dyn_resolver);
	ezapInitializeXMLRPC(dyn_resolver);
	httpd->addResolver(dyn_resolver);
	
	eHTTPFilePathResolver *fileresolver=new eHTTPFilePathResolver();
	fileresolver->addTranslation("/usr/httpd/htdocs", "/www");
	fileresolver->addTranslation("/var/elitedvb/htdocs", "/");
	httpd->addResolver(fileresolver);
	qDebug("[EZAP] ok, beginning mainloop");
	__u32 lastchannel;

	int bootcount;
	if (eDVB::getInstance()->config.getKey("/elitedvb/system/bootCount", bootcount))
		bootcount=0;

	if (!bootcount)
	{
		eMessageBox msg("Willkommen zu EliteDVB.\n\nBitte f�hren sie zun�chst eine Kanalsuche durch, indem sie die d-Box-Taste dr�cken um in das "
			"Hauptmen� zu gelangen. Dort gibt es den Unterpunkt \"Transponder Scan\", der genau das macht, was sie glauben.\n", "EliteDVB - erster Start");
		msg.show();
		msg.exec();
		msg.hide();
	}
	
	bootcount++;
	eDVB::getInstance()->config.setKey("/elitedvb/system/bootCount", bootcount);

	int e;
	if ((!(e=eDVB::getInstance()->config.getKey("/ezap/ui/lastChannel", lastchannel))) && (eDVB::getInstance()->getTransponders()))
	{
		eService *t=eDVB::getInstance()->getTransponders()->searchService(lastchannel>>16, lastchannel&0xFFFF);
		if (t)
			eDVB::getInstance()->switchService(t);
	}
}

eZap::~eZap()
{
	eSkin_close();
	if (eDVB::getInstance()->service)
		eDVB::getInstance()->config.setKey("/ezap/ui/lastChannel", (__u32)((eDVB::getInstance()->original_network_id<<16)|eDVB::getInstance()->service_id));
	qDebug("[EZAP] beginning clean shutdown");
	qDebug("[EZAP] main");
	delete main;
	qDebug("[EZAP] serviceSelector");
	delete serviceSelector;
	qDebug("[EZAP] eDVB");
	delete eDVB::getInstance();
	qDebug("[EZAP] font");
	delete font;
	qDebug("[EZAP] rc");
	delete rc;
	qDebug("[EZAP] grc");
	delete grc;
	qDebug("[EZAP] gfbdc");
	delete gfbdc;
	qDebug("[EZAP] lcd");
	delete glcddc;
	qDebug("[EZAP] fertig");
	instance=0;
}

void eZap::Fade(gRGB *pal, int i, __u32 rgb1, __u32 rgb2, int level)
{
	int *r=&pal[i].r;
	int *g=&pal[i].g;
	int *b=&pal[i].b;
	int *a=&pal[i].a;
	*a= (((rgb2&0xFF000000)>>24)*level)>>8;
	*r= (((rgb2&0x00FF0000)>>16)*level)>>8;
	*g= (((rgb2&0x0000FF00)>>8 )*level)>>8;
	*b= (((rgb2&0x000000FF)    )*level)>>8;
	*a+=(((rgb1&0xFF000000)>>24)*(256-level))>>8;
	*r+=(((rgb1&0x00FF0000)>>16)*(256-level))>>8;
	*g+=(((rgb1&0x0000FF00)>>8 )*(256-level))>>8;
	*b+=(((rgb1&0x000000FF)    )*(256-level))>>8;
}

void eZap::GenFade(gRGB *pal, int in, __u32 rgb1, __u32 rgb2, int num, int tr=0)
{
	for (int i=0; i<num; i++)
	{
		Fade(pal, in+i, rgb1, rgb2, i*255/(num-1));
		tr++;
	}
}

void eZap::Set(gRGB *pal, int i, __u32 rgb)
{
	pal[i].r=(rgb&0x00FF0000)>>16;
	pal[i].g=(rgb&0x0000FF00)>>8;
	pal[i].b=(rgb&0x000000FF);
	pal[i].a=(rgb&0xFF000000)>>24;
}

__u32 eZap::Fade(__u32 val, int h)
{
	int r, g, b, a;
	r=val&0xFF;
	g=(val&0xFF00)>>8;
	b=(val&0xFF0000)>>16;
	a=(val&0xFF000000)>>24;
	r*=h;
	g*=h;
	b*=h;
	r>>=8; 
	g>>=8; 
	b>>=8; 
	if (r>255)
		r=255;
	if (g>255)
		g=255;
	if (b>255)
		b=255;
	return (r)|(g<<8)|(b<<16)|(a<<24);
}

int main(int argc, char **argv)
{
	time_t t=0;
	stime(&t);
	fprintf(stderr, "%s", copyright);
	{
		eZap ezap(argc, argv);
		ezap.exec();
	}
	// system("/sbin/halt &");
}

eMainMenu::eMainMenu()
{
	window=new eLBWindow("31337DVB " __DATE__ , eListbox::tBorder, 12, eZap::FontSize, 240);
	window->move(QPoint(70, 150));
	connect(new eListboxEntryText(window->list, "TV Mode"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_close(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "VCR Mode"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_vcr(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Transponder Scan"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_scan(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Setup"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_setup(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Streaminfo"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_streaminfo(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Show BN version"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_bnversion(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Plugins"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_plugins(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Quit EliteDVB"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_quit(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "RECORD MODE"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_record(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "About..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_about(eListboxEntry*)));
}

int eMainMenu::exec()
{
	window->show();
	int res=window->exec();
	window->hide();
	return res;
}

eMainMenu::~eMainMenu()
{
	delete window;
}

void eMainMenu::sel_close(eListboxEntry *lbe)
{
	window->close(0);
}

void eMainMenu::sel_vcr(eListboxEntry *lbe)
{
	window->hide();
	eAVSwitch::getInstance()->setInput(1);
	eMessageBox mb("If you can read this, your scartswitch doesn't work", "VCR");
	mb.show();
	mb.exec();
	mb.hide();
	eAVSwitch::getInstance()->setInput(0);
	window->show();
}

void eMainMenu::sel_scan(eListboxEntry *)
{
	TransponderScan ts;
	window->hide();
	ts.exec();
	window->show();
}

void eMainMenu::sel_streaminfo(eListboxEntry *)
{
	eStreaminfo si;
	window->hide();
	si.show();
	si.exec();
	si.hide();
	window->show();
}

void eMainMenu::sel_setup(eListboxEntry *)
{
	eZapSetup setup;
	window->hide();
	setup.exec();
	window->show();
}

void eMainMenu::sel_plugins(eListboxEntry *)
{
	window->hide();
	eZapPlugins plugins;
	plugins.exec();
	window->show();
}

void eMainMenu::sel_quit(eListboxEntry *)
{
	window->close(1);
}

void eMainMenu::sel_bnversion(eListboxEntry *)
{
	ShowBNVersion bn;
	window->hide();
	bn.show();
	bn.exec();
	bn.hide();
	window->show();
}

void eMainMenu::sel_record(eListboxEntry *)
{
	Decoder::parms.recordmode=1;
	Decoder::Set(0);
	eStreaminfo si(1);
	window->hide();
	si.show();
	si.exec();
	si.hide();
	window->show();
	Decoder::parms.recordmode=0;
	Decoder::Set(eDVB::getInstance()->useAC3);
};

void eMainMenu::sel_about(eListboxEntry *)
{
	window->hide();
	eMessageBox msgbox(
"eZap was constructed by Felix Domke <tmbinc@gmx.net> in 2001, 2002 for the dbox2-linux-project.\n"
"Special thanks and respects go out to:\n"
"Farbrausch Consumer Consulting (http://www.farb-rausch.de)\n"
"some unnamed guy (WN) who provided us lowlevel information (without you, the whole think wouldn't be possible. thanks again.)\n"
"and off course all dbox2-linux-team-members.\n",
	"About eZap");
	msgbox.show();
	msgbox.exec();
	msgbox.hide();
	window->show();
};

#include <time.h>
#include <errno.h>
#include "enigma_main.h"
#include "elistbox.h"
#include "iso639.h"
#include "edvb.h"
#include "enigma_mainmenu.h"
#include "elabel.h"
#include "eprogress.h"
#include "enigma_event.h"
#include "sselect.h"
#include "enumber.h"
#include "eskin.h"
#include "streamwd.h"
#include "font.h"
#include "rc.h"
#include "enigma.h"
#include "enigma_lcd.h"
#include "decoder.h"
#include "enigma_plugins.h"
#include "download.h"
#include "epgcache.h"
#include "epgwindow.h"

#include "ebutton.h"
#include "esection.h"

/*

schoene kleine demonstration von enigma:

ein fenster (eWindow) das die datenrate fuer einen sectionstream 
(eSection(0x12, 0, -1, -1, 0, 0): pid 0x12, tid 0 tid-mask 0 (am ende), tableidext -1,
version egal) misst und jede sekunde (eTimer::timeout) das fenster updated
(->setText vom eLabel). das ganze ist NICHT geskinnt.

das ganze demonstriert imho recht schoen die einfachheit wie man sowas machen kann.

class BlaTest: public eWindow, eSection
{
	int recv, bytes, last;
	eLabel *l_recv;
	eButton *b_quit;
	eTimer timer;
	void update()
	{
		char bla[100];
		sprintf(bla, "%d (%d kb, %d kb/s)", recv, bytes/1024, last/1024);
		last=0;
		l_recv->setText(bla);
	}
	int sectionRead(__u8 *data)
	{
		recv++;
		bytes+=(data[1]&0x3F)<<8;
		bytes+=data[2];
		last+=(data[1]&0x3F)<<8;
		last+=data[2];
		return 0;
	}
	void quit()
	{
		close(0);
	}
public:
	BlaTest(): eWindow(0), eSection(0x12, 0, -1, -1, 0, 0), timer(eApp)
	{
		setText("test");  // fenstertitel setzen
		move(ePoint(100, 100)); // verschieben auf 100 100
		resize(eSize(500, 400)); // vergroessern auf 500 400
		l_recv=new eLabel(this);  // neues label erstellen
		l_recv->move(ePoint(0, 0));  // label an 0 0 (links oben) schieben
		l_recv->resize(eSize(clientrect.width(), 30));  // irgendwie gross machen
		
		b_quit=new eButton(this);    // button zum beenden machen
		b_quit->move(ePoint(0, 30));  // groesse anpassen
		b_quit->resize(eSize(clientrect.width(), 40));  // ""
		b_quit->setText("oki");  // text vom button setzen
		CONNECT(timer.timeout, BlaTest::update);  // time_out vom updatetimer (1s) auf update setzen
		CONNECT(b_quit->selected, BlaTest::quit);  // button druecken auf quit connecten
		timer.start(1000);  // updatetimer starten (1s)
		recv=0;  // stats initialisieren
		last=0;  // ""
		bytes=0; // ""
		
		start(); // und section filter anmachen
	}
};

*/

static eString getISO639Description(char *iso)
{
	for (unsigned int i=0; i<sizeof(iso639)/sizeof(*iso639); ++i)
	{
		if (!strnicmp(iso639[i].iso639foreign, iso, 3))
			return iso639[i].description1;
		if (!strnicmp(iso639[i].iso639int, iso, 3))
			return iso639[i].description1;
	}
	return eString()+iso[0]+iso[1]+iso[2];
}


void NVODStream::EITready(int error)
{
	eDebug("NVOD eit ready: %d", error);
	listbox->sort();
	if (listbox && listbox->isVisible())
		listbox->invalidate();
}

NVODStream::NVODStream(eListbox *listbox, int transport_stream_id, int original_network_id, int service_id)
	: eListboxEntry(listbox), transport_stream_id(transport_stream_id), original_network_id(original_network_id),
		service_id(service_id), eit(EIT::typeNowNext, service_id,
		(			(eDVB::getInstance()->transport_stream_id==transport_stream_id)
			&&	(eDVB::getInstance()->original_network_id==original_network_id))?EIT::tsActual:EIT::tsOther		)
{
	CONNECT(eit.tableReady, NVODStream::EITready);
	eit.start();
}

eString NVODStream::getText(int col=0) const
{
	if (eit.ready && !eit.error)
	{
		for (ePtrList<EITEvent>::const_iterator i(eit.events); i != eit.events.end(); ++i)		// always take the first one
		{
			eString s="--:--";
			EITEvent *event=*i;
			if (col==-1)
				return eString().sprintf("%08x", (unsigned int)event->start_time);
			tm *begin=event->start_time!=-1?localtime(&event->start_time):0;
			if (begin)
				s.sprintf("%02d:%02d", begin->tm_hour, begin->tm_min);
			time_t endtime=event->start_time+event->duration;
			tm *end=event->start_time!=-1?localtime(&endtime):0;
			if (end)
				s+=eString().sprintf(" bis %02d:%02d", end->tm_hour, end->tm_min);
			time_t now=time(0)+eDVB::getInstance()->time_difference;
			if ((event->start_time <= now) && (now < endtime))
			{
				int perc=(now-event->start_time)*100/event->duration;
				s+=+" ("+eString().sprintf("%d%%, %d.%02d Euro lost)", perc, perc*3/100, (perc*3)%100);
			}
			return s;
		}
	}
	eString s;
	s.sprintf("Service %04x", service_id);
	return s;
}

void eNVODSelector::selected(eListboxEntry *l)
{
	NVODStream *nv=(NVODStream*)l;
	if (nv)
		eDVB::getInstance()->switchService(nv->service_id, nv->original_network_id, nv->transport_stream_id, 5);	// faked service_type
	close(0);
}

eNVODSelector::eNVODSelector(): eWindow(0)
{
	setText("NVOD");
	move(ePoint(100, 100));
	resize(eSize(440, 380));
	list=new eListbox(this, eListbox::tLitebar, eSkin::getActive()->queryValue("fontsize", 20));
	list->move(ePoint(0, 0));
	list->resize(getClientSize());
	CONNECT(list->selected, eNVODSelector::selected);
}

void eNVODSelector::clear()
{
	list->clearList();
}

void eNVODSelector::add(NVODReferenceEntry *ref)
{
	new NVODStream(list, ref->transport_stream_id, ref->original_network_id, ref->service_id);
}

AudioStream::AudioStream(eListbox *listbox, PMTEntry *stream): eListboxEntry(listbox), stream(stream)
{
}

eString AudioStream::getText(int col=0) const
{
	int isAC3=0;
	eString language;
	int component_tag=-1;
	language.sprintf("PID %04x", stream->elementary_PID);
	for (ePtrList<Descriptor>::iterator i(stream->ES_info); i != stream->ES_info.end(); ++i)
	{
		Descriptor *c=*i;
		if (c->Tag()==DESCR_AC3)
			isAC3=1;
		else if (c->Tag()==DESCR_ISO639_LANGUAGE)
			language=getISO639Description(((ISO639LanguageDescriptor*)c)->language_code);
		else if (c->Tag()==DESCR_STREAM_ID)
			component_tag=((StreamIdentifierDescriptor*)c)->component_tag;
		else if (c->Tag()==DESCR_LESRADIOS)
		{
			language=eString().sprintf("%d.) ", (((LesRadiosDescriptor*)c)->id));
			language+=((LesRadiosDescriptor*)c)->name;
		}
	}
	if (component_tag!=-1)
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		if (eit)
		{
			for (ePtrList<EITEvent>::iterator e(eit->events); e != eit->events.end(); ++e)
				if ((e->running_status>=2)||(!e->running_status))		// currently running service
					for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
						if (d->Tag()==DESCR_COMPONENT)
							if (((ComponentDescriptor*)*d)->component_tag==component_tag)
								language=((ComponentDescriptor*)*d)->text;
			eit->unlock();
		}
	}
	if (isAC3)
		language+=" (AC3)";
	return language;
}

void eAudioSelector::selected(eListboxEntry *l)
{
	if (l)
	{
		eDVB::getInstance()->setPID(((AudioStream*)l)->stream);
		eDVB::getInstance()->setDecoder();
	}
	close(0);
}

eAudioSelector::eAudioSelector(): eWindow(0)
{
	setText("Audio");
	move(ePoint(100, 100));
	resize(eSize(300, 330));
	list=new eListbox(this, eListbox::tLitebar, eSkin::getActive()->queryValue("fontsize", 20));
	list->move(ePoint(0, 0));
	list->resize(getClientSize());
	CONNECT(list->selected, eAudioSelector::selected);
}

void eAudioSelector::clear()
{
	list->clearList();
}

void eAudioSelector::add(PMTEntry *pmt)
{
	new AudioStream(list, pmt);
}

SubService::SubService(eListbox *listbox, LinkageDescriptor *descr): eListboxEntry(listbox)
{
	name=eString((const char*)descr->private_data);
	transport_stream_id=descr->transport_stream_id;
	original_network_id=descr->original_network_id;
	service_id=descr->service_id;
}

eString SubService::getText(int col=0) const
{
	return name;
}

eSubServiceSelector::eSubServiceSelector(): eWindow(0)
{
	setText("Bildregie");
	move(ePoint(100, 100));
	resize(eSize(350, 330));
	list=new eListbox(this, eListbox::tLitebar, eSkin::getActive()->queryValue("fontsize", 20));
	list->move(ePoint(0, 0));
	list->resize(getClientSize());
	CONNECT(list->selected, eSubServiceSelector::selected);
}

void eSubServiceSelector::selected(eListboxEntry *l)
{
	SubService *ss=(SubService*)l;

	if (ss)
		eDVB::getInstance()->switchService(ss->service_id, ss->original_network_id, ss->transport_stream_id, 5);	// faked service_type
	close(0);
}

void eSubServiceSelector::clear()
{
	list->clearList();
}

void eSubServiceSelector::add(LinkageDescriptor *ref)
{
	new SubService(list, ref);
}

void eServiceNumberWidget::selected(int *res)
{
	if (!res)
		close(-1);
	chnum=*res;
	close(chnum);
//	timer->start(100);
}

void eServiceNumberWidget::timeout()
{
	close(chnum);
}

eServiceNumberWidget::eServiceNumberWidget(int initial)
										:eWindow(0)
{
	setText("Channel");
	move(ePoint(200, 140));
	resize(eSize(280, 120));
	eLabel *label;
	label=new eLabel(this);
	label->setText("Channel:");
	label->move(ePoint(50, 00));
	label->resize(eSize(110, eSkin::getActive()->queryValue("fontsize", 20)+4));
	
	number=new eNumber(this, 1, 1, 999, 3, &initial, 1, label);
	number->move(ePoint(160, 0));
	number->resize(eSize(50, eSkin::getActive()->queryValue("fontsize", 20)+4));

	CONNECT(number->selected, eServiceNumberWidget::selected);
	
/*	timer=new eTimer(eApp);
	timer->start(2000);
	CONNECT(timer->timeout, eServiceNumberWidget::timeout);	
*/
	chnum=initial;
}

eServiceNumberWidget::~eServiceNumberWidget()
{
}

void eZapMain::redrawWidget(gPainter *painter, const eRect &where)
{
}

void eZapMain::eraseBackground(gPainter *painter, const eRect &where)
{
}

eZapMain::eZapMain(): eWidget(0, 1), timeout(eApp), clocktimer(eApp)
{
	isVT=0;
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "ezap_main"))
		eFatal("skin load of \"ezap_main\" failed");

	eDebug("[PROFILE] eZapMain");
	lcdmain.show();
	eDebug("<-- show lcd.");

	ASSIGN(ChannelNumber, eLabel, "ch_number");
	ASSIGN(ChannelName, eLabel, "ch_name");

	ASSIGN(EINow, eLabel, "e_now_title");
	ASSIGN(EINext, eLabel, "e_next_title");
	
	ASSIGN(EINowDuration, eLabel, "e_now_duration");
	ASSIGN(EINextDuration, eLabel, "e_next_duration");

	ASSIGN(EINowTime, eLabel, "e_now_time");
	ASSIGN(EINextTime, eLabel, "e_next_time");

	ASSIGN(Description, eLabel, "description");
	ASSIGN(VolumeBar, eProgress, "volume_bar");
	ASSIGN(Progress, eProgress, "progress_bar");
	
	ASSIGN(ButtonRedEn, eLabel, "button_red_enabled");
	ASSIGN(ButtonGreenEn, eLabel, "button_green_enabled");
	ASSIGN(ButtonYellowEn, eLabel, "button_yellow_enabled");
	ASSIGN(ButtonBlueEn, eLabel, "button_blue_enabled");
	ASSIGN(ButtonRedDis, eLabel, "button_red_disabled");
	ASSIGN(ButtonGreenDis, eLabel, "button_green_disabled");
	ASSIGN(ButtonYellowDis, eLabel, "button_yellow_disabled");
	ASSIGN(ButtonBlueDis, eLabel, "button_blue_disabled");

	ASSIGN(DolbyOn, eLabel, "osd_dolby_on");
	ASSIGN(CryptOn, eLabel, "osd_crypt_on");
	ASSIGN(WideOn, eLabel, "osd_format_on");
	ASSIGN(DolbyOff, eLabel, "osd_dolby_off");
	ASSIGN(CryptOff, eLabel, "osd_crypt_off");
	ASSIGN(WideOff, eLabel, "osd_format_off");
	DolbyOn->hide();
	CryptOn->hide();
	WideOn->hide();
	DolbyOff->show();
	CryptOff->show();
	WideOff->show();

	ButtonRedEn->setFlags(RS_DIRECT);
	ButtonRedEn->setText("\x19");
	ButtonGreenEn->setFlags(RS_DIRECT);
	ButtonGreenEn->setText("\x19");
	ButtonYellowEn->setFlags(RS_DIRECT);
	ButtonYellowEn->setText("\x19");
	ButtonBlueEn->setFlags(RS_DIRECT);
	ButtonBlueEn->setText("\x19");

	ButtonRedDis->setFlags(RS_DIRECT);
	ButtonRedDis->setText("\x19");
	ButtonGreenDis->setFlags(RS_DIRECT);
	ButtonGreenDis->setText("\x19");
	ButtonYellowDis->setFlags(RS_DIRECT);
	ButtonYellowDis->setText("\x19");
	ButtonBlueDis->setFlags(RS_DIRECT);
	ButtonBlueDis->setText("\x19");
	
	ButtonRedEn->hide();
	ButtonRedDis->show();
	ButtonGreenEn->hide();
	ButtonGreenDis->show();
	ButtonYellowEn->hide();
	ButtonYellowDis->show();
	ButtonBlueEn->hide();
	ButtonBlueDis->show();

	Clock=new eLabel(this);
	ASSIGN(Clock, eLabel, "time");

	cur_start=cur_duration=-1;
	CONNECT(eStreamWatchdog::getInstance()->AspectRatioChanged, eZapMain::set16_9Logo);
	CONNECT(eEPGCache::getInstance()->EPGAvail, eZapMain::setEPGButton);
	CONNECT(eDVB::getInstance()->scrambled, eZapMain::setSmartcardLogo);
	CONNECT(eDVB::getInstance()->switchedService, eZapMain::serviceChanged);
	CONNECT(eDVB::getInstance()->gotEIT, eZapMain::gotEIT);
	CONNECT(eDVB::getInstance()->gotSDT, eZapMain::gotSDT);
	CONNECT(eDVB::getInstance()->gotPMT, eZapMain::gotPMT);
	CONNECT(timeout.timeout, eZapMain::timeOut);
	CONNECT(clocktimer.timeout, eZapMain::clockUpdate);
	CONNECT(eDVB::getInstance()->timeUpdated, eZapMain::clockUpdate);
	CONNECT(eDVB::getInstance()->leaveService, eZapMain::leaveService);
	CONNECT(eDVB::getInstance()->volumeChanged, eZapMain::updateVolume);

	actual_eventDisplay=0;

	eDebug("...");
	clockUpdate();	
	eDebug("<-- clockUpdate");
}

eZapMain::~eZapMain()
{
}

void eZapMain::set16_9Logo(int aspect)
{
	if (aspect)
	{
		WideOff->hide();
		WideOn->show();
	} else
	{
		WideOn->hide();
		WideOff->show();
	}
}

void eZapMain::setEPGButton(bool b)
{
	if (b)
	{
		isEPG=1;
		ButtonRedDis->hide();
		ButtonRedEn->show();
	}
	else
	{
		isEPG=0;
		ButtonRedEn->hide();
		ButtonRedDis->show();
	}
}

void eZapMain::setVTButton(bool b)
{
	if (b)
	{
		ButtonBlueDis->hide();
		ButtonBlueEn->show();
	}
	else
	{
		ButtonBlueEn->hide();
		ButtonBlueDis->show();
	}
}

void eZapMain::setAC3Logo(bool b)
{
	if (b)
	{
		DolbyOff->hide();
		DolbyOn->show();
	} else
	{
		DolbyOn->hide();
		DolbyOff->show();
	}
}

void eZapMain::setSmartcardLogo(bool b)
{
	if (b)
	{
		CryptOff->hide();
		CryptOn->show();
	} else
	{
		CryptOn->hide();
		CryptOff->show();
	}
}

void eZapMain::setEIT(EIT *eit)
{
	int numsub=0;
	subservicesel.clear();
	
	if (eit)
	{
		eString nowtext, nexttext, nowtime="", nexttime="", descr;
		int val=0;
		int p=0;
		
		for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
		{
			EITEvent *event=*i;
			if ((event->running_status>=2) || ((!p) && (!event->running_status)))
			{
				cur_start=event->start_time;
				cur_duration=event->duration;
				clockUpdate();
				for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					if (d->Tag()==DESCR_LINKAGE)
					{
						LinkageDescriptor *ld=(LinkageDescriptor*)*d;
						if (ld->linkage_type!=0xB0)
							continue;
						subservicesel.add(ld);
						numsub++;
					}
			}
			for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
					switch (p)
					{
					case 0:
						nowtext=ss->event_name;
						val|=1;
						descr=ss->text;
						break;
					case 1:
						nexttext=ss->event_name;
						val|=2;
						break;
					}
				}
			}
			tm *t=event->start_time!=-1?localtime(&event->start_time):0;
			eString start="";
			if (t && event->duration)
				start.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
			eString duration;
			if (event->duration>0)
				duration.sprintf("%d min", event->duration/60);
			else
				duration="";
			switch (p)
			{
			case 0:
				EINowDuration->setText(duration);
				nowtime=start;
				break;
			case 1:
				EINextDuration->setText(duration);
				nexttime=start;
				break;
			}
			if (!eDVB::getInstance()->service_state)
				Description->setText(descr);
			p++;
		}
		if (val&1)
		{
			EINow->setText(nowtext);
			EINowTime->setText(nowtime);
		}

		if (val&2)
		{
			EINext->setText(nexttext);
			EINextTime->setText(nexttime);
		}
	} else
	{
		EINow->setText("kein EPG verf�gbar");
		EINext->setText("");
		EINowDuration->setText("");
		EINextDuration->setText("");
		EINowTime->setText("");
		EINextTime->setText("");
	}
	if (numsub>1)
	{
		flags|=ENIGMA_SUBSERVICES;
	}
	if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES))
	{
		ButtonGreenEn->show();
		ButtonGreenDis->hide();
	}
	else
	{
		ButtonGreenDis->show();
		ButtonGreenEn->hide();
	}
	ePtrList<EITEvent> dummy;
	if (actual_eventDisplay)
		actual_eventDisplay->setList(eit?eit->events:dummy);
}

void eZapMain::handleNVODService(SDTEntry *sdtentry)
{
	nvodsel.clear();
	for (ePtrList<Descriptor>::iterator i(sdtentry->descriptors); i != sdtentry->descriptors.end(); ++i)
		if (i->Tag()==DESCR_NVOD_REF)
			for (ePtrList<NVODReferenceEntry>::iterator e(((NVODReferenceDescriptor*)*i)->entries); e != ((NVODReferenceDescriptor*)*i)->entries.end(); ++e)
				nvodsel.add(*e);
	nvodsel.setText(eDVB::getInstance()->service->service_name.c_str());
}

void eZapMain::keyDown(int code)
{
	switch (code)
	{
	case eRCInput::RC_DOWN:
	case eRCInput::RC_UP:
	{
		hide();
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		eService *service=eZap::getInstance()->getServiceSelector()->choose(0, code);
		pLCD->lcdMain->show();
		pLCD->lcdMenu->hide();
		if (!service)
			break;
		if (service)
			if (eDVB::getInstance()->switchService(service))
			{
#if 0
				serviceChanged(service, -EAGAIN);
#endif
			}
		break;
	}
	case eRCInput::RC_RIGHT:
	{
		eService *service=eZap::getInstance()->getServiceSelector()->next();
		if (!service)
			break;
		if (service)
			if (eDVB::getInstance()->switchService(service))
				serviceChanged(service, -EAGAIN);
		break;
	}
	case eRCInput::RC_LEFT:
	{
		eService *service=eZap::getInstance()->getServiceSelector()->prev();
		if (!service)
			break;
		if (service)
			if (eDVB::getInstance()->switchService(service))
				serviceChanged(service, -EAGAIN);
		break;
	}
	case eRCInput::RC_MINUS:
		eDVB::getInstance()->changeVolume(0, +4);
//		if (!isVisible())
//			show();
//		timeout.start(1000, 1);
		break;
	case eRCInput::RC_PLUS:
		eDVB::getInstance()->changeVolume(0, -4);
//		if (!isVisible())
//			show();
//		timeout.start(1000, 1);
		break;
	case eRCInput::RC_MUTE:
		eDVB::getInstance()->changeVolume(2, 1);
		break;
	case eRCInput::RC_0 ... eRCInput::RC_9:
	{
		if (!eDVB::getInstance()->getTransponders())
			break;
	
		if (isVisible())
			hide();

		eZapLCD* pLCD = eZapLCD::getInstance();				
		eServiceNumberWidget *w=new eServiceNumberWidget(code-eRCInput::RC_0);
		w->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();

//		eDebug("w is %p", w);
		w->show();
		int chnum=w->exec();
		w->hide();

		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();

		if (chnum!=-1)
		{
			eService *service=eDVB::getInstance()->getTransponders()->searchServiceByNumber(chnum);
			if (!service)
				break;
			if (service)
				if (eDVB::getInstance()->switchService(service))
					serviceChanged(service, -EAGAIN);
		}
		delete w;

		break;
	}
	}
}

void eZapMain::keyUp(int code)
{
	switch (code)
	{
	case eRCInput::RC_DBOX:
	{
		if (isVisible())
			hide();
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		eMainMenu mm;
		mm.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		if (mm.exec())
			eZap::getInstance()->quit();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
		break;
	}
	case eRCInput::RC_STANDBY:
		eZap::getInstance()->quit();
		break;
	case eRCInput::RC_OK:
		if (isVisible())
		{
			timeout.stop();
			hide();
		} else
		{
			timeout.start(10000, 1);
			show();
		}
		break;
	case eRCInput::RC_GREEN:
	{
		if (flags&ENIGMA_NVOD)
		{
			if (isVisible())
			{
				timeout.stop();
				hide();
			}
			nvodsel.show();
			nvodsel.exec();
			nvodsel.hide();
		}
		if (flags&ENIGMA_SUBSERVICES)
		{
			if (isVisible())
			{
				timeout.stop();
				hide();
			}
			subservicesel.show();
			subservicesel.exec();
			subservicesel.hide();
		}
		break;
	}
	case eRCInput::RC_YELLOW:
	{
		if (flags&ENIGMA_AUDIO)
		{
			if (isVisible())
			{
				timeout.stop();
				hide();
			}
			audiosel.show();
			audiosel.exec();
			audiosel.hide();
		}
		break;
	}
	case eRCInput::RC_BLUE:
	{
		if (isVT)
		{
			eZapPlugins plugins;
			plugins.execPluginByName("tuxtxt.cfg");
		}
		break;
	}
	case eRCInput::RC_RED:
	{
		if (isEPG)
		{
			eZapLCD* pLCD = eZapLCD::getInstance();
			pLCD->lcdMain->hide();
			pLCD->lcdMenu->show();
			eEPGWindow wnd(eDVB::getInstance()->service);
			wnd.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
			if (isVisible())
			{
				timeout.stop();
				hide();
			}
			wnd.show();
			wnd.exec();
			wnd.hide();
			pLCD->lcdMenu->hide();
			pLCD->lcdMain->show();
		}
/*		BlaTest bla;
		bla.show();
		bla.exec();
		bla.hide(); */
		break;
	}
	case eRCInput::RC_HELP:
	{
		eService* service = eDVB::getInstance()->service;

		if (!service)
			break;

		if (isVisible())
		{
			timeout.stop();
			hide();
		}

		const eventMap* pMap = eEPGCache::getInstance()->getEventMap(service->original_network_id, service->service_id);

		if (isEPG)  // EPG vorhanden
		{
			eventMap::const_iterator It = pMap->begin();
			ePtrList<EITEvent> events;
			events.setAutoDelete(true);
			events.push_back( new EITEvent(*(It++)->second));
			if (It != pMap->end())  // sicher ist sicher !
				events.push_back( new EITEvent(*It->second));
			eEventDisplay ei(service->service_name.c_str(), &events);			
			actual_eventDisplay=&ei;
			ei.show();
			ei.exec();
			ei.hide();
			actual_eventDisplay=0;
		}
		else	
		{
			EIT *eit=eDVB::getInstance()->getEIT();
			ePtrList<EITEvent> dummy;
			{
				eEventDisplay ei(service->service_name.c_str(), eit?&eit->events:&dummy);
				if (eit)
					eit->unlock();		// HIER liegt der hund begraben.
				actual_eventDisplay=&ei;
				ei.show();
				ei.exec();
				ei.hide();
				actual_eventDisplay=0;
			}
		}
		break;
	}
	}
}

void eZapMain::serviceChanged(eService *service, int err)
{
	isVT = Decoder::parms.tpid != -1;

	setVTButton(isVT);

	if (!service)
		return;

	if (service->service_type==4)
		flags|=ENIGMA_NVOD;

	ChannelName->setText(service->service_name.c_str());
	
	switch (err)
	{
	case 0:
		Description->setText("");
		break;
	case -EAGAIN:
		Description->setText("Einen Moment bitte...");
		break;
	case -ENOENT:
		Description->setText("Sender konnte nicht gefunden werden.");
		break;
	case -ENOCASYS:
		Description->setText("Dieser Sender kann nicht entschl�sselt werden.");
		break;
	case -ENOSTREAM:
		Description->setText("Dieser Sender sendet (momentan) kein Signal.");
		break;
	case -ENOSYS:
		Description->setText("Dieser Inhalt kann nicht dargestellt werden.");
		break;
	case -ENVOD:
		Description->setText("NVOD - Bitte Anfangszeit bestimmen!");
		break;
	default:
		Description->setText("<unbekannter Fehler>");
		break;
	}
	
	ChannelNumber->setText(eString().sprintf("%d", service->service_number));
	
	if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES))
	{
		ButtonGreenDis->hide();
		ButtonGreenEn->show();
	}
	else
	{
		ButtonGreenEn->hide();
		ButtonGreenDis->show();	
	}

	if (flags&ENIGMA_AUDIO)
	{
		ButtonYellowDis->hide();
		ButtonYellowEn->show();
	}
	else
	{
		ButtonYellowEn->hide();
		ButtonYellowDis->show();
	}
	
	if (!eZap::getInstance()->focus)
		show();

// Quick und Dirty ... damit die aktuelle Volume sofort angezeigt wird.
	eDVB::getInstance()->changeVolume(0, 0);
	timeout.start(eDVB::getInstance()->service_state?10000:2000, 1);
}

void eZapMain::gotEIT(EIT *eit, int error)
{
	setEIT(error?0:eit);
	if (!error)  // Ansonsten erscheint immer wenn die EIT geupdatet wurde das OSD kurz auf dem Screen
// Das nervt gerade wenn man was aufnehmen m�chte :)   Ghostrider
	// ach was das RULT  - tmb
	{
		if (!eZap::getInstance()->focus)
			show();
		timeout.start(eDVB::getInstance()->service_state?10000:2000, 1);
	}
}

void eZapMain::gotSDT(SDT *sdt)
{
	switch (eDVB::getInstance()->service_type)
	{
	case 0x4:	// nvod reference
	{
		for (ePtrList<SDTEntry>::iterator i(sdt->entries); i != sdt->entries.end(); ++i)
		{
			SDTEntry *entry=*i;
			if (entry->service_id==eDVB::getInstance()->service_id)
				handleNVODService(entry);
		}
		break;
	}
	}
}

void eZapMain::gotPMT(PMT *pmt)
{
	bool isAc3 = false;
	eDebug("got pmt");
	int numaudio=0;
	audiosel.clear();
	for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;
		int isaudio=0;
		if (pe->stream_type==3)
			isaudio=1;
		if (pe->stream_type==4)
			isaudio=1;
		if (pe->stream_type==6)
		{
			for (ePtrList<Descriptor>::iterator d(pe->ES_info); d != pe->ES_info.end(); ++d)
				if (d->Tag()==DESCR_AC3)
				{
					isaudio++;
					isAc3=true;
				}
				
		}
		if (isaudio)
		{
			audiosel.add(pe);
			numaudio++;
		}
	}
	if (numaudio>1)
		flags|=ENIGMA_AUDIO;
		
	setAC3Logo(isAc3);
}

void eZapMain::timeOut()
{
	if (eZap::getInstance()->focus==this)
		hide();
}

void eZapMain::leaveService(eService *service)
{
	eDebug("leaving service");

	flags=0;
	
	ChannelName->setText("");
	ChannelNumber->setText("");
	Description->setText("");

	EINow->setText("");
	EINowDuration->setText("");
	EINowTime->setText("");
	EINext->setText("");
	EINextDuration->setText("");
	EINextTime->setText("");
	
	Progress->clear();
	Progress->hide();
}

void eZapMain::clockUpdate()
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
			Progress->show();
		} else
		{
			Progress->clear();
			Progress->hide();
		}
	} else
	{
		Progress->clear();
		Progress->hide();
		Clock->setText("--:--");
		clocktimer.start(60000);
	}
}

void eZapMain::updateVolume(int vol)
{
	eDebug("setting volbar to %d", (63-vol)*100/63);
	VolumeBar->setPerc((63-vol)*100/63);
}


#include "enigma_main.h"

#include <errno.h>
#include <iomanip>

#include <apps/enigma/enigma_mainmenu.h>
#include <apps/enigma/enigma_event.h>
#include <apps/enigma/sselect.h>
#include <apps/enigma/enigma.h>
#include <apps/enigma/enigma_lcd.h>
#include <apps/enigma/enigma_plugins.h>
#include <apps/enigma/download.h>
#include <apps/enigma/epgwindow.h>
#include <core/base/i18n.h>
#include <core/system/init.h>
#include <core/system/econfig.h>
#include <core/dvb/servicedvb.h>
#include <core/dvb/epgcache.h>
#include <core/dvb/esection.h>
#include <core/dvb/decoder.h>
#include <core/dvb/iso639.h>
#include <core/gdi/font.h>
#include <core/gui/elabel.h>
#include <core/gui/eprogress.h>
#include <core/gui/enumber.h>
#include <core/gui/eskin.h>
#include <core/gui/ebutton.h>
#include <core/gui/actions.h>
#include <core/driver/rc.h>
#include <core/driver/streamwd.h>
#include <core/driver/eavswitch.h>
#include <core/dvb/dvbservice.h>
#include <core/gdi/lcd.h>

struct enigmaMainActions
{
	eActionMap map;
	eAction showMainMenu, standby_press, standby_repeat, standby_release, toggleInfobar, showServiceSelector,
		showSubservices, showAudio, pluginVTXT, showEPGList, showEPG, nextService,
		prevService, serviceListDown, serviceListUp, volumeUp, volumeDown, toggleMute;
	enigmaMainActions(): 
		map("enigmaMain", _("enigma Zapp")),
		showMainMenu(map, "showMainMenu", _("show main menu"), eAction::prioDialog),
		standby_press(map, "standby_press", _("go to standby (press)"), eAction::prioDialog),
		standby_repeat(map, "standby_repeat", _("go to standby (repeat)"), eAction::prioDialog),
		standby_release(map, "standby_release", _("go to standby (release)"), eAction::prioDialog),
		toggleInfobar(map, "toggleInfobar", _("toggle infobar"), eAction::prioDialog),
		showServiceSelector(map, "showServiceSelector", _("show service selector"), eAction::prioDialog),
		showSubservices(map, "showSubservices", _("show subservices/NVOD"), eAction::prioDialog),
		showAudio(map, "showAudio", _("show audio selector"), eAction::prioDialog),
		pluginVTXT(map, "pluginVTXT", _("show Videotext"), eAction::prioDialog),
		showEPGList(map, "showEPGList", _("show epg schedule list"), eAction::prioDialog),
		showEPG(map, "showEPG", _("show extended info"), eAction::prioDialog),
		nextService(map, "nextService", _("quickzap next"), eAction::prioDialog),
		prevService(map, "prevService", _("quickzap prev"), eAction::prioDialog),
		serviceListDown(map, "serviceListDown", _("service list and down"), eAction::prioDialog),
		serviceListUp(map, "serviceListUp", _("service list and up"), eAction::prioDialog),
		volumeUp(map, "volumeUp", _("volume up"), eAction::prioDialog),
		volumeDown(map, "volumeDown", _("volume down"), eAction::prioDialog),
		toggleMute(map, "toggleMute", _("toggle mute flag"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaMainActions> i_enigmaMainActions(5, "enigma main actions");

struct enigmaStandbyActions
{
	eActionMap map;
	eAction wakeUp;
	enigmaStandbyActions(): 
		map("enigmaStandby", _("enigma standby")),
		wakeUp(map, "wakeUp", _("wake up enigma"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaStandbyActions> i_enigmaStandbyActions(5, "enigma standby actions");

class eZapStandby: public eWidget
{
protected:
	int eventHandler(const eWidgetEvent &);
public:
	eZapStandby();
};

int eZapStandby::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_enigmaStandbyActions->wakeUp)
			close(0);
		else
			break;
		return 0;
	case eWidgetEvent::execBegin:
	{
		eDBoxLCD::getInstance()->switchLCD(0);
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdStandby->show();

		eAVSwitch::getInstance()->setInput(1);

		break;
	}
	case eWidgetEvent::execDone:
	{
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdStandby->hide();
		pLCD->lcdMain->show();
		eAVSwitch::getInstance()->setInput(0);

		eDBoxLCD::getInstance()->switchLCD(1);
		break;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

eZapStandby::eZapStandby(): eWidget(0, 1)
{
	addActionMap(&i_enigmaStandbyActions->map);
}

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

eString getISO639Description(char *iso)
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
	
	if (eit.ready && !eit.error)
	{
		for (ePtrList<EITEvent>::const_iterator event(eit.events); event != eit.events.end(); ++event)		// always take the first one
		{
			tm *begin=event->start_time!=-1?localtime(&event->start_time):0;

			if (begin)
				text << std::setfill('0') << std::setw(2) << begin->tm_hour << ':' << std::setw(2) << begin->tm_min;

			time_t endtime=event->start_time+event->duration;
			tm *end=event->start_time!=-1?localtime(&endtime):0;

			if (end)
				text << " bis " << std::setw(2) << end->tm_hour << ':' << std::setw(2) << end->tm_min;

			time_t now=time(0)+eDVB::getInstance()->time_difference;
			if ((event->start_time <= now) && (now < endtime))
			{
				int perc=(now-event->start_time)*100/event->duration;
				text << " (" << perc << "%, " << perc*3/100 << '.' << std::setw(2) << (perc*3)%100 << " Euro lost)";
			}
			break;
		}
	}
	else
		text << "Service " << std::setw(4) << std::hex << service.service_id.get();

	((eListBox<NVODStream>*)listbox)->sort(); // <<< without explicit cast the compiler nervs ;)

	if (listbox && listbox->isVisible())
		listbox->invalidate();

}

NVODStream::NVODStream(eListBox<NVODStream> *listbox, const NVODReferenceEntry *ref, int type)
	: eListBoxEntryTextStream((eListBox<eListBoxEntryTextStream>*)listbox), 
		service(eServiceReference::idDVB, eTransportStreamID(ref->transport_stream_id), eOriginalNetworkID(ref->original_network_id),
			eServiceID(ref->service_id), 5), eit(EIT::typeNowNext, ref->service_id, type)
{
	CONNECT(eit.tableReady, NVODStream::EITready);
	eit.start();
}

void eNVODSelector::selected(NVODStream* nv)
{
	if (nv)
		eServiceInterface::getInstance()->play(nv->service);
	close(0);
}

eNVODSelector::eNVODSelector()
	:eListBoxWindow<NVODStream>(_("NVOD"), 10, 440)
{
	move(ePoint(100, 100));
	list.setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	CONNECT(list.selected, eNVODSelector::selected);
}

void eNVODSelector::clear()
{
	list.clearList();
}

void eNVODSelector::add(NVODReferenceEntry *ref)
{
	eServiceReference &service=eServiceInterface::getInstance()->service;

	int type= ((service.transport_stream_id==eTransportStreamID(ref->transport_stream_id))
			&&	(service.original_network_id==eOriginalNetworkID(ref->original_network_id))) ? EIT::tsActual:EIT::tsOther;
	new NVODStream(&list, ref, type);
}

AudioStream::AudioStream(eListBox<AudioStream> *listbox, PMTEntry *stream)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)listbox), stream(stream)
{
	int isAC3=0;
	int component_tag=-1;
	for (ePtrList<Descriptor>::iterator c(stream->ES_info); c != stream->ES_info.end(); ++c)
	{
		if (c->Tag()==DESCR_AC3)
			isAC3=1;
		else if (c->Tag()==DESCR_ISO639_LANGUAGE)
			text=getISO639Description(((ISO639LanguageDescriptor*)*c)->language_code);
		else if (c->Tag()==DESCR_STREAM_ID)
			component_tag=((StreamIdentifierDescriptor*)*c)->component_tag;
		else if (c->Tag()==DESCR_LESRADIOS)
		{
			text=eString().sprintf("%d.) ", (((LesRadiosDescriptor*)*c)->id));
			text+=((LesRadiosDescriptor*)*c)->name;
		}
	}
	if (!text)
		text.sprintf("PID %04x", stream->elementary_PID);
	if (component_tag!=-1)
	{
		eServiceHandler *service=eServiceInterface::getInstance()->getService();
		if (service)
		{
			EIT *eit=service->getEIT();
			if (eit)
			{
				for (ePtrList<EITEvent>::iterator e(eit->events); e != eit->events.end(); ++e)
					if ((e->running_status>=2)||(!e->running_status))		// currently running service
						for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
							if (d->Tag()==DESCR_COMPONENT && ((ComponentDescriptor*)*d)->component_tag == component_tag)
									text=((ComponentDescriptor*)*d)->text;
	 			eit->unlock();
			}
		}
	}
	if (isAC3)
		text+=" (AC3)";
}

void eAudioSelector::selected(AudioStream *l)
{
	eServiceHandler *service=eServiceInterface::getInstance()->getService();
	if (l && service)
	{
		service->setPID(l->stream);
//		service->setDecoder();
	}

	close(0);
}

eAudioSelector::eAudioSelector()
	:eListBoxWindow<AudioStream>(_("Audio"), 10, 330)
{
	move(ePoint(100, 100));
	CONNECT(list.selected, eAudioSelector::selected);
}

void eAudioSelector::clear()
{
	list.clearList();
}

void eAudioSelector::add(PMTEntry *pmt)
{
	new AudioStream(&list, pmt);
}

SubService::SubService(eListBox<SubService> *listbox, const LinkageDescriptor *descr)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*) listbox),
		service(eServiceReference::idDVB, eTransportStreamID(descr->transport_stream_id), 
			eOriginalNetworkID(descr->original_network_id),
			eServiceID(descr->service_id), 1)
{
	text=(const char*)descr->private_data;
}

eSubServiceSelector::eSubServiceSelector()
	:eListBoxWindow<SubService>(_("Regie"), 10, 330)
{
	move(ePoint(100, 100));
	CONNECT(list.selected, eSubServiceSelector::selected);
}

void eSubServiceSelector::selected(SubService *ss)
{
	eServiceInterface::getInstance()->play(ss->service);
	close(0);
}

void eSubServiceSelector::clear()
{
	list.clearList();
}

void eSubServiceSelector::add(const LinkageDescriptor *ref)
{
	new SubService(&list, ref);
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

eZapMain::eZapMain(): eWidget(0, 1), pMsg(0), timeout(eApp), clocktimer(eApp)
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
	CONNECT(eEPGCache::getInstance()->EPGAvail, eZapMain::setEPGButton);

	CONNECT(eServiceInterface::getInstance()->serviceEvent, eZapMain::handleServiceEvent);

	CONNECT(timeout.timeout, eZapMain::timeOut);
	CONNECT(clocktimer.timeout, eZapMain::clockUpdate);

	CONNECT(eDVB::getInstance()->timeUpdated, eZapMain::clockUpdate);
	CONNECT(eAVSwitch::getInstance()->volumeChanged, eZapMain::updateVolume);

	actual_eventDisplay=0;

	clockUpdate();
	standbyTime=-1;
	
	addActionMap(&i_enigmaMainActions->map);
	
	gotPMT();
	gotSDT();
	gotEIT();
}

eZapMain::~eZapMain()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdShutdown->show();
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

				eZapLCD* pLCD = eZapLCD::getInstance();
				pLCD->lcdMain->updateProgress(cur_start,cur_duration);

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
		flags|=ENIGMA_SUBSERVICES;
	else
		flags&=~ENIGMA_SUBSERVICES;
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
	eService *service=eServiceInterface::getInstance()->lookupService(eServiceInterface::getInstance()->service);
	if (service)
		nvodsel.setText(service->service_name.c_str());
}

void eZapMain::showServiceSelector(int dir)
{
	hide();

	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();

	eServiceSelector *e = eZap::getInstance()->getServiceSelector();
	e->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	const eServiceReference *service = e->choose(dir);

	pLCD->lcdMain->show();
	pLCD->lcdMenu->hide();

	if (!service)
		return;
	
/*	if (*/eServiceInterface::getInstance()->play(*service);/*)
		startService(*service, -EAGAIN);*/
}

void eZapMain::nextService()
{
	const eServiceReference *service=eZap::getInstance()->getServiceSelector()->next();
	if (!service)
		return;

	/*if (*/eServiceInterface::getInstance()->play(*service); /*)
		startService(*service, -EAGAIN);*/
}

void eZapMain::prevService()
{
	const eServiceReference *service=eZap::getInstance()->getServiceSelector()->prev();
	if (!service)
		return;

/*	if (*/eServiceInterface::getInstance()->play(*service);/*)
		startService(*service, -EAGAIN);*/
}

void eZapMain::volumeUp()
{
	eAVSwitch::getInstance()->changeVolume(0, -4);
//		if (!isVisible())
//			show();
//		timeout.start(1000, 1);
}

void eZapMain::volumeDown()
{
	eAVSwitch::getInstance()->changeVolume(0, +4);
//		if (!isVisible())
//			show();
//		timeout.start(1000, 1);
}

void eZapMain::toggleMute()
{
	eAVSwitch::getInstance()->toggleMute();
}

void eZapMain::showMainMenu()
{
	if (isVisible())
		hide();

	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();

	eMainMenu mm;
	mm.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	if (mm.exec() == 1)
		eZap::getInstance()->quit();

	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
}

void eZapMain::standbyPress()
{
	standbyTime = time(0);
}

void eZapMain::standbyRepeat()
{
	if (standbyTime == -1)		// just waking up
		return;
	int diff = time(0) - standbyTime;
	if (diff > 2)
		standbyRelease();
}

void eZapMain::standbyRelease()
{
	if (standbyTime == -1)		// just waking up
		return;
	int diff = time(0) - standbyTime;
	standbyTime=-1;
	if (diff > 2)
		eZap::getInstance()->quit();
	else
	{
		eZapStandby standby;
		if (isVisible())
			hide();
		standby.show();
		standby.exec();
		standby.hide();
	}
}

void eZapMain::showInfobar()
{
	timeout.start(10000, 1);
	show();
}

void eZapMain::hideInfobar()
{
	timeout.stop();
	hide();
}

void eZapMain::showSubserviceMenu()
{
	if (!(flags & (ENIGMA_NVOD|ENIGMA_SUBSERVICES)))
		return;

	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
	if (flags&ENIGMA_NVOD)
	{
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		nvodsel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		nvodsel.show();
		nvodsel.exec();
		nvodsel.hide();
	}
	else if (flags&ENIGMA_SUBSERVICES)
	{
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		subservicesel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		subservicesel.show();
		subservicesel.exec();
		subservicesel.hide();
	}
	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
}

void eZapMain::showAudioMenu()
{
	if (flags&ENIGMA_AUDIO)
	{
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		audiosel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		audiosel.show();
		audiosel.exec();
		audiosel.hide();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
	}
}

void eZapMain::runVTXT()
{
	if (isVT)
	{
		eZapPlugins plugins;
		plugins.execPluginByName("tuxtxt.cfg");
	}
}

void eZapMain::showEPGList()
{
#if 1
	if (isEPG)
	{
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		eEPGSelector wnd(eServiceInterface::getInstance()->service);
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
#else
	VCR bla;
	hide();
	bla.show();
	bla.exec();
	bla.hide();
	show();
#endif
}

void eZapMain::showEPG()
{
	eService* service = eDVB::getInstance()->settings->getTransponders()->searchService(eServiceInterface::getInstance()->service);

	if (!service)
		return;
		
	if (isVisible())
	{
		timeout.stop();
		hide();
	}

#ifdef USE_CACHED_EPG
#error no
	const eventMap* pMap = eEPGCache::getInstance()->getEventMap(service->original_network_id, service->service_id);

	if (pMap && isEPG)  // EPG vorhanden
	{
		eventMap::const_iterator It = pMap->begin();
			
		ePtrList<EITEvent> events;
		events.setAutoDelete(true);
			
		while (It != pMap->end())  // sicher ist sicher !
		{
			events.push_back( new EITEvent(*It->second));
			It++;
		}
		eEventDisplay ei(service->service_name.c_str(), &events);			
		actual_eventDisplay=&ei;
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		ei.show();
		ei.exec();
		ei.hide();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
		actual_eventDisplay=0;
	} else	
#endif
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		ePtrList<EITEvent> dummy;
		{
			eEventDisplay ei(service->service_name.c_str(), eit?&eit->events:&dummy);
			if (eit)
				eit->unlock();		// HIER liegt der hund begraben.
			actual_eventDisplay=&ei;
			eZapLCD* pLCD = eZapLCD::getInstance();
			pLCD->lcdMain->hide();
			pLCD->lcdMenu->show();
			ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
			ei.show();
			ei.exec();
			ei.hide();
			pLCD->lcdMenu->hide();
			pLCD->lcdMain->show();
			actual_eventDisplay=0;
		}
	}
}

int eZapMain::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (pMsg)
		{
			pMsg->hide();
			delete pMsg;
			pMsg=0;
		}
		if (event.action == &i_enigmaMainActions->showMainMenu)
			showMainMenu();
		else if (event.action == &i_enigmaMainActions->standby_press)
			standbyPress();
		else if (event.action == &i_enigmaMainActions->standby_repeat)
			standbyRepeat();
		else if (event.action == &i_enigmaMainActions->standby_release)
			standbyRelease();
		else if ((!isVisible()) && (event.action == &i_enigmaMainActions->toggleInfobar))
			showInfobar();
		else if (isVisible() && (event.action == &i_enigmaMainActions->toggleInfobar))
			hideInfobar();
		else if (event.action == &i_enigmaMainActions->showServiceSelector)
			showServiceSelector(-1);
		else if (event.action == &i_enigmaMainActions->showSubservices)
			showSubserviceMenu();
		else if (event.action == &i_enigmaMainActions->showAudio)
			showAudioMenu();
		else if (event.action == &i_enigmaMainActions->pluginVTXT)
			runVTXT();
		else if (event.action == &i_enigmaMainActions->showEPGList)
			showEPGList();
		else if (event.action == &i_enigmaMainActions->showEPG)
			showEPG();
		else if (event.action == &i_enigmaMainActions->nextService)
			nextService();
		else if (event.action == &i_enigmaMainActions->prevService)
			prevService();
		else if (event.action == &i_enigmaMainActions->serviceListDown)
			showServiceSelector(eServiceSelector::dirDown);
		else if (event.action == &i_enigmaMainActions->serviceListUp)
			showServiceSelector(eServiceSelector::dirUp);
		else if (event.action == &i_enigmaMainActions->volumeUp)
			volumeUp();
		else if (event.action == &i_enigmaMainActions->volumeDown) 
			volumeDown();
		else if (event.action == &i_enigmaMainActions->toggleMute)
			toggleMute();
		else 
			break;
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void eZapMain::handleServiceEvent(const eServiceEvent &event)
{
	switch (event.type)
	{
	case eServiceEvent::evtStateChanged:
		break;
	case eServiceEvent::evtFlagsChanged:
	{
		int fl = eServiceInterface::getInstance()->getService()->getFlags();
		setSmartcardLogo( fl & eServiceHandler::flagIsScrambled );
		break;
	}
	case eServiceEvent::evtAspectChanged:
	{
		int aspect = eServiceInterface::getInstance()->getService()->getAspectRatio();
		set16_9Logo(aspect);
		break;
	}
	case eServiceEvent::evtStart:
	{
		int err = eServiceInterface::getInstance()->getService()->getErrorInfo();
		startService(eServiceInterface::getInstance()->service, err);
		break;
	}
	case eServiceEvent::evtStop:
		leaveService();
		break;
	case eServiceEvent::evtGotEIT:
		gotEIT();
		break;
	case eServiceEvent::evtGotSDT:
		gotSDT();
		break;
	case eServiceEvent::evtGotPMT:
		gotPMT();
		break;
	}
}

void eZapMain::startService(const eServiceReference &serviceref, int err)
{
	eDebug("---------------START SERVICE--------------------");
	isVT = Decoder::parms.tpid != -1;

	setVTButton(isVT);

	eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(serviceref);

		// es wird nur dann versucht einen service als referenz-service zu uebernehmen, wenns den ueberhaupt
		// gibt.
	if (service)
		switch (serviceref.service_type)
		{
		case 1:	// TV
		case 2: // radio
		case 4: // nvod ref
			refservice=serviceref;
			break;
		}

	eService *rservice=0;
	if (refservice != serviceref)
		rservice=eDVB::getInstance()->settings->getTransponders()->searchService(refservice);
	
	if (refservice.service_type==4)
		flags|=ENIGMA_NVOD;
	else
		flags&=~ENIGMA_NVOD;

	eString name="";

	if (rservice)
		name=rservice->service_name + " - ";
	
	if (service)
		name+=service->service_name;
	else
		switch (serviceref.service_type)
		{
		case 5: // nvod stream
			name+="NVOD Stream";
		}

	if (!name.length())
		name="unknown service";

	ChannelName->setText(name);		

	if (pMsg)
	{
		if (pMsg->isVisible())
			pMsg->hide();

		delete pMsg;
		pMsg = 0;
	}

	switch (err)
	{
	case 0:
		Description->setText(_(""));
		break;
	case -EAGAIN:
		Description->setText(_("Einen Moment bitte..."));
		pMsg = new eMessageBox( _("One moment please..."), _("Service Switch"), true);
		break;
	case -ENOENT:
		Description->setText(_("Sender konnte nicht gefunden werden."));
		pMsg = new eMessageBox( _("Service could not be found !"), _("Service Switch"), true );
		eDebug("Sender konnte nicht gefunden werden.");
		break;
	case -ENOCASYS:
		Description->setText(_("Dieser Sender kann nicht entschl�sselt werden."));
		pMsg = new eMessageBox( _("This service could not be descrambled"), _("Service Switch"), true );
		eDebug("Dieser Sender kann nicht entschl�sselt werden.");
		break;
	case -ENOSTREAM:
		Description->setText(_("Dieser Sender sendet (momentan) kein Signal."));
		pMsg = new eMessageBox( _("This service sends (currently) no signal"), _("Service Switch"), true );
		eDebug("Dieser Sender sendet (momentan) kein Signal.");
		break;
	case -ENOSYS:
		Description->setText(_("Dieser Inhalt kann nicht dargestellt werden."));
		pMsg = new eMessageBox( _("This content could not be displayed"), _("Service Switch"), true );
		eDebug("Dieser Inhalt kann nicht dargestellt werden.");
		break;
	case -ENVOD:
		Description->setText(_("NVOD - Bitte Anfangszeit bestimmen!"));
		pMsg = new eMessageBox( _("NVOD Service - select a starttime, please"), _("Service Switch"), true );
		eDebug("NVOD - Bitte Anfangszeit bestimmen!");
		break;
	default:
		Description->setText(_("<unbekannter Fehler>"));
		pMsg = new eMessageBox( _("Unknown error !!"), _("Service Switch"), true );
		eDebug("<unbekannter Fehler>");
		break;
	}

	if (pMsg)
		pMsg->show();

	int num=-1;
	
	if (rservice)
		num=rservice->service_number;
	else if (service)
		num=service->service_number;

	if (num != -1)
		ChannelNumber->setText(eString().sprintf("%d", num));
	else
		ChannelNumber->setText("");
	
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
	eAVSwitch::getInstance()->changeVolume(0, 0);

	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;

	timeout.start((sapi->getState() == eServiceHandler::statePlaying)?10000:2000, 1);
}

void eZapMain::gotEIT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;

	EIT *eit=sapi->getEIT();
	setEIT(eit);

	if (eit)
	{
		int state=0;
		eConfig::getInstance()->getKey("/ezap/osd/showOSDOnEITUpdate", state);

		if (!eZap::getInstance()->focus && state)
		{
			show();
			timeout.start((sapi->getState() == eServiceHandler::statePlaying)?10000:2000, 1);
		}
		eit->unlock();
	}
}

void eZapMain::gotSDT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;
		
	SDT *sdt=sapi->getSDT();
	if (!sdt)
		return;

	switch (eServiceInterface::getInstance()->service.service_type)
	{
	case 0x4:	// nvod reference
	{
		for (ePtrList<SDTEntry>::iterator i(sdt->entries); i != sdt->entries.end(); ++i)
		{
			SDTEntry *entry=*i;
			if (eServiceID(entry->service_id)==eServiceInterface::getInstance()->service.service_id)
				handleNVODService(entry);
		}
		break;
	}
	}
	sdt->unlock();
}

void eZapMain::gotPMT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;
		
	PMT *pmt=sapi->getPMT();
	if (!pmt)
		return;

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
	else
		flags&=~ENIGMA_AUDIO;
		
	setAC3Logo(isAc3);
	
	pmt->unlock();
}

void eZapMain::timeOut()
{
	if (eZap::getInstance()->focus==this)
		hide();
}

void eZapMain::leaveService()
{
	eDebug("leaving service");

//	flags=0;
	
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
	if (t && eDVB::getInstance()->time_difference)
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
	VolumeBar->setPerc((63-vol)*100/63);
}

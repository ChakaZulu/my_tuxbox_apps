#include <time.h>
#include <scan.h>
#include <enigma.h>

#include <lib/dvb/frontend.h>
#include <lib/dvb/si.h>
#include <lib/dvb/dvb.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ewindow.h>
#include <lib/gdi/font.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/emessage.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/decoder.h>
#include <lib/driver/rc.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/combobox.h>
#include <lib/gui/guiactions.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/dvbscan.h>
#include <lib/dvb/dvbservice.h>
#include <lib/system/info.h>

#include <string>

tsSelectType::tsSelectType(eWidget *parent)
	:eWidget(parent,1)
{
	list=new eListBox<eListBoxEntryText>(this);
	list->setName("menu");
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsSelectType"))
		eFatal("skin load of \"tsSelectType\" failed");

	list->setFlags(eListBox<eListBoxEntryText>::flagShowEntryHelp);
	new eListBoxEntryText(list, _("auto scan"), (void*)2, 0, _("open automatic transponder scan") );
	new eListBoxEntryText(list, _("manual scan.."), (void*)1, 0, _("open manual transponder scan") );

	CONNECT(list->selected, tsSelectType::selected);
}

void tsSelectType::selected(eListBoxEntryText *entry)
{
	if (entry && entry->getKey())
		close((int)entry->getKey());
	else
		close((int)TransponderScan::stateEnd);
}

int tsSelectType::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		setFocus(list);
		break;
	case eWidgetEvent::childChangedHelpText:
		return parent->eventHandler( event );
	default:
		break;
	}
	return 0;
}

tsManual::tsManual(eWidget *parent, const eTransponder &transponder, eWidget *LCDTitle, eWidget *LCDElement)
:eWidget(parent), transponder(transponder), updateTimer(eApp)
{
#ifndef DISABLE_LCD
	setLCD(LCDTitle, LCDElement);
#endif
	int ft=0;
	switch (eSystemInfo::getInstance()->getFEType())
	{
	case eSystemInfo::feSatellite:
		ft=eTransponderWidget::deliverySatellite;
		break;
	case eSystemInfo::feCable:
		ft=eTransponderWidget::deliveryCable;
		break;
	default:
		ft=eTransponderWidget::deliverySatellite;
		break;
	}

	transponder_widget=new eTransponderWidget(this, 1, ft);
	transponder_widget->setName("transponder");

	festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");

	c_useonit=new eCheckbox(this);
	c_useonit->setName("useonit");
	
	c_usebat=new eCheckbox(this);
	c_usebat->setName("usebat");
	
	c_clearlist=new eCheckbox(this);
	c_clearlist->setName("clearlist");

	c_searchnit=new eCheckbox(this);
	c_searchnit->setName("searchnit");

	b_start=new eButton(this);
	b_start->setName("start");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsManual"))
		eFatal("skin load of \"tsManual\" failed");

	transponder_widget->load();
	transponder_widget->setTransponder(&transponder);

	CONNECT(b_start->selected, tsManual::start);
	CONNECT(transponder_widget->updated, tsManual::retune);
//	CONNECT(updateTimer.timeout, tsManual::update );
	setHelpID(62);
}

void tsManual::update()
{
	int status=eFrontend::getInstance()->Status();
	if (!(status & FE_HAS_LOCK))
	{
		if (!transponder_widget->getTransponder(&transponder))
			transponder.tune();
	}
}

void tsManual::start()
{
	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	if (!sapi)
	{	
		eWarning("no scan active");
		close(1);
	} else
	{
		sapi->addTransponder(transponder);
		sapi->setUseONIT(c_useonit->isChecked());
		sapi->setUseBAT(c_usebat->isChecked());
		sapi->setNetworkSearch(c_searchnit->isChecked());
		sapi->setClearList(c_clearlist->isChecked());
    sapi->setSkipOtherOrbitalPositions(1);
		close(0);
	}
}

void tsManual::retune()
{
	if (!transponder_widget->getTransponder(&transponder))
		transponder.tune();
}

int tsManual::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		updateTimer.start(1000);
		break;
	case eWidgetEvent::execDone:
		updateTimer.stop();
	default:
		break;
	}
	return 0;
}

tsAutomatic::tsAutomatic(eWidget *parent)
	:eWidget(parent)
{
	eLabel* l = new eLabel(this);
	l->setName("lNet");
	l_network=new eComboBox(this, 3, l);
	l_network->setName("network");

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");
	
	l_status=new eLabel(this, RS_WRAP);
	l_status->setName("status");

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
	{
		int snocircular=0;
		eConfig::getInstance()->getKey("/elitedvb/DVB/config/nocircular",snocircular);
		c_nocircular=new eCheckbox(this,snocircular);
		c_nocircular->setName("nocircular");
		c_nocircular->hide();
	}
	else
		c_nocircular=0;

	b_start=new eButton(this);
	b_start->setName("start");
	b_start->hide();

	eSkin *skin=eSkin::getActive();

	eString tmp = "tsAutomatic";

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		tmp+="_sat";
	else
		tmp+="_cable";

	if (skin->build(this, tmp.c_str()))
		eFatal("skin load of \"%s\" failed", tmp.c_str());

	eDebug("build %s", tmp.c_str() );
//	l_network->setCurrent(new eListBoxEntryText(*l_network, _("automatic"), (void*)0, eTextPara::dirCenter) );

	CONNECT(b_start->selected, tsAutomatic::start);
	CONNECT(l_network->selchanged, tsAutomatic::networkSelected);

	CONNECT(eDVB::getInstance()->eventOccured, tsAutomatic::dvbEvent);
	
	if (loadNetworks())
		eFatal("loading networks failed");

	l_network->setCurrent( 0 );	

	switch (eSystemInfo::getInstance()->getFEType())
	{
		case eSystemInfo::feSatellite:
			l_status->setText(_("To begin searching for a valid satellite press OK, or choose your desired satellite manually and press OK"));
		break;
		case eSystemInfo::feCable:
			l_status->setText(_("To begin searching for a valid cable provider press OK, or choose your desired cable provider manually and press OK"));
		break;
	}
	
	setFocus(l_network);
	setHelpID(61);
}

void tsAutomatic::start()
{
	int snocircular = c_nocircular ? c_nocircular->isChecked() : 0;
	eConfig::getInstance()->setKey("/elitedvb/DVB/config/nocircular",snocircular);    

	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	if (!sapi)
	{	
		eWarning("no scan active");
		close(1);
	} else
	{
		tpPacket *pkt=(tpPacket*)(l_network->getCurrent() -> getKey());
		for (std::list<eTransponder>::iterator i(pkt->possibleTransponders.begin()); i != pkt->possibleTransponders.end(); ++i)
		{
			if(snocircular)
				i->satellite.polarisation&=1;   // CEDR
			sapi->addTransponder(*i);
		}

		// scanflags auswerten
		sapi->setSkipKnownNIT(pkt->scanflags & 8);
		sapi->setUseONIT(pkt->scanflags & 4);
		sapi->setUseBAT(pkt->scanflags & 2);
		sapi->setNetworkSearch(pkt->scanflags & 1);

		// macht nur Probleme...bzw dauert recht lang...
		sapi->setSkipOtherOrbitalPositions(1);
		sapi->setClearList(1);
		sapi->setNoCircularPolarization(snocircular);

		close(0);
	}
}

void tsAutomatic::networkSelected(eListBoxEntryText *l)
{
	if (nextNetwork(-1)) // if "automatic" selected,
	{
		automatic=1;
		nextNetwork();  // begin with first
	} else
		automatic=0;

	tuneNext(0);
}

void tsAutomatic::dvbEvent(const eDVBEvent &event)
{
	switch (event.type)
	{
	case eDVBEvent::eventTunedIn:
		eDebug("eventTunedIn");
		if (event.err)
		{
			b_start->hide();
			tuneNext(1);
		} else
		{
			if ( c_nocircular )
				c_nocircular->show();
			b_start->show();
			setFocus(c_nocircular);
			l_status->setText(_("A valid transponder has been found. Verify that the right network is selected"));
		}
		break;
	default:
		break;
	}
}

int tsAutomatic::loadNetworks()
{
	int err;

	if(	(err = eTransponderList::getInstance()->reloadNetworks()) )
		return err;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			for ( std::list<tpPacket>::const_iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
				if ( ( i->orbital_position == s->getOrbitalPosition() ) || (eSystemInfo::getInstance()->getFEType() == eSystemInfo::feCable) )
					new eListBoxEntryText(*l_network, i->name, (void*)&*i, eTextPara::dirCenter);

	return 0;
}

int tsAutomatic::nextNetwork(int first)
{
	eDebug("next network");

	if (first != -1)
		l_network->moveSelection(first ? eListBox<eListBoxEntryText>::dirFirst : eListBox<eListBoxEntryText>::dirDown);
		
	tpPacket *pkt=(tpPacket*)(l_network->getCurrent() -> getKey());
	
	eDebug("pkt: %p", pkt);

	if (!pkt)
		return -1;

	current_tp = pkt->possibleTransponders.begin();
	last_tp = pkt->possibleTransponders.end();
	return 0;
}

int tsAutomatic::nextTransponder(int next)
{
	if (next)
	{
		current_tp->state=eTransponder::stateError;
		++current_tp;
	}

	if (current_tp == last_tp)
		return 1;

	if ( c_nocircular && c_nocircular->isChecked() )
		current_tp->satellite.polarisation&=1;   // CEDR

	return current_tp->tune();
}

int tsAutomatic::tuneNext(int next)
{
	while (nextTransponder(next))
	{
		if (automatic)
		{
			if (nextNetwork())	// wrapped around?
			{
				l_status->setText(_("All known transponders have been tried,"
					" but no lock was possible. Verify antenna-/cable-setup or try manual search "
					"if its some obscure satellite/network."));
				return -1;
			}
		}
		else
		{
			l_status->setText(_("All known transponders have been tried,"
				" but no lock was possible. Verify antenna-/cable-setup or try another satellite/network."));
			return -1;
		}
		next=0;
	}

	static int i=0;
	i++;
	std::string progress=_("Search in progress ");
	progress+="\\|/-"[i&3];
	l_status->setText(progress);

	return 0;
}

tsText::tsText(eString sheadline, eString sbody, eWidget *parent)
	:eWidget(parent, 1)
{
	addActionMap(&i_cursorActions->map);
	headline=new eLabel(this);
	headline->setText(sheadline);
	headline->setFont(eSkin::getActive()->queryFont("head"));
	body=new eLabel(this, RS_WRAP);
	body->setText(sbody);
}

int tsText::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		headline->move(ePoint(0, 0));
		headline->resize(eSize(size.width(), 40));
		body->move(ePoint(0, 40));
		body->resize(eSize(size.width(), size.height()-40));
		return 1;
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->ok)
			close(0);
		else if (event.action == &i_cursorActions->cancel)
			close(2);
		else
			break;
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

tsScan::tsScan(eWidget *parent)
	:eWidget(parent, 1), timer(eApp)
{
	addActionMap(&i_cursorActions->map);

	services_scanned = new eLabel(this);
	services_scanned->setName("services_scanned");

	transponder_scanned = new eLabel (this);
	transponder_scanned->setName("transponder_scanned");

	timeleft = new eLabel(this);
	timeleft->setName("time_left");

	service_name = new eLabel(this);
	service_name->setName("service_name");

	service_provider = new eLabel(this);
	service_provider->setName("service_provider");

	progress = new eProgress(this);
	progress->setName("scan_progress");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsScan"))
		eFatal("skin load of \"tsScan\" failed");
	
	CONNECT(eDVB::getInstance()->eventOccured, tsScan::dvbEvent);
	CONNECT(eDVB::getInstance()->stateChanged, tsScan::dvbState);
	CONNECT(timer.timeout, tsScan::updateTime);
	CONNECT(eDVB::getInstance()->settings->getTransponders()->service_found, tsScan::serviceFound);
	CONNECT(eDVB::getInstance()->settings->getTransponders()->transponder_added, tsScan::addedTransponder);
}

int tsScan::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
//		headline->move(ePoint(0, 0));
//		headline->resize(eSize(size.width(), 40));
		return 1;
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->cancel)
			close(2);
		else
			break;
		return 1;
	case eWidgetEvent::execBegin:
	{
		scantime=0;
		eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
		if (!sapi)
		{	
			eWarning("no scan active");
			close(1);
		} else
			sapi->start();
		break;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void tsScan::updateTime()
{
		scantime++;
		int sek = (int) (( (double) scantime / tpScanned) * tpLeft);
		if (sek > 59)
			timeleft->setText(eString().sprintf(_("%02i minutes and %02i seconds left"), sek / 60, sek % 60));
		else
			timeleft->setText(eString().sprintf(_("%02i seconds left"), sek ));
}

void tsScan::serviceFound(const eServiceReferenceDVB &service, bool newService)
{
	servicesScanned++;
	
	services_scanned->setText(eString().sprintf("%i", servicesScanned));

	eServiceDVB *s=eDVB::getInstance()->settings->getTransponders()->searchService(service);
	service_name->setText(s->service_name);
	service_provider->setText(s->service_provider);
	
	if (newService)
	switch(s->service_type)
	{
		case 4:	// NVOD reference service
		case 1:	// digital television service
			newTVServices++;
		break;

		case 2:	// digital radio service
			newRadioServices++;
		break;

		case 3:	// teletext service
		break;

		case 5:	// NVOD time shifted service
		break;

		case 6:	// mosaic service
		break;

		default: // data
			newDataServices++;
		break;
	}
}

void tsScan::addedTransponder( eTransponder* )
{
	newTransponders++;
	// hier landen wir jedesmal, wenn ein NEUER Transponder gefunden wurde...
}

void tsScan::dvbEvent(const eDVBEvent &event)
{
	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	
	int perc;

	switch (event.type)
	{
	case eDVBScanEvent::eventScanBegin:
			tpLeft = sapi->getknownTransponderSize();
			progress->setPerc(0);
			timer.start(1000);
			tpScanned = newTVServices = newRadioServices = newDataServices = servicesScanned = newTransponders = 0;
		break;
	case eDVBScanEvent::eventScanTPadded:
			tpLeft++;
			perc=(int) ( ( 100.00 / (tpLeft+tpScanned) ) * tpScanned );
			progress->setPerc(perc);
		break;
	case eDVBScanEvent::eventScanNext:
			tpLeft--;
			tpScanned++;
			transponder_scanned->setText(eString().sprintf("%i", tpScanned));
			perc=(int) ( ( 100.00 / (tpLeft+tpScanned) ) * tpScanned );
			progress->setPerc(perc);
		break;
	case eDVBScanEvent::eventScanCompleted:
			timer.stop();
			close(0);
		break;
	default:
		break;
	}
}

void tsScan::dvbState(const eDVBState &state)
{
}

TransponderScan::TransponderScan( eWidget *LCDTitle, eWidget *LCDElement)
#ifndef DISABLE_LCD
	:eWindow(0), current(0), LCDElement(LCDElement), LCDTitle(LCDTitle)
#endif
{
	addActionMap(&i_cursorActions->map);
	setText(_("Transponder Scan"));
	cmove(ePoint(130, 110));
	cresize(eSize(460, 400));

	statusbar=new eStatusBar(this);
	statusbar->loadDeco();
	statusbar->move(ePoint(0, getClientSize().height()-30) );
	statusbar->resize( eSize( getClientSize().width(), 30 ) );
}

TransponderScan::~TransponderScan()
{
}

void showScanPic()
{
	FILE *f = fopen(CONFIGDIR "/enigma/pictures/scan.mvi", "r");
	if ( f )
	{
		fclose(f);
		Decoder::displayIFrameFromFile(CONFIGDIR "/enigma/pictures/scan.mvi" );
	}
	else
		Decoder::displayIFrameFromFile(DATADIR "/enigma/pictures/scan.mvi" );
}

int TransponderScan::exec(tState initial)
{
	tState state=initial;

	eSize size=getClientSize()-eSize(0,30);
	int scanok=0;

	eString text;

	show();

	eTransponder oldTp(*eDVB::getInstance()->settings->getTransponders());

	while (state != stateEnd)
	{
		showScanPic();

		switch (state)
		{
		case stateMenu:
		{
			tsSelectType select(this);
#ifndef DISABLE_LCD
			select.setLCD( LCDTitle, LCDElement);
#endif
			current = &select;
			select.show();
			state = (tState) select.exec();
			current=0;
			select.hide();
			break;
		}
		case stateManual:
		{
			eTransponder transponder(*eDVB::getInstance()->settings->getTransponders());
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

			if ( oldTp.isValid() )
				transponder=oldTp;
			else if (sapi && sapi->transponder)
				transponder=*sapi->transponder;
			else
				switch (eSystemInfo::getInstance()->getFEType())
				{
				case eSystemInfo::feCable:
					transponder.setCable(402000, 6900000, 0, 3);	// some cable transponder
					break;
				case eSystemInfo::feSatellite:
					transponder.setSatellite(12551500, 22000000, eFrontend::polVert, 4, 0, 0);	// some astra transponder
					break;
				default:
					break;
				}

			eDVB::getInstance()->setMode(eDVB::controllerScan);        

			showScanPic();
#ifndef DISABLE_LCD
			tsManual manual_scan(this, transponder, LCDTitle, LCDElement);
#else
			tsManual manual_scan(this, transponder);
#endif
			manual_scan.show();
			current = &manual_scan;
			switch (manual_scan.exec())
			{
			case 0:
				state=stateScan;
				break;
			case 1:
				if ( initial == stateMenu )
					state=stateMenu;
				else
					state=stateEnd;
				break;
			}
			manual_scan.hide();
			current=0;
			oldTp=manual_scan.getTransponder();
			break;
		}
		case stateAutomatic:
		{
			eDVB::getInstance()->setMode(eDVB::controllerScan);

			showScanPic();

			tsAutomatic automatic_scan(this);
#ifndef DISABLE_LCD
			automatic_scan.setLCD( LCDTitle, LCDElement);
#endif
			automatic_scan.show();
			current = &automatic_scan;
			switch (automatic_scan.exec())
			{
			case 0:
				state=stateScan;
				break;
			case 1:
				if ( initial == stateMenu )
					state=stateMenu;
				else
					state=stateEnd;
				break;
			}
			automatic_scan.hide();
			current=0;
			break;
		}
		case stateScan:
		{
			tsScan scan(this);
#ifndef DISABLE_LCD
			scan.setLCD( LCDTitle, LCDElement);
#endif
			scan.move(ePoint(0, 0));
			scan.resize(size);
			
			scan.show();
			statusbar->setText(_("Scan is in progress... please wait"));
			scan.exec();
			scan.hide();

			text.sprintf(_("The transponder scan has finished and found \n   %i new Transponders,\n   %i new TV Services,\n   %i new Radio Services and\n   %i new Data Services.\n%i Transponders within %i Services scanned."), scan.newTransponders, scan.newTVServices, scan.newRadioServices, scan.newDataServices, scan.tpScanned, scan.servicesScanned );
			scanok=1;
			
			state=stateDone;
			eDVB::getInstance()->setMode(eDVB::controllerService);
			break;
		}
		case stateDone:
		{
			tsText finish(_("Done."), text, this);
#ifndef DISABLE_LCD
			finish.setLCD( LCDTitle, LCDElement);
#endif
			finish.move(ePoint(0, 0));
			finish.resize(size);
			finish.show();
			statusbar->setText(_("Scan is in finished, press ok to close window"));
			finish.exec();
			finish.hide();
			if ( initial == stateManual || 
				eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
			{
				eMessageBox mb(eString().sprintf(_("Do you want\nto scan another\n%s?"),initial==stateAutomatic?_("Satellite"):_("Transponder")),
					_("Scan finished"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
					eMessageBox::btYes );
				mb.show();
				switch ( mb.exec() )
				{
					case -1:
					case eMessageBox::btNo:
						state=stateEnd;
						break;
					default:
						state=initial;
				}
				mb.hide();
				break;
			}
		}
		default:
			state=stateEnd;
			break;
		}
	}
	eDVB::getInstance()->setMode(eDVB::controllerService);  
	hide();

	Decoder::Flush();

	return scanok;
}

int TransponderScan::eventHandler( const eWidgetEvent &event )
{
	switch (event.type)
	{
		case eWidgetEvent::childChangedHelpText:
			if (focus)
				statusbar->setText(focus->getHelpText());
			break;
		case eWidgetEvent::evtAction:
		{
			if ( event.action == &i_cursorActions->cancel && current )  // don't ask !
			{
				if ( focus && focus != this && focus->eventHandler(event) )
					;
				else if ( current && focus != this )
					current->close(1);
			}
			else
				break;
			return 1;
		}
		default:
			break;
	}
	return eWindow::eventHandler( event );
}

#include <rotorconfig.h>

#include <lib/base/i18n.h>
#include <lib/system/init_num.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/actions.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/edvb.h>

#include <tuxbox.h>

RotorConfig::RotorConfig(eLNB *lnb )
	:lnb(lnb)
{
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;

	useRotorInPower = new eCheckbox(this);
	useRotorInPower->setName("useRotorInPower");

	lDegPerSec = new eLabel(this);
	lDegPerSec->setName("lDegPerSec");
	lDegPerSec->hide();
	DegPerSec = new eNumber( this, 2, 0, 10, 3, 0, 0, lDegPerSec);
	DegPerSec->setFlags( eNumber::flagFixedNum );
	DegPerSec->setName("DegPerSec");
	DegPerSec->hide();

	lDeltaA = new eLabel(this);
	lDeltaA->setName("lDeltaA");
	lDeltaA->hide();
	DeltaA = new eNumber( this, 1, 0, 200, 3, 0, 0, lDeltaA);
	DeltaA->setName("DeltaA");
	DeltaA->hide();

	useGotoXX = new eCheckbox(this);
	useGotoXX->setName("useGotoXX");

	lLongitude = new eLabel(this);
	lLongitude->setName("lLongitude");
	lLongitude->hide();

	Longitude = new eNumber(this, 2, 0, 360, 3, 0, 0, lLongitude );
	Longitude->setFlags( eNumber::flagFixedNum );
	Longitude->setName("Longitude");
	Longitude->hide();

	LoDirection = new eComboBox( this, 2 );
	LoDirection->setName("LoDirection");
	LoDirection->hide();
	new eListBoxEntryText( *LoDirection, _("East"), (void*)eDiSEqC::EAST, 0, _("East") );
	new eListBoxEntryText( *LoDirection, _("West"), (void*)eDiSEqC::WEST, 0, _("West") );

	lLatitude = new eLabel(this);
	lLatitude->setName("lLatitude");
	lLatitude->hide();

	Latitude = new eNumber(this, 2, 0, 360, 3, 0, 0, lLatitude );
	Latitude->setFlags( eNumber::flagFixedNum );
	Latitude->setName("Latitude");
	Latitude->hide();

	LaDirection = new eComboBox( this, 2 );
	LaDirection->setName("LaDirection");
	LaDirection->hide();
	new eListBoxEntryText( *LaDirection, _("North"), (void*)eDiSEqC::NORTH, 0, _("North") );
	new eListBoxEntryText( *LaDirection, _("South"), (void*)eDiSEqC::SOUTH, 0, _("South") );

	positions = new eListBox< eListBoxEntryText >( this );
	positions->setFlags( eListBoxBase::flagNoPageMovement );
	positions->setName("positions");
	positions->hide();

	lStoredRotorNo = new eLabel(this);
	lStoredRotorNo->setName("lStoredRotorNo");
	lStoredRotorNo->hide();
	number = new eNumber( this, 1, 0, 255, 3, 0, 0, lStoredRotorNo);
	number->setName("StoredRotorNo");
	number->hide();

	lOrbitalPosition = new eLabel(this);
	lOrbitalPosition->setName("lOrbitalPosition");
	lOrbitalPosition->hide();
	orbital_position = new eNumber( this, 1, 0, 3600, 4, 0, 0, lOrbitalPosition);

	orbital_position->setName("OrbitalPosition");
	orbital_position->hide();

	lDirection = new eLabel(this);
	lDirection->setName("lDirection");
	lDirection->hide();
	direction = new eComboBox( this, 2, lDirection );
	direction->setName("Direction");
	direction->hide();
	new eListBoxEntryText( *direction, _("East"), (void*)0, 0, _("East") );
	new eListBoxEntryText( *direction, _("West"), (void*)1, 0, _("West") );

	add = new eButton( this );
	add->setName("add");
	add->hide();

	remove = new eButton ( this );
	remove->setName("remove");
	remove->hide();

	save = new eButton(this);
	save->setName("save");

	cancel = new eButton(this);
	cancel->setName("cancel");

	next = new eButton(this);
	next->setName("next");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "RotorConfig"))
		eFatal("skin load of \"RotorConfig\" failed");

	CONNECT( orbital_position->selected, RotorConfig::numSelected );
	CONNECT( Longitude->selected, RotorConfig::numSelected );
	CONNECT( Latitude->selected, RotorConfig::numSelected );
	CONNECT( number->selected, RotorConfig::numSelected );
	CONNECT( DegPerSec->selected, RotorConfig::numSelected );
	CONNECT( DeltaA->selected, RotorConfig::numSelected );
	CONNECT( add->selected, RotorConfig::onAdd );
	CONNECT( remove->selected, RotorConfig::onRemove );
	CONNECT( positions->selchanged, RotorConfig::posChanged );
	CONNECT( useGotoXX->checked, RotorConfig::gotoXXChanged );
	CONNECT( useRotorInPower->checked, RotorConfig::useRotorInPowerChanged );	

	CONNECT( save->selected, RotorConfig::onSavePressed );
	CONNECT( cancel->selected, RotorConfig::reject );
	CONNECT( next->selected, RotorConfig::onNextPressed );

	addActionMap(&i_focusActions->map);

	if (lnb)
		setLNBData(lnb);

	if (tuxbox_get_model() == TUXBOX_MODEL_DBOX2)
	{
		eDebug("useRotorInputPower can only used on dreambox");
		useRotorInPower->hide();
	}
}

struct savePosition: public std::unary_function< eListBoxEntryText&, void>
{
	std::map<int,int> &map;

	savePosition(std::map<int,int> &map): map(map)
	{
	}

	bool operator()(eListBoxEntryText& s)
	{
		if ( (int)s.getKey() == 0xFFFF )
			return 0; // ignore sample Entry... delete me...

		int num = atoi( s.getText().left( s.getText().find('/') ).c_str() );
		map[ (int)s.getKey() ] = num;
		return 0;
	}
};

void RotorConfig::onSavePressed()
{
	lnb->getDiSEqC().useGotoXX = useGotoXX->isChecked();
	lnb->getDiSEqC().useRotorInPower = useRotorInPower->isChecked()?1:0;
	lnb->getDiSEqC().useRotorInPower |= DeltaA->getNumber()<<8;
	lnb->getDiSEqC().DegPerSec = DegPerSec->getFixedNum();
	lnb->getDiSEqC().gotoXXLaDirection = (int) LaDirection->getCurrent()->getKey();
	lnb->getDiSEqC().gotoXXLoDirection = (int) LoDirection->getCurrent()->getKey();
	lnb->getDiSEqC().gotoXXLatitude = Latitude->getFixedNum();
	lnb->getDiSEqC().gotoXXLongitude = Longitude->getFixedNum();
	lnb->getDiSEqC().RotorTable.clear();
	positions->forEachEntry( savePosition( lnb->getDiSEqC().RotorTable ) );
	eTransponderList::getInstance()->writeLNBData();	
	close(0);
}

void RotorConfig::useRotorInPowerChanged( int state )
{
	eDebug("useRotorInPowerChanged to %d", state);
	if (state)
	{
		lDegPerSec->hide();
		DegPerSec->hide();
		lDeltaA->show();
		DeltaA->show();
	}
	else
	{
		lDeltaA->hide();
		DeltaA->hide();
		lDegPerSec->show();
		DegPerSec->show();
	}
}

void RotorConfig::gotoXXChanged( int state )
{
	eDebug("gotoXXChanged to %d", state);
	if ( state )
	{
		add->hide();
		remove->hide();
		lOrbitalPosition->hide();
		orbital_position->hide();
		lStoredRotorNo->hide();
		number->hide();
		lDirection->hide();
		direction->hide();
		positions->hide();

		lLongitude->show();
		Longitude->show();
		LoDirection->show();
		lLatitude->show();
		Latitude->show();
		LaDirection->show();
	}
	else
	{
		lLongitude->hide();
		Longitude->hide();
		LoDirection->hide();
		lLatitude->hide();
		Latitude->hide();
		LaDirection->hide();
		
		positions->show();
		add->show();
		remove->show();
		lOrbitalPosition->show();
		orbital_position->show();
		lStoredRotorNo->show();
		number->show();
		lDirection->show();
		direction->show();
		positions->show();
	}
}

int RotorConfig::eventHandler( const eWidgetEvent& e)
{
	switch(e.type)
	{
	case eWidgetEvent::execBegin:
		// send no more DiSEqC Commands on transponder::tune to Rotor
		eFrontend::getInstance()->disableRotor();
	break;

	case eWidgetEvent::execDone:
		// enable send DiSEqC Commands to Rotor on eTransponder::tune
		eFrontend::getInstance()->enableRotor();
	break;
	
	default:
		return eWindow::eventHandler(e);
	break;
	}
	return 1;
}

void RotorConfig::setLNBData( eLNB *lnb )
{
	positions->beginAtomic();
	positions->clearList();
	eDiSEqC &DiSEqC = lnb->getDiSEqC();

	if ( lnb )
	{
		for ( std::map<int, int>::iterator it ( DiSEqC.RotorTable.begin() ); it != DiSEqC.RotorTable.end(); it++ )
			new eListBoxEntryText( positions, eString().sprintf(" %d / %03d %c", it->second, abs(it->first), it->first > 0 ? 'E' : 'W'), (void*) it->first );

		useGotoXX->setCheck( (int) (lnb->getDiSEqC().useGotoXX & 1 ? 1 : 0) );
		gotoXXChanged( (int) lnb->getDiSEqC().useGotoXX & 1 );
		useRotorInPower->setCheck( (int) lnb->getDiSEqC().useRotorInPower & 1 );
		useRotorInPowerChanged( (int) lnb->getDiSEqC().useRotorInPower & 1 );
		Latitude->setFixedNum( lnb->getDiSEqC().gotoXXLatitude );
		LaDirection->setCurrent( (void*) lnb->getDiSEqC().gotoXXLaDirection );
		Longitude->setFixedNum( lnb->getDiSEqC().gotoXXLongitude );
		LoDirection->setCurrent( (void*) lnb->getDiSEqC().gotoXXLoDirection );
		DegPerSec->setFixedNum( lnb->getDiSEqC().DegPerSec );
		DeltaA->setNumber( (lnb->getDiSEqC().useRotorInPower & 0x0FFFFFFF) >> 8 );
	}
	else
	{
		Latitude->setFixedNum(0);
		LaDirection->setCurrent(0);
		Longitude->setFixedNum(0);
		LoDirection->setCurrent(0);
		DegPerSec->setFixedNum( 1.0 );
		DeltaA->setNumber(40);
		useGotoXX->setCheck( 1 );
		useRotorInPower->setCheck( 0 );
	}

	if ( positions->getCount() )
	{
		positions->sort();
		positions->moveSelection(eListBox<eListBoxEntryText>::dirFirst);
	}
	else
	{
		new eListBoxEntryText( positions, _("delete me"), (void*) 0xFFFF );
		posChanged(0);
	}

	positions->endAtomic();
}

void RotorConfig::posChanged( eListBoxEntryText *e )
{
	if ( e && (int)e->getKey() != 0xFFFF )
	{
		direction->setCurrent( e->getText().right( 1 ) == "E" ? 0 : 1 );
		orbital_position->setNumber( (int) e->getKey() );
		number->setNumber( atoi( e->getText().mid( 1 , e->getText().find('/')-1 ).c_str()) );
	}
	else
	{
		orbital_position->setNumber( 0 );
		number->setNumber( 0 );
		direction->setCurrent( 0 );
	}
}

void RotorConfig::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

void RotorConfig::onAdd()
{
	positions->beginAtomic();

	new eListBoxEntryText( positions,eString().sprintf(" %d / %03d %c",
																											number->getNumber(),
																											orbital_position->getNumber(),
																											direction->getCurrent()->getKey() ? 'W':'E'
																										),
													(void*) ( direction->getCurrent()->getKey()
													? - orbital_position->getNumber()
													: orbital_position->getNumber() )
												);

	positions->sort();
	positions->invalidateContent();
	positions->endAtomic();
}

void RotorConfig::onRemove()
{
	positions->beginAtomic();

	if (positions->getCurrent())
		positions->remove( positions->getCurrent() );

	if (!positions->getCount())
	{
		new eListBoxEntryText( positions, _("delete me"), (void*) 0xFFFF );
		posChanged(0);
	}

	positions->invalidate();
	positions->endAtomic();
}

void RotorConfig::onNextPressed()
{
	if (lnb)
	{
		hide();
		eRotorManual c(lnb);
		c.setLCD( LCDTitle, LCDElement );
		c.show();
		c.exec();
		if (c.changed)
		{
			setLNBData(lnb);
			useGotoXX->setCheck(0);
			positions->forEachEntry( savePosition( lnb->getDiSEqC().RotorTable ) );
			eTransponderList::getInstance()->writeLNBData();
		}
		c.hide();
		show();
	}
}

struct rotorMenuActions
{
	eActionMap map;
	eAction east, eastFine, eastStop, west, westFine, westStop;
	rotorMenuActions():
		map("rotorMenu", "rotorMenu"),
		east(map, "driveEast", _("drive Motor East"), eAction::prioWidget),
		eastFine(map, "driveEastStep", _("drive Motor East.. one step"), eAction::prioWidget),
		eastStop(map, "driveEastStop", _("stop Motor drive East"), eAction::prioWidget),
		west(map, "driveWest", _("drive Motor East"), eAction::prioWidget),
		westFine(map, "driveWestStep", _("drive Motor West.. one step"), eAction::prioWidget),
		westStop(map, "driveWestStop", _("stop Motor drive West"), eAction::prioWidget)
	{
	}
};

eAutoInitP0<rotorMenuActions> i_rotorMenuActions(eAutoInitNumbers::actions, "rotor menu actions");

eRotorManual::eRotorManual(eLNB *lnb)
	:lnb(lnb), retuneTimer(new eTimer(eApp)), transponder(0), changed(0)
{
	lMode = new eLabel(this);
	lMode->setName("lMode");

	lSat = new eLabel(this);
	lSat->setName("lSat");

	lTransponder = new eLabel(this);
	lTransponder->setName("lTransponder");

	lDirection = new eLabel(this);
	lDirection->setName("lDirection");

	lCounter = new eLabel(this);
	lCounter->setName("lCounter");

/*	lRecalcParams = new eLabel(this);
	lRecalcParams->setName("lRecalcParams");*/

	Mode = new eComboBox(this, 4, lMode );
	Mode->setName("Mode");
	CONNECT(Mode->selchanged, eRotorManual::modeChanged);

	eFrontend::getInstance()->disableRotor();

	num = new eNumber( this, 1, 1, 80, 2, 0, 0, lSat );
	num->setName("num");
	num->setNumber(1);

	CONNECT(num->selected, eRotorManual::nextfield);

	Sat = new eComboBox( this, 7, lSat );
	Sat->setName("Sat");
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			new eListBoxEntryText(*Sat, s->getDescription().c_str(), (void*) *s);
	CONNECT(Sat->selchanged, eRotorManual::satChanged );
	eTransponderList::getInstance()->reloadNetworks();
	Sat->setCurrent(0);

	Transponder = new eComboBox(this, 5 );
	Transponder->setName("Transponder");
	CONNECT(Transponder->selchanged, eRotorManual::tpChanged);

	if (Sat->getCount())
		satChanged(Sat->getCurrent());

/*	num1 = new eNumber( this, 1, 0, 255, 2, 0, 0, lRecalcParams );
	num1->setName("num1");
	num1->setNumber(0);
	num1->setFlags(eNumber::flagPosNeg);

	num2 = new eNumber( this, 1, 0, 255, 2, 0, 0, lRecalcParams );
	num2->setName("num2");                          
	num2->setNumber(0);
	num2->setFlags(eNumber::flagPosNeg);

	num3 = new eNumber( this, 1, 0, 255, 2, 0, 0, lRecalcParams );
	num3->setName("num3");
	num3->setNumber(0);
	num3->setFlags(eNumber::flagPosNeg);*/

	Direction = new eButton(this);
	Direction->setName("Direction");

	Exit = new eButton(this);
	Exit->setName("Exit");
	CONNECT( Exit->selected, eRotorManual::reject );
	
	Save = new eButton(this);
	Save->setName("Save");
	CONNECT(Save->selected, eRotorManual::onButtonPressed );

	Search = new eButton(this);
	Search->setName("Search");
	CONNECT( Search->selected, eRotorManual::onScanPressed );

	status = new eFEStatusWidget( this, eFrontend::getInstance() );
	status->setName("Status");

	new eListBoxEntryText(*Mode, _("position"), (void*) 0, 0, _("store new sat positions"));
	new eListBoxEntryText(*Mode, _("drive to stored pos"), (void*) 1, 0, _("drive to stored position"));
	new eListBoxEntryText(*Mode, _("drive to satellite"), (void*) 8, 0, _("drive to stored satellite"));	
	new eListBoxEntryText(*Mode, _("drive to 0�"), (void*) 2, 0, _("drives to 0�"));
	new eListBoxEntryText(*Mode, _("recalculate"), (void*) 3, 0, _("recalculate stored positions rel. to current pos"));
	new eListBoxEntryText(*Mode, _("set east limit"), (void*) 4, 0, _("set east soft limit"));
	new eListBoxEntryText(*Mode, _("set west limit"), (void*) 5, 0, _("set west soft limit"));
	new eListBoxEntryText(*Mode, _("disable limits"), (void*) 6, 0, _("disable soft limits"));
	new eListBoxEntryText(*Mode, _("enable limits"), (void*) 7, 0, _("enable soft limits"));
	Mode->setCurrent(0);
	modeChanged(Mode->getCurrent());

	if ( eSkin::getActive()->build(this, "RotorManual"))
		eFatal("skin load of \"RotorManual\" failed");

	CONNECT( retuneTimer->timeout, eRotorManual::retune );
	addActionMap(&i_rotorMenuActions->map);	
	Direction->setText("<    Stop    >");
}

void eRotorManual::modeChanged( eListBoxEntryText *e)
{
	eString buttonText, helpText;

	switch((int)e->getKey())
	{
		default:
		case 0:
			helptext=_("store current pos in motor");
			buttonText=_("store");
		break;
		case 1:
			helptext=_("drive motor to stored pos");
			buttonText=_("go");
		break;
		case 2:
			helptext=_("drive motor to reference position");
			buttonText=_("go");
		break;
		case 3:
			helptext=_("recalculate all stored positions");
			buttonText=_("recalc");
		break;
		case 4:
			helptext=_("store current pos as east soft limit");
			buttonText=_("store");
		break;
		case 5:
			helptext=_("store current pos as west soft limit");
			buttonText=_("store");
		break;
		case 6:
			helptext=_("disable soft limits");
			buttonText=_("do it");
		break;
		case 7:
			helptext=_("enable soft limits");
			buttonText=_("do it");
		break;
		case 8:
			helptext=_("drive motor to satellite");
			buttonText=_("go");
		break;
	}
	Save->setHelpText(helptext);
	Save->setText(buttonText);

	switch((int)e->getKey())
	{
		default:

		case 0: // store new positions
		case 4: // set east limit
		case 5: // set west limit
/*			lRecalcParams->hide();
			num1->hide();
			num2->hide();
			num3->hide();*/
		case 3: // recalculate
			num->hide();
		break;

		case 6: // clear limits
		case 7: // set limits
		case 2: // goto ref pos
/*			lRecalcParams->hide();
			num1->hide();
			num2->hide();
			num3->hide();*/
			num->hide();
		case 1: // go to stored pos
			lSat->hide();
			Sat->hide();
		case 8: // goto satellite
			lTransponder->hide();
			Transponder->hide();
			Direction->hide();
			lDirection->hide();
		break;
	}

	switch((int)e->getKey())
	{
		default:
		case 3: // recalculate
/*			lRecalcParams->show();
			num1->show();
			num2->show();
			num3->show();*/
		case 0: // store new positions
		case 4: // set east limit
		case 5: // set west limit
			lTransponder->show();
			Transponder->show();
			Direction->show();
			lDirection->show();
		case 8: // goto sat pos
			num->hide();
			lSat->setText(_("Satellite:"));
			lSat->show();
			Sat->show();
		break;

		case 1: // go to stored pos
			lSat->show();
			lSat->setText("Position:");
			num->show();
		case 6: // clear limits
		case 7: // set limits
		case 2: // goto ref pos
		break;
	}

}

void eRotorManual::retune()
{
	if(transponder)
		transponder->tune();
}

void eRotorManual::satChanged( eListBoxEntryText *sat)
{
	Transponder->clear();
	if (sat && sat->getKey())
	{
		eSatellite *Sat = (eSatellite*) (sat->getKey());
		for ( std::list<tpPacket>::const_iterator i( eTransponderList::getInstance()->getNetworks().begin() ); i != eTransponderList::getInstance()->getNetworks().end(); i++ )
			if ( i->orbital_position == Sat->getOrbitalPosition() )
				for (std::list<eTransponder>::const_iterator it( i->possibleTransponders.begin() ); it != i->possibleTransponders.end(); it++)
					new eListBoxEntryText( *Transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );

		if (Transponder->getCount())
		{
			Transponder->setCurrent(0);
			tpChanged(Transponder->getCurrent());
		}
	}
}

void eRotorManual::tpChanged( eListBoxEntryText *tp )
{
	if (tp && tp->getKey() )
	{
		transponder = (eTransponder*)(tp->getKey());
		transponder->tune();
	}
	else
		transponder = 0;
}

void eRotorManual::onButtonPressed()
{
	switch((int)Mode->getCurrent()->getKey())
	{
		default:
		case 0: // store current pos in Rotor
		{
			eStoreWindow w( lnb, ((eSatellite*) Sat->getCurrent()->getKey())->getOrbitalPosition() );
			hide();
			w.show();
			int ret = w.exec();
			if (ret && ret != -1)
			{
				eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6A, eString().sprintf("%02x",ret) );
				changed=1;
			}
			w.hide();
			show();
		}
		break;
		case 1: //drive rotor to stored pos
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6B, eString().sprintf("%02x",num->getNumber()).c_str() );
		break;
		case 2: //driver rotor to reference position
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6B, "00");
		break;
		case 3: //recalculate all stored positions
		{
			eMessageBox mb( _("Wrong use of this function can corrupt all stored sat positions.\n"
				"Are you sure you want to use this function?"), _("Warning"), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
			hide();
			mb.show();
			switch( mb.exec() )
			{
				case eMessageBox::btYes:
					eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6F, "00");
					mb.hide();
					show();
					break;
				case eMessageBox::btNo:
					mb.hide();
					show();
				return;
				break;
			}
		}
		break;
		case 4: //store current pos as east soft limit
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x66 );
		break;
		case 5: //store current pos as west soft limit
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x67 );
		break;
		case 6: //disable soft limits
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x63 );
		break;
		case 7: //enable soft limits
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6A, "00");
		break;
		case 8: //goto sat pos
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6B, eString().sprintf("%02x", lnb->getDiSEqC().RotorTable[ ((eSatellite*)Sat->getCurrent()->getKey())->getOrbitalPosition()] ) );
		break;
	}
	retune();
}

void eRotorManual::onScanPressed()
{
	TransponderScan setup(LCDTitle, LCDElement);
	hide();
	setup.exec();
	show();
}

int eRotorManual::eventHandler( const eWidgetEvent& e)
{
	static timeval begTime=0;
	static bool running=false;
	switch (e.type)
	{
		case eWidgetEvent::execDone:
			// enable send DiSEqC Commands to Rotor on eTransponder::tune
			eFrontend::getInstance()->enableRotor();
		break;

		case eWidgetEvent::evtAction:
			if ( focus == Direction )
			{
				if (e.action == &i_rotorMenuActions->eastFine)
				{
					if (!running)
					{
						Direction->setText(_("one step East..."));
						eDebug("east Fine");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x68, "FF" );
					}
				}
				else if (e.action == &i_rotorMenuActions->westFine)
				{
					if (!running)
					{
						Direction->setText(_("one step west..."));
						eDebug("west fine");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x69, "FF" );
					}
				}
				else if (e.action == &i_rotorMenuActions->east)
				{
					if ( !running )
					{
						gettimeofday( &begTime, 0 );
						begTime+=400;
						running=true;
						eDebug("east");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x68, "00" );
						retuneTimer->start(500, false);
					}
					Direction->setText(_("driving to east..."));
				}
				else if (e.action == &i_rotorMenuActions->west)
				{
					if ( !running )
					{
						gettimeofday( &begTime, 0 );
						begTime+=400;
						running=true;
						eDebug("west");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x69, "00" );
						retuneTimer->start(500, false);
					}
					Direction->setText(_("driving to west..."));
				}
				else if (e.action == &i_rotorMenuActions->eastStop || e.action == &i_rotorMenuActions->westStop )
				{
					if (running && timeout_usec(begTime) <= 0 )
					{
							eDebug("send stop");
							eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x60 );
							running=false;
							retuneTimer->stop();
					}
					retune();
					Direction->setText(_("<    Stop    >"));
				}
				else
					break;
			}
			else
				break;
		return 1;
		
		default:
		break;
	}
	return eWindow::eventHandler(e);	
}

eRotorManual::~eRotorManual()
{
	if (retuneTimer)
		delete retuneTimer;
}

void eRotorManual::nextfield(int*)
{
	focusNext(eWidget::focusDirNext);
}


eStoreWindow::eStoreWindow(eLNB *lnb, int orbital_pos)
	:lnb(lnb), orbital_pos(orbital_pos)
{
	cresize(eSize(360,140));
	cmove(ePoint(150,150));
	setText("Store Satellite");
	lStorageLoc = new eLabel(this);
	lStorageLoc->resize(eSize(250, 35));
	lStorageLoc->move(ePoint(10,10));
	lStorageLoc->setText(_("Storage Location:"));
	StorageLoc = new eNumber( this, 1, 1, 80, 2, 0, 0, lStorageLoc );
	StorageLoc->setHelpText(_("change storage Location or store"));
	StorageLoc->resize(eSize(80, 35));
	StorageLoc->move(ePoint(270, 10));
	StorageLoc->loadDeco();
	CONNECT(StorageLoc->selected, eStoreWindow::nextfield );
	Store = new eButton(this);
	Store->setShortcut("green");
	Store->setShortcutPixmap("green");
	Store->move(ePoint(10,60));
	Store->resize(eSize(170,40));
	Store->setText(_("store"));
	Store->loadDeco();
	CONNECT( Store->selected, eStoreWindow::onStorePressed );
	Cancel = new eButton(this);
	Cancel->move(ePoint(190, 60));
	Cancel->resize(eSize(170, 40));
	Cancel->setText("Cancel");
	Cancel->loadDeco();
	CONNECT( Cancel->selected, eStoreWindow::reject );
	eStatusBar *sbar = new eStatusBar(this);
	sbar->move( ePoint(0, getClientSize().height()-30) );
	sbar->resize( eSize( getClientSize().width(), 30) );
	sbar->loadDeco();
	int i = 0;
	while (1)
	{
		i++;
		std::map<int,int>::iterator it(lnb->getDiSEqC().RotorTable.begin());
		for ( ; it != lnb->getDiSEqC().RotorTable.end(); it++ )
			if ( it->second == i )
				break;

		if (it != lnb->getDiSEqC().RotorTable.end() )
			continue;
			
		StorageLoc->setNumber(i);
		break;
	}
}

void eStoreWindow::nextfield(int*)
{
	focusNext(eWidget::focusDirNext);
}

void eStoreWindow::onStorePressed()
{
	std::map<int,int>::iterator it = lnb->getDiSEqC().RotorTable.find( orbital_pos );
	if ( it != lnb->getDiSEqC().RotorTable.end() )
	{
		eMessageBox mb( eString().sprintf(_("%d.%d�%c is currently stored at location %d!\nWhen you store this now at Location %d, we must remove the old Location.\nAre you sure you want to do this?"),abs(orbital_pos)/10, abs(orbital_pos)%10, orbital_pos>0?'E':'W', it->second, StorageLoc->getNumber() ), _("Warning"), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
		hide();
		mb.show();
		switch( mb.exec() )
		{
			case eMessageBox::btYes:
				lnb->getDiSEqC().RotorTable.erase(it);
				lnb->getDiSEqC().useGotoXX=0;
				lnb->getDiSEqC().RotorTable[orbital_pos] = StorageLoc->getNumber();
				mb.hide();
				show();
				close(StorageLoc->getNumber());
			break;
			case eMessageBox::btNo:
				mb.hide();
				show();
				return;
			break;
		}
	}
	else
	{
		eMessageBox mb( eString().sprintf(_("Store %d.%d�%c at location %d.\n"
			"If you want another location, then say no and change the location manually.\n"
			"Are you sure you want to store at this location?"),abs(orbital_pos)/10, abs(orbital_pos)%10, orbital_pos>0?'E':'W', StorageLoc->getNumber() ), _("Information"), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
		hide();
		mb.show();
		switch( mb.exec() )
		{
			case eMessageBox::btYes:
				lnb->getDiSEqC().useGotoXX=0;
				lnb->getDiSEqC().RotorTable[orbital_pos] = StorageLoc->getNumber();
				mb.hide();
				show();
				close(StorageLoc->getNumber());
			break;
			case eMessageBox::btNo:
				mb.hide();
				show();
				return;
			break;
		}
	}
}

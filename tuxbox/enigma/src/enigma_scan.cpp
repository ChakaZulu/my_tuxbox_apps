/*
 * enigma_scan.cpp
 *
 * Copyright (C) 2002 Felix Domke <tmbinc@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: enigma_scan.cpp,v 1.19 2003/11/05 13:33:37 ghostrider Exp $
 */

#include <enigma_scan.h>

#include <satconfig.h>
#include <rotorconfig.h>
#include <scan.h>
#include <satfind.h>
#include <tpeditwindow.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/frontend.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/emessage.h>
#include <lib/system/info.h>

eZapScan::eZapScan()
	:eSetupWindow(_("Service Searching"),
	eSystemInfo::getInstance()->getFEType()
		== eSystemInfo::feSatellite ? 9 : 7, 400)
{
	int entry=0;
	move(ePoint(160, 130));
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )  // only when a sat box is avail we shows a satellite config
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Satellite Configuration"), eString().sprintf("(%d) %s", ++entry, _("open satellite config"))))->selected, eZapScan::sel_satconfig);
		CONNECT((new eListBoxEntryMenu(&list, _("Satfind"), eString().sprintf("(%d) %s", ++entry, _("open the satfinder"))))->selected, eZapScan::sel_satfind);
		CONNECT((new eListBoxEntryMenu(&list, _("Motor Setup"), eString().sprintf("(%d) %s", ++entry, _("open Motor Setup"))))->selected, eZapScan::sel_rotorConfig);
		CONNECT((new eListBoxEntryMenu(&list, _("Transponder Edit"), eString().sprintf("(%d) %s", ++entry, _("for automatic scan"))))->selected, eZapScan::sel_transponderEdit);
		new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	}
	CONNECT((new eListBoxEntryMenu(&list, _("Automatic Transponder Scan"), eString().sprintf("(%d) %s", ++entry, _("open automatic transponder scan"))))->selected, eZapScan::sel_autoScan);
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )  // only when a sat box is avail we shows a satellite config
		CONNECT((new eListBoxEntryMenu(&list, _("Automatic Multisat Scan"), eString().sprintf("(%d) %s", ++entry, _("open automatic multisat transponder scan"))))->selected, eZapScan::sel_multiScan);

	CONNECT((new eListBoxEntryMenu(&list, _("Manual Transponder Scan"), eString().sprintf("(%d) %s", ++entry, _("open manual transponder scan"))))->selected, eZapScan::sel_manualScan);
}

void eZapScan::sel_satfind()
{
	hide();
	eSatfind s(eFrontend::getInstance());
	s.show();
	s.exec();
	s.hide();
	show();
}

void eZapScan::sel_autoScan()
{
#ifndef DISABLE_LCD
	TransponderScan setup(LCDTitle, LCDElement);
#else
	TransponderScan setup;
#endif
	hide();
	setup.exec(TransponderScan::stateAutomatic);
	show();
}

void eZapScan::sel_multiScan()
{
#ifndef DISABLE_LCD
	TransponderScan setup(LCDTitle, LCDElement);
#else
	TransponderScan setup;
#endif
	hide();
	setup.exec(TransponderScan::stateMulti);
	show();
}

void eZapScan::sel_manualScan()
{
#ifndef DISABLE_LCD
	TransponderScan setup(LCDTitle, LCDElement);
#else
	TransponderScan setup;
#endif
	hide();
	setup.exec(TransponderScan::stateManual);
	show();
}

void eZapScan::sel_satconfig()
{
	hide();
	eSatelliteConfigurationManager satconfig;
#ifndef DISABLE_LCD
	satconfig.setLCD(LCDTitle, LCDElement);
#endif
	satconfig.show();
	satconfig.exec();
	satconfig.hide();
	show();
}

eLNB* eZapScan::getRotorLNB(int silent)
{
	int c=0;
	std::list<eLNB>::iterator RotorLnb = eTransponderList::getInstance()->getLNBs().end();
	std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin());
	for (; it != eTransponderList::getInstance()->getLNBs().end(); it++ )
	{
		if ( it->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
		{
			if (!c++)
				RotorLnb=it;
		}
	}
	if ( c > 1 )  // we have more than one LNBs with DiSEqC 1.2
	{
		eMessageBox mb(_("DiSEqC 1.2 is enabled on more than one LNB, please select the LNB the motor is connected to"), _("Info"), eMessageBox::iconWarning|eMessageBox::btOK );
		mb.show();
		mb.exec();
		mb.hide();
		eLNBSelector sel;
		sel.show();
		int ret = sel.exec();
		sel.hide();
		return (eLNB*) ret;
	}
	else if ( !c )
	{
		if (!silent)
		{
			eMessageBox mb( _("Found no LNB with DiSEqC 1.2 enabled,\nplease goto Satellite Config first, and enable DiSEqC 1.2"), _("Warning"), eMessageBox::iconWarning|eMessageBox::btOK );
			mb.show();
			mb.exec();
			mb.hide();
		}
		return 0;
	}
	else // only one lnb with DiSEqC 1.2 is found.. this is correct :)
		return &(*RotorLnb);
}

void eZapScan::sel_transponderEdit()
{
	hide();
	eTransponderEditWindow wnd;
#ifndef DISABLE_LCD
	wnd.setLCD(LCDTitle, LCDElement);
#endif
	wnd.show();
	wnd.exec();
	wnd.hide();
	show();
}

void eZapScan::sel_rotorConfig()
{
	hide();
	eLNB* lnb = getRotorLNB(0);
	if (lnb)
	{
		RotorConfig c(lnb);
#ifndef DISABLE_LCD
		c.setLCD( LCDTitle, LCDElement );
#endif
		c.show();
		c.exec();
		c.hide();
	}
	show();
}

eLNBSelector::eLNBSelector()
	:eListBoxWindow<eListBoxEntryText>(_("Select LNB"), 5, 300, true)
{
	move(ePoint(140, 156));
	int cnt=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin()); it != eTransponderList::getInstance()->getLNBs().end(); it++, cnt++ )
	{
		if ( it->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
			new eListBoxEntryText( &list, eString().sprintf("LNB %d", cnt), (void*)&(*it), 0, eString().sprintf(_("use LNB %d for Motor"), cnt ).c_str());
	}
	CONNECT( list.selected, eLNBSelector::selected );
}

void eLNBSelector::selected( eListBoxEntryText *e )
{
	if ( e && e->getKey() )
		close((int)e->getKey());
	else
		close(0);
}

/*
 * enigma_setup.cpp
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
 * $Id: setupskin.cpp,v 1.15 2003/01/12 00:49:03 Ghostrider Exp $
 */

#include <setupskin.h>

#include <dirent.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/system/econfig.h>

extern eString getInfo(const char *file, const char *info);

void eSkinSetup::loadSkins()
{
	eListBoxEntrySkin* selection=0;

	const char *skinPaths[] = { CONFIGDIR "/enigma/skins/", DATADIR "/enigma/skins/", 0 };

	char *current_skin=0;
	eConfig::getInstance()->getKey("/ezap/ui/skin", current_skin);

	std::set<eString> parsedSkins;

	for (int i=0; skinPaths[i]; ++i)
	{
		struct dirent **namelist=0;
		int n = scandir(skinPaths[i], &namelist, 0, alphasort);

		if ( n < 0 && i)
		{
			eDebug("error reading skin directory");
			eMessageBox msg("error reading skin directory", "error");
			msg.show();
			msg.exec();
			msg.hide();
			continue;
		}

		for(int count=0;count<n;count++)
		{
			if (i && parsedSkins.find(namelist[count]->d_name) != parsedSkins.end() )
				// ignore loaded skins in var... (jffs2)
				continue;

			eString	fileName=eString(skinPaths[i]) + eString(namelist[count]->d_name);

			if (fileName.find(".info") != eString::npos)
			{
				eString esml=skinPaths[i] + getInfo(fileName.c_str(), "esml");
				eString name=getInfo(fileName.c_str(), "name");
				eDebug("esml = %s, name = %s", esml.c_str(), name.c_str());
				if (esml.size() && name.size())
				{
					parsedSkins.insert(namelist[count]->d_name);
					eListBoxEntrySkin *s=new eListBoxEntrySkin(lskins, name, esml);
					if (current_skin && esml == current_skin)
						selection=s;
				}
			}
			free(namelist[count]);
		}

		if (namelist)
			free(namelist);
	}

	if (selection)
		lskins->setCurrent(selection);
}

void eSkinSetup::accept()
{
	skinSelected(lskins->getCurrent());
}

void eSkinSetup::skinSelected(eListBoxEntrySkin *skin)
{
	if (!skin)
	{
		close(1);
		return;
	}

	eConfig::getInstance()->setKey("/ezap/ui/skin", skin->getESML().c_str());
	eConfig::getInstance()->flush();

	close(0);
}

eSkinSetup::eSkinSetup()
{
	baccept=new eButton(this);
	baccept->setName("accept");
	breject=new eButton(this);
	breject->setName("reject");
	lskins=new eListBox<eListBoxEntrySkin>(this);
	lskins->setName("skins");
	lskins->setFlags(eListBoxBase::flagNoPageMovement);
	statusbar=new eStatusBar(this);
	statusbar->setName("statusbar");

	CONNECT(baccept->selected, eSkinSetup::accept);
	CONNECT(breject->selected, eSkinSetup::reject);
	CONNECT(lskins->selected, eSkinSetup::skinSelected);
	
	setFocus(lskins);

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "setup.skins"))
		eFatal("skin load of \"setup.skins\" failed");

	loadSkins();
}

eSkinSetup::~eSkinSetup()
{
}

int eSkinSetup::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->left)
			focusNext(eWidget::focusDirW);
		else if (event.action == &i_cursorActions->right)
			focusNext(eWidget::focusDirE);
		else
			break;
		return 1;

	default:
		break;
	}
	return eWindow::eventHandler(event);
}

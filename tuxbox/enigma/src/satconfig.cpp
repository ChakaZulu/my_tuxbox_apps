#include "satconfig.h"

#include <core/gui/eskin.h>
#include <core/gui/ebutton.h>
#include <core/driver/rc.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
{
	list=new eListBox<eListBoxEntryMenu>(this);
	list->setName("list");

	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::okPressed);

	sat_new=new eButton(this);
	sat_new->setName("new");
	
	sat_delete=new eButton(this);
	sat_delete->setName("delete");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
}

void eSatelliteConfigurationManager::okPressed()
{
	close(0);
}

void eSatelliteConfigurationManager::newSatellite()
{
}

void eSatelliteConfigurationManager::deleteSatellite()
{
}

int eSatelliteConfigurationManager::eventFilter(const eWidgetEvent &event)
{
#if 0
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_RIGHT:
			focusNext(eWidget::focusDirE);
			return 1;
		case eRCInput::RC_DOWN:
			focusNext(eWidget::focusDirS);
			return 1;
		case eRCInput::RC_LEFT:
			focusNext(eWidget::focusDirW);
			return 1;
		case eRCInput::RC_UP:
			focusNext(eWidget::focusDirN);
			return 1;
		}
	}
#endif
	return 0;
}

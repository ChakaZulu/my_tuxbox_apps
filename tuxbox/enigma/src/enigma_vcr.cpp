#include <enigma_vcr.h>

#include <enigma_standby.h>
#include <lib/gui/actions.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/driver/eavswitch.h>
#include <lib/driver/streamwd.h>

struct enigmaVCRActions
{
	eActionMap map;
	eAction volumeUp, volumeDown;
	enigmaVCRActions():
		map("enigmaVCR", "enigma VCR"),
		volumeUp(map, "volumeUp", "volume up", eAction::prioDialog),
		volumeDown(map, "volumeDown", "volume down", eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaVCRActions> i_enigmaVCRActions(eAutoInitNumbers::actions, "enigma vcr actions");

enigmaVCR* enigmaVCR::instance = 0;

enigmaVCR::enigmaVCR(eString string, eString caption)
	:eMessageBox(string,caption)
{
	if ( !instance )
		instance = this;
	else
		eFatal("create more than one enigmaVCR instances");
	addActionMap(&i_enigmaVCRActions->map);
}

void enigmaVCR::switchBack()
{
	close(0);
}

int enigmaVCR::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::execBegin:
			eAVSwitch::getInstance()->setInput(1);
			break;
		case eWidgetEvent::evtAction:
			if (event.action == &i_enigmaVCRActions->volumeUp)
				volumeUp();
			else if (event.action == &i_enigmaVCRActions->volumeDown)
				volumeDown();
			else
				break;
			return 1;
		default:
			break;
	}
	return eMessageBox::eventHandler(event);
}

enigmaVCR::~enigmaVCR()
{
	instance=0;
	eAVSwitch::getInstance()->setInput(0);
	if ( eZapStandby::getInstance() )
		eAVSwitch::getInstance()->setTVPin8(0);
	else
		eStreamWatchdog::getInstance()->reloadSettings();
}

void enigmaVCR::volumeUp()
{
	eAVSwitch::getInstance()->changeVCRVolume(0, -4);
}

void enigmaVCR::volumeDown()
{
	eAVSwitch::getInstance()->changeVCRVolume(0, +4);
}

#include <lib/driver/rc.h>

#include <config.h>
#include <asm/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/econfig.h>
#include <lib/base/eerror.h>

/*
  *  note on the enigma input layer:
  *  the enigma input layer (rc*) supports n different devices which
  *  all have completely different interfaces, mapped down to 32bit +
  *  make/break/release codes mapped down (via xml files) to "actions".
  *  this was necessary to support multiple remote controls with proprietary
  *  interfaces. now everybody is using input devices, and thus adding
  *  another input layer seems to be a bit overkill. BUT:
  *  image a remote control with two hundred buttons. each and every function
  *  in enigma can be bound to a button. no need to use them twice.
  *  for example, you would have KEY_MENU assigned to a menu for setup etc.,
  *  but no audio and video settings, since you have special keys for that,
  *  and you don't want to display a big menu with entries that are available
  *  with another single key.
  *  then image a remote control with ten buttons. do you really want to waste
  *  KEY_MENU for a simple menu? you need the audio/video settings there too.
  *  take this just as a (bad) example. another (better) example might be front-
  *  button-keys. usually you have KEY_UP, KEY_DOWN, KEY_POWER. you don't want
  *  them to behave like the remote-control-KEY_UP, KEY_DOWN and KEY_POWER,
  *  don't you?
  *  so here we can map same keys of different input devices to different
  *  actions. have fun.
  */

int eRCDevice::getKeyCompatibleCode(const eRCKey &) const
{
	return -1;
}

eRCDevice::eRCDevice(const eString &id, eRCDriver *driver): driver(driver), id(id)
{
	input=driver->getInput();
	driver->addCodeListener(this);
	eRCInput::getInstance()->addDevice(id, this);
}

eRCDevice::~eRCDevice()
{
	driver->removeCodeListener(this);
	eRCInput::getInstance()->removeDevice(id.c_str());
}

eRCDriver::eRCDriver(eRCInput *input): input(input), enabled(1)
{
}

eRCDriver::~eRCDriver()
{
	for (std::list<eRCDevice*>::iterator i=listeners.begin(); i!=listeners.end(); ++i)
		delete *i;
}

eRCConfig::eRCConfig()
{
	reload();
}

eRCConfig::~eRCConfig()
{
	save();
}

void eRCConfig::set( int delay, int repeat )
{
	rdelay = delay;
	rrate = repeat;
}

void eRCConfig::reload()
{
	rdelay=500;
	rrate=100;
	if ( eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate) )
		save();
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
}

void eRCConfig::save()
{
	eConfig::getInstance()->setKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->setKey("/ezap/rc/repeatDelay", rdelay);
}

eRCInput *eRCInput::instance;

eRCInput::eRCInput()
{
	ASSERT( !instance);
	instance=this;
	handle = -1;
	locked = 0;
	keyboardMode = kmNone;
}

eRCInput::~eRCInput()
{
}

void eRCInput::close()
{
}

bool eRCInput::open()
{
	return false;
}

int eRCInput::lock()
{
	locked=1;
	return handle;
}

void eRCInput::unlock()
{
	if (locked)
	{
		for ( std::map<eString,eRCDevice*,lstr>::iterator it( devices.begin() );
			it != devices.end(); ++it)
		{
			it->second->getDriver()->flushBuffer();
		}
		locked=0;
	}
}

void eRCInput::setFile(int newh)
{
	handle=newh;
}

void eRCInput::addDevice(const eString &id, eRCDevice *dev)
{
	devices.insert(std::pair<eString,eRCDevice*>(id, dev));
}

void eRCInput::removeDevice(const eString &id)
{
	devices.erase(id.c_str());
}

eRCDevice *eRCInput::getDevice(const eString &id)
{
	std::map<eString,eRCDevice*>::iterator i=devices.find(id);
	if (i == devices.end())
	{
		eDebug("failed, possible choices are:");
		for (std::map<eString,eRCDevice*>::iterator i=devices.begin(); i != devices.end(); ++i)	
			eDebug("%s", i->first.c_str());
		return 0;
	}
	return i->second;
}

void eRCInput::setKeyboardMode(int mode)
{
	keyboardMode = mode;
}

std::map<eString,eRCDevice*,eRCInput::lstr> &eRCInput::getDevices()
{
	return devices;
}

eAutoInitP0<eRCInput> init_rcinput(eAutoInitNumbers::rc, "RC Input layer");

#if HAVE_DVB_API_VERSION < 3
eRCShortDriver::eRCShortDriver(const char *filename): eRCDriver(eRCInput::getInstance())
{
	handle=open(filename, O_RDONLY|O_NONBLOCK);
	if (handle<0)
	{
		eDebug("failed to open %s", filename);
		sn=0;
	} else
	{
		sn=new eSocketNotifier(eApp, handle, eSocketNotifier::Read);
		CONNECT(sn->activated, eRCShortDriver::keyPressed);
		eRCInput::getInstance()->setFile(handle);
	}
}

eRCShortDriver::~eRCShortDriver()
{
	if (handle>=0)
		close(handle);
	if (sn)
		delete sn;
}

void eRCShortDriver::keyPressed(int)
{
	if ( input->islocked() )
		return;
	__u16 rccode;
	while (1)
	{
		if (read(handle, &rccode, 2)!=2)
			break;
		if (enabled)
			for (std::list<eRCDevice*>::iterator i(listeners.begin()); i!=listeners.end(); ++i)
				(*i)->handleCode(rccode);
	}
}

#endif

eRCInputEventDriver::eRCInputEventDriver(const char *filename): eRCDriver(eRCInput::getInstance())
{
	handle=open(filename, O_RDONLY|O_NONBLOCK);

	if (handle<0)
	{
		eDebug("failed to open %s", filename);
		sn=0;
	}
	else
	{
		sn=new eSocketNotifier(eApp, handle, eSocketNotifier::Read);
		CONNECT(sn->activated, eRCInputEventDriver::keyPressed);
		eRCInput::getInstance()->setFile(handle);
	}
}

eRCInputEventDriver::~eRCInputEventDriver()
{
	if (handle>=0)
		close(handle);
}

void eRCInputEventDriver::keyPressed(int)
{
	if ( input->islocked() )
		return;
	struct input_event ev;
	while (1)
	{
		if (read(handle, &ev, sizeof(struct input_event))!=sizeof(struct input_event))
			break;

		if (enabled)
			for (std::list<eRCDevice*>::iterator i(listeners.begin()); i!=listeners.end(); ++i)
				(*i)->handleCode((int)&ev);
	}
}

eString eRCInputEventDriver::getDeviceName()
{
	char name[128]="";

	if (handle >= 0)
		::ioctl(handle, EVIOCGNAME(128), name);

	return name;
}


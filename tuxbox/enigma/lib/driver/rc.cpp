#include <lib/driver/rc.h>

#include <asm/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <lib/system/init.h>
#include <lib/system/econfig.h>
#include <lib/base/eerror.h>

int eRCDevice::getKeyCompatibleCode(const eRCKey &) const
{
	return -1;
}

eRCDevice::eRCDevice(const char *id, eRCDriver *driver): driver(driver), id(id)
{
	input=driver->getInput();
	driver->addCodeListener(this);
	eRCInput::getInstance()->addDevice(id, this);
}

eRCDevice::~eRCDevice()
{
	driver->removeCodeListener(this);
	eRCInput::getInstance()->removeDevice(id);
}

eRCDriver::eRCDriver(eRCInput *input): input(input), enabled(1)
{
}

eRCDriver::~eRCDriver()
{
	for (std::list<eRCDevice*>::iterator i=listeners.begin(); i!=listeners.end(); ++i)
		delete *i;
}

void eRCShortDriver::keyPressed(int)
{
	__u16 rccode;
	while (1)
	{
		if (read(handle, &rccode, 2)!=2)
			break;
		if (enabled && !input->islocked())
			for (std::list<eRCDevice*>::iterator i(listeners.begin()); i!=listeners.end(); ++i)
				(*i)->handleCode(rccode);
	}
}

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

void eRCInputEventDriver::keyPressed(int)
{
	struct input_event ev;
	while (1)
	{
		if (read(handle, &ev, sizeof(struct input_event))!=sizeof(struct input_event))
			break;
		if (enabled && !input->islocked())
			for (std::list<eRCDevice*>::iterator i(listeners.begin()); i!=listeners.end(); ++i)
				(*i)->handleCode((int)&ev);
	}
}

eRCInputEventDriver::eRCInputEventDriver(const char *filename): eRCDriver(eRCInput::getInstance())
{
	handle=open(filename, O_RDONLY|O_NONBLOCK);
	if (handle<0)
	{
		eDebug("failed to open %s", filename);
		sn=0;
	} else
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
	if (sn)
		delete sn;
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
		locked=0;
}

void eRCInput::setFile(int newh)
{
	handle=newh;
}

void eRCInput::addDevice(const char *id, eRCDevice *dev)
{
	devices.insert(std::pair<const char*,eRCDevice*>(id, dev));
}

void eRCInput::removeDevice(const char *id)
{
	devices.erase(id);
}

eRCDevice *eRCInput::getDevice(const char *id)
{
	std::map<const char*,eRCDevice*>::iterator i=devices.find(id);
	if (i == devices.end())
	{
		eDebug("failed, possible choices are:");
		for (std::map<const char*,eRCDevice*>::iterator i=devices.begin(); i != devices.end(); ++i)	
			eDebug("%s", i->first);
		return 0;
	}
	return i->second;
}

eAutoInitP0<eRCInput> init_rcinput(1, "RC Input layer");

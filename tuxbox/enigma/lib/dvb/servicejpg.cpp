#ifndef DISABLE_FILE

#include <config.h>
#include <lib/dvb/servicejpg.h>
#include <lib/dvb/servicefile.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/base/i18n.h>
#include <lib/gdi/fb.h>
#include <lib/picviewer/pictureviewer.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

eServiceHandlerJPG::eServiceHandlerJPG(): eServiceHandler(0x2000)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this) < 0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerJPG::addFile);
}

eServiceHandlerJPG::~eServiceHandlerJPG()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceHandlerJPG::addFile(void *node, const eString &filename)
{
	if (filename.right(4).upper() == ".JPG" ||
	    filename.right(4).upper() == ".GIF" ||
	    filename.right(4).upper() == ".PNG" ||
	    filename.right(4).upper() == ".BMP")
	{
		struct stat s;
		if (!(::stat(filename.c_str(), &s)))
		{
			eServiceReference ref(id, 0, filename);
			ref.descr = filename.substr(filename.find_last_of("/") + 1, filename.length() - 1);
			eServiceFileHandler::getInstance()->addReference(node, ref);
		}
	}
}

int eServiceHandlerJPG::play(const eServiceReference &service, int workaround )
{
	// we never more use this
	return 0;
}

int eServiceHandlerJPG::stop(int workaround)
{
	return 0;
}

eService *eServiceHandlerJPG::addRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->addRef(service);
}

void eServiceHandlerJPG::removeRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->removeRef(service);
}

eAutoInitP0<eServiceHandlerJPG> i_eServiceHandlerJPG(eAutoInitNumbers::service + 2, "eServiceHandlerJPG");

#endif //DISABLE_FILE

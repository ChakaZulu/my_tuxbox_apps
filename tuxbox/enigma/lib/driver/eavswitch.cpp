#include "eavswitch.h"
#include <unistd.h>
#include <fcntl.h>
#include <dbox/avs_core.h>
#include <sys/ioctl.h>
#include "qglobal.h"
#include "edvb.h"

/* sucks */

#define SAAIOGREG               1 /* read registers                             */
#define SAAIOSINP               2 /* input control                              */
#define SAAIOSOUT               3 /* output control                     */
#define SAAIOSENC               4 /* set encoder (pal/ntsc)             */
#define SAAIOSMODE              5 /* set mode (rgb/fbas/svideo) */

#define SAA_MODE_RGB    0
#define SAA_MODE_FBAS   1
#define SAA_MODE_SVIDEO 2

#define SAA_NTSC                0
#define SAA_PAL                 1

#define SAA_INP_MP1             1
#define SAA_INP_MP2             2
#define SAA_INP_CSYNC   4
#define SAA_INP_DEMOFF  6
#define SAA_INP_SYMP    8
#define SAA_INP_CBENB   128

eAVSwitch *eAVSwitch::instance=0;

eAVSwitch::eAVSwitch()
{
	if (!instance)
		instance=this;
}

eAVSwitch *eAVSwitch::getInstance()
{
	return instance;
}

eAVSwitch::~eAVSwitch()
{
	if (instance==this)
		instance=0;
}

void eAVSwitch::reloadSettings()
{
	unsigned int colorformat;
	eDVB::getInstance()->config.getKey("/elitedvb/video/colorformat", colorformat);
	setColorFormat((eAVColorFormat)colorformat);
}

eAVSwitchNokia::eAVSwitchNokia()
{
	fd=open("/dev/dbox/avs0", O_RDWR);
	saafd=open("/dev/dbox/saa0", O_RDWR);
	
	reloadSettings();
}

eAVSwitchNokia::~eAVSwitchNokia()
{
	if (fd>=0)
		close(fd);
	if (saafd>=0)
		close(saafd);
}

int eAVSwitchNokia::setVolume(int vol)
{
	vol=63-vol/(65536/64);
	if (vol<0)
		vol=0;
	if (vol>63)
		vol=63;
	return ioctl(fd, AVSIOSVOL, &vol);
}

int eAVSwitchNokia::setTVPin8(int vol)
{
	int fnc;
	switch (vol)
	{
	case 0:
		fnc=0;
		break;
	case 6:
		fnc=1;
		break;
	case 12:
		fnc=2;
		break;
	}
	return ioctl(fd, AVSIOSFNC, &fnc);
}

int eAVSwitchNokia::setColorFormat(eAVColorFormat c)
{
	int arg=0;
	switch (c)
	{
	case cfNull:
		return -1;
	case cfCVBS:
		arg=SAA_MODE_FBAS;
		break;
	case cfRGB:
		arg=SAA_MODE_RGB;
		break;
	case cfYC:
		arg=SAA_MODE_SVIDEO;
		break;
	}
	return ioctl(saafd, SAAIOSMODE, &arg);
}

int eAVSwitchNokia::setAspectRatio(eAVAspectRatio as)
{
	aspect=as;
	return setTVPin8(active?0:((aspect==r169)?6:12));
}

int eAVSwitchNokia::isVCRActive()
{
	return 0;
}

int eAVSwitchNokia::setActive(int a)
{
	active=a;
	return setTVPin8(active?0:((aspect==r169)?6:12));
}

int eAVSwitchNokia::setInput(int v)
{
	qDebug("setInput %d, fd=%d", v, fd);
	switch (v)
	{
	case 0:
		v=5;
		ioctl(fd, AVSIOSVSW1, &v);
		v=1;
		ioctl(fd, AVSIOSASW1, &v);
		v=7;
		ioctl(fd, AVSIOSVSW2, &v);
		v=1;
		ioctl(fd, AVSIOSFBLK, &v);
		break;
	case 1:
		v=3;
		ioctl(fd, AVSIOSVSW1, &v);
		v=2;
		ioctl(fd, AVSIOSASW1, &v);
		v=7;
		ioctl(fd, AVSIOSVSW2, &v);
		v=2;
		ioctl(fd, AVSIOSFBLK, &v);
		break;
	}
	return 0;
}

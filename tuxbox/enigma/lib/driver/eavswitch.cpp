#include "eavswitch.h"
#include <unistd.h>
#include <fcntl.h>
#include <dbox/avs_core.h>
#include <sys/ioctl.h>

#include "config.h"

#include <core/system/econfig.h>
#include <core/dvb/edvb.h>

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
	active=0;
	if (!instance)
		instance=this;
	fd=open("/dev/dbox/avs0", O_RDWR);
	saafd=open("/dev/dbox/saa0", O_RDWR);
	reloadSettings();
}

eAVSwitch *eAVSwitch::getInstance()
{
	return instance;
}

eAVSwitch::~eAVSwitch()
{
	if (instance==this)
		instance=0;

	if (fd>=0)
		close(fd);
	if (saafd>=0)
		close(saafd);
}


void eAVSwitch::reloadSettings()
{
	unsigned int colorformat;
	eConfig::getInstance()->getKey("/elitedvb/video/colorformat", colorformat);
	setColorFormat((eAVColorFormat)colorformat);
}

int eAVSwitch::setVolume(int vol)
{
	vol=63-vol/(65536/64);
	if (vol<0)
		vol=0;
	if (vol>63)
		vol=63;

	return ioctl(fd, AVSIOSVOL, &vol);
}

int eAVSwitch::setTVPin8(int vol)
{
/* results from philips:	fnc=0 -> 0V
				fnc=1 -> 0V
				fnc=2 -> 6V
				fnc=3 -> 12V
*/
	int fnc;
	switch (vol)
	{
	case 0:
		fnc=(Type==PHILIPS?1:0);
		break;
	case 6:
		fnc=(Type==PHILIPS?2:1);
		break;
	case 12:
		fnc=(Type==PHILIPS?3:2);
		break;
	}
	return ioctl(fd, AVSIOSFNC, &fnc);
}

int eAVSwitch::setColorFormat(eAVColorFormat c)
{
	colorformat=c;
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
	int fblk = (c == cfRGB)?1:0;
	ioctl(saafd, SAAIOSMODE, &arg);
	ioctl(fd, AVSIOSFBLK, &fblk);
	return 0;
}

int eAVSwitch::setInput(int v)
{	
	eDebug("setInput %d, fd=%d", v, fd);
	switch (v)
	{
	case 0:	//	Switch to DVB
		ioctl(fd, AVSIOSVSW1, dvb);
		ioctl(fd, AVSIOSASW1, dvb+1);
		ioctl(fd, AVSIOSVSW2, dvb+2);
		ioctl(fd, AVSIOSASW2, dvb+3);
		ioctl(fd, AVSIOSVSW3, dvb+4);
		ioctl(fd, AVSIOSASW3, dvb+5);
		if (!eDVB::getInstance()->mute)
			ioctl(fd, AVSIOSVOL, &eDVB::getInstance()->volume);
		reloadSettings();
		break;
	case 1:   // Switch to VCR
		v = (Type == SAGEM)? 0 : 2;
		ioctl(fd, AVSIOSFBLK, &v);
		ioctl(fd, AVSIOSVSW1, scart);
		ioctl(fd, AVSIOSASW1, scart+1);
		ioctl(fd, AVSIOSVSW2, scart+2);
		ioctl(fd, AVSIOSASW2, scart+3);
		ioctl(fd, AVSIOSVSW3, scart+4);
		ioctl(fd, AVSIOSASW3, scart+5);
		v = 0;  // full Volume
		ioctl(fd, AVSIOSVOL, &v);
		break;
	}
	return 0;
}

int eAVSwitch::setAspectRatio(eAVAspectRatio as)
{
	aspect=as;
	return setTVPin8(active?((aspect==r169)?6:12):0);
}

int eAVSwitch::isVCRActive()
{
	return 0;
}

int eAVSwitch::setActive(int a)
{
	active=a;
	return setTVPin8(active?((aspect==r169)?6:12):0);
}

bool eAVSwitch::loadScartConfig()
{
	FILE* fd = fopen(CONFIGDIR"/scart.conf", "r");
	if(fd)
	{
		eDebug("[eAVSwitch] loading scart-config (scart.conf)");

		char buf[1000];
		fgets(buf,sizeof(buf),fd);

		eString readline = "_scart: %d %d %d %d %d %d\n";

		switch (Type)
		{
			case NOKIA:
				readline.insert(0, "nokia");
			break;
			case PHILIPS:
				readline.insert(0, "philips");
			break;
			case SAGEM:
				readline.insert(0, "sagem");
			break;
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, readline.c_str(), &scart[0], &scart[1], &scart[2], &scart[3], &scart[4], &scart[5] );
			eDebug("[eAVSwitch] readed scart conf : %i %i %i %i %i %i", scart[0], scart[1], scart[2], scart[3], scart[4], scart[5] );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			int i = readline.find("_scart");
			readline.replace(i, 6,"_dvb");
			sscanf( buf, readline.c_str(), &dvb[0], &dvb[1], &dvb[2], &dvb[3], &dvb[4], &dvb[5] );
			eDebug("[eAVSwitch] readed dvb conf : %i %i %i %i %i %i", dvb[0], dvb[1], dvb[2], dvb[3], scart[4], scart[5] );
		}
		fclose(fd);
	}
	else
	{
		eDebug("[eAVSwitch] failed to load scart-config (scart.conf), using standard-values");
	}
	return 0;
}

#ifdef ENABLE_DYN_FLASH

#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <enigma.h>
#include <enigma_main.h>
#include <enigma_standby.h>
#include <timer.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/dvb/record.h>
#include <lib/dvb/serviceplaylist.h>

#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_conf.h>
#include <configfile.h>

using namespace std;

eString getConfigFlashMgr(void)
{
	eString dev, size, erasesize, name;
	eString h1, h2, h3, h4;
	eString thead, tbody;
	std::stringstream tmp;
	eString result = readFile(TEMPLATE_DIR + "flashMgr.tmp");
	eString procmtd = readFile("/proc/mtd");
	eString t;
	tmp.str(procmtd);
	tmp >> h1 >> h2 >> h3 >> h4;
	thead += "<th>" + h1 + "</th>";
	thead += "<th>" + h2 + "</th>";
	thead += "<th>" + h3 + "</th>";
	thead += "<th>" + h4 + "</th>";
	result.strReplace("#THEAD#", thead);
	tmp >> dev;
	while (tmp)
	{
		size = erasesize = name = "";
		tbody += "<tr>";
		tbody += "<td>" + dev + "</td>";
		tmp >> size;
		tbody += "<td>" + size + "</td>";
		tmp >> erasesize;
		tbody += "<td>" + erasesize + "</td>";
		tmp >> t;
		while ((t.find("mtd") == eString::npos) && tmp)
		{
			name += t + " ";
			tmp >> t;
		}
		dev = t;
		tbody += "<td>" + name + "</td>";
		tbody += "</tr>";
	}
	result.strReplace("#TBODY#", tbody);

	return result;
}

void ezapFlashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
//	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSwapFile", setConfigSwapFile, lockWeb);
//	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigMultiBoot", setConfigMultiBoot, lockWeb);
}
#endif

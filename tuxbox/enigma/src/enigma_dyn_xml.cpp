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
#include <enigma_dyn_xml.h>

using namespace std;

extern eString zap[5][5];
extern eString firmwareLevel(eString verid);
extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp
extern eString getIP(void);

static eString getImageInfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::stringstream result;

	eString myVersion = getAttribute("/.version", "version");
	eString myCatalogURL = getAttribute("/.version", "catalog");
	eString myComment = getAttribute("/.version", "comment");
	eString myImageURL = getAttribute("/.version", "url");

	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		<< "<image>"
		<< "<version>" << firmwareLevel(myVersion) << "</version>"
		<< "<url>" << myImageURL << "</url>"
		<< "<comment>" << myComment << "</comment>"
		<< "<catalog>" << myCatalogURL << "</catalog>"
		<< "</image>";

	return result.str();
}

static eString getStatus(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString name, provider, vpid, apid, pcrpid, tpid, vidform("n/a"), tsid, onid, sid, pmt;

	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	eString result;
	time_t atime;
	time(&atime);
	atime += eDVB::getInstance()->time_difference;
	result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<status>"
		"<current_time>" + eString(ctime(&atime)) + "</current_time>"
		"<standby>";
		if (eZapMain::getInstance()->isSleeping())
			result += "ON";
		else
			result += "OFF";
	result += "</standby>";
	result += "<recording>";
#ifndef DISABLE_FILE
		if (eZapMain::getInstance()->isRecording())
			result += "ON";
		else
#endif
			result += "OFF";
	result += "</recording>";
	result += "<mode>" + eString().sprintf("%d", eZapMain::getInstance()->getMode()) + "</mode>";

	eString sRef;
	if (eServiceInterface::getInstance()->service)
		sRef = eServiceInterface::getInstance()->service.toString();
	result += "<service_reference>" + sRef + "</service_reference>";

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eServiceDVB *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (service)
		{
			name = filter_string(service->service_name);
			provider = filter_string(service->service_provider);
		}
	}
	vpid = eString().sprintf("%04x", Decoder::current.vpid);
	apid = eString().sprintf("%04x", Decoder::current.apid);
	pcrpid = eString().sprintf("%04x", Decoder::current.pcrpid);
	tpid = eString().sprintf("%04x", Decoder::current.tpid);
	tsid = eString().sprintf("%04x", sapi->service.getTransportStreamID().get());
	onid = eString().sprintf("%04x", sapi->service.getOriginalNetworkID().get());
	sid = eString().sprintf("%04x", sapi->service.getServiceID().get());
	pmt = eString().sprintf("%04x", Decoder::current.pmtpid);

	FILE *bitstream = 0;

	if (Decoder::current.vpid != -1)
		bitstream = fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres = 0, yres = 0, aspect = 0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
		}
		fclose(bitstream);
		vidform.sprintf("%dx%d ", xres, yres);
		switch (aspect)
		{
			case 1:
				vidform += "(square)"; break;
			case 2:
				vidform += "(4:3)"; break;
			case 3:
				vidform += "(16:9)"; break;
			case 4:
				vidform += "(20:9)"; break;
		}
	}
	result += "<service_name>" + name + "</service_name>";
	result += "<provider>" + provider + "</provider>";
	result += "<vpid>" + vpid + "</vpid>";
	result += "<apid>" + apid + "</apid>";
	result += "<pcrpid>" + pcrpid + "</pcrpid>";
	result += "<tpid>" + tpid + "</tpid>";
	result += "<tsid>" + tsid + "</tsid>";
	result += "<onid>" + onid + "</onid>";
	result += "<sid>" + sid + "</sid>";
	result += "<pmt>" + pmt + "</pmt>";
	result += "<videoformat>" + vidform + "</videoformat>";
	result += "</status>";
	
	return result;
}

static eString getAudioChannels(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::stringstream result;

	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		<< "<audio_channels>";
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it)
		{
			result  << "<channel>" 
				<< "<pid>"
				<< eString().sprintf("0x%04x", it->pmtentry->elementary_PID)
				<< "</pid>"
				<< "<selected>";
			if (it->pmtentry->elementary_PID == Decoder::current.apid)
				result << "1";
			else
				result << "0";
			result  << "</selected>"
				<< "<name>" << it->text << "</name>"
				<< "</channel";
		}
	}
	else
		result << "<audio>none</audio>";

	result << "</audio_channels>";
	
	return result.str();
}

static eString getEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString description, ext_description, genre;
	int genreCategory = 0;
	result << std::setfill('0');

	eService* current;
	eServiceReference ref;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><content id=\"sapi\">No EPG available</content>";

	eString type = opt["type"];
	
	eString serviceRef = opt["ref"];
	
	if (serviceRef)
		ref = string2ref(serviceRef);
	else
		ref = sapi->service;

	current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);

	if (!current)
		return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><content id=\"current\">No EPG available</content>";

	eServiceReferenceDVB &rref = (eServiceReferenceDVB&)ref;
	eEPGCache::getInstance()->Lock();
	const timeMap* evt = eEPGCache::getInstance()->getTimeMap(rref);

	if (!evt)
		return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><content id=\"evt\">No EPG available</content>";
	else
	{
		timeMap::const_iterator It;
		int tsidonid = (rref.getTransportStreamID().get()<<16) | rref.getOriginalNetworkID().get();
		result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			<< "<epg>"
			<< "<service_reference>" << ref2string(ref) << "</service_reference>"
			<< "<service_name>" << filter_string(current->service_name) << "</service_name>";
		
		int i = 0;
		for(It=evt->begin(); It!= evt->end(); ++It)
		{
			ext_description = "";
			EITEvent event(*It->second,tsidonid);
			for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
			{
				Descriptor *descriptor = *d;
				if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
					ext_description += ((ExtendedEventDescriptor*)descriptor)->text;
				else
				if (descriptor->Tag() == DESCR_SHORT_EVENT)
					description = ((ShortEventDescriptor*)descriptor)->event_name;
				else
				if (descriptor->Tag() == DESCR_CONTENT)
				{
					genre = "";
					genreCategory = 0;
					ContentDescriptor *cod = (ContentDescriptor *)descriptor;

					for (ePtrList<descr_content_entry_struct>::iterator ce(cod->contentList.begin()); ce != cod->contentList.end(); ++ce)
					{
						if (genreCategory == 0)
							genreCategory = ce->content_nibble_level_1;
						if (eChannelInfo::getGenre(genreCategory * 16 + ce->content_nibble_level_2))
						{
							if (!genre)
								genre += gettext(eChannelInfo::getGenre(genreCategory * 16 + ce->content_nibble_level_2).c_str());
						}
					}
				}
			}
			
			if (!genre)
				genre = "n/a";

			tm* t = localtime(&event.start_time);

			result << "<event id=\"" << i << "\">";
			eString tmp = filter_string(description);
			tmp.strReplace("&", "&amp;");
			result  << "<date>"
				<< std::setw(2) << t->tm_mday << '.'
				<< std::setw(2) << t->tm_mon+1 << '.' 
				<< std::setw(2) << t->tm_year + 1900
				<< "</date>"
				<< "<time>"
				<< std::setw(2) << t->tm_hour << ':'
				<< std::setw(2) << t->tm_min 
				<< "</time>"
				<< "<duration>" << event.duration / 60 << "</duration>"
				<< "<description>" << filter_string(tmp) << "</description>";
				
			if (type == "extended")
			{
				eString ext_tmp = filter_string(ext_description);
				ext_tmp.strReplace("&", "&amp;");

				result  << "<genre>" << genre << "</genre>"
					<< "<start>" << event.start_time << "</start>"
					<< "<details>" << filter_string(ext_tmp) << "</details>";
			}
			result << "</event>";
			i++;
		}
	}
	eEPGCache::getInstance()->Unlock();

	result << "</epg>";

	return result.str();
}

static eString mPlayer(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString vpid = eString().sprintf("%04x", Decoder::current.vpid);
	eString apid = eString().sprintf("%04x", Decoder::current.apid);

	content->local_header["Content-Type"]="video/mpegfile";
	content->local_header["Cache-Control"] = "no-cache";
	content->local_header["vpid"] = vpid;
	content->local_header["apid"] = apid;

	return "http://" + getIP() + ":31339/" + vpid  + "," + apid;
}

class treeNode
{
public:
	eString serviceNode;
	eString serviceName;
	bool isDirectory;
	eServiceReference serviceReference;
	treeNode(bool isdir, eString sname, eString snode, eServiceReference ref)
	{
		serviceName = sname;
		serviceNode = snode;
		isDirectory = isdir;
		serviceReference = ref;
	};
	~treeNode() {};
	bool operator < (const treeNode &a) const {return serviceName < a.serviceName;}
};

eString genNode(int submode, std::list <treeNode>::iterator myIt)
{
	eString result = myIt->serviceNode + "\n";
	return result;
}

eString getTag(int id)
{
	eString tag;
	switch(id)
	{
		case 2: tag = "satellites"; break;
		case 3: tag = "providers"; break;
		case 4: tag = "bouquets"; break;
		default: tag = "unknown"; break;
	}
	return tag;
}

eString genNodeHeader(int submode, std::list <treeNode>::iterator myIt)
{
	eString ext_tmp = filter_string(myIt->serviceName);
	ext_tmp.strReplace("&", "&amp;");
	eString result = "<" + getTag(submode) + "s name=\"" + filter_string(ext_tmp) + "\">\n";
	return result;
}

eString genNodeFooter(int submode, std::list <treeNode>::iterator myIt)
{
	eString result = "</" + getTag(submode) + "s>\n";
	return result;
}

struct listChannels: public Object
{
	std::list <treeNode> myList;
	eServiceInterface *iface;
	int submode;
	eString& result;
	bool sort;
	bool addEPG;

	listChannels(int submode, const eServiceReference &service, eString &result, bool sort, bool addEPG)
		:myList(myList), iface(eServiceInterface::getInstance()), submode(submode), result(result), sort(sort), addEPG(addEPG)
	{
		std::list <treeNode>::iterator myIt;
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, listChannels::addTreeNode);
		iface->enterDirectory(service, cbSignal);
		iface->leaveDirectory(service);
//		result += genNodes(submode, sort, myList);
		if (sort)
			myList.sort();
		for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
		{
			if (myIt->isDirectory)
			{
				result += genNodeHeader(submode, myIt);
				listChannels(submode, myIt->serviceReference, result, sort, addEPG);
				result += genNodeFooter(submode, myIt);
			}
			else
				result += genNode(submode, myIt);
		}
	}

	void addTreeNode(const eServiceReference& ref)
	{
		eString serviceReference, serviceName, serviceDescription, serviceNode, orbitalPosition;

		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		eService *service = iface ? iface->addRef(ref) : 0;

		serviceReference = ref.toString();
		if (ref.descr) serviceName = filter_string(ref.descr);
		else
		{
			if (service)
			{
				serviceName = filter_string(service->service_name);
				iface->removeRef(ref);
			}
			else
				serviceName = "unnamed service";
		}

		if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
		{
			const eServiceReferenceDVB& dvb_ref = (const eServiceReferenceDVB&)ref;
			eTransponder *tp = eTransponderList::getInstance()->searchTS(
				dvb_ref.getDVBNamespace(),
				dvb_ref.getTransportStreamID(),
				dvb_ref.getOriginalNetworkID());
			if (tp && tp->satellite.isValid())
				orbitalPosition = eString().setNum(tp->satellite.orbital_position);
			else
				orbitalPosition = "0";
		}

		eString epg;
		if (addEPG && epg)
			serviceDescription = serviceName + " - " + epg;
		else
			serviceDescription = serviceName;

		serviceDescription.strReplace("'", "\\\'");
		serviceDescription.strReplace("\"", "\\\"");
		serviceDescription.strReplace("&", "&amp;");
		serviceNode = "<service reference=\"" + serviceReference + "\" orbitalposition=\"" + orbitalPosition + "\">" + serviceDescription + "</service>";
		myList.push_back(treeNode(ref.flags & eServiceReference::isDirectory, serviceName, serviceNode, ref));
	}
};

static eString getServices(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	eString result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

	std::map<eString,eString> opts=getRequestOptions(opt, '&');

	eString mode = "0";
	if (opts["mode"])
		mode = opts["mode"];

	eString submode = "2";
	if (opts["submode"])
		submode = opts["submode"];

	eString sref = zap[atoi(mode.c_str())][atoi(submode.c_str())];
	eServiceReference ref(sref);

	int subm = atoi(submode.c_str());

	result += "<" + getTag(subm) + ">\n";
	listChannels t(subm, ref, result, false, false);
	result += "</" + getTag(subm) + ">\n";

	return result;
}

void ezapXMLInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/xml/status", getStatus, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/epg", getEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/imageinfo", getImageInfo, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/audiochannels", getAudioChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/mplayer.mply", mPlayer, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/getservices", getServices, lockWeb);
}


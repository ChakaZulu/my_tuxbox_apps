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
#include <lib/dvb/frontend.h>

#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_xml.h>
#include <streaminfo.h>

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

static eString getBoxStatus(eString request, eString dirpath, eString opt, eHTTPConnection *content)
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

	result += "</status>";
	
	return result;
}

extern eString getCurrentSubChannel(eString);

static eString getCurrentServiceData(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString now_start, now_date, now_time, now_duration, now_text, now_longtext,
		next_start, next_date, next_time, next_duration, next_text, next_longtext;
	
	eString result = readFile(TEMPLATE_DIR + "currentServiceData.tmp");
	
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eService *current = eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (current)
		{
			eString curService = current->service_name;
			eString curSubService = getCurrentSubChannel(ref2string(sapi->service));
			if (curSubService)
			{
				if (curService)
					curService += ": " + curSubService;
				else
					curService = curSubService;
			}
			result.strReplace("#NAME#", curService);
			result.strReplace("#REFERENCE#", ref2string(sapi->service));
		}
		else
		{
			result.strReplace("#NAME#", "");
			result.strReplace("#REFERENCE#", "");
		}
	}

	EIT *eit = eDVB::getInstance()->getEIT();
	if (eit)
	{
		int p = 0;

		for (ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
		{
			if (*event)
			{
				if (p == 0)
				{
					if (event->start_time)
					{
						now_start = eString().sprintf("%d", (int)event->start_time);
						now_time.sprintf("%s", ctime(&event->start_time));
						eDebug("[ENIGMA_DYN_XML] now_time = %s", now_time.c_str());
						now_date = now_time.mid(5, 5);
						now_time = now_time.mid(11, 5);
					}

					now_duration.sprintf("%d", (int)(event->duration));
				}
				if (p == 1)
				{
					if (event->start_time)
					{
						next_start = eString().sprintf("%d", (int)event->start_time);
 						next_time.sprintf("%s", ctime(&event->start_time));
						eDebug("[ENIGMA_DYN_XML] next_time = %s", next_time.c_str());
						next_date = next_time.mid(5, 5);
						next_time = next_time.mid(11, 5);
						next_duration.sprintf("%d", (int)(event->duration));
					}
				}
				LocalEventData led;
				switch(p)
				{
				case 0:
					led.getLocalData(event, &now_text, 0, &now_longtext);
					break;
				case 1:
					led.getLocalData(event, &next_text, 0, &next_longtext);
					break;
				}
				p++;
		 	}
		}
		eit->unlock();
	}

	result.strReplace("#NOWSTART#", now_start);
	result.strReplace("#NOWT#", now_time);
	result.strReplace("#NOWDATE#", now_date);
	result.strReplace("#NOWD#", now_duration);
	result.strReplace("#NOWST#", filter_string(now_text.strReplace("\"", "'")));
	result.strReplace("#NOWLT#", filter_string(now_longtext.strReplace("\"", "'")));
	result.strReplace("#NEXTSTART#", next_start);
	result.strReplace("#NEXTT#", next_time);
	result.strReplace("#NEXTDATE#", next_date);
	result.strReplace("#NEXTD#", next_duration);
	result.strReplace("#NEXTST#", filter_string(next_text.strReplace("\"", "'")));
	result.strReplace("#NEXTLT#", filter_string(next_longtext.strReplace("\"", "'")));
	
	std::stringstream tmp;
	tmp << "<audiochannels>";
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it)
		{
			tmp	<< "<channel>" 
				<< "<pid>"
				<< eString().sprintf("0x%04x", it->pmtentry->elementary_PID)
				<< "</pid>"
				<< "<selected>";
			if (it->pmtentry->elementary_PID == Decoder::current.apid)
				tmp << "1";
			else
				tmp << "0";
			result += "</selected><name>" + it->text + "</name>";
			result += "</channel";
		}
	}
	tmp << "</audio_channels>";
	result.strReplace("#AUDIOCHANNELS#", tmp.str());
	
	switch (eAVSwitch::getInstance()->getAudioChannel())
	{
		case 0: result.strReplace("#AUDIOTRACK#", "LEFT"); break;
		case 1: result.strReplace("#AUDIOTRACK#", "STEREO"); break;
		case 2: result.strReplace("#AUDIOTRACK#", "RIGHT"); break;
		default: result.strReplace("#AUDIOTRACK#", ""); break;
	}
	
	tmp.clear();
	tmp << "<video_channels>";
	eString curServiceRef = ref2string(eServiceInterface::getInstance()->service);
	if (curServiceRef)
	{
		eString s1 = curServiceRef; int pos; eString nspace;
		for (int i = 0; i < 7 && s1.find(":") != eString::npos; i++)
		{
			pos = s1.find(":");
			nspace = s1.substr(0, pos);
			s1 = s1.substr(pos + 1);
		}
		EIT *eit = eDVB::getInstance()->getEIT();
		if (eit)
		{
			int p = 0;
			for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
			{
				EITEvent *event = *i;
				if ((event->running_status >= 2) || ((!p) && (!event->running_status)))
				{
					for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					{
						if (d->Tag() == DESCR_LINKAGE)
						{
							LinkageDescriptor *ld = (LinkageDescriptor *)*d;
							if (ld->linkage_type == 0xB0) //subchannel
							{
								eString subService((char *)ld->private_data, ld->priv_len);
								eString subServiceRef = "1:0:7:" + eString().sprintf("%x", ld->service_id) + ":" + eString().sprintf("%x", ld->transport_stream_id) + ":" + eString().sprintf("%x", ld->original_network_id) + ":"
									+ eString(nspace) + ":0:0:0:";
								tmp << "<service>";
								tmp << "<reference>" << subServiceRef << "</reference>";
								tmp << "<name>" << subService << "</name>";
								if (subServiceRef == curServiceRef)
									tmp << "<selected>1</selected>";
								else
									tmp << "<selected>0</selected>";
								tmp << "</service>";
							}
						}
					}
				}
				++p;
			}
			eit->unlock();
		}
	}
	tmp << "</video_channels>";
	result += tmp.str();
	
	return result;
}

static eString getServiceEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString description, ext_description, genre;
	int genreCategory = 0;
	result << std::setfill('0');

	eService* current;
	eServiceReference ref;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		<< "<?xml-stylesheet type=\"text/xsl\" href=\"/xml/serviceepg.xsl\"?>"
		<< "<service_epg>";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eString serviceRef = opt["ref"];
		ref = (serviceRef) ? string2ref(serviceRef) : sapi->service;
		current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);
		if (current)
		{
			result	<< "<service>"
				<< "<reference>" << ref2string(ref) << "</reference>"
				<< "<name>" << filter_string(current->service_name) << "</name>"
				<< "</service>";
			eServiceReferenceDVB &rref = (eServiceReferenceDVB&)ref;
			eEPGCache::getInstance()->Lock();
			const timeMap* evt = eEPGCache::getInstance()->getTimeMap(rref);

			if (evt)
			{
				timeMap::const_iterator It;
				int tsidonid = (rref.getTransportStreamID().get() << 16) | rref.getOriginalNetworkID().get();
			
				int i = 0;
				for (It = evt->begin(); It != evt->end(); ++It)
				{
					ext_description = "";
					EITEvent event(*It->second, tsidonid);
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
						<< "<duration>" << event.duration<< "</duration>"
						<< "<description>" << tmp << "</description>";
					
					eString ext_tmp = filter_string(ext_description);
					ext_tmp.strReplace("&", "&amp;");	

					result  << "<genre>" << genre << "</genre>"
						<< "<genrecategory>" << "genre" << eString().sprintf("%02d", genreCategory) << "</genrecategory>"
						<< "<start>" << event.start_time << "</start>"
						<< "<details>" << ext_tmp << "</details>"
						<< "</event>";
					i++;
				}
			}
			eEPGCache::getInstance()->Unlock();
		}
	}
	result << "</service_epg>";

	return result.str();
}

eString getTag(int mode, int submode)
{
	eString tag;
	switch(mode)
	{
		case 3: 
			tag = "movie"; 
			break;
		case 4: 
			tag = "file"; 
			break;
		default: 
			switch(submode)
			{
				case 2: tag = "satellite"; break;
				case 3: tag = "provider"; break;
				case 4: tag = "bouquet"; break;
				default: tag = "unknown"; break;
			}
	}
	return tag;
}

struct getContent: public Object
{
	int mode;
	int subm;
	eString &result;
	eServiceInterface *iface;
	bool listCont;
	getContent(int mode, int subm, const eServiceReference &service, eString &result, bool listCont)
		:mode(mode), subm(subm), result(result), iface(eServiceInterface::getInstance()), listCont(listCont)
	{
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, getContent::addToString);
		iface->enterDirectory(service, cbSignal);
		iface->leaveDirectory(service);
	}
	void addToString(const eServiceReference& ref)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		eService *service = iface ? iface->addRef(ref) : 0;
		if (!(ref.data[0] == -1 && ref.data[2] != (int)0xFFFFFFFF))
		{
		
			if (ref.flags & eServiceReference::isDirectory)
				result += "\n<" + getTag(mode, subm) + ">";
			else
				result += "\n<service>";
			
			result += "<reference>" + ref.toString() + "</reference>";

			if (ref.descr)
				result += "<name>" + filter_string(ref.descr) + "</name>";
			else
			if (service)
			{
				result += "<name>" + filter_string(service->service_name) + "</name>";
				if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
					result += "<provider>" + filter_string(((eServiceDVB*)service)->service_provider) + "</provider>";
			}

			if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
			{
				const eServiceReferenceDVB& dvb_ref = (const eServiceReferenceDVB&)ref;
				eTransponder *tp = eTransponderList::getInstance()->searchTS(
					dvb_ref.getDVBNamespace(),
					dvb_ref.getTransportStreamID(),
					dvb_ref.getOriginalNetworkID());
				if (tp && tp->satellite.isValid())
					result += "<orbital_position>" + eString().setNum(tp->satellite.orbital_position) + "</orbital_position>";
			}
		
			if (service)
				iface->removeRef(ref);
			
			if (listCont && ref.flags & eServiceReference::isDirectory)
			{
				getContent(mode, subm, ref, result, false);
				result += "\n</" + getTag(mode, subm) + ">";
			}
			else
				result += "</service>";
		}
	}
};

static eString getServices(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	/* MODE: 0 = TV, 1 = Radio, 2 = Data, 3 = Movies, 4 = Root */
	/* SUBMODE: 0 = n/a, 1 = All, 2 = Satellites, 2 = Providers, 4 = Bouquets */
	
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	std::map<eString,eString> opts = getRequestOptions(opt, '&');
	
	eString mode = "0";
	if (opts["mode"])
		mode = opts["mode"];
	int mod = atoi(mode.c_str());

	eString submode = "2";
	if (opts["submode"])
		submode = opts["submode"];
	int subm = atoi(submode.c_str());
		
	eString sref = zap[mod][subm];
	eServiceReference ref(sref);

	eString result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	result += "<" + getTag(mod, subm) + "s>";
	getContent t(mod, subm, ref, result, true);
	result += "\n</" + getTag(mod, subm) + "s>";
	result.strReplace("&", "&amp;");
//	result.strReplace("<", "&lt;");
//	result.strReplace(">", "&gt;");
//	result.strReplace("\'", "&apos;");
//	result.strReplace("\"", "&quot;");
	return result;
}

static eString getStreamInfoXSL(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	switch(eSystemInfo::getInstance()->getFEType())
	{
		case eSystemInfo::feSatellite:
			result = readFile(TEMPLATE_DIR + "streaminfo_satellite.xsl");
			break;
		case eSystemInfo::feCable:
			result = readFile(TEMPLATE_DIR + "streaminfo_cable.xsl");
			break;
		case eSystemInfo::feTerrestrial:
			result = readFile(TEMPLATE_DIR + "streaminfo_terrestrial.xsl");
			break;
	}
	
	return result;
}

static eString getServiceEPGXSL(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	return readFile(TEMPLATE_DIR + "serviceepg.xsl");
}

static eString getTimersXSL(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	return readFile(TEMPLATE_DIR + "timers.xsl");
}

static eString getStreamInfo(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result = readFile(TEMPLATE_DIR + "XMLStreaminfo.tmp");

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eServiceDVB *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if (service)
	{
		result.strReplace("#SERVICENAME#", filter_string(service->service_name));
		result.strReplace("#PROVIDER#", filter_string(service->service_provider));
	}
	else
	{
		result.strReplace("#SERVICENAME#", "n/a");
		result.strReplace("#PROVIDER#", "n/a");	
	}
	result.strReplace("#VPID#", eString().sprintf("%04xh (%dd)", Decoder::current.vpid, Decoder::current.vpid));
	result.strReplace("#APID#", eString().sprintf("%04xh (%dd)", Decoder::current.apid, Decoder::current.apid));
	result.strReplace("#PCRPID#", eString().sprintf("%04xh (%dd)", Decoder::current.pcrpid, Decoder::current.pcrpid));
	result.strReplace("#TPID#", eString().sprintf("%04xh (%dd)", Decoder::current.tpid, Decoder::current.tpid));
	result.strReplace("#TSID#", eString().sprintf("%04xh", sapi->service.getTransportStreamID().get()));
	result.strReplace("#ONID#", eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get()));
	result.strReplace("#SID#", eString().sprintf("%04xh", sapi->service.getServiceID().get()));
	result.strReplace("#PMT#", eString().sprintf("%04xh", Decoder::current.pmtpid));
	result.strReplace("#NAMESPACE#", eString().sprintf("%04xh", sapi->service.getDVBNamespace().get()));

	FILE *bitstream = 0;

	eString vidform;
	if (Decoder::current.vpid != -1)
		bitstream = fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres = 0, yres = 0, aspect = 0, framerate = 0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
			if (!strncmp(buffer, "F_RATE: ", 8))
				framerate=atoi(buffer+8);
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
		switch (framerate)
		{
			case 1:
				vidform += ", 23.976 fps"; break;
			case 2:
				vidform += ", 24 fps"; break;
			case 3:
				vidform += ", 25 fps"; break;
			case 4:
				vidform += ", 29.97 fps"; break;
			case 5:
				vidform += ", 30 fps"; break;
			case 6:
				vidform += ", 50 fps"; break;
			case 7:
				vidform += ", 59.94 fps"; break;
			case 8:
				vidform += ", 80 fps"; break;
		}
	}

	eString sRef;
	if (eServiceInterface::getInstance()->service)
		sRef = eServiceInterface::getInstance()->service.toString();
	result.strReplace("#SERVICEREFERENCE#", sRef);
	result.strReplace("#VIDEOFORMAT#", vidform);
	
	extern struct caids_t caids[];
	extern unsigned int caids_cnt;

	clearCA();
	eString cryptSystems;
	// singleLock s(eDVBServiceController::availCALock);
	std::set<int>& availCA = sapi->availableCASystems;
	for (std::set<int>::iterator i(availCA.begin()); i != availCA.end(); ++i)
	{
		eString caname = eStreaminfo::getInstance()->getCAName(*i, 0);
		if (caname)
		{
			if (cryptSystems)
				cryptSystems += ", ";
			cryptSystems += caname;
		}
	}
  	result.strReplace("#SUPPORTEDCRYPTSYSTEMS#", cryptSystems);
	
	int foundone = 0;
	cryptSystems = "";
	std::set<int>& calist = sapi->usedCASystems;
	for (std::set<int>::iterator i(calist.begin()); i != calist.end(); ++i)
	{
		eString caname = eStreaminfo::getInstance()->getCAName(*i, 1);
		eString codesys = eString().sprintf("%04xh:  ", *i) + caname;
		if (cryptSystems)
			cryptSystems += ", ";
		cryptSystems += codesys;
		foundone++;
	}
	if (!foundone)
		cryptSystems = "None";
	result.strReplace("#USEDCRYPTSYSTEMS#", cryptSystems);

	int tpData = 0;
	eTransponder *tp = sapi->transponder;
	if (tp)
	{
		switch(eSystemInfo::getInstance()->getFEType())
		{
			case eSystemInfo::feSatellite:
			{
				result.strReplace("#FRONTEND#", "DVB-S");
				for (std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin());
					tpData == 0 && it != eTransponderList::getInstance()->getLNBs().end(); it++)
					for (ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin());
						s != it->getSatelliteList().end(); s++)
						if (s->getOrbitalPosition() == tp->satellite.orbital_position) 
						{
							result.strReplace("#SATELLITE#", s->getDescription().c_str());
							tpData = 1;
							break;
						}
				if (tpData == 1)
				{
					result.strReplace("#FREQUENCY#", eString().sprintf("%d", tp->satellite.frequency / 1000));
					result.strReplace("#SYMBOLRATE#", eString().sprintf("%d", tp->satellite.symbol_rate / 1000));
					result.strReplace("#POLARISATION#", tp->satellite.polarisation ? "Vertical" : "Horizontal");
					result.strReplace("#INVERSION#", tp->satellite.inversion ? "Yes" : "No");

					switch (tp->satellite.fec)
					{
						case 0: result.strReplace("#FEC#", "Auto"); break;
						case 1: result.strReplace("#FEC#", "1/2"); break;
						case 2: result.strReplace("#FEC#", "2/3"); break;
						case 3: result.strReplace("#FEC#", "3/4"); break;
						case 4: result.strReplace("#FEC#", "5/6"); break;
						case 5: result.strReplace("#FEC#", "7/8"); break;
						case 6: result.strReplace("#FEC#", "8/9"); break;
					}

					eFrontend *fe = eFrontend::getInstance();
					int status = fe->Status();
					bool lock = status & FE_HAS_LOCK;
					bool sync = status & FE_HAS_SYNC;
					result.strReplace("#SNR#", eString().sprintf("%d%%", fe->SNR() * 100/65535));
					result.strReplace("#AGC#", eString().sprintf("%d%%", fe->SignalStrength() * 100/65535));
					result.strReplace("#BER#", eString().sprintf("%u", fe->BER()));
					result.strReplace("#LOCK#", (lock ? "Yes" : "No"));
					result.strReplace("#SYNC#", (sync ? "Yes" : "No"));
				}
				break;
			}
			case eSystemInfo::feCable:
			{
				result.strReplace("#FRONTEND#", "DVB-C");
				result.strReplace("#FREQUENCY#", eString().sprintf("%d", tp->cable.frequency / 1000));
				result.strReplace("#SYMBOLRATE#", eString().sprintf("%d", tp->cable.symbol_rate / 1000));
				result.strReplace("#INVERSION#", tp->cable.inversion ? "Yes" : "No");

				switch (tp->cable.modulation)
				{
					case 0: result.strReplace("#MODULATION#", "Auto"); break;
					case 1: result.strReplace("#MODULATION#", "16-QAM"); break;
					case 2: result.strReplace("#MODULATION#", "32-QAM"); break;
					case 3: result.strReplace("#MODULATION#", "64-QAM"); break;
					case 4: result.strReplace("#MODULATION#", "128-QAM"); break;
					case 5: result.strReplace("#MODULATION#", "256-QAM"); break;
				}

				switch (tp->cable.fec_inner)
				{
					case 0: result.strReplace("FEC#", "Auto"); break;
					case 1: result.strReplace("FEC#", "1/2"); break;
					case 2: result.strReplace("FEC#", "2/3"); break;
					case 3: result.strReplace("FEC#", "3/4"); break;
					case 4: result.strReplace("FEC#", "5/6"); break;
					case 5: result.strReplace("FEC#", "7/8"); break;
					case 6: result.strReplace("FEC#", "8/9"); break;
				}
				break;
			}
			case eSystemInfo::feTerrestrial:
			{
				result.strReplace("#FRONTEND#", "DVB-T");
				result.strReplace("#CENTERFREQUENCY#", eString().sprintf("%d",  tp->terrestrial.centre_frequency / 1000));
				result.strReplace("#INVERSION#", eString().sprintf("%d",  tp->terrestrial.inversion));
				result.strReplace("#HIERARCHYINFO#", eString().sprintf("%d",   tp->terrestrial.hierarchy_information));

				switch (tp->terrestrial.bandwidth)
				{
					case 0: result.strReplace("#BANDWIDTH#", "8"); break;
					case 1: result.strReplace("#BANDWIDTH#", "7"); break;
					case 2: result.strReplace("#BANDWIDTH#", "6"); break;
				}

				switch (tp->terrestrial.constellation)
				{
					case 0: result.strReplace("#CONSTELLATION#", "Auto"); break;
					case 1: result.strReplace("#CONSTELLATION#", "QPSK"); break;
					case 2: result.strReplace("#CONSTELLATION#", "16-QAM"); break;
					case 3: result.strReplace("#CONSTELLATION#", "64-QAM"); break;
				}

				switch (tp->terrestrial.guard_interval)
				{
					case 0: result.strReplace("#GUARDINTERVAL#", "Auto"); break;
					case 1: result.strReplace("#GUARDINTERVAL#", "1/32"); break;
					case 2: result.strReplace("#GUARDINTERVAL#", "1/16"); break;
					case 3: result.strReplace("#GUARDINTERVAL#", "1/8"); break;
					case 4: result.strReplace("#GUARDINTERVAL#", "1/4"); break;
				}

				switch (tp->terrestrial.transmission_mode)
				{
					case 0: result.strReplace("#TRANSMISSION#", "Auto"); break;
					case 1: result.strReplace("#TRANSMISSION#", "2k"); break;
					case 2: result.strReplace("#TRANSMISSION#", "8k"); break;
				}

				switch (tp->terrestrial.code_rate_lp)
				{
					case 0: result.strReplace("#CODERATELP#", "Auto"); break;
					case 1: result.strReplace("#CODERATELP#", "1/2"); break;
					case 2: result.strReplace("#CODERATELP#", "2/3"); break;
					case 3: result.strReplace("#CODERATELP#", "3/4"); break;
					case 4: result.strReplace("#CODERATELP#", "5/6"); break;
					case 5: result.strReplace("#CODERATELP#", "7/8"); break;
				}
				
				switch (tp->terrestrial.code_rate_hp)
				{
					case 0: result.strReplace("#CODERATEHP#", "Auto"); break;
					case 1: result.strReplace("#CODERATEHP#", "1/2"); break;
					case 2: result.strReplace("#CODERATEHP#", "2/3"); break;
					case 3: result.strReplace("#CODERATEHP#", "3/4"); break;
					case 4: result.strReplace("#CODERATEHP#", "5/6"); break;
					case 5: result.strReplace("#CODERATEHP#", "7/8"); break;
				}
				
				break;
			}
			default:
				result.strReplace("#FRONTEND#", "NONE");
		}
	}
	
	if (tpData == 0)
	{
		result.strReplace("#SATELLITE#", "n/a");
		result.strReplace("#FREQUENCY#", "n/a");
		result.strReplace("#SYMBOLRATE#", "n/a");
		result.strReplace("#POLARISATION#", "n/a");
		result.strReplace("#INVERSION#", "n/a");
		result.strReplace("#FEC#", "n/a");
		result.strReplace("#SNR#", "n/a");
		result.strReplace("#AGC#", "n/a");
		result.strReplace("#BER#", "n/a");
		result.strReplace("#LOCK#", "n/a");
		result.strReplace("#SYNC#", "n/a");
	}
		
	return result;
}

struct getTimer
{
	std::list<myTimerEntry> &myList;

	getTimer(std::list<myTimerEntry> &myList)
		:myList(myList)
	{
	}

	void operator()(ePlaylistEntry* se)
	{
		eString tmp = readFile(TEMPLATE_DIR + "XMLTimer.tmp");
		
		if (se->type & ePlaylistEntry::isRepeating)
			tmp.strReplace("#TYPE#", "REPEATING");
		else
			tmp.strReplace("#TYPE#", "SINGLE");
		
		tm startTime = *localtime(&se->time_begin);
		
		tmp.strReplace("#DATE#", eString().sprintf("%02d.%02d.%04d", startTime.tm_mday, startTime.tm_mon + 1, startTime.tm_year + 1900));
		tmp.strReplace("#TIME#", eString().sprintf("%02d:%02d", startTime.tm_hour, startTime.tm_min));
		tmp.strReplace("#START#", eString().sprintf("%d", se->time_begin));
		tmp.strReplace("#DURATION#", eString().sprintf("%d", se->duration));

		eString description = htmlChars(se->service.descr);
		eString channel = getLeft(description, '/');
		if (!channel)
		{
			eService *service = eDVB::getInstance()->settings->getTransponders()->searchService(se->service);
			if (service)
				channel = filter_string(service->service_name);
		}
		if (!channel)
			channel = "No channel available";

		description = getRight(description, '/');
		if (!description)
			description = "No description available";

		if (se->type & ePlaylistEntry::stateFinished)
			tmp.strReplace("#STATUS#", "FINISHED");
		else
		if (se->type & ePlaylistEntry::stateError)
			tmp.strReplace("#STATUS#", "ERROR");
		else
			tmp.strReplace("#STATUS#", "ACTIVE");

		eString days;
		if (se->type & ePlaylistEntry::isRepeating)
		{
			if (se->type & ePlaylistEntry::Su)
				days += "Su ";
			if (se->type & ePlaylistEntry::Mo)
				days += "Mo ";
			if (se->type & ePlaylistEntry::Tue)
				days += "Tue ";
			if (se->type & ePlaylistEntry::Wed)
				days += "Wed ";
			if (se->type & ePlaylistEntry::Thu)
				days += "Thu ";
			if (se->type & ePlaylistEntry::Fr)
				days += "Fr ";
			if (se->type & ePlaylistEntry::Sa)
				days += "Sa";
		}
		
		tmp.strReplace("#DAYS#", days);
		
		tmp.strReplace("#NAME#", channel);
		tmp.strReplace("#DESCRIPTION#", description);
		tmp.strReplace("#REFERENCE#", ref2string(se->service));
		
		if (se->type & ePlaylistEntry::SwitchTimerEntry)
			tmp.strReplace("#ACTION#", "ZAP");
		else
		if (se->type & ePlaylistEntry::recDVR)
			tmp.strReplace("#ACTION#", "DVR");
		else
		if (se->type & ePlaylistEntry::recNgrab)
			tmp.strReplace("#ACTION#", "NGRAB");
		else
			tmp.strReplace("#ACTION#", "NONE");

		myList.push_back(myTimerEntry(se->time_begin, tmp));
	}
};

static eString getTimers(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::stringstream result;
	std::list<myTimerEntry> myList;
	std::list<myTimerEntry>::iterator myIt;
	
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	
	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
//		<< "<?xml-stylesheet type=\"text/xsl\" href=\"/xml/timers.xsl\"?>"
		<< "<timers>";

	eTimerManager::getInstance()->forEachEntry(getTimer(myList));
	myList.sort();
	for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
		result << myIt->timerData;
		
	result << "</timers>";

	return result.str();
}

void ezapXMLInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/xml/boxstatus", getBoxStatus, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/serviceepg", getServiceEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/imageinfo", getImageInfo, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/currentservicedata", getCurrentServiceData, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/services", getServices, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/streaminfo", getStreamInfo, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/timers", getTimers, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/streaminfo.xsl", getStreamInfoXSL, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/serviceepg.xsl", getServiceEPGXSL, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/timers.xsl", getTimersXSL, lockWeb);
}


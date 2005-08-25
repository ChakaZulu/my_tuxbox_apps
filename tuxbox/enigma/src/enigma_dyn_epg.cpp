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
#include <linux/input.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <enigma.h>
#include <timer.h>
#include <enigma_main.h>
#include <enigma_plugins.h>
#include <enigma_standby.h>
#include <sselect.h>
#include <upgrade.h>
#include <math.h>

#include <lib/dvb/frontend.h>
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
#include <lib/gdi/fb.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/emessage.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/dmfp.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <epgwindow.h>
#include <streaminfo.h>
#include <enigma_dyn_epg.h>

using namespace std;

extern int pdaScreen;
extern eString getCurService(void);
extern eString getCurrentSubChannel(eString curServiceRef);

eString getServiceEPG(eString format, eString opts)
{
	eString result, result1, events;
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	result1 = readFile(TEMPLATE_DIR + format + "ServiceEPG.tmp");
		
	eString description, ext_description, genre;
	int genreCategory = 0;

	eService* current;
	eServiceReference ref;

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eString serviceRef = opt["ref"];
		ref = (serviceRef) ? string2ref(serviceRef) : sapi->service;
		current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);
		if (current)
		{
			result1.strReplace("#REFERENCE#", ref2string(ref));
			result1.strReplace("#NAME#", filter_string(current->service_name));
			
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
					
					if (format == "XML")
						result = readFile(TEMPLATE_DIR + "XMLEPGEntry.tmp");
					else
						result = readFile(TEMPLATE_DIR + "HTMLEPGEntry.tmp");
				
					result.strReplace("#NO#", eString().sprintf("%d", i));
					result.strReplace("#DATE#", eString().sprintf("%02d.%02d.%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900));
					result.strReplace("#TIME#", eString().sprintf("%02d:%02d", t->tm_hour, t->tm_min));
					result.strReplace("#DURATION#", eString().sprintf("%d", event.duration));
					eString tmp = filter_string(description);
					tmp.strReplace("&", "&amp;");
					result.strReplace("#DESCRIPTION#", tmp); 
					tmp = filter_string(ext_description);
					tmp.strReplace("&", "&amp;");	
					result.strReplace("#DETAILS#", tmp);
					result.strReplace("#GENRE#", genre);
					result.strReplace("#GENRECATEGORY#", eString().sprintf("%02d", genreCategory));
					result.strReplace("#START#", eString().sprintf("%d", event.start_time));
					result.strReplace("#REFERENCE#", ref2string(ref));
					result.strReplace("#NAME#", filter_string(current->service_name));
					
					events += result;
					i++;
				}
			}
			eEPGCache::getInstance()->Unlock();
		}
	}
	result1.strReplace("#BODY#", events);

	return result1;
}

eString getEITC(eString result)
{
	eString now_start, now_date, now_time, now_duration, now_text, now_longtext,
		next_start, next_date, next_time, next_duration, next_text, next_longtext;

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
						tm* t = localtime(&event->start_time);
						now_time.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
						now_date.sprintf("%02d.%02d.%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
						now_duration.sprintf("%d", (int)(event->duration / 60));
					}
				}
				if (p == 1)
				{
					if (event->start_time)
					{
						next_start = eString().sprintf("%d", (int)event->start_time);
						tm* t = localtime(&event->start_time);
						next_time.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
						next_date.sprintf("%02d.%02d.%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
						next_duration.sprintf("%d", (int)(event->duration / 60));
					}
				}
				LocalEventData led;
				switch (p)
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
	result.strReplace("#NOWDURATION#", now_duration);
	if (now_duration)
		now_duration = "(" + now_duration + ")";
	result.strReplace("#NOWD#", now_duration);
	result.strReplace("#NOWST#", filter_string(now_text.strReplace("\"", "'")));
	result.strReplace("#NOWLT#", filter_string(now_longtext.strReplace("\"", "'")));
	result.strReplace("#NEXTSTART#", next_start);
	result.strReplace("#NEXTT#", next_time);
	result.strReplace("#NEXTDATE#", next_date);
	result.strReplace("#NEXTDURATION#", next_duration);
	if (next_duration)
		next_duration = "(" + next_duration + ")";
	result.strReplace("#NEXTD#", next_duration);
	result.strReplace("#NEXTST#", filter_string(next_text.strReplace("\"", "'")));
	result.strReplace("#NEXTLT#", filter_string(next_longtext.strReplace("\"", "'")));

	eString curService = getCurService();
	eString curServiceRef;
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
		curServiceRef = ref2string(sapi->service);
	eString curSubService = getCurrentSubChannel(curServiceRef);
	if (curSubService)
	{
		if (curService)
			curService += ": " + curSubService;
		else
			curService = curSubService;
	}
	result.strReplace("#SERVICENAME#", curService);

	return result;
}

#define CHANNELWIDTH 200

class eMEPG: public Object
{
	int d_min;
	eString multiEPG;
	int hours;
	time_t start;
	time_t end;
	int tableWidth;
	int channelWidth;
public:
	int getTableWidth(void)
	{
		return tableWidth;
	}

	time_t adjust2FifteenMinutes(time_t seconds)
	{
		int minutes = seconds / 60;
		int quarterHours = minutes / 15;
		if (minutes % 15 > 7)
			quarterHours++;
		return quarterHours * 15 * 60;
	}

	void getcurepg(const eServiceReference &ref)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		time_t now = time(0) + eDVB::getInstance()->time_difference;

		std::stringstream result;
		result << std::setfill('0');
		eService* current;

		eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
		if (sapi)
		{
			current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);
			if (current)
			{
				eEPGCache::getInstance()->Lock();
				eServiceReferenceDVB &dref = (eServiceReferenceDVB&)ref;
				const timeMap* evt = eEPGCache::getInstance()->getTimeMap(dref);
				if (evt)
				{
					int tsidonid = (dref.getTransportStreamID().get() << 16) | dref.getOriginalNetworkID().get();
					int tablePos = 0;
					time_t tableTime = start;
					result  << "<tr>"
						<< "<td id=\"channel\" width=" << eString().sprintf("%d", channelWidth) << ">"
						<< "<span class=\"channel\">"
						<< filter_string(current->service_name)
						<< "</span>"
						<< "</td>";
					tablePos += CHANNELWIDTH;

					timeMap::const_iterator It;

					for (It = evt->begin(); It != evt->end(); ++It)
					{
						eString ext_description;
						eString short_description;
						eString genre;
						int genreCategory = 0; //none
						EITEvent event(*It->second, tsidonid);
						LocalEventData led;
						led.getLocalData(&event, &short_description, 0, &ext_description);
						for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
						{
							Descriptor *descriptor = *d;
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
											genre = gettext(eChannelInfo::getGenre(genreCategory * 16 + ce->content_nibble_level_2).c_str());
									}
								}
							}
						}

						if (!genre)
							genre = "n/a";

						time_t eventStart = adjust2FifteenMinutes(event.start_time);
						time_t eventEnd = eventStart + adjust2FifteenMinutes(event.duration);

						int eventDuration = 0;
						int colUnits = 0;
						if ((eventStart > end) || (eventEnd < tableTime))
						{
							eventDuration = 0;
						}
						else
						if ((eventStart < tableTime) && (eventEnd > tableTime))
						{
							eventDuration = eventEnd - tableTime;
						}
						else
						if (eventStart == tableTime)
						{
							eventDuration = adjust2FifteenMinutes(event.duration);
						}
						else
						if ((eventStart > tableTime) && (eventStart < end))
						{
							eventDuration = eventStart - tableTime;
							colUnits = eventDuration / 60 / 15;
							if (colUnits == 1)
							{
								eventStart = tableTime;
								eventDuration += adjust2FifteenMinutes(event.duration);
							}
							else
							{
								result << "<td colspan=" << colUnits << ">&nbsp;</td>";
								tableTime = eventStart;
								tablePos += colUnits * 15 * d_min;
								eventDuration = adjust2FifteenMinutes(event.duration);
							}
						}

						if ((eventDuration > 0) && (eventDuration < 15 * 60))
							eventDuration = 15 * 60;

						if (tableTime + eventDuration > end)
							eventDuration = end - tableTime;

						colUnits = eventDuration / 60 / 15;
						if (colUnits > 0)
						{
							result  << "<td class=\"genre"
								<< eString().sprintf("%02d", genreCategory)
								<< "\" colspan=" << colUnits << "\">";
#ifndef DISABLE_FILE
							result  << "<a href=\"javascript:record('"
								<< ref2string(ref) << "','"
								<< event.start_time << "','"
								<< event.duration << "','";
							eString tmp = filter_string(short_description);
							tmp.strReplace("\'", "\\\'");
							tmp.strReplace("\"", "\\\"");
							tmp.strReplace("&", "~");
							result  << tmp << "','"
								<< filter_string(current->service_name)
								<< "')\"><img src=\"timer.gif\" border=0></a>"
								<< "&nbsp;&nbsp;";
#endif
							tm* t = localtime(&event.start_time);
							result  << std::setfill('0')
								<< "<span class=\"time\">"
								<< std::setw(2) << t->tm_mday << '.'
								<< std::setw(2) << t->tm_mon+1 << ". - "
								<< std::setw(2) << t->tm_hour << ':'
								<< std::setw(2) << t->tm_min << ' '
								<< "</span>"
								<< "<span class=\"duration\">"
								<< " (" << event.duration / 60 << " min)"
								<< "</span>"
								<< "<br>";
							if ((eventStart <= now) && (eventEnd >= now))
								result << "<a href=\'javascript:switchChannel(\"" << ref2string(ref) << "\", \"0\", \"-1\")\'>";
							result  << "<span class=\"event\">"
								<< "<b>" << short_description << "</b>"
								<< "</span>";
							if ((eventStart <= now) && (eventEnd >= now))
								result << "</a>";

							result	<< "<br>"
								<< "Genre: " << genre
								<< "<br>";

							if ((eventDuration >= 15 * 60) && (pdaScreen == 0))
							{
								result  << "<span class=\"description\">"
									<< filter_string(ext_description)
									<< "</span>";
							}

							result  << "</td>\n";
							tablePos += colUnits * 15 * d_min;
							tableTime += eventDuration;
						}
					}
					if (tablePos < tableWidth)
						result << "<td colspan=" << (tableWidth - tablePos) / d_min / 15 << ">&nbsp;</td>";

					result << "</tr>\n";
				}
				eEPGCache::getInstance()->Unlock();

				multiEPG += result.str();
			}
		}
	}

	eMEPG(int hours, time_t start, const eServiceReference & bouquetRef, int channelWidth)
		:d_min((pdaScreen == 0) ? 5 : 3)  // distance on time scale for 1 minute
		,hours(hours)   // horizontally visible hours
		,start(start)
		,end(start + hours * 3600)
		,tableWidth((end - start) / 60 * d_min + channelWidth)
		,channelWidth((pdaScreen == 0) ? CHANNELWIDTH : CHANNELWIDTH / 2)
	{
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, eMEPG::getcurepg);
		eServiceInterface::getInstance()->enterDirectory(bouquetRef, cbSignal);
		eServiceInterface::getInstance()->leaveDirectory(bouquetRef);
	}

	eString getMultiEPG()
	{
		return multiEPG;
	}

	eString getTimeScale(int channelWidth)
	{
		std::stringstream result;

		result << "<tr>"
			<< "<th width=" << eString().sprintf("%d", channelWidth) << ">"
			<< "CHANNEL"
			<< "<br>"
			<< "<img src=\"trans.gif\" border=\"0\" height=\"1\" width=\"" << eString().sprintf("%d", channelWidth) << "\">"
			<< "</th>";

		for (time_t i = start; i < end; i += 15 * 60)
		{
			tm* t = localtime(&i);
			result << "<th width=" << d_min * 15 << ">"
				<< std::setfill('0')
				<< std::setw(2) << t->tm_mday << '.'
				<< std::setw(2) << t->tm_mon+1 << "."
				<< "<br>"
				<< std::setw(2) << t->tm_hour << ':'
				<< std::setw(2) << t->tm_min << ' '
				<< "<br>"
				<< "<img src=\"trans.gif\" border=\"0\" height=\"1\" width=\"" << eString().sprintf("%d", 15 * d_min) << "\">"
				<< "</th>";
		}
		result << "</tr>";

		return result.str();
	}
};

static eString getMultiEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString>opt = getRequestOptions(opts, '&');
	eString refs = opt["ref"];
	eServiceReference bouquetRef = string2ref(refs);
	int channelWidth = (pdaScreen == 0) ? CHANNELWIDTH : CHANNELWIDTH / 2;

	time_t start = time(0) + eDVB::getInstance()->time_difference;
	start -= ((start % 900) + (60 * 60)); // align to 15 mins & start 1 hour before now
	int hours = 24;
	eConfig::getInstance()->getKey("/elitedvb/multiepg/hours", hours); // horizontally visible hours

	eMEPG mepg(hours, start, bouquetRef, channelWidth);

	eString result = (pdaScreen == 0) ? readFile(TEMPLATE_DIR + "mepg.tmp") : readFile(TEMPLATE_DIR + "mepg_small.tmp");
	result.strReplace("#TIMESCALE#", mepg.getTimeScale(channelWidth));
	result.strReplace("#BODY#", mepg.getMultiEPG());
	return result;
}

static eString getHTMLServiceEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	return getServiceEPG("HTML", opts);
}

static eString getchannelinfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{   
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = getEITC(readFile(TEMPLATE_DIR + "eit.tmp"));
	result.strReplace("#SERVICENAME#", getCurService());
    
	return result;
}

void ezapEPGInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg", getHTMLServiceEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/getcurrentepg", getHTMLServiceEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/getMultiEPG", getMultiEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/channelinfo", getchannelinfo, lockWeb);
}


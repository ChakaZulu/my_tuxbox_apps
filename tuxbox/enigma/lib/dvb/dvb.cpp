#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <set>
#include <sstream>

#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/si.h>
#include <lib/dvb/frontend.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/dvb/servicestructure.h>

eTransponderList* eTransponderList::instance=0;

void eDiSEqC::setRotorDefaultOptions()
{
	useGotoXX=1;
	useRotorInPower=40<<8;
	DegPerSec=1.0;
	gotoXXLatitude=0.0;
	gotoXXLaDirection=eDiSEqC::NORTH;
	gotoXXLongitude=0.0;
	gotoXXLoDirection=eDiSEqC::EAST;
}

void eTransponder::cable::set(const CableDeliverySystemDescriptor *descriptor)
{
			// reject <100Mhz, >1000Mhz
	if (descriptor->frequency < 100*1000)
		return;
	if (descriptor->frequency > 1000*1000)
		return;
	frequency=descriptor->frequency;
	symbol_rate=descriptor->symbol_rate;
	modulation=descriptor->modulation;
	fec_inner=descriptor->FEC_inner;
	inversion=0;
	valid=1;
}

int eTransponder::cable::tune(eTransponder *trans)
{
	eDebug("[TUNE] tuning to %d/%d", frequency, symbol_rate);
	int inv=0;
	return eFrontend::getInstance()->tune_qam(trans, frequency, symbol_rate, fec_inner, inv, modulation);
}

void eTransponder::satellite::set(const SatelliteDeliverySystemDescriptor *descriptor)
{
	if (descriptor->frequency < 3500*1000) // reject < 3.5GHz
		return;
	frequency=descriptor->frequency;
	symbol_rate=descriptor->symbol_rate;
	polarisation=descriptor->polarisation;
	fec=descriptor->FEC_inner;
	orbital_position=descriptor->orbital_position;
	if (!descriptor->west_east_flag)
		orbital_position=-orbital_position;
//	eDebug("%d %d", descriptor->orbital_position, descriptor->west_east_flag);
	inversion=INVERSION_AUTO;
	valid=1;
}

int eTransponder::satellite::tune(eTransponder *trans)
{
	eDebug("[TUNE] tuning to %d/%d/%s/%d@%d", frequency, symbol_rate, polarisation?"V":"H", fec, orbital_position);

	std::multimap< int, eSatellite* >::iterator it ( trans->tplist.begin() );

	while ( it != trans->tplist.end() && it->second->getOrbitalPosition() != orbital_position )
		++it;
		
	if (it == trans->tplist.end())
	{
		eDebug("no such satellite %d", orbital_position);
		return -ENOENT;
	}

	return eFrontend::getInstance()->tune_qpsk(trans, frequency, polarisation, symbol_rate, fec, inversion, *it->second );
}

eService::eService(const eString &service_name, int spflags)
	: service_name(service_name), spflags(spflags), dvb(0)
#ifndef DISABLE_FILE
	, id3(0)
#endif
{
}

eService::~eService()
{
}

eServiceDVB::eServiceDVB(eDVBNamespace dvb_namespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, eServiceID service_id, int service_number ):
		eService(""), dvb_namespace(dvb_namespace), transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_id(service_id), service_number(service_number), dxflags(0)
{
	dvb=this;
	clearCache();
}

eServiceDVB::eServiceDVB(eServiceID service_id, const char *name)
	: eService(name), service_id(service_id), service_number(-1), dxflags(0)
{
	dvb=this;
	clearCache();
}

eServiceDVB::eServiceDVB(eDVBNamespace dvb_namespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, const SDTEntry *sdtentry, int service_number):
		eService(""), dvb_namespace(dvb_namespace), transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_number(service_number), dxflags(0)
{
	dvb=this;
	clearCache();
	service_id=sdtentry->service_id;
	update(sdtentry);
}

eServiceDVB::eServiceDVB(const eServiceDVB &c):
	eService(c)
{
	transport_stream_id=c.transport_stream_id;
	original_network_id=c.original_network_id;
	dvb_namespace=c.dvb_namespace;
	service_id=c.service_id;
	service_type=c.service_type;
	service_provider=c.service_provider;
	service_number=c.service_number;
	dxflags=c.dxflags;
	memcpy(cache, c.cache, sizeof(cache));
	dvb=this;
}

void eBouquet::add(const eServiceReferenceDVB &service)
{
	list.push_back(service);
}

int eBouquet::remove(const eServiceReferenceDVB &service)
{
	list.remove(service);
	return 0;
}

eTransponder::eTransponder(eTransponderList &tplist, eDVBNamespace dvb_namespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id):
	tplist(tplist), dvb_namespace(dvb_namespace), transport_stream_id(transport_stream_id), original_network_id(original_network_id)
{
	cable.valid=0;
	satellite.valid=0;
	state=stateToScan;
}

eTransponder::eTransponder(eTransponderList &tplist): tplist(tplist), dvb_namespace(-1), transport_stream_id(-1), original_network_id(-1)
{
	cable.valid=satellite.valid=0;
	state=stateToScan;
}

void eTransponder::setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int orbital_position, int inversion)
{
//	eDebug("setSatellite");
	satellite.frequency=frequency;
	satellite.symbol_rate=symbol_rate;
	satellite.polarisation=polarisation;
	satellite.fec=fec;
	satellite.orbital_position=orbital_position;
	satellite.valid=1;
	satellite.inversion=inversion;
}

void eTransponder::setCable(int frequency, int symbol_rate, int inversion, int modulation)
{
	cable.frequency=frequency;
	cable.symbol_rate=symbol_rate;
	cable.inversion=inversion;
	cable.modulation=modulation;
	cable.valid=1;
}

int eTransponder::tune()
{
	switch (eFrontend::getInstance()->Type())
	{
	case eSystemInfo::feCable:
		if (cable.isValid())
			return cable.tune(this);
		else
			return -ENOENT;
	case eSystemInfo::feSatellite:
		if (satellite.isValid())
			return satellite.tune(this);
		else
			return -ENOENT;
	default:
		return -ENOSYS;
	}
}

int eTransponder::isValid()
{
	switch (eFrontend::getInstance()->Type())
	{
	case eSystemInfo::feCable:
		return cable.isValid();
	case eSystemInfo::feSatellite:
		return satellite.isValid();
	default:
		return 0;
	}
}

void eServiceDVB::update(const SDTEntry *sdtentry)
{
	if (dxflags & dxNoDVB) // never ever update "manual" pids.
		return;
	if (eServiceID(sdtentry->service_id) != service_id)
	{
		eDebug("tried to update sid %x with sdt-sid %x", service_id.get(), sdtentry->service_id);
		return;
	}
	service_name="unknown";
	service_provider="unknown";
	service_type=0;
	service_id=sdtentry->service_id;
	for (ePtrList<Descriptor>::const_iterator d(sdtentry->descriptors); d != sdtentry->descriptors.end(); ++d)
		if (d->Tag()==DESCR_SERVICE)
		{
			const ServiceDescriptor *nd=(ServiceDescriptor*)*d;

			service_name=nd->service_name;
			if (!nd->service_provider.empty())
				service_provider=nd->service_provider;

			if (!service_name)
				service_name="unnamed service";

			service_type=nd->service_type;
		}

//	printf("%04x:%04x %02x %s", transport_stream_id, service_id, service_type, (const char*)service_name);
}

eSatellite::eSatellite(eTransponderList &tplist, int orbital_position, eLNB &lnb):
		tplist(tplist), orbital_position(orbital_position), lnb(&lnb)
{
	tpiterator=tplist.satellites.insert(std::pair<int,eSatellite*>(orbital_position,this));
}

eSatellite::~eSatellite()
{
	tplist.satellites.erase(tpiterator);
}

void eSatellite::setOrbitalPosition(int orbital_position)
{
	tplist.satellites.erase(tpiterator);		// we must renew the entry in the map in eTransponderList
	this->orbital_position=orbital_position;
	tplist.satellites.insert( std::pair< int, eSatellite*>( orbital_position, this ));
}

void eLNB::setDefaultOptions()
{
	lof_hi=10600000;
	lof_lo=9750000;
	lof_threshold=11700000;
	increased_voltage=0;
}

eSatellite *eLNB::addSatellite(int orbital_position)
{
	satellites.push_back(new eSatellite(tplist, orbital_position, *this));
	return satellites.back();
}

void eLNB::addSatellite( eSatellite *satellite)
{
	satellites.push_back(satellite);
}

eSatellite* eLNB::takeSatellite( eSatellite *satellite)
{
	satellites.take( satellite );
	return satellite;
}

void eLNB::deleteSatellite(eSatellite *satellite)
{
	satellites.remove(satellite);
}

existNetworks::existNetworks()
:networksLoaded(false), fetype( eFrontend::getInstance()->Type() )
{

}

std::list<tpPacket>& existNetworks::getNetworks()
{
	if (!networksLoaded)
	{
		reloadNetworks();
		networksLoaded=true;
	}
	return networks;
}

std::map<int,tpPacket>& existNetworks::getNetworkNameMap()
{
	if (!networksLoaded)
	{
		reloadNetworks();
		networksLoaded=true;
	}
	return names;
}

int existNetworks::saveNetworks()
{
	const char *filename=0;

	switch (fetype)
	{
	case eSystemInfo::feSatellite:
		filename="/etc/satellites.xml";
		break;
	case eSystemInfo::feCable:
		filename="/etc/cables.xml";
		break;
	default:
		break;
	}

	if (!filename)
		return -1;

	FILE *out=fopen(filename, "w+");
	if (!out)
	{
		eWarning("unable to open %s", filename);
		return -1;
	}

	std::stringstream FileText;
	FileText << "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
		"<!-- useable flags are\n"
		"1\t->\tNetwork Scan\n"
		"2\t->\tuse BAT\n"
		"4\t->\tuse ONIT\n"
		"8\t->\tskip NITs of known networks\n"
		"and combinations of this.-->\n\n"
		"<satellites>\n";
	for (std::list<tpPacket>::iterator p( networks.begin() ); p != networks.end(); ++p )
	{
		FileText << '\t' << "<sat name=\"" << p->name
						 << "\" flags=\"" << p->scanflags
						 << "\" position=\"" << p->orbital_position << "\">\n";
		for (std::list<eTransponder>::iterator t( p->possibleTransponders.begin() ); t != p->possibleTransponders.end() ; ++t )
			FileText << "\t\t<transponder frequency=\"" << t->satellite.frequency
							 << "\" symbol_rate=\"" << t->satellite.symbol_rate
							 << "\" polarization=\"" << t->satellite.polarisation
							 << "\" fec_inner=\"" << t->satellite.fec
							 << "\"/>\n";
		 FileText << "\t</sat>\n";
	}
	FileText << "</satellites>\n";

	const std::string &str = FileText.str();

	unsigned int bwritten = fwrite(str.c_str(),
											sizeof(str[0]),
											str.length(),
											out );
	fclose(out);
	return bwritten != str.length()*sizeof(str[0]);
}

int existNetworks::reloadNetworks()
{
	names.clear();
	networks.clear();
	XMLTreeParser parser("ISO-8859-1");

	int done=0;
	const char *filename=0;

	switch (fetype)
	{
	case eSystemInfo::feSatellite:
		filename="/etc/satellites.xml";
		break;
	case eSystemInfo::feCable:
		filename="/etc/cables.xml";
		break;
	default:
		break;
	}

	if (!filename)
		return -1;

	FILE *in=fopen(filename, "rt");
	if (!in)
	{
		eWarning("unable to open %s", filename);
		if ( fetype == eSystemInfo::feSatellite )
		{
			filename="/share/tuxbox/satellites.xml";
			in = fopen(filename, "rt");
			if (!in)
				return -1;
		}
		else
			return -1;
	}

	do
	{
		char buf[2048];
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser.Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser.ErrorString(parser.GetErrorCode()),
				parser.GetCurrentLineNumber());
			fclose(in);
			return -1;
		}
	} while (!done);

	fclose(in);

	XMLTreeNode *root=parser.RootNode();

	if (!root)
		return -1;

	for (XMLTreeNode *node = root->GetChild(); node; node = node->GetNext())
		if (!strcmp(node->GetType(), "cable"))
		{
			tpPacket pkt;
			if (!addNetwork(pkt, node, eSystemInfo::feCable))
				networks.push_back(pkt);
		} else if (!strcmp(node->GetType(), "sat"))
		{
			tpPacket pkt;
			if (!addNetwork(pkt, node, eSystemInfo::feSatellite))
			{
				networks.push_back(pkt);
				names[pkt.orbital_position]=networks.back();
			}
		} else
			eFatal("unknown packet %s", node->GetType());

	return 0;
}

int existNetworks::addNetwork(tpPacket &packet, XMLTreeNode *node, int type)
{
	const char *name=node->GetAttributeValue("name");
	if (!name)
	{
		eFatal("no name");
		return -1;
	}
	packet.name=name;

	const char *flags=node->GetAttributeValue("flags");
	if (flags)
	{
		packet.scanflags=atoi(flags);
//		eDebug("name = %s, scanflags = %i", name, packet.scanflags );
	}
	else
	{
		packet.scanflags=1; // default use Network ??
//		eDebug("packet has no scanflags... we use default scanflags (1)");
	}

	const char *position=node->GetAttributeValue("position");
	if (!position)
		position="0";

	int orbital_position=atoi(position);
	packet.orbital_position = orbital_position;

	for (node=node->GetChild(); node; node=node->GetNext())
	{
		eTransponder t(*eDVB::getInstance()->settings->getTransponders());
		switch (type)
		{
		case eSystemInfo::feCable:
		{
			const char *afrequency=node->GetAttributeValue("frequency"),
					*asymbol_rate=node->GetAttributeValue("symbol_rate"),
					*ainversion=node->GetAttributeValue("inversion"),
					*amodulation=node->GetAttributeValue("modulation");
			if (!afrequency)
				continue;
			if (!asymbol_rate)
				asymbol_rate="6900000";
			if (!ainversion)
				ainversion="0";
			if (!amodulation)
				amodulation="3";
			int frequency=atoi(afrequency)/*/1000*/,
					symbol_rate=atoi(asymbol_rate),
					inversion=atoi(ainversion),
					modulation=atoi(amodulation);
			t.setCable(frequency, symbol_rate, inversion, modulation );
			break;
		}
		case eSystemInfo::feSatellite:
		{
			const char *afrequency=node->GetAttributeValue("frequency"),
					*asymbol_rate=node->GetAttributeValue("symbol_rate"),
					*apolarisation=node->GetAttributeValue("polarization"),
					*afec_inner=node->GetAttributeValue("fec_inner"),
					*ainversion=node->GetAttributeValue("inversion");
			if (!afrequency)
				continue;
			if (!asymbol_rate)
				continue;
			if (!apolarisation)
				continue;
			if (!afec_inner)
				continue;
			if (!ainversion)
				ainversion="2";
			int frequency=atoi(afrequency), symbol_rate=atoi(asymbol_rate),
					polarisation=atoi(apolarisation), fec_inner=atoi(afec_inner),
					inversion=atoi(ainversion);
			t.setSatellite(frequency, symbol_rate, polarisation, fec_inner, orbital_position, inversion);
			break;
		}
		default:
			continue;
		}
		packet.possibleTransponders.push_back(t);
	}
	packet.possibleTransponders.sort();
	return 0;
}

void eTransponderList::readTimeOffsetData( const char* filename )
{
	TimeOffsetMap.clear();
	FILE *f=fopen(filename, "r");
	if (!f)
		return;
	char line[256];
	fgets(line, 256, f);
	while (true)
	{
		if (!fgets( line, 256, f ))
			break;
		if (strstr(line, "Transponder UTC Time Offsets\n"))
			continue;
		int dvbnamespace,tsid,onid,offs;
		if ( sscanf( line, "%08x,%04x,%04x:%d\n",&dvbnamespace,&tsid,&onid,&offs ) == 4 )
			TimeOffsetMap[tsref(dvbnamespace,tsid,onid)]=offs;
	}
	fclose(f);
}

void eTransponderList::writeTimeOffsetData( const char* filename )
{
	FILE *f=fopen(filename, "w+");
	if ( f )
	{
		fprintf(f, "Transponder UTC Time Offsets\n");
		for ( std::map<tsref,int>::iterator it ( TimeOffsetMap.begin() ); it != TimeOffsetMap.end(); ++it )
			fprintf(f, "%08x,%04x,%04x:%d\n",
				it->first.dvbnamespace.get(),
				it->first.tsid.get(), it->first.onid.get(), it->second );
		fclose(f);
	}
}

eTransponderList::eTransponderList()
	:curSDTEntry(ePtrList<SDTEntry>().end())
	,callback(0), pmt(0)
{
	if (!instance)
		instance = this;

/*
	eServicePlaylistHandler::getInstance()->addNum( 6 );
	newServicesRef=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 6);
	newServices=(ePlaylist*)eServiceInterface::getInstance()->addRef(newServicesRef);
	ASSERT(newServices);
	newServices->service_name=_("new services");
	newServices->load(CONFIGDIR "/enigma/newservices.epl");
*/

	CONNECT( eDVB::getInstance()->leaveTransponder, eTransponderList::leaveTransponder );
	CONNECT( eDVB::getInstance()->leaveService, eTransponderList::leaveService );
	readLNBData();
}

void eTransponderList::removeOrbitalPosition(int orbital_position)
{
		// search for all transponders on given orbital_position
	for (std::map<tsref,eTransponder>::iterator it(transponders.begin());
		it != transponders.end(); )
	{
		eTransponder &t=it->second;
				// is this transponder on the removed orbital_position ?
//		eDebug("transponder on %d, remove %d", t.satellite.orbital_position, orbital_position);
		if (t.satellite.isValid() && t.satellite.orbital_position == orbital_position)
		{
//			eDebug("found transponder to remove");
			t.satellite.valid=0;
			if (!t.cable.isValid())
			{
//				eDebug("removing transponder");
				// delete this transponder (including services)
				eTransportStreamID tsid=it->first.tsid;
				eOriginalNetworkID onid=it->first.onid;
				eDVBNamespace dvbnamespace=it->first.dvbnamespace;
				transponders.erase(it++); // remove transponder from list

				for (std::map<eServiceReferenceDVB,eServiceDVB>::iterator sit=services.begin();
					sit != services.end(); )
				{
					const eServiceReferenceDVB &ref=sit->first;
						// service on this transponder?
					if ((ref.getOriginalNetworkID() == onid) &&
							(ref.getTransportStreamID() == tsid) &&
							(ref.getDVBNamespace() == dvbnamespace))
					{
//						eDebug("removing service");
						services.erase(sit++);
					}
					else
						++sit;
				}
			}
		}
		else
			++it;
	}
}

eTransponder &eTransponderList::createTransponder(eDVBNamespace dvb_namespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id)
{
	std::map<tsref,eTransponder>::iterator i=transponders.find(tsref(dvb_namespace, transport_stream_id, original_network_id));
	if (i==transponders.end())
	{
		i=transponders.insert(
				std::pair<tsref,eTransponder>
					(tsref(dvb_namespace, transport_stream_id, original_network_id),
						eTransponder(*this, dvb_namespace, transport_stream_id, original_network_id)
					)
			).first;
		/*emit*/ transponder_added(&(*i).second);
	}
	return (*i).second;
}

eServiceDVB &eTransponderList::createService(const eServiceReferenceDVB &service, int chnum, bool *newService)
{
	std::map<eServiceReferenceDVB,eServiceDVB>::iterator i=services.find(service);
                                                  	
	if (newService)
			*newService = ( i == services.end() );

	if ( i == services.end() )
	{
/*		if (chnum==-1)
			chnum=beautifyChannelNumber(service.getTransportStreamID().get(), service.getOriginalNetworkID().get(), service.getServiceID().get()); */
		
/*		if (chnum==-1)
		{

			if (channel_number.end()==channel_number.begin())
				chnum=200;
			else
			{
				std::map<int,eServiceReferenceDVB>::iterator i=channel_number.end();
				--i;
				chnum=i->first+1;	// letzte kanalnummer +1
			}
		}	*/
		
		if (chnum == -1)
			chnum=200;

		while (channel_number.find(chnum)!=channel_number.end())
			++chnum;
	
		eServiceDVB *n=&services.insert(
					std::pair<eServiceReferenceDVB,eServiceDVB>
						(service,
							eServiceDVB(service.getDVBNamespace(),
							service.getTransportStreamID(),
							service.getOriginalNetworkID(),
							service.getServiceID(),chnum))
							).first->second;

		channel_number.insert(std::pair<int,eServiceReferenceDVB>(chnum,service));

		return *n;
	}
	return (*i).second;
}

void eTransponderList::leaveService( const eServiceReferenceDVB& )
{
	leaveTransponder(0);
}

void eTransponderList::leaveTransponder( eTransponder* )
{
	newServiceIds.clear();
	delete pmt;
	pmt=0;
	this->callback=0;
}

void eTransponderList::handleSDT(const SDT *sdt, eDVBNamespace dvbnamespace, eOriginalNetworkID onid, eTransportStreamID tsid, Signal0<void> *callback )
{
	if (onid.get() == -1)
		onid=sdt->original_network_id;
	if (tsid.get() == -1)
		tsid=sdt->transport_stream_id;

	if ( callback )
	{
		delete pmt;
		pmt=0;
		this->callback = callback;
		curSDTEntry = sdt->entries;
	}
	else if ( !this->callback )
	{
		delete pmt;
		pmt=0;
		curSDTEntry = sdt->entries;
	}

	for (; curSDTEntry != sdt->entries.end(); ++curSDTEntry )
	{
		int service_type=-1;
		for (ePtrList<Descriptor>::const_iterator d(curSDTEntry->descriptors); d != curSDTEntry->descriptors.end(); ++d)
			if (d->Tag()==DESCR_SERVICE)
			{
				const ServiceDescriptor *nd=(ServiceDescriptor*)*d;
				service_type=nd->service_type;
				break;
			}

		if (service_type == -1)
			continue;

		newService =
				eServiceReferenceDVB(
					eDVBNamespace(dvbnamespace),
					eTransportStreamID(tsid), 
					eOriginalNetworkID(onid), 
					eServiceID(curSDTEntry->service_id),
					service_type);

		if ( this->callback )  // we must check if service is scrambled..
		{
			if ( !eDVB::getInstance()->tPAT.ready() )
			{
				eDebug("PAT not ready...");
				continue;
			}
			PAT * pat = eDVB::getInstance()->tPAT.getCurrent();
			if ( !pat )
			{
				eDebug("no PAT in handleSDT.. ignore service");
				continue;
			}
			PATEntry *pe = pat->searchService(curSDTEntry->service_id);
			if ( !pe )
			{
				eDebug("no PAT Entry for service_id %04x.. ignore service", curSDTEntry->service_id);
				pat->unlock();
				continue;
			}
			int pmtpid = pe->program_map_PID;
			pat->unlock();
			delete pmt;
			pmt=new PMT( pmtpid, curSDTEntry->service_id );
			CONNECT( pmt->tableReady, eTransponderList::gotPMT );
			pmt->start();
			return;
		}

		addService();
	}

	bool changed=false;

	for (std::map<eServiceReferenceDVB,eServiceDVB>::iterator i(services.begin()); i != services.end(); ++i)
		if ((!(i->second.dxflags & eServiceDVB::dxNoDVB)) &&  // never ever touch non-dvb services
				(i->first.getOriginalNetworkID() == onid)	&& // if service on this on
				(i->first.getTransportStreamID() == tsid) && 	// and on this transponder (war das "first" hier wichtig?)
				(i->first.getDVBNamespace() == dvbnamespace) && // and in this namespace
				(newServiceIds.find(i->first.getServiceID())==newServiceIds.end())) // but does not exist
		{
				for (std::map<int,eServiceReferenceDVB>::iterator m(channel_number.begin()); m != channel_number.end(); ++m)
					if (i->first == m->second)
					{
						channel_number.erase(m);
						break;
					}
				services.erase(i->first);
				i=services.begin();
				changed=true;
		}

	newServiceIds.clear();

	if ( this->callback )  // all SDT Entrys checked ?
	{
		delete pmt;
		pmt=0;

		Signal0<void> &cb = *this->callback;

		this->callback = 0;
		/*emit*/ cb();
	}

	if ( changed )
		/* emit */ eDVB::getInstance()->serviceListChanged();
}


void eTransponderList::addService()
{
	bool newAdded;

	eServiceDVB &service=createService(newService, -1, &newAdded);
	service.update(*curSDTEntry);
	newServiceIds.insert(eServiceID(curSDTEntry->service_id));

//		eDebug("service_found, newadded = %i", newAdded);
	/*emit*/ service_found(newService, newAdded);
}

void eTransponderList::gotPMT(int err)
{
	if ( !err )
	{
		pmt->lock();

		bool scrambled=false;

		for (ePtrList<Descriptor>::const_iterator i(pmt->program_info); i != pmt->program_info.end(); ++i)
			if (i->Tag()==9)	// CADescriptor
			{
				scrambled=true;
				break;
			}

		if (!scrambled)
		{
			for (ePtrList<PMTEntry>::iterator pe(pmt->streams); pe != pmt->streams.end(); ++pe)
			{
				switch (pe->stream_type)
				{
					case 1: // ISO/IEC 11172 Video
					case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
					case 3: // ISO/IEC 11172 Audio
					case 4: // ISO/IEC 13818-3 Audio
					case 6:
						for (ePtrList<Descriptor>::const_iterator i(pe->ES_info); i != pe->ES_info.end(); ++i)
							if (i->Tag()==9)	// CADescriptor
							{
								scrambled=true;
								break;
							}
					default:
						break;
				}
				if ( scrambled )
					break;
			}
		}

		pmt->unlock();

		if (!scrambled)
			addService();
	}
	else
		eDebug("gotPMT err(%d)");

	if ( !eDVB::getInstance()->tSDT.ready() )
	{
		eDebug("SDT not ready..");
		return;
	}
	SDT *sdt = eDVB::getInstance()->tSDT.getCurrent();
	if ( sdt )
	{
		++curSDTEntry;
		handleSDT(sdt, newService.getDVBNamespace(), newService.getOriginalNetworkID(), newService.getTransportStreamID(), 0 );
		sdt->unlock();
	}
}

eTransponder *eTransponderList::searchTS(eDVBNamespace dvbnamespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id)
{
	std::map<tsref,eTransponder>::iterator i=transponders.find(tsref(dvbnamespace, transport_stream_id, original_network_id));
	if (i==transponders.end())
		return 0;
	return &i->second;
}

eServiceDVB *eTransponderList::searchService(const eServiceReference &service)
{
	if (service.type != eServiceReference::idDVB)
		return 0;
	const eServiceReferenceDVB &dvbservice=(const eServiceReferenceDVB&)service;
	std::map<eServiceReferenceDVB,eServiceDVB>::iterator i=services.find(dvbservice);
	if (i==services.end())
		return 0;
	return &i->second;
}

const eServiceReferenceDVB *eTransponderList::searchService(eDVBNamespace dvb_namespace, eOriginalNetworkID original_network_id, eServiceID service_id)
{
	for (std::map<eServiceReferenceDVB,eServiceDVB>::iterator i(services.begin()); i != services.end(); ++i)
		if ((i->first.getDVBNamespace() == dvb_namespace) &&
				(i->first.getOriginalNetworkID() == original_network_id) &&
				(i->first.getServiceID() == service_id))
					return &i->first;
	return 0;
}

eServiceReferenceDVB eTransponderList::searchServiceByNumber(int chnum)
{
	std::map<int,eServiceReferenceDVB>::iterator i=channel_number.find(chnum);
	if (i==channel_number.end())
		return eServiceReferenceDVB();
	return i->second;
}

eTransponder *eTransponderList::getFirstTransponder(int state)
{
	for (std::map<tsref,eTransponder>::iterator i(transponders.begin()); i!=transponders.end(); ++i)
		if (i->second.state==state)
			return &i->second;
	return 0;
}

eSatellite *eTransponderList::findSatellite(int orbital_position)
{
	std::multimap<int,eSatellite*>::iterator i=satellites.find(orbital_position);
	if (i == satellites.end())
		return 0;
	return i->second;
}

void eTransponderList::readLNBData()
{
	eString basepath="/elitedvb/DVB/config/lnbs/";

	int lnbread=0;
	while (1)
	{
		unsigned int tmp=0;
		int tmpint=0;
		double tmpdouble=0;

		if ( eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/lofH").c_str(), tmp) )
			break;

		lnbs.push_back(eLNB(*this));
		eLNB &lnb=lnbs.back();

		lnb.setLOFHi(tmp);

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/lofL").c_str(), tmp);
		lnb.setLOFLo(tmp);

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/lofThreshold").c_str(), tmp);
		lnb.setLOFThreshold(tmp);

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/IncreasedVoltage").c_str(), tmpint);
		lnb.setIncreasedVoltage(tmpint);
    
		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/DiSEqCMode").c_str(), tmpint );
		lnb.getDiSEqC().DiSEqCMode = (eDiSEqC::tDiSEqCMode) tmpint;

		tmpint=0;
		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/FastDiSEqC").c_str(), tmpint );
		lnb.getDiSEqC().FastDiSEqC = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/DiSEqCParam").c_str(), tmpint );
		lnb.getDiSEqC().DiSEqCParam = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/DiSEqCRepeats").c_str(), tmpint );
		lnb.getDiSEqC().DiSEqCRepeats = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/MiniDiSEqCParam").c_str(), tmpint );
		lnb.getDiSEqC().MiniDiSEqCParam = (eDiSEqC::tMiniDiSEqCParam) tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/SeqRepeat").c_str(), tmpint );
		lnb.getDiSEqC().SeqRepeat = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/SwapCmds").c_str(), tmpint );
		lnb.getDiSEqC().SwapCmds = tmpint;

		tmpint=0;
		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/uncommitted_cmd").c_str(), tmpint );
		lnb.getDiSEqC().uncommitted_cmd = tmpint;
    
		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/useGotoXX").c_str(), tmpint );
		lnb.getDiSEqC().useGotoXX = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/useRotorInPower").c_str(), tmpint );
		lnb.getDiSEqC().useRotorInPower = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/DegPerSec").c_str(), tmpdouble );
		lnb.getDiSEqC().DegPerSec = tmpdouble;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/gotoXXLoDirection").c_str(), tmpint );
		lnb.getDiSEqC().gotoXXLoDirection = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/gotoXXLaDirection").c_str(), tmpint );
		lnb.getDiSEqC().gotoXXLaDirection = tmpint;

		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/gotoXXLatitude").c_str(), tmpdouble );
		lnb.getDiSEqC().gotoXXLatitude = tmpdouble;
  
		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/gotoXXLongitude").c_str(), tmpdouble );
		lnb.getDiSEqC().gotoXXLongitude = tmpdouble;

		char* tmpStr=0;
		eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/RotorTable").c_str(), tmpStr );    

		if (tmpStr)
		{
			eString tmp = tmpStr;
			for (unsigned int i=0; i < tmp.length(); i+=7)
			{
				eString cur = tmp.mid(i);
				int x = atoi( cur.mid(0,4).c_str() );
				int y = atoi( cur.mid(4,3).c_str() );
//				eDebug("satpos = %d, pos=%d", y,x);
				lnb.getDiSEqC().RotorTable[x] = y;
			}
		}
                                                                   
		int satread=0;
		while(1)
		{
			char * descr=0;
			if ( eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/satellites/"+eString().setNum(satread)+"/OrbitalPosition").c_str(), tmpint) )
				break;  // no satellite for this lnb found

			eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/satellites/"+eString().setNum(satread)+"/description").c_str(), descr) ;
			eSatellite *sat = lnb.addSatellite(tmpint);
			sat->setDescription(descr);

			eSwitchParameter &sParams = sat->getSwitchParams();

			eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/satellites/"+eString().setNum(satread)+"/VoltageMode").c_str(), tmpint);
			sParams.VoltageMode = (eSwitchParameter::VMODE)tmpint;

			eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbread)+"/satellites/"+eString().setNum(satread)+"/HiLoSignal").c_str(), tmpint);
			sParams.HiLoSignal = (eSwitchParameter::SIG22)tmpint;

			++satread;
		}
		++lnbread;
	}

	if (lnbread<1)
	{
		eDebug("couldn't read satellite data.. use default");
		{	
			lnbs.push_back(eLNB(*this));
			eLNB &lnb=lnbs.back();
			lnb.setDefaultOptions();
			lnb.getDiSEqC().MiniDiSEqCParam=eDiSEqC::NO;
			lnb.getDiSEqC().DiSEqCParam=eDiSEqC::AA;
			lnb.getDiSEqC().DiSEqCMode=eDiSEqC::V1_0;
			lnb.getDiSEqC().DiSEqCRepeats=0;
			lnb.getDiSEqC().FastDiSEqC=0;
			lnb.getDiSEqC().SeqRepeat=0;
			lnb.getDiSEqC().SwapCmds=0;
			lnb.getDiSEqC().uncommitted_cmd=0;
			lnb.getDiSEqC().setRotorDefaultOptions();
			eSatellite *sat = lnb.addSatellite(192);
			sat->setDescription("Astra 19.2E");
			eSwitchParameter &sParams = sat->getSwitchParams();
			sParams.VoltageMode = eSwitchParameter::HV;
			sParams.HiLoSignal = eSwitchParameter::HILO;
		}
		{
			lnbs.push_back(eLNB(*this));
			eLNB &lnb=lnbs.back();
			lnb.setDefaultOptions();
			lnb.getDiSEqC().MiniDiSEqCParam=eDiSEqC::NO;
			lnb.getDiSEqC().DiSEqCParam=eDiSEqC::AB;
			lnb.getDiSEqC().DiSEqCMode=eDiSEqC::V1_0;
			lnb.getDiSEqC().DiSEqCRepeats=0;
			lnb.getDiSEqC().FastDiSEqC=0;
			lnb.getDiSEqC().SeqRepeat=0;
			lnb.getDiSEqC().SwapCmds=0;
			lnb.getDiSEqC().uncommitted_cmd=0;
			lnb.getDiSEqC().setRotorDefaultOptions();
			eSatellite *sat = lnb.addSatellite(130);
			sat->setDescription("Eutelsat 13.0E");
			eSwitchParameter &sParams = sat->getSwitchParams();
			sParams.VoltageMode = eSwitchParameter::HV;
			sParams.HiLoSignal = eSwitchParameter::HILO;
		}
	}
	eDebug("%i lnbs readed", lnbread);
}

void eTransponderList::writeLNBData()
{
	eString basepath="/elitedvb/DVB/config/lnbs/";

	int lnbwrite=0;
	for ( std::list<eLNB>::iterator it( lnbs.begin() ); it != lnbs.end(); ++it)
	{
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/lofH").c_str(), it->getLOFHi() );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/lofL").c_str(), it->getLOFLo() );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/lofThreshold").c_str(), it->getLOFThreshold() );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/IncreasedVoltage").c_str(), it->getIncreasedVoltage() );    
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/DiSEqCMode").c_str(), (int) it->getDiSEqC().DiSEqCMode );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/MiniDiSEqCParam").c_str(), (int) it->getDiSEqC().MiniDiSEqCParam );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/DiSEqCParam").c_str(), (int) it->getDiSEqC().DiSEqCParam );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/DiSEqCRepeats").c_str(), (int) it->getDiSEqC().DiSEqCRepeats );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/FastDiSEqC").c_str(), (int) it->getDiSEqC().FastDiSEqC );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/SeqRepeat").c_str(), (int) it->getDiSEqC().SeqRepeat );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/SwapCmds").c_str(), (int) it->getDiSEqC().SwapCmds );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/uncommitted_cmd").c_str(), (int) it->getDiSEqC().uncommitted_cmd );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/useGotoXX").c_str(), (int) it->getDiSEqC().useGotoXX );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/useRotorInPower").c_str(), (int) it->getDiSEqC().useRotorInPower );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/DegPerSec").c_str(), (double) it->getDiSEqC().DegPerSec );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/gotoXXLaDirection").c_str(), it->getDiSEqC().gotoXXLaDirection );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/gotoXXLoDirection").c_str(), it->getDiSEqC().gotoXXLoDirection );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/gotoXXLatitude").c_str(), it->getDiSEqC().gotoXXLatitude );
		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/gotoXXLongitude").c_str(), it->getDiSEqC().gotoXXLongitude );
		eString tmpStr;
		for ( std::map<int,int>::iterator i( it->getDiSEqC().RotorTable.begin() ); i != it->getDiSEqC().RotorTable.end(); ++i )
		{
			if ( i->first > 0 )
				tmpStr+=eString().sprintf("+%03d%03d", i->first, i->second );
			else
				tmpStr+=eString().sprintf("%04d%03d", i->first, i->second );
		}
//		eDebug("satpos %s",tmpStr.c_str());

		eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/RotorTable").c_str(), tmpStr.c_str() );
        
		int satwrite=0;
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); ++s)
		{
			eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/satellites/"+eString().setNum(satwrite)+"/OrbitalPosition").c_str(), s->getOrbitalPosition() );
			eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/satellites/"+eString().setNum(satwrite)+"/description").c_str(), s->getDescription().c_str() );
			eSwitchParameter &sParams = s->getSwitchParams();
			eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/satellites/"+eString().setNum(satwrite)+"/VoltageMode").c_str(), (int) sParams.VoltageMode );
			eConfig::getInstance()->setKey( (basepath+eString().setNum(lnbwrite)+"/satellites/"+eString().setNum(satwrite)+"/HiLoSignal").c_str(), (int) sParams.HiLoSignal );
			++satwrite;
		}
		// we must delete no more exist Satellites from registry...
		int tmp;
		while ( !eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbwrite)+"/satellites/"+eString().setNum(satwrite)+"/OrbitalPosition").c_str(), tmp ) )
		{
			eConfig::getInstance()->delKey( (basepath+eString().setNum(lnbwrite)+"/satellites/"+eString().setNum(satwrite++)).c_str() );
			eDebug("delete satellite");
		}
		///////////////////////
		++lnbwrite;
	}
	eDebug("%i LNBs written", lnbwrite);
	// we must delete no more exist lnbs from registry
	unsigned int tmp;
	while (	!eConfig::getInstance()->getKey( (basepath+eString().setNum(lnbwrite)+"/lofH").c_str(), tmp) )	// erase no more exist lnbs...
	{
		eConfig::getInstance()->delKey( (basepath+eString().setNum(lnbwrite++)).c_str() );
		eDebug("delete lnb");		
	}
	////////////////////////
	eConfig::getInstance()->flush();
}


eServiceReference::eServiceReference(const eString &string)
{
	const char *c=string.c_str();
	int pathl=-1;
	
	if ( sscanf(c, "%d:%d:%x:%x:%x:%x:%x:%x:%x:%x:%n", &type, &flags, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6], &data[7], &pathl) < 8 )
	{
		memset( data, 0, sizeof(data) );
		eDebug("find old format eServiceReference string");
		sscanf(c, "%d:%d:%x:%x:%x:%x:%n", &type, &flags, &data[0], &data[1], &data[2], &data[3], &pathl);
	}

	if (pathl)
		path=c+pathl;
}

eString eServiceReference::toString() const
{
	eString ret;
	ret+=eString().sprintf("%d:", type);
	ret+=eString().sprintf("%d", flags);
	for (unsigned int i=0; i<sizeof(data)/sizeof(*data); ++i)
		ret+=":"+eString().sprintf("%x", data[i]);
	ret+=":"+path;
	return ret;
}

void eServicePath::setString( const eString& str )
{
	// Method to build from an serialized eServicePath string a eServicePath object
	unsigned int i=0,i2;
	while( ( i2 = str.find(';', i ) ) != eString::npos )
	{
		eServiceReference e(str.mid( i, i2-i ));
		path.push_back( e );
		i=i2;
		++i;
	}
}

std::set<eServiceReference> eServiceReference::locked;
bool eServiceReference::lockedListChanged=false;

void eServiceReference::loadLockedList( const char* filename )
{
	locked.clear();
	FILE *f=fopen(filename, "r");
	if (!f)
		return;
	char line[256];
	fgets(line, 256, f);
	while (true)
	{
		if (!fgets( line, 256, f ))
			break;
		if (strstr(line, "Parentallocked Services\n"))
			continue;
		line[strlen(line)-1]=0;
		locked.insert( eServiceReference(line) );
	}
	fclose(f);
}

void eServiceReference::saveLockedList( const char* filename )
{
	FILE *f=fopen(filename, "wt");
	if (!f)
		return;
	fprintf(f, "Parentallocked Services\n");
	for ( std::set<eServiceReference>::iterator it = locked.begin(); it != locked.end(); ++it )
		fprintf( f, "%s\n", it->toString().c_str() );
	fclose(f);
}

eServicePath::eServicePath( const eString& str )
{
	setString(str);
}

eServicePath::eServicePath(const eServiceReference &ref)
{
	path.push_back(ref);
}

eString eServicePath::toString()
{
	// Method to serialize a eServicePath object to a string
	eString erg;
	if ( path.size() )
	{
		eString tmp = path.back().toString()+";";
		path.pop_back();
		erg=toString()+=tmp;
	}
	return erg;
}

bool eServicePath::up()
{
	if (path.size()>1)
		path.pop_back();
	else
		return false;
	return true;
}

void eServicePath::down(const eServiceReference &ref)
{
	path.push_back(ref);
}

eServiceReference eServicePath::current() const
{
	if (path.size())
		return path.back();
	return eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot);
}

eServiceReference eServicePath::bottom() const
{
	if (path.size())
		return path.front();
	return eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot);
}

int eServicePath::size() const
{
	return path.size();
}

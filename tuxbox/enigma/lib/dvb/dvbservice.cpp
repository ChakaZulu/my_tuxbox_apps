#include "dvbservice.h"
#include "si.h"
#include <errno.h>
#include <core/dvb/decoder.h>

eDVBServiceController::eDVBServiceController(eDVB &dvb): eDVBController(dvb)
{
	CONNECT(dvb.tPAT.tableReady, eDVBServiceController::PATready);
	CONNECT(dvb.tPMT.tableReady, eDVBServiceController::PMTready);
	CONNECT(dvb.tSDT.tableReady, eDVBServiceController::SDTready);
	CONNECT(dvb.tEIT.tableReady, eDVBServiceController::EITready);

	availableCASystems.push_back(0x1702);	// BetaCrypt C (sat)
	availableCASystems.push_back(0x1722);	// BetaCrypt D (cable)
	availableCASystems.push_back(0x1762);	// BetaCrypt F (ORF)
	
	transponder=0;
	tdt=0;
	tMHWEIT=0;
}

eDVBServiceController::~eDVBServiceController()
{
	Decoder::Flush();
}

void eDVBServiceController::handleEvent(const eDVBEvent &event)
{
#ifdef PROFILE
	static timeval last_event;
	
	timeval now;
	gettimeofday(&now, 0);
	
	int diff=(now.tv_sec-last_event.tv_sec)*1000000+(now.tv_usec-last_event.tv_usec);
	last_event=now;
	
	char *what="unknown";

	switch (event.type)
	{
	case eDVBServiceEvent::eventTunedIn: what="eventTunedIn"; break;  
	case eDVBServiceEvent::eventServiceSwitch: what="ServiceSwitch"; break;
	case eDVBServiceEvent::eventServiceTuneOK: what="TuneOK"; break;
	case eDVBServiceEvent::eventServiceTuneFailed: what="TuneFailed"; break;
	case eDVBServiceEvent::eventServiceGotPAT: what="GotPAT"; break;
	case eDVBServiceEvent::eventServiceGotPMT: what="GotPMT"; break;
	case eDVBServiceEvent::eventServiceNewPIDs: what="NewPIDs"; break;
	case eDVBServiceEvent::eventServiceGotSDT: what="GotSDT"; break;
	case eDVBServiceEvent::eventServiceSwitched: what="Switched"; break;
	case eDVBServiceEvent::eventServiceFailed: what="Failed"; break;
	default: { static char bug[100]; sprintf(bug, "%d", event.type); what=bug; }
	}
	
	eDebug("[PROFILE] [%s] +%dus", what, diff);
#endif
	switch (event.type)
	{
	case eDVBEvent::eventTunedIn:
		if (transponder==event.transponder)
			dvb.event(eDVBServiceEvent(event.err?eDVBServiceEvent::eventServiceTuneFailed: eDVBServiceEvent::eventServiceTuneOK, event.err, event.transponder));
		break;
	case eDVBServiceEvent::eventServiceSwitch:
	{
		Decoder::Flush();
		if (!dvb.settings->transponderlist)
		{
			eDebug("no tranponderlist");
			service_state=ENOENT;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
			return;
		}
		eTransponder *n=dvb.settings->transponderlist->searchTS(service.getTransportStreamID(), service.getOriginalNetworkID());
		if (!n)
		{
			eDebug("no transponder %x %x", service.getOriginalNetworkID().get(), service.getTransportStreamID().get());
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneFailed));
			break;
		}
		if (n->state!=eTransponder::stateOK)
		{
			eDebug("couldn't tune");
			service_state=ENOENT;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
			return;
		}

		if (n==transponder)
		{
//			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceTune));
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneOK));
		} else
		{
			/*emit*/ dvb.leaveTransponder(transponder);
			transponder=n;
			if (n->tune())
			{
				eDebug("tune failed");
				dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneFailed));
			} else
				dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceTune));
		}
		eDebug("<-- tuned");
		break;
	}
	case eDVBServiceEvent::eventServiceTuneOK:
		currentTransponder=event.transponder;
		currentTransponderState=event.err;

		/*emit*/ dvb.enterTransponder(event.transponder);
		dvb.tPAT.start(new PAT());
		if (tdt)
			delete tdt;
		if (tMHWEIT)
			delete tMHWEIT;
		tMHWEIT=0;
		tdt=new TDT();
		CONNECT(tdt->tableReady, eDVBServiceController::TDTready);
		tdt->start();

		/*emit*/ dvb.enterTransponder(transponder);
		dvb.tSDT.start(new SDT());
		switch (service.getServiceType())
		{
		case 1:	// digital television service
		case 2:	// digital radio service
		case 3:	// teletext service
			dvb.tEIT.start(new EIT(EIT::typeNowNext, service.getServiceID().get(), EIT::tsActual));
		case 5:	// NVOD time shifted service
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPAT));
			dvb.tPAT.get();
			break;
		case 4:	// NVOD reference service
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetSDT));
			dvb.tEIT.start(new EIT(EIT::typeNowNext, service.getServiceID().get(), EIT::tsActual));
			break;
		case 6:	// mosaic service
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPAT));
			dvb.tPAT.get();
			break;
		case -1: // data
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPAT));
			dvb.tPAT.get();
			break;
		default:
			service_state=ENOSYS;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
			break;
		}
		break;
	case eDVBServiceEvent::eventServiceTuneFailed:
		eDebug("[TUNE] tune failed");
		service_state=ENOENT;
		dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
		transponder=0;
		break;
	case eDVBServiceEvent::eventServiceGotPAT:
	{
		if (dvb.getState() != eDVBServiceState::stateServiceGetPAT)
			break;

		eDebug("eventServiceGotPAT");
		PAT *pat=dvb.tPAT.getCurrent();
		PATEntry *pe=pat->searchService(service.getServiceID().get());
		if (!pe)
		{
			pmtpid=-1;
		}	else
			pmtpid=pe->program_map_PID;
		pat->unlock();
		if (pmtpid==-1)
		{
			eDebug("[PAT] no pat entry");
			service_state=ENOENT;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
			return;
		}
		dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPMT));
		dvb.tPMT.start(new PMT(pmtpid, service.getServiceID().get()));
		break;
	}	
	case eDVBServiceEvent::eventServiceGotPMT:
		eDebug("eventServiceGotPMT");
		service_state=0;
		scanPMT();
		{
			PMT *pmt=dvb.tPMT.ready()?dvb.tPMT.getCurrent():0;
			if (pmt)
			{
				/*emit*/ dvb.gotPMT(pmt);
				pmt->unlock();
			}
		}
		if (dvb.getState()==eDVBServiceState::stateServiceGetPMT)
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceSwitched));
 		else
			eDebug("nee, doch nicht (state ist %d)", (int)dvb.getState());
		break;
	case eDVBServiceEvent::eventServiceGotSDT:
	{
		eDebug("eventServiceGotSDT");

		if (dvb.getState() != eDVBServiceState::stateServiceGetSDT)
			break;

		SDT *sdt=dvb.tSDT.ready()?dvb.tSDT.getCurrent():0;
		if (sdt)
		{
			dvb.setState(eDVBServiceState(eDVBServiceState::stateIdle));
			/*emit*/ dvb.gotSDT(sdt);
			sdt->unlock();
			if (service.getServiceType()==4)
				service_state=ENVOD;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceSwitched));
		} else
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
		break;
	}
	case eDVBServiceEvent::eventServiceNewPIDs:
		Decoder::Set();
		break;
	case eDVBServiceEvent::eventServiceSwitched:
		/*emit*/ dvb.enterService(service);
	case eDVBServiceEvent::eventServiceFailed:
		/*emit*/ dvb.switchedService(service, -service_state);
		dvb.setState(eDVBServiceState(eDVBServiceState::stateIdle));
		break;
	}
}

void eDVBServiceController::PATready(int error)
{
	dvb.event(eDVBServiceEvent(error?eDVBServiceEvent::eventServiceFailed:eDVBServiceEvent::eventServiceGotPAT));
}

void eDVBServiceController::SDTready(int error)
{
	dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceGotSDT));
	if (dvb.settings->transponderlist)
	{
		SDT *sdt=dvb.tSDT.ready()?dvb.tSDT.getCurrent():0;
		if (sdt)
		{
			if (dvb.settings->transponderlist->handleSDT(sdt))
				dvb.serviceListChanged();

			sdt->unlock();
		}
	}
}

void eDVBServiceController::PMTready(int error)
{
	dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceGotPMT));
}

void eDVBServiceController::EITready(int error)
{
	eDebug("EITready %s", strerror(-error));
	if (!error)
	{
		EIT *eit=dvb.tEIT.getCurrent();
		/*emit*/ dvb.gotEIT(eit, 0);
		eit->unlock();
	} else
		/*emit*/ dvb.gotEIT(0, error);
}

void eDVBServiceController::TDTready(int error)
{
	eDebug("TDTready %d", error);
	if (!error)
	{
		eDebug("[TIME] time update to %s", ctime(&tdt->UTC_time));
		dvb.time_difference=tdt->UTC_time-time(0);
		/*emit*/ dvb.timeUpdated();
	}
}

int eDVBServiceController::switchService(const eServiceReferenceDVB &newservice)
{
	if (newservice == service)
	{
		eDebug("is same service..");
		return 0;
	}
	
	/*emit*/ dvb.leaveService(service);
	
	service=newservice;
	
	if (service)
		dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceSwitch));
	return 1;
}

void eDVBServiceController::scanPMT()
{
	PMT *pmt=dvb.tPMT.ready()?dvb.tPMT.getCurrent():0;
	if (!pmt)
	{
		eDebug("scanPMT with no available pmt");
		return;
	}
	Decoder::parms.pmtpid=pmtpid;
	Decoder::parms.pcrpid=pmt->PCR_PID;
	Decoder::parms.ecmpid=Decoder::parms.emmpid=Decoder::parms.casystemid=-1;
	Decoder::parms.vpid=Decoder::parms.apid=-1;
	
	int isca=0;
	
	calist.clear();
	Decoder::parms.descriptor_length=0;

	isca+=checkCA(calist, pmt->program_info);
	
	PMTEntry *audio=0, *video=0, *teletext=0;
	
	for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;
		switch (pe->stream_type)
		{
		case 1:	// ISO/IEC 11172 Video
		case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
			isca+=checkCA(calist, pe->ES_info);
			if (!video)
				video=pe;
			break;
		case 3:	// ISO/IEC 11172 Audio
		case 4: // ISO/IEC 13818-3 Audio
			isca+=checkCA(calist, pe->ES_info);
			if (!audio)
				audio=pe;
			break;
		case 6:
		{
			isca+=checkCA(calist, pe->ES_info);
			for (ePtrList<Descriptor>::iterator i(pe->ES_info); i != pe->ES_info.end(); ++i)
			{
				/* if ((i->Tag()==DESCR_AC3))
					audio=pe; */
				if (i->Tag()==DESCR_TELETEXT)
					teletext=pe;
			}
			break;
		}
		case 0xC1:
		{
			if (tMHWEIT)	// nur eine zur zeit
				delete tMHWEIT;
			tMHWEIT=0;
			for (ePtrList<Descriptor>::iterator i(pe->ES_info); i != pe->ES_info.end(); ++i)
				if (i->Tag()==DESCR_MHW_DATA)
				{
					MHWDataDescriptor *mhwd=(MHWDataDescriptor*)*i;
					if (!strncmp(mhwd->type, "PILOTE", 6))
					{
						eDebug("starting MHWEIT on pid %x, sid %x", pe->elementary_PID, service.getServiceID().get());
						tMHWEIT=new MHWEIT(pe->elementary_PID, service.getServiceID().get());
						CONNECT(tMHWEIT->ready, eDVBServiceController::MHWEITready);
						tMHWEIT->start();
						break;
					}
				}
			break;
		}
		}
	}

	setPID(video);
	setPID(audio);
	setPID(teletext);

	/*emit*/ dvb.scrambled(isca);

	if (isca && !calist)
	{
		eDebug("NO CASYS");
		service_state=ENOCASYS;
	}

	if ((Decoder::parms.vpid==-1) && (Decoder::parms.apid==-1))
		service_state=ENOSTREAM;

	for (ePtrList<CA>::iterator i(calist); i != calist.end(); ++i)
	{
		eDebug("CA %04x ECMPID %04x", i->casysid, i->ecmpid);
	}

	pmt->unlock();
	setDecoder();
}


void eDVBServiceController::setPID(const PMTEntry *entry)
{
	if (entry)
	{
		int isvideo=0, isaudio=0, isteletext=0, isAC3=0;
		switch (entry->stream_type)
		{
			case 1:	// ISO/IEC 11172 Video
			case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
				isvideo=1;
			break;
			case 3:	// ISO/IEC 11172 Audio
			case 4: // ISO/IEC 13818-3 Audio
				isaudio=1;
			break;
			case 6:
			{
				for (ePtrList<Descriptor>::const_iterator i(entry->ES_info); i != entry->ES_info.end(); ++i)
				{
					if (i->Tag()==DESCR_AC3)
					{
						isaudio=1;
						isAC3=1;
					}
					if (i->Tag()==DESCR_TELETEXT)
						isteletext=1;
				}
			}
		}
		if (isaudio)
		{
			Decoder::parms.audio_type=isAC3?DECODE_AUDIO_AC3:DECODE_AUDIO_MPEG;
			Decoder::parms.apid=entry->elementary_PID;
		}
		if (isvideo)
			Decoder::parms.vpid=entry->elementary_PID;
		if (isteletext)
			Decoder::parms.tpid=entry->elementary_PID;
	}
}

void eDVBServiceController::setDecoder()
{
	dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceNewPIDs));
}

void eDVBServiceController::MHWEITready(int error)
{
	if (!error)
	{
		EIT *e=new EIT();
		e->ts=EIT::tsFaked;
		e->type=EIT::typeNowNext;
		e->version_number=0;
		e->current_next_indicator=0;
		e->transport_stream_id=service.getTransportStreamID().get();
		e->original_network_id=service.getOriginalNetworkID().get();
		
		for (int i=0; i<2; i++)
		{
			MHWEITEvent *me=&tMHWEIT->events[i];
			EITEvent *ev=new EITEvent;
			int thisday=dvb.time_difference+time(0);
			thisday-=thisday%(60*60*24);
			if (thisday < (dvb.time_difference+time(0)))
				thisday+=60*60*24;
			e->service_id=me->service_id;
			ev->event_id=0xFFFF;
			ev->start_time=thisday+(me->starttime>>8)*60*60+(me->starttime&0xFF)*60;
			ev->duration=(me->duration>>8)*60*60+(me->duration&0xFF)*60;
			ev->running_status=1;
			ev->free_CA_mode=0;
			ShortEventDescriptor *se=new ShortEventDescriptor();
			se->language_code[0]='?';
			se->language_code[1]='?';
			se->language_code[2]='?';
			se->event_name=me->event_name;
			se->text=me->short_description;
			ev->descriptor.push_back(se);
			e->events.push_back(ev);
		}
		e->ready=1;
		dvb.tEIT.inject(e);
	} else
	{
		delete tMHWEIT;
		tMHWEIT=0;
	}
}

int eDVBServiceController::checkCA(ePtrList<CA> &list, const ePtrList<Descriptor> &descriptors)
{
	int found=0;
	for (ePtrList<Descriptor>::const_iterator i(descriptors); i != descriptors.end(); ++i)
	{
		if (i->Tag()==9)	// CADescriptor
		{
			found++;
			CADescriptor *ca=(CADescriptor*)*i;
			Decoder::addCADescriptor((__u8*)(ca->data));
			int avail=0;
			for (std::list<int>::iterator i = availableCASystems.begin(); i != availableCASystems.end() && !avail; i++)
				if (*i == ca->CA_system_ID)
					avail++;

			if (avail)
			{
				for (ePtrList<CA>::iterator a = list.begin(); a != list.end(); a++)
					if (a->casysid==ca->CA_system_ID)
					{
						avail=0;
						break;
					}
				if (avail)
				{
					CA *n=new CA;
					n->ecmpid=ca->CA_PID;
					n->casysid=ca->CA_system_ID;
					n->emmpid=-1;					
					list.push_back(n);
				}
			}
		}
	}
	return found;
}


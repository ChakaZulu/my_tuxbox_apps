#include <lib/dvb/epgcache.h>

#undef NVOD
#undef EPG_DEBUG  

#include <time.h>
#include <unistd.h>  // for usleep
#include <sys/vfs.h> // for statfs
#include <libmd5sum.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/dvb/lowlevel/dvb.h>
#include <lib/dvb/si.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>

int eventData::CacheSize=0;

eEPGCache *eEPGCache::instance;
pthread_mutex_t eEPGCache::cache_lock=
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

descriptorMap eventData::descriptors;

static const __u16 crc_ccitt_table[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

eventData::eventData(const eit_event_struct* e, int size, int type)
	:ByteSize(size&0xFF), type(type&0xFF)
{
	if (!e)
		return;

	__u32 descr[65];
	__u32 *pdescr=descr;

	__u8 *data = (__u8*)e;
	int ptr=10;
	int descriptors_length = (data[ptr++]&0x0F) << 8;
	descriptors_length |= data[ptr++];
	while ( descriptors_length > 0 )
	{
		__u16 crc=0;
		__u32 hash = (data[ptr++] << 24);
		int descr_len = data[ptr++];
		hash |= (descr_len << 16);

		__u8 *descr = data+ptr;
		int cnt=0;
		while(cnt++ < descr_len)
			crc = (crc >> 8) ^ crc_ccitt_table[(crc ^ data[ptr++]) & 0xFF];
		hash |= crc;

		descriptorMap::iterator it =
			descriptors.find(hash);
		if ( it == descriptors.end() )
		{
			CacheSize+=descr_len;
			__u8 *d = new __u8[descr_len];
			memcpy(d, descr, descr_len);
			descriptors[hash] = descriptorPair(1, d);
		}
		else
			++it->second.first;
		*pdescr++=hash;
		descriptors_length -= (descr_len+2);
	}
	ByteSize = 12+((pdescr-descr)*4);
	EITdata = new __u8[ByteSize];
	CacheSize+=ByteSize;
	memcpy(EITdata, (__u8*) e, 12);
	memcpy(EITdata+12, descr, ByteSize-12);
}

const eit_event_struct* eventData::get() const
{
	static __u8 *data=NULL;
	if ( data )
		delete [] data;

	int bytes = 12;
	int tmp = ByteSize-12;

	descriptorMap::iterator descr[tmp/4];
	descriptorMap::iterator *pdescr = descr;

	// cnt needed bytes
	__u32 *p = (__u32*)(EITdata+12);
	while(tmp>0)
	{
		descriptorMap::iterator it =
			descriptors.find(*p++);
		if ( it != descriptors.end() )
		{
			*pdescr++=it;
			bytes += (it->first>>16)&0xFF;
			bytes += 2; // descr_type, descr_len
		}
		tmp-=4;
	}

// copy eit_event header to buffer
	data = new __u8[bytes];
	memcpy(data, EITdata, 12);

	tmp=12;
// copy all descriptors to buffer
	descriptorMap::iterator *d = descr;
	while(d < pdescr)
	{
		const descriptorMap::iterator &i = *d++;
		const __u32 &hash = i->first;
		data[tmp++]=(hash>>24)&0xFF;
		data[tmp++]=(hash>>16)&0xFF;
		memcpy(data+tmp, i->second.second, data[tmp-1]);
		tmp+=data[tmp-1];
	}

	return (const eit_event_struct*)data;
}

eventData::~eventData()
{
	if ( ByteSize )
	{
		CacheSize-=ByteSize;
		ByteSize-=12;
		__u32 *d = (__u32*)(EITdata+12);
		while(ByteSize)
		{
			descriptorMap::iterator it =
				descriptors.find(*d++);
			if ( it != descriptors.end() )
			{
				descriptorPair &p = it->second;
				if (!--p.first) // no more used descriptor
				{
					CacheSize -= (it->first>>16)&0xFF;
					delete [] p.second;  	// free descriptor memory
					descriptors.erase(it);	// remove entry from descriptor map
				}
			}
			ByteSize-=4;
		}
		delete [] EITdata;
	}
}

void eventData::load(FILE *f)
{
	int size=0;
	int id=0;
	descriptorPair p;
	fread(&size, sizeof(int), 1, f);
	eDebug("read %d descriptors", size );
	while(size)
	{
		fread(&id, sizeof(__u32), 1, f);
		fread(&p.first, sizeof(__u16), 1, f);
		int bytes = (id>>16)&0xFF;
		p.second = new __u8[bytes];
		fread(p.second, bytes, 1, f);
		descriptors[id]=p;
		--size;
		CacheSize+=bytes;
	}
}

void eventData::save(FILE *f)
{
	int size=descriptors.size();
	eDebug("save %d descriptors", size );
	descriptorMap::iterator it(descriptors.begin());
	fwrite(&size, sizeof(int), 1, f);
	while(size)
	{
		fwrite(&it->first, sizeof(__u32), 1, f);
		fwrite(&it->second.first, sizeof(__u16), 1, f);
		fwrite(it->second.second, (it->first>>16)&0xFF, 1, f);
		++it;
		--size;
	}
}

eEPGCache::eEPGCache()
	:messages(this,1), paused(0), firstStart(1)
	,CleanTimer(this), zapTimer(this), abortTimer(this)
{
	eDebug("[EPGC] Initialized EPGCache");
	isRunning=0;
	scheduleReader.setContext(this);
	scheduleOtherReader.setContext(this);
	nownextReader.setContext(this);
#ifdef ENABLE_PRIVATE_EPG
	contentReader.setContext(this);
	CONNECT(eDVB::getInstance()->gotContentPid, eEPGCache::setContentPid);
#endif
	CONNECT(messages.recv_msg, eEPGCache::gotMessage);
	CONNECT(eDVB::getInstance()->switchedService, eEPGCache::enterService);
	CONNECT(eDVB::getInstance()->leaveService, eEPGCache::leaveService);
	CONNECT(eDVB::getInstance()->timeUpdated, eEPGCache::timeUpdated);
	CONNECT(zapTimer.timeout, eEPGCache::startEPG);
	CONNECT(CleanTimer.timeout, eEPGCache::cleanLoop);
	CONNECT(abortTimer.timeout, eEPGCache::abortNonAvail);
	instance=this;
}

void eEPGCache::timeUpdated()
{
	if ( !thread_running() )
	{
		eDebug("[EPGC] time updated.. start EPG Mainloop");
		run();
		if ( cached_service )
			enterService(cached_service, cached_err);
	}
	else
		messages.send(Message(Message::timeChanged));
}

#ifdef ENABLE_PRIVATE_EPG

struct date_time
{
	__u8 data[5];
	time_t tm;
	date_time( const date_time &a )
	{
		memcpy(data, a.data, 5);
		tm = a.tm;
	}
	date_time( const __u8 data[5])
	{
		memcpy(this->data, data, 5);
		tm = parseDVBtime(data[0], data[1], data[2], data[3], data[4]);
	}
	date_time()
	{
	}
	const __u8& operator[](int pos) const
	{
		return data[pos];
	}
};

struct less_datetime
{
	bool operator()( const date_time &a, const date_time &b ) const
	{
		return abs(a.tm-b.tm) < 360 ? false : a.tm < b.tm;
	}
};

int ePrivateContent::sectionRead(__u8 *data)
{
	eEPGCache &instance = *eEPGCache::getInstance();
	uniqueEPGKey &current_service = instance.current_service;
	contentMap &content_time_table = instance.content_time_tables[current_service];
	singleLock s(instance.cache_lock);
	if ( instance.paused )
		return 0;
	if ( seenSections.find( data[6] ) == seenSections.end() )
	{
		std::map< date_time, std::list<uniqueEPGKey>, less_datetime > start_times;
		eventCache &eventDB = instance.eventDB;
		eventMap &evMap = eventDB[current_service].first;
		timeMap &tmMap = eventDB[current_service].second;
		int ptr=8;
		int content_id = data[ptr++] << 24;
		content_id |= data[ptr++] << 16;
		content_id |= data[ptr++] << 8;
		content_id |= data[ptr++];

		contentTimeMap &time_event_map =
			content_time_table[content_id];
		for ( contentTimeMap::iterator it( time_event_map.begin() );
			it != time_event_map.end(); ++it )
		{
			eventMap::iterator evIt( evMap.find(it->second.second) );
			if ( evIt != evMap.end() )
			{
				delete evIt->second;
				evMap.erase(evIt);
			}
			tmMap.erase(it->second.first);
		}
		time_event_map.clear();

		__u8 duration[3];
		memcpy(duration, data+ptr, 3);
		ptr+=3;
		int duration_sec =
			fromBCD(duration[0])*3600+fromBCD(duration[1])*60+fromBCD(duration[2]);

		__u8 *descriptors[65];
		__u8 **pdescr = descriptors;

		int descriptors_length = (data[ptr++]&0x0F) << 8;
		descriptors_length |= data[ptr++];
		while ( descriptors_length > 0 )
		{
			int descr_type = data[ptr];
			int descr_len = data[ptr+1];
			descriptors_length -= (descr_len+2);
			if ( descr_type == 0xf2 )
			{
				ptr+=2;
				int tsid = data[ptr++] << 8;
				tsid |= data[ptr++];
				int onid = data[ptr++] << 8;
				onid |= data[ptr++];
				int sid = data[ptr++] << 8;
				sid |= data[ptr++];
				uniqueEPGKey service( sid, onid, tsid );
				descr_len -= 6;
				while( descr_len > 0 )
				{
					__u8 datetime[5];
					datetime[0] = data[ptr++];
					datetime[1] = data[ptr++];
					int tmp_len = data[ptr++];
					descr_len -= 3;
					while( tmp_len > 0 )
					{
						memcpy(datetime+2, data+ptr, 3);
						ptr+=3;
						descr_len -= 3;
						tmp_len -= 3;
						start_times[datetime].push_back(service);
					}
				}
			}
			else
			{
				*pdescr++=data+ptr;
				ptr += 2;
				ptr += descr_len;
			}
		}
		__u8 event[4098];
		eit_event_struct *ev_struct = (eit_event_struct*) event;
		ev_struct->running_status = 0;
		ev_struct->free_CA_mode = 1;
		memcpy(event+7, duration, 3);
		ptr = 12;
		__u8 **d=descriptors;
		while ( d < pdescr )
		{
			memcpy(event+ptr, *d, ((*d)[1])+2);
			ptr+=(*d++)[1];
			ptr+=2;
		}
		for ( std::map< date_time, std::list<uniqueEPGKey> >::iterator it(start_times.begin()); it != start_times.end(); ++it )
		{
			time_t now = time(0)+eDVB::getInstance()->time_difference;

			if ( (it->first.tm + duration_sec) < now )
				continue;

			memcpy(event+2, it->first.data, 5);
			int bptr = ptr;
			int cnt=0;
			for (std::list<uniqueEPGKey>::iterator i(it->second.begin()); i != it->second.end(); ++i)
			{
				event[bptr++] = 0x4A;
				__u8 *len = event+(bptr++);
				event[bptr++] = (i->tsid & 0xFF00) >> 8;
				event[bptr++] = (i->tsid & 0xFF);
				event[bptr++] = (i->onid & 0xFF00) >> 8;
				event[bptr++] = (i->onid & 0xFF);
				event[bptr++] = (i->sid & 0xFF00) >> 8;
				event[bptr++] = (i->sid & 0xFF);
				event[bptr++] = 0xB0;
				bptr += sprintf((char*)(event+bptr), "Option %d", ++cnt);
				*len = ((event+bptr) - len)-1;
			}
			int llen = bptr - 12;
			ev_struct->descriptors_loop_length_hi = (llen & 0xF00) >> 8;
			ev_struct->descriptors_loop_length_lo = (llen & 0xFF);

			time_t stime = it->first.tm;
			while( tmMap.find(stime) != tmMap.end() )
				++stime;
			event[6] += (stime - it->first.tm);
			__u16 event_id = 0;
			while( evMap.find(event_id) != evMap.end() )
				++event_id;
			event[0] = (event_id & 0xFF00) >> 8;
			event[1] = (event_id & 0xFF);
			time_event_map[it->first.tm]=std::pair<time_t, __u16>(stime, event_id);
			eventData *d = new eventData( ev_struct, bptr, eEPGCache::SCHEDULE );
			evMap[event_id] = d;
			tmMap[stime] = d;
		}
		seenSections.insert(data[6]);
	}
	if ( seenSections.size() == (unsigned int)(data[7] + 1) )
	{
		if (!instance.eventDB[current_service].first.empty())
		{
			/*emit*/ instance.EPGAvail(1);
			instance.temp[current_service] =
				std::pair<time_t, int>(time(0)+eDVB::getInstance()->time_difference, eEPGCache::NOWNEXT);
			/*emit*/ instance.EPGUpdated();
		}
		int version = data[5];
		if ( eSystemInfo::getInstance()->hasNegFilter() )
			version = ((version & 0x3E) >> 1);
		else
			version = (version&0xC1)|((version+2)&0x3E);
		start_filter(pid,version);
	}
	return 0;
}
#endif // ENABLE_PRIVATE_EPG

int eEPGCache::sectionRead(__u8 *data, int source)
{
	if ( !data || state > 1 )
		return -ECANCELED;

	eit_t *eit = (eit_t*) data;
	bool seen=false;
	tidMap &seenSections = this->seenSections[source];
	tidMap &calcedSections = this->calcedSections[source];

	__u32 sectionNo = data[0] << 24;
	sectionNo |= data[3] << 16;
	sectionNo |= data[4] << 8;
	sectionNo |= eit->section_number;

	tidMap::iterator it =
		seenSections.find(sectionNo);

	if ( it != seenSections.end() )
		seen=true;
	else
	{
		seenSections.insert(sectionNo);
		calcedSections.insert(sectionNo);
		__u32 tmpval = sectionNo & 0xFFFFFF00;
		__u8 incr = source == NOWNEXT ? 1 : 8;
		for ( int i = 0; i <= eit->last_section_number; i+=incr )
		{
			if ( i == eit->section_number )
			{
				for (int x=i; x <= eit->segment_last_section_number; ++x)
					calcedSections.insert(tmpval|(x&0xFF));
			}
			else
				calcedSections.insert(tmpval|(i&0xFF));
		}
	}

	int len=HILO(eit->section_length)-1;//+3-4;
	int ptr=EIT_SIZE;
	if ( ptr < len && !seen )
	{
		// This fixed the EPG on the Multichoice irdeto systems
		// the EIT packet is non-compliant.. their EIT packet stinks
		if ( data[ptr-1] < 0x40 )
			--ptr;

		uniqueEPGKey service( HILO(eit->service_id), HILO(eit->original_network_id), HILO(eit->transport_stream_id) );

		eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
		int eit_event_size;
		int duration;

		time_t TM = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4, eit_event->start_time_5);
		time_t now = time(0)+eDVB::getInstance()->time_difference;

		if ( TM != 3599 && TM > -1)
		{
			switch(source)
			{
			case NOWNEXT:
				haveData |= 2;
				break;
			case SCHEDULE:
				haveData |= 1;
				break;
			case SCHEDULE_OTHER:
				haveData |= 4;
				break;
			}
		}
	
		Lock();
		// hier wird immer eine eventMap zur�ck gegeben.. entweder eine vorhandene..
		// oder eine durch [] erzeugte
		std::pair<eventMap,timeMap> &servicemap = eventDB[service];
		eventMap::iterator prevEventIt = servicemap.first.end();
		timeMap::iterator prevTimeIt = servicemap.second.end();
	
		while (ptr<len)
		{
			eit_event_size = HILO(eit_event->descriptors_loop_length)+EIT_LOOP_SIZE;
	
			duration = fromBCD(eit_event->duration_1)*3600+fromBCD(eit_event->duration_2)*60+fromBCD(eit_event->duration_3);
			TM = parseDVBtime(
				eit_event->start_time_1,
				eit_event->start_time_2,
				eit_event->start_time_3,
				eit_event->start_time_4,
				eit_event->start_time_5);
	
#ifndef NVOD
			if ( TM == 3599 )
				goto next;
#endif
	
			if ( TM != 3599 && (TM+duration < now || TM > now+14*24*60*60) )
				goto next;

			if ( now <= (TM+duration) || TM == 3599 /*NVOD Service*/ )  // old events should not be cached
			{
#ifdef NVOD
				// look in 1st descriptor tag.. i hope all time shifted events have the
				// time shifted event descriptor at the begin of the descriptors..
				if ( ((unsigned char*)eit_event)[12] == 0x4F ) // TIME_SHIFTED_EVENT_DESCR
				{
					// get Service ID of NVOD Service ( parent )
					int sid = ((unsigned char*)eit_event)[14] << 8 | ((unsigned char*)eit_event)[15];
					uniqueEPGKey parent( sid, HILO(eit->original_network_id), HILO(eit->transport_stream_id) );
					// check that this nvod reference currently is not in the NVOD List
					std::list<NVODReferenceEntry>::iterator it( NVOD[parent].begin() );
					for ( ; it != NVOD[parent].end(); it++ )
						if ( it->service_id == HILO(eit->service_id) && it->original_network_id == HILO( eit->original_network_id ) )
							break;
					if ( it == NVOD[parent].end() )  // not in list ?
						NVOD[parent].push_back( NVODReferenceEntry( HILO(eit->transport_stream_id), HILO(eit->original_network_id), HILO(eit->service_id) ) );
				}
#endif
				__u16 event_id = HILO(eit_event->event_id);
//				eDebug("event_id is %d sid is %04x", event_id, service.sid);

				eventData *evt = 0;
				int ev_erase_count = 0;
				int tm_erase_count = 0;

// search in eventmap
				eventMap::iterator ev_it =
					servicemap.first.find(event_id);

				// entry with this event_id is already exist ?
				if ( ev_it != servicemap.first.end() )
				{
					if ( source > ev_it->second->type )  // update needed ?
						goto next; // when not.. the skip this entry
// search this event in timemap
					timeMap::iterator tm_it_tmp =
						servicemap.second.find(ev_it->second->getStartTime());

					if ( tm_it_tmp != servicemap.second.end() )
					{
						if ( tm_it_tmp->first == TM ) // correct eventData
						{
							// exempt memory
							delete ev_it->second;
							evt = new eventData(eit_event, eit_event_size, source);
							ev_it->second=evt;
							tm_it_tmp->second=evt;
							goto next;
						}
						else
						{
							tm_erase_count++;
							// delete the found record from timemap
							servicemap.second.erase(tm_it_tmp);
							prevTimeIt=servicemap.second.end();
						}
					}
				}

// search in timemap, for check of a case if new time has coincided with time of other event
// or event was is not found in eventmap
				timeMap::iterator tm_it =
					servicemap.second.find(TM);

				if ( tm_it != servicemap.second.end() )
				{

// i think, if event is not found on eventmap, but found on timemap updating nevertheless demands
#if 0
					if ( source > tm_it->second->type && tm_erase_count == 0 ) // update needed ?
						goto next; // when not.. the skip this entry
#endif

// search this time in eventmap
					eventMap::iterator ev_it_tmp =
						servicemap.first.find(tm_it->second->getEventID());
	
					if ( ev_it_tmp != servicemap.first.end() )
					{
						ev_erase_count++;
						// delete the found record from eventmap
						servicemap.first.erase(ev_it_tmp);
						prevEventIt=servicemap.first.end();
					}
				}

				evt = new eventData(eit_event, eit_event_size, source);
#if EPG_DEBUG
				bool consistencyCheck=true;
#endif
				if (ev_erase_count > 0 && tm_erase_count > 0) // 2 different pairs have been removed
				{
					// exempt memory
					delete ev_it->second;
					delete tm_it->second;
					ev_it->second=evt;
					tm_it->second=evt;
				}
				else if (ev_erase_count == 0 && tm_erase_count > 0)
				{
					// exempt memory
					delete ev_it->second;
					tm_it=prevTimeIt=servicemap.second.insert( prevTimeIt, std::pair<const time_t, eventData*>( TM, evt ) );
					ev_it->second=evt;
				}
				else if (ev_erase_count > 0 && tm_erase_count == 0)
				{
					// exempt memory
					delete tm_it->second;
					ev_it=prevEventIt=servicemap.first.insert( prevEventIt, std::pair<const __u16, eventData*>( event_id, evt) );
					tm_it->second=evt;
				}
				else // added new eventData
				{
#if EPG_DEBUG
					consistencyCheck=false;
#endif
					prevEventIt=servicemap.first.insert( prevEventIt, std::pair<const __u16, eventData*>( event_id, evt) );
					prevTimeIt=servicemap.second.insert( prevTimeIt, std::pair<const time_t, eventData*>( TM, evt ) );
				}
#if EPG_DEBUG
				if ( consistencyCheck )
				{
					if ( tm_it->second != evt || ev_it->second != evt )
						eFatal("tm_it->second != ev_it->second");
					else if ( tm_it->second->getStartTime() != tm_it->first )
						eFatal("event start_time(%d) non equal timemap key(%d)",
							tm_it->second->getStartTime(), tm_it->first );
					else if ( tm_it->first != TM )
						eFatal("timemap key(%d) non equal TM(%d)",
							tm_it->first, TM);
					else if ( ev_it->second->getEventID() != ev_it->first )
						eFatal("event_id (%d) non equal event_map key(%d)",
							ev_it->second->getEventID(), ev_it->first);
					else if ( ev_it->first != event_id )
						eFatal("eventmap key(%d) non equal event_id(%d)",
							ev_it->first, event_id );
				}
#endif
			}
next:
#if EPG_DEBUG
			if ( servicemap.first.size() != servicemap.second.size() )
			{
				FILE *f = fopen("/hdd/event_map.txt", "w+");
				int i=0;
				for (eventMap::iterator it(servicemap.first.begin())
					; it != servicemap.first.end(); ++it )
					fprintf(f, "%d(key %d) -> time %d, event_id %d, data %p\n",
						i++, (int)it->first, (int)it->second->getStartTime(), (int)it->second->getEventID(), it->second );
				fclose(f);
				f = fopen("/hdd/time_map.txt", "w+");
				i=0;
				for (timeMap::iterator it(servicemap.second.begin())
					; it != servicemap.second.end(); ++it )
				fprintf(f, "%d(key %d) -> time %d, event_id %d, data %p\n",
					i++, (int)it->first, (int)it->second->getStartTime(), (int)it->second->getEventID(), it->second );
				fclose(f);
	
				eFatal("(1)map sizes not equal :( sid %04x tsid %04x onid %04x size %d size2 %d",
					service.sid, service.tsid, service.onid,
					servicemap.first.size(), servicemap.second.size() );
			}
#endif
			ptr += eit_event_size;
			eit_event=(eit_event_struct*)(((__u8*)eit_event)+eit_event_size);
		}

		tmpMap::iterator it = temp.find( service );
		if ( it != temp.end() )
		{
			if ( source > it->second.second )
			{
				it->second.first=now;
				it->second.second=source;
			}
		}
		else
			temp[service] = std::pair< time_t, int> (now, source);

		Unlock();
	}

	if ( state == 1 && seenSections == calcedSections  )
		return -ECANCELED;

	return 0;
}

bool eEPGCache::finishEPG()
{
	if (!isRunning)  // epg ready
	{
		eDebug("[EPGC] stop caching events(%d)", time(0)+eDVB::getInstance()->time_difference );
		for (int i=0; i < 3; ++i)
		{
			seenSections[i].clear();
			calcedSections[i].clear();
		}
		zapTimer.start(UPDATE_INTERVAL, 1);
		eDebug("[EPGC] next update in %i min", UPDATE_INTERVAL / 60000);

		singleLock l(cache_lock);
		tmpMap::iterator It = temp.begin();
		abortTimer.stop();

		while (It != temp.end())
		{
//			eDebug("sid = %02x, onid = %02x, type %d", It->first.sid, It->first.onid, It->second.second );
			if ( It->first.tsid == current_service.tsid || It->second.second == SCHEDULE )
			{
//				eDebug("ADD to last updated Map");
				serviceLastUpdated[It->first]=It->second.first;
			}
			if ( eventDB.find( It->first ) == eventDB.end() )
			{
//				eDebug("REMOVE from update Map");
				temp.erase(It++);
			}
			else
				It++;
		}
		if (!eventDB[current_service].first.empty())
			/*emit*/ EPGAvail(1);

		/*emit*/ EPGUpdated();

		return true;
	}
	return false;
}

void eEPGCache::flushEPG(const uniqueEPGKey & s)
{
	eDebug("[EPGC] flushEPG %d", (int)(bool)s);
	singleLock l(cache_lock);
	if (s)  // clear only this service
	{
		eventCache::iterator it = eventDB.find(s);
		if ( it != eventDB.end() )
		{
			eventMap &evMap = it->second.first;
			timeMap &tmMap = it->second.second;
			tmMap.clear();
			for (eventMap::iterator i = evMap.begin(); i != evMap.end(); ++i)
				delete i->second;
			evMap.clear();
			eventDB.erase(it);
			updateMap::iterator u = serviceLastUpdated.find(s);
			if ( u != serviceLastUpdated.end() )
				serviceLastUpdated.erase(u);
#ifdef ENABLE_PRIVATE_EPG
			contentMaps::iterator it =
				content_time_tables.find(s);
			if ( it != content_time_tables.end() )
			{
				it->second.clear();
				content_time_tables.erase(it);
			}
#endif
		}
	}
	else // clear complete EPG Cache
	{
		for (eventCache::iterator it(eventDB.begin());
			it != eventDB.end(); ++it)
		{
			eventMap &evMap = it->second.first;
			timeMap &tmMap = it->second.second;
			for (eventMap::iterator i = evMap.begin(); i != evMap.end(); ++i)
				delete i->second;
			evMap.clear();
			tmMap.clear();
		}
		serviceLastUpdated.clear();
		eventDB.clear();
#ifdef ENABLE_PRIVATE_EPG
		content_time_tables.clear();
#endif
	}
	eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
}

void eEPGCache::cleanLoop()
{
	singleLock s(cache_lock);
	if ( isRunning || (temp.size() && haveData) )
	{
		CleanTimer.start(5000,true);
		eDebug("[EPGC] schedule cleanloop");
		return;
	}
	if (!eventDB.empty() && !paused )
	{
		eDebug("[EPGC] start cleanloop");

		time_t now = time(0)+eDVB::getInstance()->time_difference;

		for (eventCache::iterator DBIt = eventDB.begin(); DBIt != eventDB.end(); DBIt++)
		{
			bool updated=false;
			for (timeMap::iterator It = DBIt->second.second.begin(); It != DBIt->second.second.end() && It->first < now;)
			{
				if ( now > (It->first+It->second->getDuration()) )  // outdated normal entry (nvod references to)
				{
					// remove entry from eventMap
					eventMap::iterator b(DBIt->second.first.find(It->second->getEventID()));
					if ( b != DBIt->second.first.end() )
					{
						// release Heap Memory for this entry   (new ....)
//						eDebug("[EPGC] delete old event (evmap)");
						DBIt->second.first.erase(b);
					}

					// remove entry from timeMap
//					eDebug("[EPGC] release heap mem");
					delete It->second;
					DBIt->second.second.erase(It++);
//					eDebug("[EPGC] delete old event (timeMap)");

					// add this (changed) service to temp map...
					if ( temp.find(DBIt->first) == temp.end() )
					{
						temp[DBIt->first]=std::pair<time_t, int>(now, NOWNEXT);
						updated=true;
					}
				}
				else
					++It;
			}

#ifdef ENABLE_PRIVATE_EPG
			if ( updated )
			{
				contentMaps::iterator x =
					content_time_tables.find( DBIt->first );
				if ( x != content_time_tables.end() )
				{
					timeMap &tmMap = eventDB[DBIt->first].second;
					for ( contentMap::iterator i = x->second.begin(); i != x->second.end(); )
					{
						for ( contentTimeMap::iterator it(i->second.begin());
							it != i->second.end(); )
						{
							if ( tmMap.find(it->second.first) == tmMap.end() )
								i->second.erase(it++);
							else
								++it;
						}
						if ( i->second.size() )
							++i;
						else
							x->second.erase(i++);
					}
				}
			}
#endif

			if ( DBIt->second.second.size() == 1 )
			// less than two events for this service in cache..
			{
				updateMap::iterator u = serviceLastUpdated.find(DBIt->first);
				if ( u != serviceLastUpdated.end() )
				{
					// remove from lastupdated map..
					serviceLastUpdated.erase(u);
					// current service?
					if ( DBIt->first == current_service )
					{
					// immediate .. after leave cleanloop
					// update epgdata for this service
						zapTimer.start(0,true);
					}
				}
			}
		}

#ifdef NVOD
		for (nvodMap::iterator it(NVOD.begin()); it != NVOD.end(); ++it )
		{
//			eDebug("check NVOD Service");
			eventCache::iterator evIt(eventDB.find(it->first));
			if ( evIt != eventDB.end() && evIt->second.first.size() )
			{
				for ( eventMap::iterator i(evIt->second.first.begin());
					i != evIt->second.first.end(); )
				{
#if EPG_DEBUG
					ASSERT(i->second->getStartTime() == 3599);
#endif
					int cnt=0;
					for ( std::list<NVODReferenceEntry>::iterator ni(it->second.begin());
						ni != it->second.end(); ++ni )
					{
						eventCache::iterator nie(eventDB.find(uniqueEPGKey(ni->service_id, ni->original_network_id, ni->transport_stream_id) ) );
						if (nie != eventDB.end() && nie->second.first.find( i->first ) != nie->second.first.end() )
						{
							++cnt;
							break;
						}
					}
					if ( !cnt ) // no more referenced
					{
						delete i->second;  // free memory
						evIt->second.first.erase(i++);  // remove from eventmap
					}
					else
						++i;
				}
			}
		}
#endif

		if (temp.size())
			/*emit*/ EPGUpdated();

		eDebug("[EPGC] stop cleanloop");
		eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
	}
	CleanTimer.start(CLEAN_INTERVAL,true);
}

eEPGCache::~eEPGCache()
{
	messages.send(Message::quit);
	kill(); // waiting for thread shutdown
	Lock();
	for (eventCache::iterator evIt = eventDB.begin(); evIt != eventDB.end(); evIt++)
		for (eventMap::iterator It = evIt->second.first.begin(); It != evIt->second.first.end(); It++)
			delete It->second;
	Unlock();
}

EITEvent *eEPGCache::lookupEvent(const eServiceReferenceDVB &service, int event_id, bool plain)
{
	singleLock s(cache_lock);
	uniqueEPGKey key( service );

	eventCache::iterator It = eventDB.find( key );
	if ( It != eventDB.end() && !It->second.first.empty() ) // entrys cached?
	{
		eventMap::iterator i( It->second.first.find( event_id ));
		if ( i != It->second.first.end() )
		{
			if ( service.getServiceType() == 4 ) // nvod ref
				return lookupEvent( service, i->second->getStartTime(), plain );
			else if ( plain )
		// get plain data... the returned pointer is in eventData format !!
		// to get a eit_event_struct .. use ->get() on the eventData*
		// but take care the eit_event_struct is only valid until the next ->get() call of
		// any eventData* .. when you will use the event_data for a longer time then make
		// a copy of it.. with memcpy or by another way or use EITEvents (not plain)
				return (EITEvent*) i->second;
			else
				return new EITEvent( *i->second, (It->first.tsid<<16)|It->first.onid );
		}
		else
			eDebug("event %04x not found in epgcache", event_id);
	}
	return 0;
}

EITEvent *eEPGCache::lookupEvent(const eServiceReferenceDVB &service, time_t t, bool plain )
// if t == 0 we search the current event...
{
	singleLock s(cache_lock);
	uniqueEPGKey key(service);

	// check if EPG for this service is ready...
	eventCache::iterator It = eventDB.find( key );
	if ( It != eventDB.end() && !It->second.first.empty() ) // entrys cached ?
	{
		if (!t)
			t = time(0)+eDVB::getInstance()->time_difference;

#ifdef NVOD
		if (service.getServiceType() == 4)// NVOD
		{
			// get NVOD Refs from this NVDO Service
			for ( std::list<NVODReferenceEntry>::iterator it( NVOD[key].begin() ); it != NVOD[key].end(); it++ )
			{
				eventCache::iterator evIt = eventDB.find(uniqueEPGKey( it->service_id, it->original_network_id, it->transport_stream_id ));
				if ( evIt != eventDB.end() )
				{
					for ( eventMap::iterator emIt( evIt->second.first.begin() ); emIt != evIt->second.first.end(); emIt++)
					{
						EITEvent refEvt(*emIt->second, (evIt->first.tsid<<16)|evIt->first.onid);
						if ( t >= refEvt.start_time && t <= refEvt.start_time+refEvt.duration)
						{ // found the current start_time..
							for (ePtrList<Descriptor>::iterator d(refEvt.descriptor); d != refEvt.descriptor.end(); ++d)
							{
								Descriptor *descriptor=*d;
								if (descriptor->Tag()==DESCR_TIME_SHIFTED_EVENT)
								{
									int event_id = ((TimeShiftedEventDescriptor*)descriptor)->reference_event_id;
									for ( eventMap::iterator i( It->second.first.begin() ); i != It->second.first.end(); i++)
									{
										if ( i->second->getEventID() == event_id )
										{
											if ( plain )
												// get plain data... not in EITEvent Format !!!
												// before use .. cast it to eit_event_struct*
												return (EITEvent*) i->second;
											EITEvent *evt = new EITEvent( *i->second, (evIt->first.tsid<<16)|evIt->first.onid);
											evt->start_time = refEvt.start_time;
											evt->duration = refEvt.duration;
											evt->event_id = refEvt.event_id;
											evt->free_CA_mode = refEvt.free_CA_mode;
											evt->running_status = refEvt.running_status;
											return evt;
										}
									}
								}
							}
						}
					}
				}
			}
		}
#endif

		timeMap::iterator i = It->second.second.lower_bound(t);
		if ( i != It->second.second.end() )
		{
			i--;
			if ( i != It->second.second.end() )
			{
				if ( t <= i->first+i->second->getDuration() )
				{
					if ( plain )
		// get plain data... the returned pointer is in eventData format !!
		// to get a eit_event_struct .. use ->get() on the eventData*
		// but take care the eit_event_struct is only valid until the next ->get() call of
		// any eventData* .. when you will use the event_data for a longer time then make
		// a copy of it.. with memcpy or by another way or use EITEvents (not plain)
						return (EITEvent*) i->second;
					return new EITEvent( *i->second, (It->first.tsid<<16)|It->first.onid  );
				}
			}
		}

		for ( eventMap::iterator i( It->second.first.begin() ); i != It->second.first.end(); i++)
		{
			time_t begTime = i->second->getStartTime();
			if ( t >= begTime && t <= begTime+i->second->getDuration()) // then we have found
			{
				if ( plain )
					// get plain data... not in EITEvent Format !!!
					// before use .. cast it to eit_event_struct*
					return (EITEvent*) i->second;
				return new EITEvent( *i->second, (It->first.tsid<<16)|It->first.onid );
			}
		}
	}
	return 0;
}

void eEPGCache::pauseEPG()
{
	if (!paused)
	{
		abortEPG();
		eDebug("[EPGC] paused]");
		paused=1;
	}
}

void eEPGCache::restartEPG()
{
	if (paused)
	{
		isRunning=0;
		eDebug("[EPGC] restarted");
		paused--;
		if (paused)
		{
			paused = 0;
			startEPG();   // updateEPG
		}
		cleanLoop();
	}
}

void eEPGCache::startEPG()
{
	if (paused)  // called from the updateTimer during pause...
	{
		paused++;
		return;
	}
	if (eDVB::getInstance()->time_difference)
	{
		if ( firstStart )
		{
			eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
			if ( !sapi || !sapi->transponder )
				return;
			firstStart=0;
		}
		Lock();
		temp.clear();
		for (int i=0; i < 3; ++i)
		{
			seenSections[i].clear();
			calcedSections[i].clear();
		}
		Unlock();
		eDebug("[EPGC] start caching events(%d)", time(0)+eDVB::getInstance()->time_difference);
		state=0;
		haveData=0;
		scheduleReader.start();
		isRunning |= 1;
		nownextReader.start();
		isRunning |= 2;
		scheduleOtherReader.start();
		isRunning |= 4;
		abortTimer.start(7000,true);
	}
	else
	{
		eDebug("[EPGC] wait for clock update");
		zapTimer.start(1000, 1); // restart Timer
	}
}

void eEPGCache::abortNonAvail()
{
	if (!state)
	{
		if ( !(haveData&2) && (isRunning&2) )
		{
			eDebug("[EPGC] abort non avail nownext reading");
			isRunning &= ~2;
			nownextReader.abort();
		}
		if ( !(haveData&1) && (isRunning&1) )
		{
			eDebug("[EPGC] abort non avail schedule reading");
			isRunning &= ~1;
			scheduleReader.abort();
		}
		if ( !(haveData&4) && (isRunning&4) )
		{
			eDebug("[EPGC] abort non avail schedule_other reading");
			isRunning &= ~4;
			scheduleOtherReader.abort();
		}
		if ( isRunning )
			abortTimer.start(90000, true);
		else
		{
			eDebug("[EPGC] no data .. abort");
			++state;
		}
	}
	else
		eDebug("[EPGC] timeout");
	++state;
}

void eEPGCache::enterService(const eServiceReferenceDVB& ref, int err)
{
	if ( ref.path )
		err = 2222;
	else if ( ref.getServiceType() == 7 )
		switch ( eSystemInfo::getInstance()->getHwType() )
		{
			case eSystemInfo::dbox2Nokia:
			case eSystemInfo::dbox2Sagem:
			case eSystemInfo::dbox2Philips:
				err = 1111; //faked
			default:
				break;
		}

	if ( thread_running() )
	// -> gotMessage -> changedService
		messages.send(Message(Message::enterService, ref, err));
	else
	{
		cached_service = ref;
		cached_err = err;
	}
}

void eEPGCache::leaveService(const eServiceReferenceDVB& ref)
{
	if ( thread_running() )
		messages.send(Message(Message::leaveService, ref));
	else
		cached_service=eServiceReferenceDVB();
	// -> gotMessage -> abortEPG
}

#ifdef ENABLE_PRIVATE_EPG
void eEPGCache::setContentPid( int pid )
{
	messages.send(Message(Message::content_pid, pid));
	// -> gotMessage -> content_pid
}
#endif

void eEPGCache::changedService(const uniqueEPGKey &service, int err)
{
	if ( err == 2222 )
	{
		eDebug("[EPGC] dont start ... its a replay");
		/*emit*/ EPGAvail(0);
		return;
	}

	current_service = service;
	updateMap::iterator It = serviceLastUpdated.find( current_service );

	int update;

// check if this is a subservice and this is only a dbox2
// then we dont start epgcache on subservice change..
// ever and ever..

	if ( !err || err == -ENOCASYS || err == -ENVOD )
	{
		update = ( It != serviceLastUpdated.end() ? ( UPDATE_INTERVAL - ( (time(0)+eDVB::getInstance()->time_difference-It->second) * 1000 ) ) : ZAP_DELAY );

		if (update < ZAP_DELAY)
			update = ZAP_DELAY;

		zapTimer.start(update, 1);
		if (update >= 60000)
			eDebug("[EPGC] next update in %i min", update/60000);
		else if (update >= 1000)
			eDebug("[EPGC] next update in %i sec", update/1000);
	}

	Lock();
	bool empty=eventDB[current_service].first.empty();
	Unlock();

	if (!empty)
	{
		eDebug("[EPGC] yet cached");
		/*emit*/ EPGAvail(1);
	}
	else
	{
		eDebug("[EPGC] not cached yet");
		/*emit*/ EPGAvail(0);
	}
}

void eEPGCache::abortEPG()
{
	abortTimer.stop();
	zapTimer.stop();
	if (isRunning)
	{
		if (isRunning & 1)
		{
			isRunning &= ~1;
			scheduleReader.abort();
		}
		if (isRunning & 2)
		{
			isRunning &= ~2;
			nownextReader.abort();
		}
		if (isRunning & 4)
		{
			isRunning &= ~4;
			scheduleOtherReader.abort();
		}
		eDebug("[EPGC] abort caching events !!");
		Lock();
		temp.clear();
		Unlock();
	}
}

void eEPGCache::gotMessage( const Message &msg )
{
	switch (msg.type)
	{
		case Message::flush:
			flushEPG(msg.service);
			break;
		case Message::enterService:
			changedService(msg.service, msg.err);
			break;
		case Message::leaveService:
			// msg.service is valid.. but not needed
			abortEPG();
#ifdef ENABLE_PRIVATE_EPG
			contentReader.stop();
#endif
			break;
		case Message::pause:
			pauseEPG();
			break;
		case Message::restart:
			restartEPG();
			break;
		case Message::quit:
			quit(0);
			break;
		case Message::timeChanged:
			cleanLoop();
			break;
#ifdef ENABLE_PRIVATE_EPG
		case Message::content_pid:
			contentReader.start(msg.pid);
			break;
#endif
		case Message::save:
			save();
			break;
		case Message::load:
			flushEPG();
			load();
			break;
		default:
			eDebug("unhandled EPGCache Message!!");
			break;
	}
}

void eEPGCache::thread()
{
	nice(4);
	load();
	cleanLoop();
	exec();
	save();
}

void eEPGCache::load()
{
	FILE *f = fopen("/hdd/epg.dat", "r");
	if (f)
	{
		unsigned char md5_saved[16];
		unsigned char md5[16];
		int size=0;
		int cnt=0;
		bool md5ok=false;
		if (!md5_file("/hdd/epg.dat", 1, md5))
		{
			FILE *f = fopen("/hdd/epg.dat.md5", "r");
			if (f)
			{
				fread( md5_saved, 16, 1, f);
				fclose(f);
				if ( !memcmp(md5_saved, md5, 16) )
					md5ok=true;
			}
		}
		if ( md5ok )
		{
			char text1[13];
			fread( text1, 13, 1, f);
			if ( !strncmp( text1, "ENIGMA_EPG_V3", 13) )
			{
				fread( &size, sizeof(int), 1, f);
				while(size--)
				{
					uniqueEPGKey key;
					eventMap evMap;
					timeMap tmMap;
					int size=0;
					fread( &key, sizeof(uniqueEPGKey), 1, f);
					fread( &size, sizeof(int), 1, f);
					while(size--)
					{
						__u8 len=0;
						__u8 type=0;
						eventData *event=0;
						fread( &type, sizeof(__u8), 1, f);
						fread( &len, sizeof(__u8), 1, f);
						event = new eventData(0, len, type);
						event->EITdata = new __u8[len];
						eventData::CacheSize+=len;
						fread( event->EITdata, len, 1, f);
						evMap[ event->getEventID() ]=event;
						tmMap[ event->getStartTime() ]=event;
						++cnt;
					}
					eventDB[key]=std::pair<eventMap,timeMap>(evMap,tmMap);
				}
				eventData::load(f);
				eDebug("%d events read from /hdd/epg.dat", cnt);
#ifdef ENABLE_PRIVATE_EPG
				char text2[11];
				fread( text2, 11, 1, f);
				if ( !strncmp( text2, "PRIVATE_EPG", 11) )
				{
					size=0;
					fread( &size, sizeof(int), 1, f);
					while(size--)
					{
						int size=0;
						uniqueEPGKey key;
						fread( &key, sizeof(uniqueEPGKey), 1, f);
						fread( &size, sizeof(int), 1, f);
						while(size--)
						{
							int size;
							int content_id;
							fread( &content_id, sizeof(int), 1, f);
							fread( &size, sizeof(int), 1, f);
							while(size--)
							{
								time_t time1, time2;
								__u16 event_id;
								fread( &time1, sizeof(time_t), 1, f);
								fread( &time2, sizeof(time_t), 1, f);
								fread( &event_id, sizeof(__u16), 1, f);
								content_time_tables[key][content_id][time1]=std::pair<time_t, __u16>(time2, event_id);
							}
						}
					}
				}
#endif // ENABLE_PRIVATE_EPG
			}
			else
				eDebug("[EPGC] don't read old epg database");
			fclose(f);
		}
	}
}

void eEPGCache::save()
{
	struct statfs s;
	off64_t tmp;
	if (statfs("/hdd", &s)<0)
		tmp=0;
	else
	{
		tmp=s.f_blocks;
		tmp*=s.f_bsize;
	}

	// prevent writes to builtin flash
	if ( tmp < 1024*1024*50 ) // storage size < 50MB
		return;

	// check for enough free space on storage
	tmp=s.f_bfree;
	tmp*=s.f_bsize;
	if ( tmp < (eventData::CacheSize*12)/10 ) // 20% overhead
		return;

	FILE *f = fopen("/hdd/epg.dat", "w");
	int cnt=0;
	if ( f )
	{
		const char *text = "ENIGMA_EPG_V3";
		fwrite( text, 13, 1, f );
		int size = eventDB.size();
		fwrite( &size, sizeof(int), 1, f );
		for (eventCache::iterator service_it(eventDB.begin()); service_it != eventDB.end(); ++service_it)
		{
			timeMap &timemap = service_it->second.second;
			fwrite( &service_it->first, sizeof(uniqueEPGKey), 1, f);
			size = timemap.size();
			fwrite( &size, sizeof(int), 1, f);
			for (timeMap::iterator time_it(timemap.begin()); time_it != timemap.end(); ++time_it)
			{
				__u8 len = time_it->second->ByteSize;
				fwrite( &time_it->second->type, sizeof(__u8), 1, f );
				fwrite( &len, sizeof(__u8), 1, f);
				fwrite( time_it->second->EITdata, len, 1, f);
				++cnt;
			}
		}
		eDebug("%d events written to /hdd/epg.dat", cnt);
		eventData::save(f);
#ifdef ENABLE_PRIVATE_EPG
		const char* text3 = "PRIVATE_EPG";
		fwrite( text3, 11, 1, f );
		size = content_time_tables.size();
		fwrite( &size, sizeof(int), 1, f);
		for (contentMaps::iterator a = content_time_tables.begin(); a != content_time_tables.end(); ++a)
		{
			contentMap &content_time_table = a->second;
			fwrite( &a->first, sizeof(uniqueEPGKey), 1, f);
			int size = content_time_table.size();
			fwrite( &size, sizeof(int), 1, f);
			for (contentMap::iterator i = content_time_table.begin(); i != content_time_table.end(); ++i )
			{
				int size = i->second.size();
				fwrite( &i->first, sizeof(int), 1, f);
				fwrite( &size, sizeof(int), 1, f);
				for ( contentTimeMap::iterator it(i->second.begin());
					it != i->second.end(); ++it )
				{
					fwrite( &it->first, sizeof(time_t), 1, f);
					fwrite( &it->second.first, sizeof(time_t), 1, f);
					fwrite( &it->second.second, sizeof(__u16), 1, f);
				}
			}
		}
#endif
		fclose(f);
		unsigned char md5[16];
		if (!md5_file("/hdd/epg.dat", 1, md5))
		{
			FILE *f = fopen("/hdd/epg.dat.md5", "w");
			if (f)
			{
				fwrite( md5, 16, 1, f);
				fclose(f);
			}
		}
	}
}

eAutoInitP0<eEPGCache> init_eEPGCacheInit(eAutoInitNumbers::service+3, "EPG cache");

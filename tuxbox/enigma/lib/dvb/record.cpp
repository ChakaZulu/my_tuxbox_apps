#ifndef DISABLE_FILE

#include <lib/dvb/record.h>
#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <lib/dvb/dvbservice.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#define DVR_DEV "/dev/dvb/card0/dvr1"
#define DEMUX1_DEV "/dev/dvb/card0/demux1"
#else
#include <linux/dvb/dmx.h>
#define DVR_DEV "/dev/dvb/adapter0/dvr1"
#define DEMUX1_DEV "/dev/dvb/adapter0/demux1"
#endif

static int section_length(const unsigned char *buf)
{
	return ((buf[1] << 8) | buf[2]) & 0x0fff;
}

static int ts_header(unsigned char *dest, int pusi, int pid, int scrmbl, int adap, unsigned int &cc)
{
	dest[0] = 0x47;
	dest[1] = (!!pusi << 6) | (pid >> 8);
	dest[2] = pid;
	dest[3] = (scrmbl << 6) | (adap << 4) | (cc++ & 0x0f);

	return 4;
}

static int section2ts(unsigned char *dest, const unsigned char *src, int pid, unsigned int &ccount )
{
	unsigned char *orig = dest;
	int pusi = 1;
	int len, cplen;

	orig = dest;

	for (len = section_length(src) + 3; len > 0; len -= cplen) {
		dest += ts_header(dest, pusi, pid, 0, 1, ccount);

		if (pusi) {
			*dest++ = 0x00;	/* pointer_field */
			cplen = MIN(len, 183);
			pusi = 0;
		}
		else {
			cplen = MIN(len, 184);
		}

		memcpy(dest, src, cplen);
		dest += cplen;
		src += cplen;
	}

	if ((cplen = (dest - orig) % 188)) {
		cplen = 188 - cplen;
		memset(dest, 0xff, cplen);
		dest += cplen;
	}

	return dest - orig;
}

int eDVBRecorder::flushBuffer()
{
	if (!bufptr)
		return 0;
	int wr = ::write(outfd, buf, bufptr);
	if ( wr < bufptr )
	{
		eDebug("recording write error, maybe disk full");
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
		return -1;
	}
	size+=wr;
	bufptr=0;
	return 0;
}

void eDVBRecorder::thread()
{
	while (state == stateRunning)
	{
		singleLock s(bufferLock);

		int r = ::read(dvrfd, buf+bufptr, 65424-bufptr );
		if ( r < 0 )
			continue;

		bufptr += r;

		if ( bufptr > 65423 )
			if (flushBuffer())
				state = stateError;

		if (size > splitsize)
			if (openFile(++splits))
			{
				state = stateError;
				rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
			}
	}
}

void eDVBRecorder::PMTready(int error)
{
	eDebug("eDVBRecorder PMTready");
	if ( !error )
	{
		PMT *pmt=tPMT.ready()?tPMT.getCurrent():0;
		if ( pmt )
		{
			eDVBCaPMTClientHandler::distribute_gotPMT(recRef, pmt);

			eDebug("UpdatePIDs");
//			addNewPID(0); // PAT
//			addNewPID(pmt->pid);  // PMT
			addNewPID(pmt->PCR_PID);  // PCR

			for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
			{
				int record=0;
				switch (i->stream_type)
				{
					case 1:	// video..
					case 2:
						record=1;
						break;
					case 3:	// audio..
					case 4:
						record=1;
						break;
					case 6:
					for (ePtrList<Descriptor>::iterator it(i->ES_info); it != i->ES_info.end(); ++it)
					{
						if (it->Tag() == DESCR_AC3)
						{
							record=1;
							break;
						}
#ifdef RECORD_TELETEXT
						if (it->Tag() == DESCR_TELETEXT)
						{
							record=1;
							break;
						}
#endif
					}
					break;
				}
				if (record)
					addNewPID(i->elementary_PID);
			}
			validatePIDs();

			delete [] PmtData;
			PmtData = pmt->getRAW();

			pmt->unlock();
		}
	}
}

void eDVBRecorder::gotBackMessage(const eDVBRecorderMessage &msg)
{
	switch (msg.code)
	{
	case eDVBRecorderMessage::rWriteError:
		/* emit */ recMessage(recWriteError);
		break;
	default:
		break;
	}
}

int eDVBRecorder::openFile(int suffix)
{
	eString tfilename=filename;
	if (suffix)
		tfilename+=eString().sprintf(".%03d", suffix);

	size=0;

	if (outfd >= 0)
		::close(outfd);

	::unlink(tfilename.c_str());
	outfd=::open(tfilename.c_str(), O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE, 0555);

	if (outfd < 0)
	{
		eDebug("failed to open DVR file: %s (%m)", tfilename.c_str());	
		return -1;
	}
	return 0;
}

void eDVBRecorder::open(const char *_filename)
{
	eDebug("eDVBRecorder::open(%s)", _filename);
	pids.clear();
	newpids.clear();

	filename=_filename;

	splitsize=1024*1024*1024; // 1G

	openFile(splits=0);

	if ( dvrfd >= 0 )
		::close(dvrfd);

	dvrfd=::open(DVR_DEV, O_RDONLY);
	if (dvrfd < 0)
	{
		eDebug("failed to open "DVR_DEV" (%m)");
		::close(outfd);
		outfd=-1;
		return;
	}
	if (outfd < 0)
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
}

std::pair<std::set<eDVBRecorder::pid_t>::iterator,bool> eDVBRecorder::addPID(int pid)
{
	eDebug("eDVBRecorder::addPID(0x%x)", pid );
	pid_t p;
	p.pid=pid;
	if ( pids.find(p) != pids.end() )
	{
		eDebug("we already have this pid... skip!");
		return std::pair<std::set<pid_t>::iterator, bool>(pids.end(),false);
	}
	p.fd=::open(DEMUX1_DEV, O_RDWR);
	if (p.fd < 0)
	{
		eDebug("failed to open demux1");
		return std::pair<std::set<pid_t>::iterator, bool>(pids.end(),false);
	}
#if HAVE_DVB_API_VERSION < 3
	dmxPesFilterParams flt;
	flt.pesType=DMX_PES_OTHER;
#else
	dmx_pes_filter_params flt;
	flt.pes_type=DMX_PES_OTHER;
#endif
	flt.pid=p.pid;
	flt.input=DMX_IN_FRONTEND;
	flt.output=DMX_OUT_TS_TAP;

	flt.flags=0;

	if (::ioctl(p.fd, DMX_SET_PES_FILTER, &flt)<0)
	{
		eDebug("DMX_SET_PES_FILTER failed (for pid %d)", flt.pid);
		::close(p.fd);
		return std::pair<std::set<pid_t>::iterator, bool>(pids.end(),false);
	}

	return pids.insert(p);
}

void eDVBRecorder::addNewPID(int pid)
{
	pid_t p;
	p.pid = pid;
	newpids.insert(p);
}

void eDVBRecorder::validatePIDs()
{
	eDebug("validatePIDs");
	for (std::set<pid_t>::iterator it(pids.begin()); it != pids.end(); ++it )
	{
		std::set<pid_t>::iterator i = newpids.find(*it);
		if ( i == newpids.end() )  // no more existing pid...
		{
			removePID(it->pid);
			it = pids.begin();
		}
	}
	for (std::set<pid_t>::iterator it(newpids.begin()); it != newpids.end(); ++it )
	{
		std::pair<std::set<pid_t>::iterator,bool> newpid = addPID(it->pid);
		if ( newpid.second )
		{
			if ( state == stateRunning )
			{
				if (::ioctl(newpid.first->fd, DMX_START, 0)<0)
				{
					eDebug("DMX_START failed (%m)");
					::close(newpid.first->fd);
				}
			}
		}
		else
			eDebug("error during add new pid");
	}
	newpids.clear();
}

void eDVBRecorder::removePID(int pid)
{
	pid_t p;
	p.pid=pid;
	std::set<pid_t>::iterator pi=pids.find(p);
	if (pi != pids.end())
	{
		if (pi->fd >= 0)
			::close(pi->fd);
		pids.erase(pi);
	}
	eDebug("eDVBRecorder::removePID(0x%x)", pid);
}

void eDVBRecorder::start()
{
	if ( state == stateRunning )
		return;

	eDebug("eDVBRecorder::start()");

	state = stateRunning;

	if ( !thread_running() )
	{
		eDebug("run thread");
		run();
	}
	while(!thread_running() )
		usleep(1000);
	PatPmtTimer.start(0,true);

	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
	{
		printf("starting pidfilter for pid %d\n", i->pid );

		if (::ioctl(i->fd, DMX_START, 0)<0)
		{
			eDebug("DMX_START failed");
			::close(i->fd);
			continue;
		}
	}
}

void eDVBRecorder::stop()
{
	if ( state == stateStopped )
		return;

	state = stateStopped;

	PatPmtTimer.stop();
	int timeout=20;
	while ( thread_running() && timeout )
	{
		usleep(100000);  // 2 sec time for thread shutdown
		--timeout;
	}

	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::ioctl(i->fd, DMX_STOP, 0);

	flushBuffer();
}

void eDVBRecorder::close()
{
	if (state != stateStopped)
		stop();

	eDebug("eDVBRecorder::close()");

	if (outfd < 0)
		return;

	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::close(i->fd);

	::close(dvrfd);
	::close(outfd);

	if ( thread_running() )
		kill(true);
}

eDVBRecorder::eDVBRecorder(PMT *pmt,PAT *pat)
:state(stateStopped), rmessagepump(eApp, 1), dvrfd(-1) ,outfd(-1)
,bufptr(0), PatPmtTimer(eApp), PmtData(NULL), PatData(NULL)
,PmtCC(0), PatCC(0)
{
	CONNECT(rmessagepump.recv_msg, eDVBRecorder::gotBackMessage);
	rmessagepump.start();

	if (pmt)
	{
		CONNECT( tPMT.tableReady, eDVBRecorder::PMTready );
		// use DEMUX1_DEV when it can handle.. but at moment not :(
		tPMT.start((PMT*)pmt->createNext(), DEMUX1_DEV );
		PmtData=pmt->getRAW();
		pmtpid=pmt->pid;
		CONNECT( PatPmtTimer.timeout, eDVBRecorder::PatPmtWrite);
		if (pat)
		{
			PAT p;
			p.entries.setAutoDelete(false);
			p.version=pat->version;
			p.transport_stream_id=pat->transport_stream_id;
			for (ePtrList<PATEntry>::iterator it(pat->entries);
				it != pat->entries.end(); ++it)
			{
				if ( it->program_map_PID == pmtpid )
				{
					p.entries.push_back(*it);
					PatData=p.getRAW();
					break;
				}
			}
		}
	}
	pthread_mutex_init(&bufferLock, 0);
}

eDVBRecorder::~eDVBRecorder()
{
	delete [] PatData;
	delete [] PmtData;
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if ( sapi && sapi->service != recRef )
		eDVBCaPMTClientHandler::distribute_leaveService(recRef);
	close();
}

void eDVBRecorder::writeSection(void *data, int pid, unsigned int &cc)
{
	if ( !data )
		return;

	__u8 secbuf[4096];

	int len = section2ts(secbuf, (__u8*)data, pid, cc);

	if ( len )
	{
		singleLock s(bufferLock);

		if ( (bufptr+len) > 65423 )
			flushBuffer();

		memcpy(buf+bufptr, secbuf, len);
		bufptr+=len;
	}
}

void eDVBRecorder::PatPmtWrite()
{
	if ( PatData )
		writeSection(PatData, 0, PatCC );

	if ( PmtData )
		writeSection(PmtData, pmtpid, PmtCC );

	PatPmtTimer.start(1500,true);
}

#endif //DISABLE_FILE

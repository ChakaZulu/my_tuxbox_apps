#include "esection.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#include <core/base/ebase.h>
#include <core/base/eerror.h>

#ifdef DBOX
	#define DEMUX "/dev/ost/demux0"
#else
	#define DEMUX "/dev/demuxapi0"
#endif

#ifdef DBOX
	#include <ost/dmx.h>
#else
	extern "C"
	{
		#include <xp/xp_osd_user.h>
	}
#endif



ePtrList<eSection> eSection::active;

eSectionReader::eSectionReader()
{
	handle=-1;
}

int eSectionReader::getHandle()
{
	return handle;
}

void eSectionReader::close()
{
	if (handle != -1)
		::close(handle);
	handle=-1;
}

int eSectionReader::open(int pid, __u8 *data, __u8 *mask, int len, int _flags)
{
	flags=flags;
#ifdef DBOX
	dmxSctFilterParams secFilterParams;
#else
	demux_filter_para secFilterParams;
#endif

	close();

	handle=::open(DEMUX, O_RDWR|O_NONBLOCK);
	eDebug("opened handle %d", handle);
	if (handle<0)
	{
		perror(DEMUX);
		eDebug("DEMUX OPEN FAILED");
		return -errno;
	}

	secFilterParams.pid=pid;
#ifdef DBOX
	const int maxsize=DMX_FILTER_SIZE;
	memset(secFilterParams.filter.filter, 0, maxsize);
	memset(secFilterParams.filter.mask, 0, maxsize);

	secFilterParams.timeout=0;
	secFilterParams.flags=DMX_IMMEDIATE_START;
	if (flags&SECREAD_CRC)
		secFilterParams.flags|=DMX_CHECK_CRC;
	for (int i = 0; i < DMX_FILTER_SIZE; i++)
	{
		secFilterParams.filter.filter[i]=i<len?data[i]:0;
		secFilterParams.filter.mask[i]=i<len?mask[i]:0;
	}

#ifdef ESECTION_DEBUG
	eDebugNoNewLine("%02x: ", pid);
	for (int i=0; i<DMX_FILTER_SIZE; i++)
		eDebugNoNewLine("%02x ", secFilterParams.filter.filter[i]);
	eDebugNoNewLine(" (napi)\n    ");
	for (int i=0; i<DMX_FILTER_SIZE; i++)
		eDebugNoNewLine("%02x ", secFilterParams.filter.mask[i]);
	eDebug("");
#endif

	if (ioctl(handle, DMX_SET_FILTER, &secFilterParams) < 0)
	{	
		perror("DMX_SET_FILTER\n");
		::close(handle);
		return -1;
	}
#else
	if (len>FILTER_LENGTH)
		len=FILTER_LENGTH;
	memset(secFilterParams.filter, 0, FILTER_LENGTH);
	memset(secFilterParams.mask, 0, FILTER_LENGTH);
	for (int i=0; i<len; i++)
	{
		int src=i;
		int dst=i;
		if (dst)
			dst+=2;
		secFilterParams.filter[dst]=data[src];
		secFilterParams.mask[dst]=mask[src];
	}

	if (len>1)
		len+=2;

	secFilterParams.filter_length=len;
	eDebugNoNewLine("%02x: ", pid);
	for (int i=0; i<len; i++)
		eDebugNoNewLine("%02x ", secFilterParams.filter[i]);
	eDebugNoNewLine("\n    ");
	for (int i=0; i<len; i++)
		eDebugNoNewLine("%02x ", secFilterParams.mask[i]);
	eDebug("");
	if (ioctl(handle, DEMUX_FILTER_SET, &secFilterParams))
	{
		perror("DEMUX_FILTER_SET\n");
		::close(handle);
		return -1;
	}
	if (ioctl(handle, DEMUX_START, 0))
	{
		perror("DEMUX_START\n");
		::close(handle);
		return -1;
	}
#endif

	return 0;
}

int eSectionReader::read(__u8 *buf)
{
#ifdef DBOX
	if (::read(handle, buf, 3)<0)
	{
		if (errno==EAGAIN)
			return errno;
		perror("read section");
		return errno;
	}
	int seclen=0;
	seclen |= ((buf[1] & 0x0F) << 8);
	seclen |= (buf[2] & 0xFF);
	::read(handle, buf+3, seclen);
	seclen+=3;
#else
	if (::read(handle, buf, 16384)<0)
	{
		if (errno==EAGAIN)
			return errno; 
		perror("read section");
		return errno;
	}
#endif
	return 0;
}

eSection::eSection(int pid, int tableid, int tableidext, int version, int flags, int tableidmask): pid(pid), tableid(tableid), tableidext(tableidext), tableidmask(tableidmask), flags(flags), version(version)
{
	notifier=0;
	section=0;
	lockcount=0;
	timer=new eTimer(eApp);
	CONNECT(timer->timeout, eSection::timeout);
}

eSection::eSection()
{
	timer=0;
	notifier=0;
	section=0;
	lockcount=0;
}

eSection::~eSection()
{
	closeFilter();
	if (lockcount)
		eDebug("deleted still locked table");
}

int eSection::start()
{
	if ((version==-1) && !(flags&SECREAD_NOTIMEOUT))
		timer->start((pid==0x14)?60000:10000, true);
	return setFilter(pid, tableid, tableidext, version);
}

int eSection::setFilter(int pid, int tableid, int tableidext, int version)
{
	closeFilter();
	__u8 data[4], mask[4];
	data[0]=tableid; mask[0]=tableidmask;
	data[1]=0; mask[1]=0;
	data[2]=0; mask[2]=0;
	data[3]=0; mask[3]=0;
	if (tableidext!=-1)
	{
		data[1]=tableidext>>8; mask[1]=0xFF;
		data[2]=tableidext&0xFF; mask[2]=0xFF;
	} 
	if (version!=-1)
	{
		data[3]=version; mask[3]=0xFF;
	}

	if (!(flags&SECREAD_NOABORT))
		active.push_back(this);
	reader.open(pid, data, mask, 4, flags);
	
	if (notifier)
		delete notifier;

	notifier=new eSocketNotifier(eApp, reader.getHandle(), eSocketNotifier::Read);

	CONNECT(notifier->activated, eSection::data);

	return 0;
}

void eSection::closeFilter()
{
	if (reader.getHandle()>0)
	{
		if (!(flags&SECREAD_NOABORT))
			active.remove(this);
		delete notifier;
		notifier=0;
		timer->stop();
		reader.close();
	}
}

void eSection::data(int socket)
{
	int max = 100;
	while (max--)
	{
		if (lockcount)
			eDebug("eSection::data on locked section!");
		timer->start(10000, true);
		if (reader.read(buf))
			break;
		maxsec=buf[7];
	
//		printf("%d/%d, we want %d  | service_id %04x | version %04x\n", buf[6], maxsec, section, (buf[3]<<8)|buf[4], buf[5]);

		if (flags&SECREAD_INORDER)
			version=buf[5];

		if ((!(flags&SECREAD_INORDER)) || (section==buf[6]))
		{
			int err;
			if ((err=sectionRead(buf)))
			{
				if (err>0)
					err=0;
				closeFilter();
				sectionFinish(err);
				return;
			}
			section=buf[6]+1;
		}

		if ((section>maxsec) && (flags&SECREAD_INORDER))		// last section?
		{
			closeFilter();										// stop feeding
			sectionFinish(0);
			break;
		}
	}
}

void eSection::timeout()
{
	closeFilter();
	sectionFinish(-ETIMEDOUT);
}

int eSection::abort()
{
	if (reader.getHandle()>0)
	{
		closeFilter();
		sectionFinish(-ECANCELED);
		return 0;
	} else
		return -ENOENT;
}

int eSection::abortAll()
{
	while (active.begin() != active.end())
		active.begin()->abort();
	return 0;
}

int eSection::sectionRead(__u8 *data)
{
	return -1;
}

void eSection::sectionFinish(int)
{
}

int eSection::lock()
{
	return ++lockcount;
}

int eSection::unlock()
{
	if (lockcount)
		return lockcount--;
	else
		eDebug("unlocking while not locked");
	return 0;
}

void eTable::sectionFinish(int err)
{
	if (err)
		error=err;
	ready=1;
	/*emit*/ tableReady(error);
}

eTable::eTable(int pid, int tableid, int tableidext, int version): eSection(pid, tableid, tableidext, version, (pid==0x14)?0:(SECREAD_INORDER|SECREAD_CRC))
{
	error=0;
	ready=0;
}

eTable::eTable(): eSection()
{
	error=0;
	ready=0;
}

eTable *eTable::createNext()
{
	return 0;
}

void eAUGTable::slotTableReady(int error)
{
	getNext(error);
}

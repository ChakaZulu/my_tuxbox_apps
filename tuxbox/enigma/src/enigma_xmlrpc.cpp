#include "xmlrpc.h"
#include "dvb.h"
#include "edvb.h"
#include <errno.h>

static eBouquet *getBouquetByID(const char *id)
{
	ePtrList<eBouquet>* pBouquets=eDVB::getInstance()->getBouquets();
	int bouquet_id;
	if (sscanf(id, "B:%x", &bouquet_id)!=1)
		return 0;
	if (pBouquets)
		for (ePtrList<eBouquet>::iterator i(*pBouquets); i != pBouquets->end(); ++i)
			if (i->bouquet_id==bouquet_id)
				return *i;
	return 0;
}

static eService *getServiceByID(const char *id)
{
	eTransponderList *tl=eDVB::getInstance()->getTransponders();
	if (!tl)
		return 0;
	int original_network_id, transport_stream_id, service_id;
	if (sscanf(id, "S:%x:%x:%x", &original_network_id, &transport_stream_id, &service_id)!=3)
		if (sscanf(id, "E:%x:%x:%x", &original_network_id, &transport_stream_id, &service_id)!=3)
			return 0;

	return tl->searchService(original_network_id, service_id);
}

static int testrpc(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("ii", params, result))
		return 1;
	
	QMap<eString, eXMLRPCVariant*> *s=new QMap<eString, eXMLRPCVariant*>;
	s->insert("sum", new eXMLRPCVariant(new __s32(*params[0]->getI4()+*params[1]->getI4())));
	s->insert("difference", new eXMLRPCVariant(new __s32(*params[0]->getI4()-*params[1]->getI4())));
	
	result.append(new eXMLRPCVariant(s));
	
	return 0;
}

			// SID2

static int getList(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("s", params, result))
		return 1;
	
	eString &param=*params[0]->getString();
	
	eDebug("getList(%s);", param.c_str());
	
	if (!param.length())		// root
	{
		QList<eXMLRPCVariant> l;
		ePtrList<eBouquet>* pBouquets=eDVB::getInstance()->getBouquets();
		if (pBouquets)
		{
			for (ePtrList<eBouquet>::iterator i(*pBouquets); i != pBouquets->end(); ++i)
			{
				eBouquet *b=*i;
				QMap<eString, eXMLRPCVariant*> *s=new QMap<eString, eXMLRPCVariant*>;
				static eString s0("caption");
				static eString s1("type");
				static eString s2("handle");
				static eString s3("zappable");
				
				s->insert(s0, new eXMLRPCVariant(new eString(b->bouquet_name.c_str())));
				static eString g("Group");
				s->insert(s1, new eXMLRPCVariant(new eString(g)));
				static eString bs("B:");
				eString handle=bs;
				handle+=eString().setNum(b->bouquet_id, 16);
				s->insert(s2, new eXMLRPCVariant(new eString(handle)));
				s->insert(s3, new eXMLRPCVariant(new bool(0)));
				l.append(new eXMLRPCVariant(s));
			}
		}
		QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
		nv->setAutoDelete(true);
		nv->resize(l.count());
		l.toVector(nv);
		result.append(new eXMLRPCVariant(nv));
	} else if (param[0]=='B')
	{
		eBouquet *b=getBouquetByID(param.c_str());
		if (!b)
			xmlrpc_fault(result, 3, "invalid handle");
		else
		{
			QList<eXMLRPCVariant> l;

			for (std::list<eServiceReference>::iterator s = b->list.begin(); s != b->list.end(); s++)
			{
				eService *service=s->service;
				if (!service)
					continue;
				QMap<eString, eXMLRPCVariant*> *s=new QMap<eString, eXMLRPCVariant*>;
				static eString s0("caption");
				static eString s1("type");
				static eString s2("handle");
				static eString s3("zappable");

				s->insert(s0, new eXMLRPCVariant(new eString(service->service_name.c_str())));
				static eString g("Service");
				s->insert(s1, new eXMLRPCVariant(new eString(g)));
				static eString bs("S:");
				eString handle=bs;
				handle+=eString().setNum(service->original_network_id, 16);
				handle+=':';
				handle+=eString().setNum(service->transport_stream_id, 16);
				handle+=':';
				handle+=eString().setNum(service->service_id, 16);
				s->insert(s2, new eXMLRPCVariant(new eString(handle)));
				s->insert(s3, new eXMLRPCVariant(new bool(1)));
				l.append(new eXMLRPCVariant(s));
			}
			QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
			nv->setAutoDelete(true);
			nv->resize(l.count());
			l.toVector(nv);
			result.append(new eXMLRPCVariant(nv));
		}
	} else if (param[0]=='S')
	{
		eService *service=getServiceByID(param.c_str());
		if (!service)
		{
			xmlrpc_fault(result, 3, "invalid handle");
			return 0;
		}
		if (eDVB::getInstance()->service != service)
		{
			xmlrpc_fault(result, 4, "service currently not tuned in");
			return 0;
		}
		if (eDVB::getInstance()->service_state==ENOENT)
		{
			xmlrpc_fault(result, 4, "service couldn't be tuned in");
			return 0;
		}
		EIT *eit=eDVB::getInstance()->getEIT();
		if (!eit)
		{
			xmlrpc_fault(result, 1, "no EIT yet");
			return 0;
		}

		QList<eXMLRPCVariant> l;

		for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
		{
			EITEvent *event=*i;

			eString event_name=0;

			for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
						event_name=ss->event_name;
				}
			}
			
			if (!event_name)
				continue;

			QMap<eString, eXMLRPCVariant*> *s=new QMap<eString, eXMLRPCVariant*>;
			static eString s0("caption");
			static eString s1("type");
			static eString s2("handle");
			static eString s3("zappable");

			s->insert(s0, new eXMLRPCVariant(new eString(event_name)));
			static eString g("Event");
			s->insert(s1, new eXMLRPCVariant(new eString(g)));
			static eString bs("E:");
			eString handle=bs;
			handle+=eString().setNum(service->original_network_id, 16);
			handle+=':';
			handle+=eString().setNum(service->transport_stream_id, 16);
			handle+=':';
			handle+=eString().setNum(service->service_id, 16);
			handle+=':';
			handle+=eString().setNum(event->event_id, 16);

			s->insert(s2, new eXMLRPCVariant(new eString(handle)));
			s->insert(s3, new eXMLRPCVariant(new bool(1)));
			l.append(new eXMLRPCVariant(s));
		}
		eit->unlock();

		QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
		nv->setAutoDelete(true);
		nv->resize(l.count());
		l.toVector(nv);
		result.append(new eXMLRPCVariant(nv));
	} else
		xmlrpc_fault(result, 3, "couldn't get of this");
	return 0;
}

static int zapTo(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("s", params, result))
		return 1;

	eString &param=*params[0]->getString();

	eDebug("zapTo(%s);", param.c_str());
	
	eService *s=getServiceByID(param.c_str());
	if (!s)
		xmlrpc_fault(result, 3, "invalid handle");
	else
		eDVB::getInstance()->switchService(s);
	return 0;
}

static int getInfo(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("s", params, result))
		return 1;

	eString &param=*params[0]->getString();

	eDebug("getInfo(%s);", param.c_str());
	
	eService *service=0;
	
	if (!param.length())    // currently running
	{
			// mal gucken
  } else if (param[0]=='S')
  {
		QMap<eString, eXMLRPCVariant*> *s=new QMap<eString, eXMLRPCVariant*>;

		eService *service=getServiceByID(param.c_str());
		if (!service)
		{
			xmlrpc_fault(result, 3, "invalid handle");
			return 0;
		}
		if (eDVB::getInstance()->service != service)
		{
			xmlrpc_fault(result, 4, "service currently not tuned in");
			return 0;
		}
		if (eDVB::getInstance()->service_state==ENOENT)
		{
			xmlrpc_fault(result, 4, "service couldn't be tuned in");
			return 0;
		}
		
		static eString s1("type");
		static eString g("Service");
		s->insert(s1, new eXMLRPCVariant(new eString(g)));

		static eString s0("caption");
		s->insert(s0, new eXMLRPCVariant(new eString(service->service_name.c_str())));
		
		static eString s2("parentHandle");
		static eString g2("NA");
		s->insert(s2, new eXMLRPCVariant(new eString(g2)));
		
		PMT *pmt=eDVB::getInstance()->getPMT();
		if (!pmt)
		{
			xmlrpc_fault(result, 1, "no PMT yet");
			return 0;
		}

		PMTEntry *v=0;
		for (ePtrList<PMTEntry>::iterator i(pmt->streams); (!v) && i != pmt->streams.end(); ++i)
		{
			PMTEntry *pe=*i;
			switch (pe->stream_type)
			{
			case 1:	// ISO/IEC 11172 Video
			case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
				v=pe;
				break;
			}
		}
		
		if (v)
		{
			static eString s2("videoPid");
			s->insert(s2, new eXMLRPCVariant(new int(v->elementary_PID)));
		}

		QList<eXMLRPCVariant> asl;

		for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
		{
			PMTEntry *pe=*i;
			int isaudio=0, isAC3=0;
			
			switch (pe->stream_type)
			{
			case 3:	// ISO/IEC 11172 Audio
			case 4: // ISO/IEC 13818-3 Audio
				isaudio=1;
				break;
			case 6:
			{
				for (ePtrList<Descriptor>::iterator i(pe->ES_info); i != pe->ES_info.end(); ++i)
				{
					Descriptor *d=*i;
					if (d->Tag()==DESCR_AC3)
					{
						isaudio=1;
						isAC3=1;
					}
				}
			}
			}
			if (isaudio)
			{
				QMap<eString, eXMLRPCVariant*> *a=new QMap<eString, eXMLRPCVariant*>;
				static eString s1("audioPid"), s2("handle"), s3("type"), s4("language"), s5("mpeg"), s6("ac3");
				
				a->insert(s1, new eXMLRPCVariant(new int(pe->elementary_PID)));
				a->insert(s2, new eXMLRPCVariant(new eString("A")));	// nyi
				a->insert(s3, new eXMLRPCVariant(new eString(isAC3?s6:s5)));

				asl.append(new eXMLRPCVariant(a));
			}
		}
		static eString as("audioPids");

		QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
		nv->setAutoDelete(true);
		nv->resize(asl.count());
		asl.toVector(nv);	
		s->insert(as, new eXMLRPCVariant(nv));
		pmt->unlock();
		
		result.append(new eXMLRPCVariant(s));
		
		eString res="";
		result.first()->toXML(res);
		eDebug("%s", res.c_str());
	} else
		xmlrpc_fault(result, 3, "nene nur service bitte");
	return 0;
}

void ezapInitializeXMLRPC(eHTTPD *httpd)
{
	xmlrpc_initialize(httpd);
	xmlrpc_addMethod("test.test", testrpc);
	xmlrpc_addMethod("getList", getList);
	xmlrpc_addMethod("zapTo", zapTo);
	xmlrpc_addMethod("getInfo", getInfo);
}

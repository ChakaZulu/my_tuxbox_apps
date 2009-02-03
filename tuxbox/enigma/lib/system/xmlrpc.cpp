#ifndef DISABLE_NETWORK

#include <lib/system/xmlrpc.h>

#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>

static std::map<eString, int (*)(std::vector<eXMLRPCVariant>&, ePtrList<eXMLRPCVariant>&)> rpcproc;

eXMLRPCVariant::eXMLRPCVariant(const eXMLRPCVariant &c)
	:type(c.type)
{
	init_eXMLRPCVariant(c);
}
void eXMLRPCVariant::init_eXMLRPCVariant(const eXMLRPCVariant &c)
{
	switch ( type )
	{
		case STRUCT:
			if ( c._struct )
				_struct=new std::map<eString,eXMLRPCVariant*>;
			for (std::map<eString,eXMLRPCVariant*>::iterator b(c._struct->begin()); b != c._struct->end(); ++b)
				_struct->insert(std::pair<eString,eXMLRPCVariant*>(b->first, new eXMLRPCVariant(*b->second)));
			break;
		case ARRAY:
			if (c._array)
				_array = new std::vector<eXMLRPCVariant>(*c._array);
			break;
		case I4:
			if (c._i4)
				_i4=new int(*c._i4);
			break;
		case BOOLEAN:
			if (c._boolean)
				_boolean=new bool(*c._boolean);
			break;
		case STRING:
			if (c._string)
				_string=new eString(*c._string);
			break;
		case DOUBLE:
			if (c._double)
				_double=new double(*c._double);
			break;
	}
}

eXMLRPCVariant::~eXMLRPCVariant()
{
	switch ( type )
	{
		case STRUCT:
			for (std::map<eString,eXMLRPCVariant*>::iterator i(_struct->begin()); i != _struct->end(); ++i)
				delete i->second;
			delete _struct;
			break;
		case ARRAY:
			delete _array;
			break;
		case I4:
			delete _i4;
			break;
		case BOOLEAN:
			delete _boolean;
			break;
		case STRING:
			delete _string;
			break;
		case DOUBLE:
			delete _double;
			break;
	}
}

void eXMLRPCVariant::toXML(eString &result)
{
	switch (type)
	{
		case ARRAY:
		{
			static eString s1("<value><array><data>");
			result+=s1;
			for (unsigned int i=0; i<getArray()->size(); i++)
			{
				static eString s("  ");
				result+=s;
				(*getArray())[i].toXML(result);
				static eString s1("\n");
				result+=s1;
			}
			static eString s2("</data></array></value>\n");
			result+=s2;
			break;
		}
		case STRUCT:
		{
			static eString s1("<value><struct>");
			result+=s1;
			for (std::map<eString,eXMLRPCVariant*>::iterator i(_struct->begin()); i != _struct->end(); ++i)
			{
				static eString s1("  <member><name>");
				result+=s1;
				result+=i->first;
				static eString s2("</name>");
				result+=s2;
				i->second->toXML(result);
				static eString s3("</member>\n");
				result+=s3;
			}
			static eString s2("</struct></value>\n");
			result+=s2;
			break;
		}
		case I4:
		{
			static eString s1("<value><i4>");
			result+=s1;
			result+=eString().setNum(*getI4());
			static eString s2("</i4></value>");
			result+=s2;
			break;
		}
		case BOOLEAN:
		{
			static eString s0("<value><boolean>0</boolean></value>");
			static eString s1("<value><boolean>1</boolean></value>");
			result+=(*getBoolean())?s1:s0;
			break;
		}
		case STRING:
		{
			static eString s1("<value><string>");
			static eString s2("</string></value>");
			result+=s1;
			result+=*getString();
			result+=s2;
			break;
		}
		case DOUBLE:
		{
			result+=eString().sprintf("<value><double>%lf</double></value>", *getDouble());
		}
	}
}

static eXMLRPCVariant *fromXML(XMLTreeNode *n)
{
	if (strcmp(n->GetType(), "value"))
		return 0;
	n=n->GetChild();
	const char *data=n->GetData();
	if (!data)
		data="";
	if ((!strcmp(n->GetType(), "i4")) || (!strcmp(n->GetType(), "int")))
		return new eXMLRPCVariant(new int(atoi(data)));
	else if (!strcmp(n->GetType(), "boolean"))
		return new eXMLRPCVariant(new bool(atoi(data)));
	else if (!strcmp(n->GetType(), "string"))
		return new eXMLRPCVariant(new eString(data));
	else if (!strcmp(n->GetType(), "double"))
		return new eXMLRPCVariant(new double(atof(data)));
	else if (!strcmp(n->GetType(), "struct")) {
		std::map<eString,eXMLRPCVariant*> *s=new std::map<eString,eXMLRPCVariant*>;
		for (n=n->GetChild(); n; n=n->GetNext())
		{
			if (strcmp(data, "member"))
			{
				delete s;
				return 0;
			}
			eString name=0;
			eXMLRPCVariant *value;
			for (XMLTreeNode *v=n->GetChild(); v; v=v->GetNext())
			{
				if (!strcmp(v->GetType(), "name"))
					name=eString(v->GetData());
				else if (!strcmp(v->GetType(), "value"))
					value=fromXML(v);
			}
			if ((!value) || (!name))
			{
				delete s;
				return 0;
			}
			s->INSERT(name,value);
		}
		return new eXMLRPCVariant(s);
	} else if (!strcmp(n->GetType(), "array"))
	{
		ePtrList<eXMLRPCVariant> l;
		l.setAutoDelete(true);
		n=n->GetChild();
		if (strcmp(data, "data"))
			return 0;
		for (n=n->GetChild(); n; n=n->GetNext())
			if (!strcmp(n->GetType(), "value"))
			{
				eXMLRPCVariant *value=fromXML(n);
				if (!value)
					return 0;
				l.push_back(value);
			}

		return new eXMLRPCVariant( l.getVector() );
	}
	eDebug("couldn't convert %s", n->GetType());
	return 0;
}

eXMLRPCResponse::eXMLRPCResponse(eHTTPConnection *c):
	eHTTPDataSource(c), parser("ISO-8859-1")
{
	// size etc. setzen aber erst NACH data-phase
	connection->localstate=eHTTPConnection::stateWait;
}

eXMLRPCResponse::~eXMLRPCResponse()
{
}

int eXMLRPCResponse::doCall()
{
	eDebug("doing call");
	result="";
		// get method name
	eString methodName=0;
	
	if (connection->remote_header["Content-Type"]!="text/xml")
	{
		eDebug("remote header failure (%s != text/xml)", (connection->remote_header["Content-Type"]).c_str());
		return -3;
	}
	
	XMLTreeNode *methodCall=parser.RootNode();
	if (!methodCall)
	{
		eDebug("empty xml");
		return -1;
	}
	if (strcmp(methodCall->GetType(), "methodCall"))
	{
		eDebug("no methodCall found");
		return -2;
	}

	ePtrList<eXMLRPCVariant> params;
	params.setAutoDelete(true);
	
	for (XMLTreeNode *c=methodCall->GetChild(); c; c=c->GetNext())
	{
		if (!strcmp(c->GetType(), "methodName"))
			methodName=eString(c->GetData());
		else if (!strcmp(c->GetType(), "params"))
		{
			for (XMLTreeNode *p=c->GetChild(); p; p=p->GetNext())
				if (!strcmp(p->GetType(), "param"))
					params.push_back(fromXML(p->GetChild()));
		} else
		{
			eDebug("unknown stuff found");
			return 0;
		}
	}
	
	if (!methodName)
	{
		eDebug("no methodName found!");
		return -3;
	}
	
	eDebug("methodName: %s", methodName.c_str() );
	
	result="<?xml version=\"1.0\"?>\n"
		"<methodResponse>";
	
	ePtrList<eXMLRPCVariant> ret;
	ret.setAutoDelete(true);

	int (*proc)(std::vector<eXMLRPCVariant>&, ePtrList<eXMLRPCVariant> &)=rpcproc[methodName];
	int fault;

	std::vector<eXMLRPCVariant>* v = params.getVector();
	
	if (!proc)
	{
		fault=1;
		xmlrpc_fault(ret, -1, "called method not present");         	
	} else
		fault=proc( *v , ret);

	delete v;

	eDebug("converting to text...");

	if (fault)
	{
		result+="<fault>\n";
		ret.current()->toXML(result);
		result+="</fault>\n";
	} else
	{
		result+="<params>\n";
		for (ePtrList<eXMLRPCVariant>::iterator i(ret); i != ret.end(); ++i)
		{
			result+="<param>";
			i->toXML(result);
			result+="</param>";
		}
		result+="</params>";
	}
	result+="</methodResponse>";
	char buffer[10];
	snprintf(buffer, 10, "%d", size=result.length());
	wptr=0;
	connection->local_header["Content-Type"]="text/xml";
	connection->local_header["Content-Length"]=buffer;
	connection->code=200;
	connection->code_descr="OK";
	connection->localstate=eHTTPConnection::stateResponse;
	return 0;
}

int eXMLRPCResponse::doWrite(int hm)
{
	int tw=size-wptr;
	if (tw>hm)
		tw=hm;
	if (tw<=0)
		return -1;
	connection->writeBlock(result.c_str()+wptr, tw);
	wptr+=tw;
	return size > wptr ? 1 : -1;
}

void eXMLRPCResponse::haveData(void *data, int len)
{
	if (result)
		return;
	int err=0;

	if (!parser.Parse((char*)data, len, !len))
	{
		char temp[len+1];
		temp[len]=0;
		memcpy(temp, data, len);
		eDebug("%s: %s", temp, parser.ErrorString(parser.GetErrorCode()));
		err=1;
	}
	
	if ((!err) && (!len))
		err=doCall();

	if (err)
	{
		eDebug("schade: %d", err);
		connection->code=400;
		connection->code_descr="Bad request";
		char buffer[10];
		snprintf(buffer, 10, "%d", size=result.length());
		wptr=0;
		connection->local_header["Content-Type"]="text/html";
		connection->local_header["Content-Length"]=buffer;
		result.sprintf("XMLRPC error %d\n", err);
		connection->localstate=eHTTPConnection::stateResponse;
	}
}

void xmlrpc_addMethod(eString methodName, int (*proc)(std::vector<eXMLRPCVariant>&, ePtrList<eXMLRPCVariant>&))
{
	rpcproc[methodName]=proc;
}

void xmlrpc_fault(ePtrList<eXMLRPCVariant> &res, int faultCode, eString faultString)
{
	std::map<eString,eXMLRPCVariant*> *s=new std::map<eString,eXMLRPCVariant*>;
	s->INSERT("faultCode", new eXMLRPCVariant(new __s32(faultCode)));
	s->INSERT("faultString", new eXMLRPCVariant(new eString(faultString)));
	res.push_back(new eXMLRPCVariant(s));
}

int xmlrpc_checkArgs(eString args, std::vector<eXMLRPCVariant> &parm, ePtrList<eXMLRPCVariant> &res)
{
	if (parm.size() != args.length())
	{
	 	xmlrpc_fault(res, -500, eString().sprintf("parameter count mismatch (found %d, expected %d)", parm.size(), args.length()));
		return 1;
	}
	
	for (unsigned int i=0; i<args.length(); i++)
	{
		switch (args[i])
		{
		case 'i':
			if (parm[i].getI4())
				continue;
			break;
		case 'b':
			if (parm[i].getBoolean())
				continue;
			break;
		case 's':
			if (parm[i].getString())
				continue;
			break;
		case 'd':
			if (parm[i].getDouble())
				continue;
			break;
/*		case 't':
			if (parm[i].getDatetime())
				continue;
			break;
		case '6':
			if (parm[i].getBase64())
				continue;
			break;*/
		case '$':
			if (parm[i].getStruct())
				continue;
			break;
		case 'a':
			if (parm[i].getArray())
				continue;
			break;
		}
		xmlrpc_fault(res, -501, eString().sprintf("parameter type mismatch, expected %c as #%d", args[i], i));
		return 1;
	}
	return 0;
}

eHTTPXMLRPCResolver::eHTTPXMLRPCResolver()
{
}

eHTTPDataSource *eHTTPXMLRPCResolver::getDataSource(eString request, eString path, eHTTPConnection *conn)
{
	if ((path=="/RPC2") && (request=="POST"))
		return new eXMLRPCResponse(conn);
	if ((path=="/SID2") && (request=="POST"))
		return new eXMLRPCResponse(conn);
	return 0;
}

#endif //DISABLE_NETWORK

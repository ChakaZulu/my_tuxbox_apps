#include <core/dvb/serviceplaylist.h>
#include <core/dvb/servicefile.h>
#include <core/system/init.h>
#include <core/base/i18n.h>

/*
		eServicePlaylistHandler hooks into the file handler eServiceFileHandler
		and implements an "addFile" for various playlist formats.
		
		it implements also enterDirectory and leaveDirectory for reading
		the playlists.
		
		playlists are stored in eServiceFileHandler's cache in an eService-based
		structure (ePlaylist)
*/

ePlaylist::ePlaylist(): eService(0, "playlist")
{
	current=list.end();
}

int ePlaylist::load(const char *filename)
{
	eDebug("loading playlist... %s", filename);
	FILE *fp=fopen(filename, "rt");
	eString path=filename;
	int service_name_set=service_name!="playlist";
	if (!service_name_set)
		service_name=eString("playlist: ") + path.mid(path.rfind('/')+1);
	path=path.left(path.rfind('/')+1);
	
	int entries=0;
	if (!fp)	
	{
		eDebug("failed to open.");
		return -1;
	}
	int ignore_next=0;
	while (1)
	{
		char line[256];
		if (!fgets(line, 256, fp))
			break;
		line[strlen(line)-1]=0;
		if (strlen(line) && line[strlen(line)-1]=='\r')
			line[strlen(line)-1]=0;
		if (line[0]=='#')
		{
			if (!strncmp(line, "#SERVICE: ", 10))
			{
				eServiceReference ref(line+10);
				entries++;
				list.push_back(ref);
				ignore_next=1;
			}
			if (!strcmp(line, "#CURRENT"))
			{
				current=list.end();
				current--;
			}
			if (!strncmp(line, "#CURRENT_POSITION ", 18))
			{
				list.back().current_position=atoi(line+18);
				eDebug("parsed current_position (of %s) to %d", list.back().service.toString().c_str(), list.back().current_position);
			}
			if (!strncmp(line, "#TIME_BEGIN ", 12))
				list.back().time_begin=atoi(line+12);
			if (!strncmp(line, "#TIME_END ", 10))
				list.back().time_end=atoi(line+10);
			if (!strncmp(line, "#NAME ", 6))
			{
				service_name=line+6;
				service_name_set=1;
			}
				 
			continue;
		}

		if (!line[0])
			break;

		if (ignore_next)
		{
			ignore_next=0;
			continue;
		}

		eString filename="";
		if (line[0] != '/')
			filename=path;
		filename+=line;

		eServiceReference ref;
		if (eServiceFileHandler::getInstance()->lookupService(ref, filename.c_str()))
		{
			entries++;
			list.push_back(ref);
		} else
			eDebug("mit %s kann niemand was anfangen...", filename.c_str());
	}
	if (!service_name_set)
		service_name += eString().sprintf(_(" (%d entries)"), entries);
	fclose(fp);
	return 0;
}

int ePlaylist::save(const char *filename)
{
	FILE *f=fopen(filename, "wt");
	if (!f)
		return -1;
	fprintf(f, "#NAME %s\r\n", service_name.c_str());
	for (std::list<ePlaylistEntry>::iterator i(list.begin()); i != list.end(); ++i)
	{
		fprintf(f, "#SERVICE: %s\r\n", i->service.toString().c_str());
		if (i->current_position != -1)
			fprintf(f, "#CURRENT_POSITION %d\r\n", i->current_position);
		if ((int)i->time_begin != -1)
			fprintf(f, "#TIME_BEGIN %d\r\n", (int)i->time_begin);
		if ((int)i->time_end != -1)
			fprintf(f, "#TIME_END %d\r\n", (int)i->time_end);
		if (current == i)
			fprintf(f, "#CURRENT\n");
		if (i->service.path.size())
			fprintf(f, "%s\r\n", i->service.path.c_str());
	}
	fclose(f);
	return 0;
}

void eServicePlaylistHandler::addFile(void *node, const eString &filename)
{
	if (filename.right(4).upper()==".M3U")
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, eServiceReference::mustDescent|eServiceReference::canDescent|eServiceReference::sort1, filename));
}

eService *eServicePlaylistHandler::createService(const eServiceReference &node)
{
	ePlaylist *list=new ePlaylist();
	if (!node.path.empty())
	{
		if (!list->load(node.path.c_str()))
			return list;
		delete list;
		return 0;
	} else
		return list;
}

eServicePlaylistHandler *eServicePlaylistHandler::instance;

eServicePlaylistHandler::eServicePlaylistHandler(): eServiceHandler(ID)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServicePlaylistHandler::addFile);
	instance=this;
}

eServicePlaylistHandler::~eServicePlaylistHandler()
{
	instance=0;
}

eService *eServicePlaylistHandler::addRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->addRef(service);
}

void eServicePlaylistHandler::removeRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->removeRef(service);
}

void eServicePlaylistHandler::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	if (dir.type == id)
	{
		ePlaylist *service=(ePlaylist*)addRef(dir);
		if (!service)
			return;
	
		for (std::list<ePlaylistEntry>::iterator i(service->list.begin()); i != service->list.end(); ++i)
			callback(*i);
	
		removeRef(dir);
	}
	std::pair<std::multimap<eServiceReference,eServiceReference>::const_iterator,std::multimap<eServiceReference,eServiceReference>::const_iterator>
			range=playlists.equal_range(dir);
	while (range.first != range.second)
	{
		callback(range.first->second);
		++range.first;
	}
}

void eServicePlaylistHandler::leaveDirectory(const eServiceReference &dir)
{
}

int eServicePlaylistHandler::deleteService(const eServiceReference &dir, const eServiceReference &ref)
{
	ePlaylist *pl=(ePlaylist*)addRef(dir);
	if (!pl)
		return -1;
	for (std::list<ePlaylistEntry>::iterator i(pl->list.begin()); i != pl->list.end();)
		if (i->service == ref)
		{
			if (pl->current == i)
				pl->current++;
			i = pl->list.erase(i);
		} else
			++i;
	return 0;
}

int eServicePlaylistHandler::moveService(const eServiceReference &dir, const eServiceReference &ref, int dr)
{
	return -1;
}

eServiceReference eServicePlaylistHandler::newPlaylist(const eServiceReference &parent, const eServiceReference &ref)
{
	ASSERT(ref);
	playlists.insert(std::pair<eServiceReference,eServiceReference>(parent, ref));
	return ref;
}

void eServicePlaylistHandler::removePlaylist(const eServiceReference &service)
{
		// och menno.
	int found=1;
	while (found)
	{
		found=0;
		for (std::multimap<eServiceReference,eServiceReference>::iterator i(playlists.begin()); i != playlists.end();)
			if (i->second == service)
			{
				found=1;
				playlists.erase(i);
				break;
			}
	}
}

eAutoInitP0<eServicePlaylistHandler> i_eServicePlaylistHandler(7, "eServicePlaylistHandler");

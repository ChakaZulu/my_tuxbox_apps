#include <stdio.h>
#include "init.h"

int eInit::rl=-1;
std::list<std::pair<int,eAutoInit*> > *eInit::cl;

void eInit::add(int trl, eAutoInit *c)
{
	if (!cl)
		cl=new std::list<std::pair<int,eAutoInit*> >;
	cl->push_back(std::pair<int,eAutoInit*>(trl, c));
	if (rl>=trl)
		c->initNow();
}

void eInit::remove(int trl, eAutoInit *c)
{
	if (!cl)
		return;
	cl->remove(std::pair<int,eAutoInit*>(trl, c));
	if (rl>=trl)
		c->closeNow();
}

eInit::eInit()
{
}

eInit::~eInit()
{
	setRunlevel(-1);
	delete cl;
	cl=0;
}

void eInit::setRunlevel(int nrl)
{
	while (nrl>rl)
	{
		rl++;
		for (std::list<std::pair<int,eAutoInit*> >::iterator i(cl->begin()); i!=cl->end(); ++i)
		{
			if ((*i).first == rl)
			{
				printf("+ (%d) %s\n", rl, (*i).second->getDescription());
				(*i).second->initNow();
			}
		}
	}
	
	while (nrl<rl)
	{
		for (std::list<std::pair<int,eAutoInit*> >::iterator i(cl->begin()); i!=cl->end(); ++i)
			if ((*i).first == rl)
			{
				printf("- (%d) %s\n", rl, (*i).second->getDescription());
				(*i).second->closeNow();
			}
		rl--;
	}
	printf("reached rl %d\n", rl);
}

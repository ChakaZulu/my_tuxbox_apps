#include <time.h>
#include "enigma_event.h"
#include "rc.h"
#include "font.h"
#include "elabel.h"
#include "enigma.h"
#include "eskin.h"

void eEventDisplay::keyDown(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_RIGHT:
		++(*events);
		if (!events->current())
			events->toFirst();
		setEvent(events->current());
		break;
	case eRCInput::RC_LEFT:
		--(*events);
		if (!events->current())
			events->toLast();
		setEvent(events->current());
		break;
	}
}

void eEventDisplay::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_OK:
	case eRCInput::RC_HELP:
		close(0);
	}
}

#define ASSIGN(v, t, n)	\
	v =(t*)search(n); if (! v ) { qWarning("skin has undefined element: %s", n); v=new t(this); }

eEventDisplay::eEventDisplay(QString service, const QList<EITEvent> &e): eWindow(1), service(service)
	/*
			kleine anmerkung:
			
			(liste mit) pointer �bergeben ist scheisse, weil dazu THEORETISCH die eit gelockt werden M�SSTE (wird sie aber nicht,
			weil "wenn lock dann kein exec"), wenn sich die eit �ndert w�hrend wir hier im exec sind gibts nen CRASH.
			
			have fun.
			
			korrekterweise m�sste man hier ne au-eit �bergeben kriegen, sich auf dessen update connecten. das wiederrum suckt f�r
			sender�bergreifendes EI.
			
			durch das "setList" wurde das Problem zwar nicht gefixt, aber wenigstens crasht es jetzt nicht mehr, was aber nur funktioniert
			weil wir single-threading benutzen.
	*/
{
	eventlist=0;
	events=0;

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eventview"))
		qFatal("skin load of \"eventview\" failed");


	ASSIGN(long_description, eLabel, "epg_description");
	long_description->setFlags(RS_WRAP);
	ASSIGN(title, eLabel, "title");
	ASSIGN(eventTime, eLabel, "time");
	ASSIGN(eventDate, eLabel, "date");
	ASSIGN(channel, eLabel, "channel");
	
	title->setText("");
	long_description->setText("");
	eventDate->setText("");
	eventTime->setText("");
	channel->setText("");

	setList(e);
}

eEventDisplay::~eEventDisplay()
{
	delete events;
	delete eventlist;
}

void eEventDisplay::setEvent(EITEvent *event)
{

	if (event)
	{
		QString _title=0, _long_description="";
		QString _eventDate="";
		QString _eventTime="";


		tm *begin=event->start_time!=-1?localtime(&event->start_time):0;
		if (begin)
		{
			_eventTime.sprintf("%02d:%02d", begin->tm_hour, begin->tm_min);
			_eventDate=QString().sprintf("%d.%d.%4d", begin->tm_mday, begin->tm_mon+1, begin->tm_year+1900);
		}
		time_t endtime=event->start_time+event->duration;
		tm *end=event->start_time!=-1?localtime(&endtime):0;
		if (end)
		{
			_eventTime+=QString().sprintf(" - %02d:%02d", end->tm_hour, end->tm_min);
		}

		for (QListIterator<Descriptor> d(event->descriptor); d.current(); ++d)
		{
			if (d.current()->Tag()==DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *s=(ShortEventDescriptor*)d.current();
				_title=s->event_name;
				if ((s->text.length() > 0) && (s->text!=_title))
				{
					_long_description+=s->text;
					_long_description+="\n\n";
				}
			} else if (d.current()->Tag()==DESCR_EXTENDED_EVENT)
			{
				ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)d.current();
				_long_description+=ss->item_description;
			}
		}

		if (!_title)
			_title ="keine Information verf�gbar";

		eventTime->setText(_eventTime);
		eventDate->setText(_eventDate);

		title->setText(_title);
		long_description->setText(_long_description);
		channel->setText(service);
	} 
	else
	{
		title->setText(service);
		long_description->setText("keine Beschreibung verf�gbar");
	}
}

void eEventDisplay::setList(const QList<EITEvent> &e)
{
	if (eventlist)
		delete eventlist;
	if (events)
		delete events;
	eventlist=new QList<EITEvent>(e);
	events=new QListIterator<EITEvent>(*eventlist);
	setEvent(events->current());
}


#include <channelinfo.h>
#include <lib/base/i18n.h>
#include <lib/system/init.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/dvbservice.h>
#include <sys/stat.h>
#include <time.h>

eChannelInfo::eChannelInfo( eWidget* parent, const char *deco)
	:eDecoWidget(parent, 0, deco),
	ctime(this), cname(this), cdescr(this),
	cgenre(this), cdolby(this), cstereo(this),
	cformat(this), cscrambled(this), eit(0)
{
	cflags = 0;

	gFont fn = eSkin::getActive()->queryFont("eStatusBar");
	fn.pointSize = 28;
	cdescr.setFont( fn );
	cdescr.setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	cdescr.setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
	cdescr.setFlags( RS_FADE | eLabel::flagVCenter );

	fn.pointSize = 28;
	cgenre.setFont( fn );
	cgenre.setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	cgenre.setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
	cgenre.setFlags( RS_FADE | eLabel::flagVCenter );

	fn.pointSize = 30;
	cname.setFont( fn );
	cname.setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	cname.setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
	cname.setFlags( RS_FADE );
	
	ctime.setFont( fn );
	ctime.setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	ctime.setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
	ctime.setFlags( RS_FADE );

	gPixmap *pm=eSkin::getActive()->queryImage("sselect_dolby");
	cdolby.setPixmap(pm);
	cdolby.pixmap_position = ePoint(0,0);
	cdolby.hide();

	pm = eSkin::getActive()->queryImage("sselect_stereo");
	cstereo.setPixmap(pm);
	cstereo.pixmap_position = ePoint(0,0);
	cstereo.hide();

	pm = eSkin::getActive()->queryImage("sselect_format");
	cformat.setPixmap(pm);
	cformat.pixmap_position = ePoint(0,0);
	cformat.hide();

	pm = eSkin::getActive()->queryImage("sselect_crypt");
	cscrambled.setPixmap(pm);
	cscrambled.pixmap_position = ePoint(0,0);
	cscrambled.hide();
}

const char *eChannelInfo::genresTableShort[256] =
{
	/* 0x0 undefined */    	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 

	/* 0x1 Movie */        	_("Movie"),("Thriller"),_("Adventure"),_("SciFi"),_("Comedy"),
							_("Soap"),_("Romance"),_("Serious"),_("Adult"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,

	/* 0x2 News */         	_("News"),_("Weather"),_("Magazine"),_("Documentary"),_("Discussion"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,

	/* 0x3 Show */         	_("Show"),_("Game Show"),_("Variety"),_("Talk"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,

	/* 0x4 Sports */       	_("Sports"),("Special Event"),("Sports Mag."),("Football"),("Tennis"),("Team Sports"),
							_("Athletics"),("Motor Sports"),("Water Sports"),("Winter Sports"),("Equestrian"),
							_("Martial Sports"),
							NULL,NULL,NULL,NULL,

	/* 0x5 Children */     	_("Children"),("Pre-School"),("Age 6-14"),("Age 10-16"),("School"),
							_("Cartoons"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,

	/* 0x6 Music */        	("Music"),("Rock/Pop"),("Classical"),("Folk"),("Jazz"),("Musical"),("Ballet"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,

	/* 0x7 Culture */      	_("Culture"),("Perf. Arts"),("Fine Arts"),("Religion"),("Pop. Arts"),("Literatur"),
							_("Film"),("Experimental"),("Press"),("New Media"),("Art Mag."),("Fashion"),
							NULL,NULL,NULL,NULL,

	/* 0x8 Social */       	_("Social"),("Soc. Mag."),("Economics"),("Remark. People"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,

	/* 0x9 Education */    	_("Education"),("Nature"),("Technology"),("Medicine"),("Expeditions"),("Spiritual"),
							_("Further Ed."),("Languages"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

	/* 0xa Leisure */      	_("Hobbies"),("Travel"),("Handicraft"),("Motoring"),("Fitness"),("Cooking"),
							_("Shopping"),("Gardening"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

	/* 0xb Special */      	_("Orig. Lang."),("B&W"),("Unpublished"),("Live"),
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,

	/* 0xc reserved */     	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

	/* 0xd reserved */     	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

	/* 0xe reserved */     	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

	/* 0xf user defined */ 	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
							NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
};

void eChannelInfo::ParseEITInfo(EITEvent *e)
{
		eString t;
			
		if(e->start_time!=0)
		{
			tm *time=localtime(&e->start_time);
			starttime.sprintf("%02d:%02d", time->tm_hour, time->tm_min);
							
			t.sprintf("  (%dmin)", (int)(e->duration/60));
		}

		if (e->free_CA_mode )
			cflags |= cflagScrambled;
		
		for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
		{
//			eDebug(d->toString().c_str());
			Descriptor *descriptor=*d;
			if (descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
				name = ss->event_name;
				descr += ss->text;
				if( (!descr.isNull()) && (descr.c_str()[0])) descr += " ";
			}

			if (descriptor->Tag()==DESCR_COMPONENT)
			{
				ComponentDescriptor *cd=(ComponentDescriptor*)descriptor;
				
				if( cd->stream_content == 2 && cd->component_type == 5)
					cflags |= cflagDolby;
				else if( cd->stream_content == 2 && cd->component_type == 3)
					cflags |= cflagStereo;
				else if( cd->stream_content == 1 && (cd->component_type == 2 || cd->component_type == 3) )
					cflags |= cflagWide;
			}

			if(descriptor->Tag()==DESCR_CONTENT)
			{
				ContentDescriptor *cod=(ContentDescriptor*)descriptor;

				for(ePtrList<descr_content_entry_struct>::iterator ce(cod->contentList.begin()); ce != cod->contentList.end(); ++ce)
				{
					if(genresTableShort[ce->content_nibble_level_1*16+ce->content_nibble_level_2])
					{
						if ( !genre.length() )
							genre+="GENRE:";
						genre += gettext( genresTableShort[ce->content_nibble_level_1*16+ce->content_nibble_level_2] );
						genre += " ";
					}
				}
			}
		}
		if(!t.isNull()) name += t;

		int n = 0;
		cname.setText( name );
		cdescr.setText( descr );
		cgenre.setText( genre );
		ctime.setText( starttime );
		n = LayoutIcon(&cdolby, (cflags & cflagDolby), n);
		n = LayoutIcon(&cstereo, (cflags & cflagStereo), n);
		n = LayoutIcon(&cformat, (cflags & cflagWide), n );
		n = LayoutIcon(&cscrambled, (cflags & cflagScrambled), n );
}

void eChannelInfo::closeEIT()
{
	if (eit)
	{
//		eDebug("close EIT");
		eit->abort();
	}
}

void eChannelInfo::getServiceInfo( const eServiceReferenceDVB& service )
{
	closeEIT();
	if (eit)
	{
		eit=0;
		delete eit;
	}
	
	if (! service.path.size())
	{
		EITEvent *e = 0;
		e = eEPGCache::getInstance()->lookupEvent(service);
	//	eDebug(" e = %p", e);	
		if (e)  // data is in cache...
		{
	  	ParseEITInfo(e);
			delete e;
		}
		else  // we parse the eit...
		{
			cname.setText(_("no data for this service avail"));
	
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
			if (!sapi)
				return;
			eServiceReferenceDVB &ref = sapi->service;
	
			int type = ((service.getTransportStreamID()==ref.getTransportStreamID())
				&&	(service.getOriginalNetworkID()==ref.getOriginalNetworkID())) ? EIT::tsActual:EIT::tsOther;
	
			eit = new EIT( EIT::typeNowNext, service.getServiceID().get(), type );
			CONNECT( eit->tableReady, eChannelInfo::EITready );
			eit->start();
		}
	} else
	{
		// should be moved to eService
		eString filename=service.path;
		int slice=0;
		struct stat s;
		int filelength=0;
		while (!stat((filename + (slice ? eString().sprintf(".%03d", slice) : eString(""))).c_str(), &s))
		{
			filelength+=s.st_size/1024;
			slice++;
		}
		
		cname.setText(eString(_("Filesize: ")) + eString().sprintf("%d MB", filelength/1024));
	}
}
	
void eChannelInfo::EITready( int err )
{
//	eDebug("Channelinfo eit ready: %d", err);
	if (eit->ready && !eit->error)
	{
		if ( eit->events.size() )
			ParseEITInfo(eit->events.begin());
	}
	else if ( err == -ETIMEDOUT )
		closeEIT();
}

void eChannelInfo::update( const eServiceReferenceDVB& service )
{
	if (service)
	{
		current = service;
		getServiceInfo(current);
	}
}

void eChannelInfo::clear()
{
	name="";
	descr="";
	genre="";
	starttime="";
	cflags=0;
	cname.setText("");
	cdescr.setText("");
	cgenre.setText("");
	ctime.setText("");
	cdolby.hide();
	cstereo.hide();
	cformat.hide();
	cscrambled.hide();
}

int eChannelInfo::LayoutIcon(eLabel *icon, int doit, int num )
{
	if( doit )
	{
		int x,y;

		switch(num)
		{
			case 0:
				x=2;
				y=28;
				break;
			case 1:
				x=44;
				y=28;
				break;
			case 2:
				x=2;
				y=50;
				break;
			case 3:
				x=44;
				y=50;
				break;
			default:
				x=0;
				y=0;
				break;
		}
		icon->move(ePoint(x,y));
		icon->show();
		num++;
	}

	return num;

}

void eChannelInfo::redrawWidget(gPainter *target, const eRect& where)
{
	if ( deco )
		deco.drawDecoration(target, ePoint(width(), height()));

	target->line( ePoint(clientrect.left() + clientrect.width()/8 + 1, clientrect.top()),ePoint(clientrect.left() + clientrect.width()/8 + 1,clientrect.bottom()));
}

int eChannelInfo::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			if (deco)
				clientrect=crect;

			ctime.move( ePoint(0,0) );
			ctime.resize( eSize(clientrect.width() / 8, 36 ));
			cname.move( ePoint(clientrect.width() / 8 + 4, 0 ));
			cname.resize( eSize(clientrect.width() - (clientrect.width() / 8 + 4), clientrect.height()/3+2));
			cdescr.move( ePoint(clientrect.width() / 8 + 4, clientrect.height() / 3 + 2 ));
			cdescr.resize( eSize(clientrect.width() - (clientrect.width() / 8 + 4), (clientrect.height()/3)-2 ));
			cgenre.move( ePoint(clientrect.width() / 8 + 4, cdescr.getPosition().y() + cdescr.getSize().height()) );
			cgenre.resize( eSize( clientrect.width() - (clientrect.width() / 8 + 4), (clientrect.height()/3)-2 ));
			cdolby.resize( eSize(25,15) );
			cstereo.resize( eSize(25,15) );
			cformat.resize( eSize(25,15) );
			cscrambled.resize( eSize(25,15) );

			invalidate();
		break;

		default:
		break;
	}
	return eDecoWidget::eventHandler(event);
}
static eWidget *create_eChannelInfo(eWidget *parent)
{
	return new eChannelInfo(parent);
}

class eChannelInfoSkinInit
{
public:
	eChannelInfoSkinInit()
	{
		eSkin::addWidgetCreator("eChannelInfo", create_eChannelInfo);
	}
	~eChannelInfoSkinInit()
	{
		eSkin::removeWidgetCreator("eChannelInfo", create_eChannelInfo);
	}
};

eAutoInitP0<eChannelInfoSkinInit> init_eChannelInfoSkinInit(3, "eChannelInfo");

#include <streaminfo.h>

#include <stdlib.h>

#include <lib/driver/rc.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/servicemp3.h>
#include <lib/gdi/font.h>
#include <lib/gui/multipage.h>
#include <lib/gui/eskin.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/elabel.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/ebutton.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>


int eStreaminfo::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if ((event.action == &i_cursorActions->ok))
			close(0);
		else if (event.action == &i_cursorActions->right)
		{
			if (!mp.next())
				descr->setText( lb->goNext()->getHelpText() );
		}
		else if (event.action == &i_cursorActions->left)
		{
			if (!mp.prev())
				descr->setText( lb->goPrev()->getHelpText() );
		}
		else if (event.action == &i_cursorActions->up)
			;
		else if (event.action == &i_cursorActions->down)
			;
		else
			break;
		return 1;
	case eWidgetEvent::execBegin:
		takeFocus();
		lb->invalidate();
		break;
	case eWidgetEvent::execDone:
		releaseFocus();
		break;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}

static struct
{
	int value, mask;
	const char *description;
	int flag;
} caids[]=
	{{0x0100, 0xFF00, "Seca/Mediaguard (Canal+)"},
	{0x0200, 0xFF00, "CCETT"}, 
	{0x0300, 0xFF00, "MSG"}, 
	{0x0400, 0xFF00, "Eurodec"}, 
	{0x0500, 0xFF00, "Viaccess (France Telekom)"}, 
	{0x0600, 0xFF00, "Irdeto"}, 
	{0x0700, 0xFF00, "Jerrold/GI/Motorola"}, 
	{0x0800, 0xFF00, "Matra"}, 
	{0x0900, 0xFF00, "Videoguard (NDS)"}, 
	{0x0A00, 0xFF00, "Nokia"}, 
	{0x0B00, 0xFF00, "Conax (Norwegian Telekom)"}, 
	{0x0C00, 0xFF00, "NTL"}, 
	{0x0D00, 0xFF00, "Cryptoworks (Philips)"}, 
	{0x0E00, 0xFF00, "Power VU (Scientific Atlanta)"}, 
	{0x0F00, 0xFF00, "Sony"}, 
	{0x1000, 0xFF00, "Tandberg Television"}, 
	{0x1100, 0xFF00, "Thomson"}, 
	{0x1200, 0xFF00, "TV/Com"}, 
	{0x1300, 0xFF00, "HPT"}, 
	{0x1400, 0xFF00, "HRT"}, 
	{0x1500, 0xFF00, "IBM"}, 
	{0x1600, 0xFF00, "Nera"}, 
	{0x1702, 0xFFFF, "Betacrypt (Beta Technik) (C)"}, 
	{0x1722, 0xFFFF, "Betacrypt (Beta Technik) (D)"}, 
	{0x1762, 0xFFFF, "Betacrypt (Beta Technik) (F)"}, 
	{0x1700, 0xFF00, "Betacrypt (Beta Technik)"},
	{0x1800, 0xFF00, "Kudelski SA"}, 
	{0x1900, 0xFF00, "Titan Information Systems"}, 
	{0x2000, 0xFF00, "Telefonica"}, 
	{0x2100, 0xFF00, "STENTOR"}, 
	{0x2200, 0xFF00, "Tadiran Scopus"}, 
	{0x2300, 0xFF00, "BARCO AS"}, 
	{0x2400, 0xFF00, "Starguide Digital Networks"}, 
	{0x2500, 0xFF00, "Mentor Data Systems, Inc."}, 
	{0x2600, 0xFF00, "EBU"}, 
	{0x4700, 0xFF00, "General Instruments"}, 
	{0x4800, 0xFF00, "Telemann"}, 
	{0x4900, 0xFF00, "Digital TV Industry Alliance of China"}, 
	{0x4A60, 0xFFFF, "Tsinghua TongFang"}, 
	{0x4A70, 0xFFFF, "Dream Multimedia TV (DreamCrypt)"},
	{0x4A71, 0xFFFF, "Dream Multimedia TV (High security, 4096bit RSA)"},
	{0x4A72, 0xFFFF, "Dream Multimedia TV (Consumer, 48bit)"},
	{0x4A73, 0xFFFF, "Dream Multimedia TV (non-DVB)"},
	{0x4A74, 0xFFFF, "Dream Multimedia TV (export)"},
	{0x4A70, 0xFFF0, "Dream Multimedia TV (other)"},
	{0x0000, 0x0000, "other/unknown"}};

static void clearCA()
{
	for (unsigned int i=0; i< sizeof(caids)/sizeof(*caids); ++i)
		caids[i].flag=0;
}

static eString getCAName(int casysid, int always)
{
	for (unsigned int i=0; i< sizeof(caids)/sizeof(*caids); ++i)
		if ((casysid & caids[i].mask) == caids[i].value)
		{
			if ((caids[i].flag) && !always)
				return "";
			caids[i].flag=1;
			if (always)
				return caids[i].description;
			eString n;
			for (int a=0; a<4; ++a)
				if (caids[i].mask&(0xF<<((3-a)*4)))
					n+=eString().sprintf("%x", (casysid>>((3-a)*4))&0xF);
				else
					n+="x";
			n+="h ";
			n+=caids[i].description;
			return n;
		}
	return "";
}

class siTags: public eLabel
{
public:
	siTags(const eService *service, eWidget *parent);
};

static eString getDescription(eString tag)
{
	if (tag == "TALB")
		return _("album");
	else if (tag == "TIT2")
		return _("title");
	else if (tag == "TPE1")
		return _("artist");
	else if (tag == "TCON")
		return _("genre");
	else if (tag == "TDRC")
		return _("year");
	else
		return tag;
}

siTags::siTags(const eService *service, eWidget *parent): eLabel(parent)
{
	if (!service->id3)
		return;
	eString description;
	
	for (std::map<eString,eString>::const_iterator i(service->id3->tags.begin());
			i != service->id3->tags.end(); ++i)
		description+=getDescription(i->first) + ":\t" + i->second+"\n";
	setText(description);
}

class siPID: public eWidget
{
	eLabel *service_name[2], *service_provider[2], *apid[2], *vpid[2], *pcrpid[2], *pmtpid[2], *tpid[2], *vform[2], *tsid[2], *onid[2], *sid[2];
public:
	siPID(decoderParameters parms, const eService *service, eWidget *parent);
	void redrawWidget();
};

siPID::siPID(decoderParameters parms, const eService *cservice, eWidget *parent): eWidget(parent)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return;
	
	int yOffs=10;
	int fs=24;
	gFont fontfixed=eSkin::getActive()->queryFont("global.fixed");
	
	service_name[0]=new eLabel(this);
	service_name[0]->setText("Name:");
	service_name[0]->move(ePoint(10, yOffs));
	service_name[0]->resize(eSize(140, fs+5));

	service_name[1]=new eLabel(this);
	service_name[1]->setText(cservice?(cservice->service_name.c_str()):"--");
	service_name[1]->setFont(fontfixed);
	service_name[1]->move(ePoint(280, yOffs+2));
	service_name[1]->resize(eSize(260, fs+5));
	yOffs+=fs+5;

	service_provider[0]=new eLabel(this);
	service_provider[0]->setText("Provider:");
	service_provider[0]->move(ePoint(10, yOffs));
	service_provider[0]->resize(eSize(140, fs+5));
	
	service_provider[1]=new eLabel(this);
	service_provider[1]->setText((cservice && cservice->dvb)?cservice->dvb->service_provider.c_str():"--");
	service_provider[1]->setFont(fontfixed);
	service_provider[1]->move(ePoint(280, yOffs+2));
	service_provider[1]->resize(eSize(260, fs+5));
	yOffs+=fs+5;

	vpid[0]=new eLabel(this);
	vpid[0]->setText("Video PID:");
	vpid[0]->move(ePoint(10, yOffs));
	vpid[0]->resize(eSize(140, fs+5));
	
	vpid[1]=new eLabel(this);
	vpid[1]->setFont(fontfixed);
	vpid[1]->setText((parms.vpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", parms.vpid, parms.vpid));
	vpid[1]->move(ePoint(280, yOffs+2));
	vpid[1]->resize(eSize(260, fs+5));
	yOffs+=fs+5;

	apid[0]=new eLabel(this);
	apid[0]->setText("Audio PID:");
	apid[0]->move(ePoint(10, yOffs));
	apid[0]->resize(eSize(140, fs+5));
	
	apid[1]=new eLabel(this);
	apid[1]->setText((parms.apid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", parms.apid, parms.apid));
	apid[1]->move(ePoint(280, yOffs+2));
	apid[1]->resize(eSize(260, fs+5));
	apid[1]->setFont(fontfixed);
	yOffs+=fs+5;

	pcrpid[0]=new eLabel(this);
	pcrpid[0]->setText("PCR PID:");
	pcrpid[0]->move(ePoint(10, yOffs));
	pcrpid[0]->resize(eSize(140, fs+5));
	
	pcrpid[1]=new eLabel(this);
	pcrpid[1]->setText((parms.pcrpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", parms.pcrpid, parms.pcrpid));
	pcrpid[1]->move(ePoint(280, yOffs+2));
	pcrpid[1]->resize(eSize(260, fs+5));
	pcrpid[1]->setFont(fontfixed);
	yOffs+=fs+5;

	pmtpid[0]=new eLabel(this);
	pmtpid[0]->setText("PMT PID:");
	pmtpid[0]->move(ePoint(10, yOffs));
	pmtpid[0]->resize(eSize(140, fs+5));

	pmtpid[1]=new eLabel(this);
	pmtpid[1]->setText((parms.pmtpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", parms.pmtpid, parms.pmtpid));
	pmtpid[1]->move(ePoint(280, yOffs+2));
	pmtpid[1]->resize(eSize(260, fs+5));
	pmtpid[1]->setFont(fontfixed);
	yOffs+=fs+5;

	tpid[0]=new eLabel(this);
	tpid[0]->setText("Teletext PID:");
	tpid[0]->move(ePoint(10, yOffs));
	tpid[0]->resize(eSize(140, fs+5));
	
	tpid[1]=new eLabel(this);
	tpid[1]->setText((parms.tpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", parms.tpid, parms.tpid));
	tpid[1]->move(ePoint(280, yOffs+2));
	tpid[1]->resize(eSize(260, fs+5));
	tpid[1]->setFont(fontfixed);
	yOffs+=fs+5;
	
	eString vformat="n/a";
	FILE *bitstream=0;
	
	if (parms.vpid!=-1)
		bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres=0, yres=0, aspect=0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
		}
		fclose(bitstream);
		vformat.sprintf("%dx%d ", xres, yres);
		switch (aspect)
		{
		case 1:
			vformat+="square"; break;
		case 2:
			vformat+="4:3"; break;
		case 3:
			vformat+="16:9"; break;
		case 4:
			vformat+="20:9"; break;
		}
	}
	
	vform[0]=new eLabel(this);
	vform[0]->setText(_("Video format:"));
	vform[0]->move(ePoint(10, yOffs));
	vform[0]->resize(eSize(150, fs+5));
	
	vform[1]=new eLabel(this);
	vform[1]->setText(vformat);
	vform[1]->move(ePoint(280, yOffs));
	vform[1]->resize(eSize(260, fs));
	vform[1]->setFont(fontfixed);
	yOffs+=fs+5;

	tsid[0]=new eLabel(this);
	tsid[0]->setText("Transport Stream ID:");
	tsid[0]->move(ePoint(10, yOffs));
	tsid[0]->resize(eSize(230, fs+5));

	tsid[1]=new eLabel(this);
	tsid[1]->setText(eString().sprintf("%04xh", sapi->service.getTransportStreamID().get()));
	tsid[1]->move(ePoint(280, yOffs));
	tsid[1]->resize(eSize(130, fs+5));
	tsid[1]->setFont(fontfixed);
	yOffs+=fs+5;

	onid[0]=new eLabel(this);
	onid[0]->setText("Original Network ID:");
	onid[0]->move(ePoint(10, yOffs));
	onid[0]->resize(eSize(210, fs+5));

	onid[1]=new eLabel(this);
	onid[1]->setText(eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get()));
	onid[1]->move(ePoint(280, yOffs));
	onid[1]->resize(eSize(130, fs+5));
	onid[1]->setFont(fontfixed);
	yOffs+=fs+5;

	sid[0]=new eLabel(this);
	sid[0]->setText("Service ID:");
	sid[0]->move(ePoint(10, yOffs));
	sid[0]->resize(eSize(185, fs+5));

	sid[1]=new eLabel(this);
	sid[1]->setText(eString().sprintf("%04xh", sapi->service.getServiceID().get()));
	sid[1]->move(ePoint(280, yOffs));
	sid[1]->resize(eSize(130, fs+5));
	sid[1]->setFont(fontfixed);
	yOffs+=fs+5;
	
	eLabel *l=new eLabel(this);
	l->setText("Namespace:");
	l->move(ePoint(10, yOffs));
	l->resize(eSize(280, fs+5));
	
	l=new eLabel(this);
	l->setText(eString().sprintf("%04xh", sapi->service.getDVBNamespace().get()));
	l->move(ePoint(280, yOffs));
	l->resize(eSize(130, fs+5));
	l->setFont(fontfixed);
}

void siPID::redrawWidget()
{
}

class siCA: public eWidget
{
	eLabel *availca[2], *usedca[2];
public:
	siCA(eWidget *parent);
};

siCA::siCA(eWidget *parent): eWidget(parent)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return;
	
	int yOffs=0;
	int fs=eSkin::getActive()->queryValue("fontsize", 20);
	availca[0]=new eLabel(this);
	availca[0]->setText(_("supported coding systems:"));
	availca[0]->move(ePoint(10, 0));
	availca[0]->resize(eSize(420, fs+5));
	yOffs+=fs+5;

	eString availcas=_("none");
	clearCA();

	int numsys=0;
	std::set<int>& availCA = sapi->availableCASystems;
	for (std::set<int>::iterator i(availCA.begin()); i != availCA.end(); ++i)
	{
		if (!numsys)
			availcas="";
		eString caname=getCAName(*i, 0);
		if (caname)
		{
			availcas+= caname + "\n";
			if (numsys++>5)
			{
				availcas+="...\n";
				break;
			}
		}
	}

	if (!numsys)
		numsys++;

	availca[1]=new eLabel(this);
	availca[1]->setText(availcas);
	availca[1]->move(ePoint(10, fs*2));
	availca[1]->resize(eSize(420, numsys*fs+fs));
	
	int y=numsys*fs+fs*2+20;

	usedca[0]=new eLabel(this);
	usedca[0]->setText(_("Systems used in service:"));
	usedca[0]->move(ePoint(10, y));
	usedca[0]->resize(eSize(420, fs));

	eString usedcas=_("none");
	
	numsys=0;

	std::set<int>& calist = sapi->usedCASystems;

	for (std::set<int>::iterator i(calist.begin()); i != calist.end(); ++i)
	{
		if (!numsys)
			usedcas="";
		if (numsys++>5)
		{
			usedcas+="...\n";
			break;
		}
		eString caname=getCAName(*i, 1);
		usedcas+=eString().sprintf("%04xh:  ", *i) + caname + "\n";
	}
	
	if (!numsys)
		numsys++;

	usedca[1]=new eLabel(this);
	usedca[1]->setText(usedcas);
	usedca[1]->move(ePoint(10, y+50));
	usedca[1]->resize(eSize(420, numsys*fs+fs));
}

eStreaminfo::eStreaminfo(int mode, const eServiceReference &ref, decoderParameters *parms): eWindow(1), statusbar(this)
{
	setText(mode?"Record mode - read manual":"Streaminfo");
	cmove(ePoint(100, 80));
	cresize(eSize(450, 450));
	eSize s(clientrect.size());
	s.setHeight( s.height() - 40 );

	eService *service=eServiceInterface::getInstance()->addRef(ref);
	
	eWidget *w;
	
	if (ref.type == eServiceReference::idDVB)
	{
		w=new siPID(parms?*parms:Decoder::parms, service, this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	} else if (service && service->id3)
	{
		w=new siTags(service, this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	} else
	{
		w=new eLabel(this);
		w->setText(_("no information is available"));
		((eLabel*)w)->setFlags(eLabel::flagVCenter);
		((eLabel*)w)->setAlign(eTextPara::dirCenter);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	}

	mp.addPage(w);

	w=new siCA(this);
	w->move(ePoint(0, 0));
	w->resize( s );
	w->hide();

	mp.addPage(w);
	
	if ((ref.type == eServiceReference::idDVB) && (!ref.path))
	{
		w=new eWidget(this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();

		eTransponderWidget *t=new eTransponderWidget(w, 0, eTransponderWidget::deliverySatellite);
		t->move(ePoint(0, 0));
		t->resize(eSize(clientrect.width(), 200));
		t->load();
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
		if (sapi)
			t->setTransponder(sapi->transponder);

		eWidget *fe=new eFEStatusWidget(w, eFrontend::getInstance());
		fe->move(ePoint(0, 200));
		fe->resize(eSize(clientrect.width(), 100));
	} else
	{
		w=new eLabel(this);
		w->setText(_("no information is available"));
		((eLabel*)w)->setFlags(eLabel::flagVCenter);
		((eLabel*)w)->setAlign(eTextPara::dirCenter);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	}
	
	mp.addPage(w);
	mp.first();

	statusbar.loadDeco();
	statusbar.move( ePoint(0, clientrect.height()-40) );
	statusbar.resize( eSize(clientrect.width(), 40) );
	eRect rect = statusbar.getClientRect();

	lb = new eListBox<eListBoxEntryMenu>( &statusbar );
	lb->move( ePoint(rect.width()-50, 2) );
	lb->resize( eSize(45, rect.height()-5) );
	lb->setFlags( eListBoxBase::flagNoPageMovement | eListBoxBase::flagNoUpDownMovement );
	new eListBoxEntryMenu( lb, "1/3", _("Service information (right)"), eTextPara::dirCenter );
	new eListBoxEntryMenu( lb, "2/3", _("Scrambled system information (left, right)"), eTextPara::dirCenter );
	new eListBoxEntryMenu( lb, "3/3", _("Transponder information (left)"), eTextPara::dirCenter );
	descr = new eLabel( &statusbar );
	descr->move( ePoint(0,0) );
	descr->resize( eSize(rect.width() - 50, rect.height()) );
	descr->setText( lb->getCurrent()->getHelpText() );
	descr->setFlags( eLabel::flagVCenter );
	
	if (service)
		eServiceInterface::getInstance()->removeRef(ref);
}

eStreaminfo::~eStreaminfo()
{
}

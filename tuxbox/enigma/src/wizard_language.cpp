#include <locale.h>
#include <lib/gui/eskin.h>
#include <lib/gui/listbox.h>
#include <lib/gdi/font.h>
#include <lib/gdi/epng.h>
#include <lib/gui/emessage.h>
#include <lib/system/econfig.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include "wizard_language.h"

static const char * getCountry(const char *language);

class eLanguageEntry: public eListBoxEntry
{
	friend class eListBox<eLanguageEntry>;
	gPixmap *pixmap;
	eString id, name;
	eTextPara *para;
	static gFont font;
	int yOffs;
public:
	eLanguageEntry(eListBox<eLanguageEntry>* lb, eString id, eString name)
		: eListBoxEntry( (eListBox<eListBoxEntry>*)lb), id(id), name(name)
	{
		eString language=id;
		if (id.find('_') != eString::npos)
			id=id.left(id.find('_'));
		pixmap=eSkin::getActive()->queryImage(eString("country_") + getCountry(id.c_str()));
		if (!pixmap)
			pixmap=eSkin::getActive()->queryImage(eString("country_missing"));
		if (!font.pointSize)
			font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");
		para=0;
	}
	
	~eLanguageEntry()
	{	
		delete para;
	}
	
	const eString &getLanguageID() const
	{
		return id;
	}
	
protected:
	static int getEntryHeight()
	{
		return 50;
	}

	const eString& redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )
	{
		drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );

		if (!para)
		{
			para = new eTextPara( eRect( rect.left(), 0, rect.width(), rect.height() ) );
			para->setFont(font);
			para->renderString(name);
			para->realign(eTextPara::dirCenter);
			yOffs = ((rect.height() - para->getBoundBox().height()) / 2) - para->getBoundBox().top();
		}
		rc->renderPara(*para, ePoint(0, rect.top() + yOffs ) );
		if (pixmap)
			rc->blit(*pixmap, ePoint(rect.left()+20, rect.top()+5));

		return name;
	}
};

gFont eLanguageEntry::font;

		// maybe we should simply use the LANGUAGE :)
static struct
{
	char *language, *country;
} language2country[]={
		{"C", "en"},
		{"ar", "ae"},
		{"cs", "cz"},
		{"el", "gr"},
		{"sv", "se"},
		{"uk", "ua"}};

static const char * getCountry(const char *language)
{
	for (unsigned int i=0; i<sizeof(language2country)/sizeof(*language2country); ++i)
		if (!strcmp(language, language2country[i].language))
			return language2country[i].country;
	return language;
}

eWizardLanguage::eWizardLanguage()
{
	list=new eListBox<eLanguageEntry>(this);
	list->setName("list");
	
	head=new eLabel(this);
	head->setName("head");
	
	help=new eLabel(this);
	help->setName("help");
	
	CONNECT(list->selchanged, eWizardLanguage::selchanged);
	CONNECT(list->selected, eWizardLanguage::selected);

	if (eSkin::getActive()->build(this, "eWizardLanguage"))
		eFatal("skin load of \"eWizardLanguage\" failed");

	FILE *f=fopen(LOCALEDIR "/locale.alias", "rt");
	if (!f)
	{
		eDebug("eWizardLanguage: no languages defined!");
		return;
	}
	
	eLanguageEntry *cur=0;

	char *current;
	if ( eConfig::getInstance()->getKey("/elitedvb/language", current) )
		current=0;

	if ( current )
		oldLanguage=current;

	char line[256];
	while (fgets(line, 256, f))
	{
		line[strlen(line)-1]=0;
		char *d=line, *id;
		if ((id=strchr(line, ' ')))
		{
			*id++=0;
			eLanguageEntry *c=new eLanguageEntry(list, id, d);
			
			if ((current && !strcmp(id, current)) || !cur)
				cur=c;
		}
	}
	list->setCurrent(cur);
	if (current)
		free(current);
	fclose(f);
	
	setHelpID(89);

//	selchanged(cur);
}

void eWizardLanguage::selchanged(eLanguageEntry *entry)
{
	if (entry)
	{
		setlocale(LC_ALL, entry->getLanguageID().c_str());
		head->setText(_("Choose your language..."));
		help->setText(_("press up/down/ok"));
	}
}

int eWizardLanguage::eventHandler( const eWidgetEvent &e )
{
	switch( e.type )
	{
		case eWidgetEvent::wantClose:
			if (!e.parameter)
				eConfig::getInstance()->setKey("/elitedvb/language", list->getCurrent()->getLanguageID().c_str());
			else
				setlocale(LC_ALL, oldLanguage?oldLanguage.c_str():"");
			return eWindow::eventHandler(e);
		default:
			break;
	}
	return eWindow::eventHandler(e);
}

void eWizardLanguage::selected(eLanguageEntry *entry)
{
	if (entry)
		accept();
	else
		reject();
}

int eWizardLanguage::run()
{
	eWizardLanguage *wizard=new eWizardLanguage();
	wizard->show();
	int res=wizard->exec();
	wizard->hide();
	return res;
}

class eWizardLanguageInit
{
public:
	eWizardLanguageInit()
	{
		FILE *f=fopen(LOCALEDIR "/locale.alias", "rt");
			// only run wizzard when language not yet setup'ed
		char *temp;
		if (!eConfig::getInstance()->getKey("/elitedvb/language", temp) )
			free(temp);
		else if ( f )
			eWizardLanguage::run();
		else
			eDebug("no locales found (" LOCALEDIR "/locale.alias).. do not start language wizard");
		if (f)
			fclose(f);
	}
};

eAutoInitP0<eWizardLanguageInit> init_eWizardLanguageInit(eAutoInitNumbers::wizard, "wizard: language");

#include <lib/gui/statusbar.h>

#include <lib/system/init.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>

eStatusBar::eStatusBar( eWidget* parent, const char *deco)
	:eLabel(parent, 0, 0, deco), current(0)
{
	setFont( eSkin::getActive()->queryFont("eStatusBar") );
	setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
	eLabel::setFlags( RS_FADE );
	initialize();
}


void eStatusBar::initialize()
{
	if ( !(flags & flagOwnerDraw) && parent )
			CONNECT( parent->focusChanged, eStatusBar::update );
}

void eStatusBar::update( const eWidget* p )
{
	if (p)
	{
		current = p;
		setText( current->getHelpText() );
	}
}

void eStatusBar::setFlags( int fl )	
{
	if( fl == flagOwnerDraw )
	{
		flags = fl;
		initialize();
	}
	else
		eLabel::setFlags(fl);
}

int eStatusBar::setProperty(const eString &prop, const eString &value)
{
	if (prop=="ownerDraw")
		flags |= flagOwnerDraw;
	else
		return eLabel::setProperty(prop, value);

	initialize();
	
	return 0;
}

static eWidget *create_eStatusBar(eWidget *parent)
{
	return new eStatusBar(parent);
}

class eStatusBarSkinInit
{
public:
	eStatusBarSkinInit()
	{
		eSkin::addWidgetCreator("eStatusBar", create_eStatusBar);
	}
	~eStatusBarSkinInit()
	{
		eSkin::removeWidgetCreator("eStatusBar", create_eStatusBar);
	}
};

eAutoInitP0<eStatusBarSkinInit> init_eStatusBarSkinInit(3, "eStatusBar");

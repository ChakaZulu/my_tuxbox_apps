#include "echeckbox.h"

#include <core/gdi/font.h>
#include <core/system/init.h>
#include <core/gui/eskin.h>

eCheckbox::eCheckbox(eWidget *parent, int checked, int Size, eLabel* descr):
	eButton(parent, descr)
{
	setCheck(checked);
	CONNECT(selected, eCheckbox::sel);
}

eCheckbox::~eCheckbox()
{
}

void eCheckbox::sel()
{
	setCheck(ischecked^1);
	/*emit*/ checked(ischecked);
}

void eCheckbox::setCheck(int c)
{
	if (ischecked == c)
		return;
	ischecked=c;
	gPixmap *pm=eSkin::getActive()->queryImage(ischecked?"eCheckbox.checked":"eCheckbox.unchecked");
	setPixmap(pm);
}

int eCheckbox::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		pixmap_position=ePoint(0, (size.height()-16)/2);
		text_position=ePoint(20, 0);
		break;
	}
	return 0;
}


static eWidget *create_eCheckbox(eWidget *parent)
{
	return new eCheckbox(parent);
}

class eCheckboxSkinInit
{
public:
	eCheckboxSkinInit()
	{
		eSkin::addWidgetCreator("eCheckbox", create_eCheckbox);
	}
	~eCheckboxSkinInit()
	{
		eSkin::removeWidgetCreator("eCheckbox", create_eCheckbox);
	}
};

eAutoInitP0<eCheckboxSkinInit> init_eCheckboxSkinInit(3, "eCheckbox");

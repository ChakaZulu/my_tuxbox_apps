#include "menue.h"
#include "../include/debug.h"

CMenuWidget::CMenuWidget(string Name, FontsDef *Fonts, string Icon)
{
	name = Name;
	fonts = Fonts;
	iconfile = Icon;
	selected = -1;
}

CMenuWidget::~CMenuWidget()
{
	for(unsigned int count=0;count<items.size();count++)
	{
		delete items[count];
	}
	items.clear();
}

void CMenuWidget::addItem(CMenuItem* menuItem, bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	items.insert(items.end(), menuItem);
}


int CMenuWidget::exec(CFrameBuffer* frameBuffer, CRCInput* rcInput, CMenuTarget* parent, string)
{
	if(parent)
		parent->hide(frameBuffer);

	paint(frameBuffer);
	bool loop = true;
	int retval = CMenuItem::RETURN_REPAINT;
	while(loop)
	{
		int key = rcInput->getKey(300); 
		if(key==-1)
		{//timeout, close 
			loop = false;
		}
		else if (key==CRCInput::RC_up)
		{//search next selectable item
			for(unsigned int count=1;count<items.size();count++)
			{
				int pos = selected-count;
				if(pos<0)
					pos = items.size()-1;
				CMenuItem* item = items[pos];
				if( item->isSelectable())
				{
					//clear prev. selected
					items[selected]->paint(frameBuffer, false);
					//select new
					item->paint(frameBuffer, true);
					selected = pos;
					break;
				}
			}			
		}
		else if (key==CRCInput::RC_down)
		{//search next selectable item
			for(unsigned int count=1;count<items.size();count++)
			{
				int pos = (selected+count)%items.size();
				CMenuItem* item = items[pos];
				if( item->isSelectable())
				{
					//clear prev. selected
					items[selected]->paint(frameBuffer, false);
					//select new
					item->paint(frameBuffer, true);
					selected = pos;
					break;
				}
			}			
		}
		else if (key==CRCInput::RC_ok)
		{//exec this item...
			CMenuItem* item = items[selected];
			int ret = item->exec(frameBuffer, rcInput, this);			
			if(ret==CMenuItem::RETURN_EXIT)
			{
				loop = false;
			}
			else if(ret==CMenuItem::RETURN_EXIT_ALL)
			{
				ret = CMenuItem::RETURN_EXIT_ALL;
				loop = false;
			}
			else if(ret==CMenuItem::RETURN_REPAINT)
			{
				paint(frameBuffer);
			}
		}
	}
	hide(frameBuffer);
	return retval;
}

void CMenuWidget::hide(CFrameBuffer* frameBuffer)
{
		frameBuffer->paintBackgroundBoxRel(x,y, width,height );
}

void CMenuWidget::paint(CFrameBuffer* frameBuffer)
{
	width = 400;
	height = 400; // height(menu_title)+10+...
	x=((720-width)>>1) -20;
	y=(576-height)>>1;
	int hheight= 30; //fonts->menu_title->getHeight()+10;
	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD); //hheight was: 30

	fonts->menu_title->RenderString(x+36,y+30, width, name.c_str(), COL_MENUHEAD); //hheight was: 23
	//fonts->menu_title->RenderString(x+36,y+hheight-5, width, name.c_str(), COL_MENUHEAD); //hheight was: 23
	frameBuffer->paintIcon(iconfile.c_str(),x+8,y+6);

	int ypos = y+hheight; //hheight was: 30

	for(unsigned int count=0;count<items.size();count++)
	{
		CMenuItem* item = items[count];
		item->init(x,ypos, width);
		if( (item->isSelectable()) && (selected==-1) )
		{
			ypos = item->paint(frameBuffer, true);
			selected = count;
		}
		else
		{
			ypos = item->paint(frameBuffer, selected==((signed int) count) );
		}
	}
}


CMenuOptionChooser::CMenuOptionChooser(string OptionName, FontsDef* Fonts, int* OptionValue, bool Active, CChangeObserver* Observ)
{
	fonts=Fonts;
	height=fonts->menu->getHeight(); //was: 26;
	optionName = OptionName;
	active = Active;
	optionValue = OptionValue;
	observ=Observ;
}


CMenuOptionChooser::~CMenuOptionChooser()
{
	for(unsigned int count=0;count<options.size();count++)
	{
		delete options[count];
	}
	options.clear();
}

void CMenuOptionChooser::addOption(int key, string value)
{
		keyval *tmp = new keyval();
		tmp->key=key;
		tmp->value=value;
		options.insert(options.end(), tmp);
}

int CMenuOptionChooser::exec(CFrameBuffer*	frameBuffer, CRCInput*, CMenuTarget*)
{
	for(unsigned int count=0;count<options.size();count++)
	{
		keyval* kv = options[count];
		if(kv->key == *optionValue)
		{
			*optionValue = options[ (count+1)%options.size() ]->key;
			break;
		}
	}
	paint(frameBuffer, true);
	if(observ)
	{
		observ->changeNotify( optionName );
	}
	return 0;
}

int CMenuOptionChooser::paint(CFrameBuffer*	frameBuffer, bool selected)
{
	unsigned char color = COL_MENUCONTENT;
	if (selected)
		color = COL_MENUCONTENTSELECTED;
	if (!active)
		color = COL_MENUCONTENTINACTIVE;

	frameBuffer->paintBoxRel(x,y, dx, height, color );

	string option = "error";

	for(unsigned int count=0;count<options.size();count++)
	{
		keyval* kv = options[count];
		if(kv->key == *optionValue)
		{
			option = kv->value;
			break;
		}
	}

	int stringwidth =  fonts->menu->getRenderWidth(option.c_str());
	int stringstartposName = x+ 10;
	int stringstartposOption = x+ dx - stringwidth - 10;

	fonts->menu->RenderString(stringstartposName,   y+height,dx,  optionName.c_str(), color);

	fonts->menu->RenderString(stringstartposOption, y+height,dx,  option.c_str(), color);

	return y+height;
}

//+++++++++++++++++++++++++++++++++++++
CMenuForwarder::CMenuForwarder(string Text, FontsDef* Fonts, bool Active, char* Option, CMenuTarget* Target, string ActionKey="")
{
	fonts=Fonts;
	height=fonts->menu->getHeight(); //was:26
	text=Text;
	option = Option;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey;
}

int CMenuForwarder::exec(CFrameBuffer*	frameBuffer, CRCInput* rcInput, CMenuTarget* parent)
{
	if(jumpTarget)
	{
		return jumpTarget->exec(frameBuffer, rcInput, parent, actionKey);
	}
	else
	{
		return RETURN_EXIT;
	}
}


int CMenuForwarder::paint(CFrameBuffer*	frameBuffer, bool selected)
{
	int stringstartposX = x+10;

	unsigned char color = COL_MENUCONTENT;
	if (selected)
		color = COL_MENUCONTENTSELECTED;
	if (!active)
		color = COL_MENUCONTENTINACTIVE;

	frameBuffer->paintBoxRel(x,y, dx, height, color );
	fonts->menu->RenderString(stringstartposX, y+height,dx,  text.c_str(), color);

	if(option)
	{
		int stringwidth =  fonts->menu->getRenderWidth(option);
		int stringstartposOption = x+ dx - stringwidth - 10;
		fonts->menu->RenderString(stringstartposOption, y+height,dx,  option, color);
	}

	return y+height;
}

//+++++++++++++++++++++++++++++++++++++++

CMenuSeparator::CMenuSeparator(int Type, string Text, FontsDef* Fonts)
{
	fonts = Fonts;
	if(fonts!=NULL)
	    height = fonts->menu->getHeight(); // was: Height;
	else
	    height=6;
	text = Text;

	if ( (Type & ALIGN_LEFT) || (Type & ALIGN_CENTER) || (Type & ALIGN_RIGHT) )
	{
		type=Type;
	}
	else
	{
		type=Type | ALIGN_CENTER;
	}

}


int CMenuSeparator::paint(CFrameBuffer*	frameBuffer, bool selected)
{
	frameBuffer->paintBoxRel(x,y, dx, height, COL_MENUCONTENT );
	if(type&LINE)
	{
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1), COL_MENUCONTENT+5 );
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1)+1, COL_MENUCONTENT+2 );
	}
	if(type&STRING)
	{
		int stringwidth =  fonts->menu->getRenderWidth(text.c_str());
		int stringstartposX = 0;

		if(type&ALIGN_CENTER)
		{
			stringstartposX = (x+ (dx >> 1)) - (stringwidth>>1);
		}
		else if(type&ALIGN_LEFT)
		{
			stringstartposX = x+ 20;
		}
		else if(type&ALIGN_RIGHT)
		{
			stringstartposX = x+ dx - stringwidth - 20;
		}
	

		frameBuffer->paintBoxRel(stringstartposX-5, y, stringwidth +10, height, COL_MENUCONTENT );

		fonts->menu->RenderString(stringstartposX, y+height,dx,  text.c_str(), COL_MENUCONTENT);
	}
	return y+height;
}

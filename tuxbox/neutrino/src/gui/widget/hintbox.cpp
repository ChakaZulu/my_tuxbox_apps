#include "hintbox.h"
#include "../global.h"

CHintBox::CHintBox( CMenuWidget* Parent, string Caption, string Text)
{
	width = 500;
	height = 150;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	caption = Caption;
	text = Text;
	parent = Parent;
}

void CHintBox::paint()
{
	if (parent)
		parent->hide();
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(caption).c_str(), COL_MENUHEAD);

	g_FrameBuffer->paintBoxRel(x,y+theight+0, width,height - theight + 0, COL_MENUCONTENT);
	g_Fonts->menu_info->RenderString(x+10,y+theight+0+20, width, g_Locale->getText(text).c_str(), COL_MENUCONTENT);
}

void CHintBox::hide( bool showParent=true)
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
	if (showParent && parent)
		parent->paint();
}


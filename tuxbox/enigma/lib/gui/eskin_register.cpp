#include <lib/gui/eskin_register.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/gfbdc.h>
#include <lib/system/init.h>
#include <lib/system/econfig.h>

#define DEFAULTSKIN "chk_blue2.esml"

class eSkinInit
{
	eSkin default_skin;
public:
	eSkinInit()
	{
		if (default_skin.load( DATADIR "/enigma/skins/default.esml"))
			eFatal("skin load failed (" DATADIR "/enigma/skins/default.esml)");

		eString skinfile=DEFAULTSKIN;

		char *temp=0;
		if (!eConfig::getInstance()->getKey("/ezap/ui/skin", temp))
		{
			skinfile=temp;
			free(temp);
		}

		if (default_skin.load(skinfile.c_str()))
		{
			eWarning("failed to load user defined skin %s, falling back to " DEFAULTSKIN, skinfile.c_str());
			if (default_skin.load(DATADIR "/enigma/skins/" DEFAULTSKIN))
				eFatal("couldn't load fallback skin " DATADIR "/enigma/skins/" DEFAULTSKIN);
		}

		default_skin.parseSkins();

		default_skin.setPalette(gFBDC::getInstance());
		default_skin.makeActive();
	}
};

eAutoInitP0<eSkinInit> init_skin(2, "skin subsystem");

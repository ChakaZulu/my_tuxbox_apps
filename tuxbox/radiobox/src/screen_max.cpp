/*
 * $Header: /cvs/tuxbox/apps/tuxbox/radiobox/src/Attic/screen_max.cpp,v 1.1 2006/06/05 18:15:19 the_moon Exp $
 *
 * -- some odd module to calc max. screen usage of an menue
 * -- this should be somewhere else (neutrino needs redesign)
 *
 * (C) 2004 by  rasc
 *
 *
 */




#include "global.h"
#include "driver/screen_max.h"


// -- this is a simple odd class provided for 'static' usage
// -- to calculate max. usage of a preferred  menue size (x,y)
// -- this is due to 16:9 TV zoom functions, which are cutting menues.
// -- so using this function will make menues to obey max. screen settings.
// 
// usage:  e.g.  try to paint menue h: 500 w, 400  (but screen is limited
//         to (450 x 420)), functions will return 450 and 400
//         the _add factor is for boundary...
//
//         16:9 Zoom-Mode on a Thomson TV set will e.g. be 625x415
//
//  2004-03-17 rasc


int w_max (int w_size, int w_add)
{
	int dw;
	int ret;

	 dw = (g_settings.screen_EndX - g_settings.screen_StartX);

	 ret = w_size;
	 if (dw <= (w_size + w_add) ) ret = dw - w_add;

	 return ret;
}


int h_max (int h_size, int h_add)
{
	int dh;
	int ret;

	 dh = (g_settings.screen_EndY - g_settings.screen_StartY);

	 ret = h_size;
	 if (dh <= (h_size + h_add) ) ret = dh - h_add;

	 return ret;
}




/*
	Neutrino-GUI  -   DBoxII-Project


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __ch_mosaic__
#define __ch_mosaic__


#include "widget/menue.h"

#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>

#include "gui/color.h"


using namespace std;

class CChMosaicHandler
{
	public:
		int  exec( CMenuTarget* parent, string actionKey );	// menu call

};


class CChMosaic
{
	public:
		void CChMosaic (void);
		void ~CChMosaic (void);
		void doMosaic (void);

	private:
		CPIG	*pig;


#endif


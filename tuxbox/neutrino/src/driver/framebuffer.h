/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.
	

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

/*
$Id: framebuffer.h,v 1.7 2001/11/15 11:42:41 McClean Exp $


$Log: framebuffer.h,v $
Revision 1.7  2001/11/15 11:42:41  McClean
gpl-headers added

Revision 1.6  2001/10/16 19:11:16  rasc
-- CR LF --> LF in einigen Modulen




*/


#ifndef __framebuffer__
#define __framebuffer__

#include <linux/fb.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>

#include <string>

using namespace std;


class CFrameBuffer
{
	private:
	
		struct rgbData
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
		};

		string iconBasePath;

		int				fd;
		int				available;
		unsigned char	*background;
		int				backgroundColor;
		bool			useBackgroundPaint;
		unsigned int xRes, yRes, stride, bpp;
		struct fb_var_screeninfo screeninfo, oldscreen;
		fb_cmap cmap;
		__u16 red[256], green[256], blue[256], trans[256];

		void paletteFade(int i, __u32 rgb1, __u32 rgb2, int level);

	public:
		int getFileHandle(){return fd;}; //only used for plugins (games) !!

		//pointer to framebuffer
		unsigned char *lfb;
		unsigned int Stride() { return stride; }
  
		CFrameBuffer(const char *fb="/dev/fb/0");
		~CFrameBuffer();

		int setMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);


		//Palette stuff
		void paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
		void paletteSetColor(int i, __u32 rgb, int tr);
		void paletteSet();

		//paint functions
		void paintPixel(int x, int y, unsigned char col);
		
		void paintBox(int xa, int ya, int xb, int yb, unsigned char col);
		void paintBoxRel(int x, int y, int dx, int dy, unsigned char col);

		void paintLine(int xa, int ya, int xb, int yb, unsigned char col);

		void paintVLine(int x, int ya, int yb, unsigned char col);
		void paintVLineRel(int x, int y, int dy, unsigned char col);

		void paintHLine(int xa, int xb, int y, unsigned char col);
		void paintHLineRel(int x, int dx, int y, unsigned char col);


		bool paintIcon(string filename, int x, int y, unsigned char offset=0);
		bool paintIcon8(string filename, int x, int y, unsigned char offset=0);
		void loadPal(string filename, unsigned char offset=0, unsigned char endidx=255 );
		void setIconBasePath(string);

		bool CFrameBuffer::loadPicture2Mem(string filename, unsigned char* memp);

		void setBackgroundColor(int color);
		bool loadBackground(string filename, unsigned char col = 0);
		void useBackground(bool);

		void paintBackgroundBox(int xa, int ya, int xb, int yb);
		void paintBackgroundBoxRel(int x, int y, int dx, int dy);
};



#endif



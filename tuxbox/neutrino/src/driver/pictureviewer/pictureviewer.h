#ifndef __pictureviewer__
#define __pictureviewer__

/*
  pictureviewer  -   DBoxII-Project

  Copyright (C) 2001 Steffen Hehn 'McClean'
  Homepage: http://dbox.cyberphoria.org/



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


#include <string>
#include <stdio.h>    /* printf       */
#include <sys/time.h> /* gettimeofday */

#define FBV_SUPPORT_PNG
#define FBV_SUPPORT_BMP
#define FBV_SUPPORT_JPEG
//#define FBV_SUPPORT_GIF
#define FBV_SUPPORT_CRW

class CPictureViewer
{
	struct cformathandler 
	{
		struct cformathandler *next;
		int (*get_size)(const char *,int *,int*, int, int);
		int (*get_pic)(const char *,unsigned char **,int* ,int*);
		int (*id_pic)(const char *);
	};
	typedef  struct cformathandler CFormathandler;

 public:
	enum ScalingMode
		{
			NONE=0,
			SIMPLE=1,
			COLOR=2
		};
	CPictureViewer();
	~CPictureViewer(){Cleanup();};
	bool ShowImage(const std::string & filename, bool unscaled=false);
	bool DecodeImage(const std::string & name, bool showBusySign=false, bool unscaled=false);
	bool DisplayNextImage();
	void SetScaling(ScalingMode s){m_scaling=s;}
	void SetAspectRatio(float aspect_ratio) {m_aspect=aspect_ratio;}
	void showBusy(int sx, int sy, int width, char r, char g, char b);
	void hideBusy();
	void Zoom(float factor);
	void Move(int dx, int dy);
	void Cleanup();
	void SetVisible(int startx, int endx, int starty, int endy);
	static double m_aspect_ratio_correction;

 private:
	CFormathandler *fh_root;
	ScalingMode m_scaling;
	float m_aspect;
	std::string m_NextPic_Name;
	unsigned char* m_NextPic_Buffer;
	int m_NextPic_X;
	int m_NextPic_Y;
	int m_NextPic_XPos;
	int m_NextPic_YPos;
	int m_NextPic_XPan;
	int m_NextPic_YPan;
	std::string m_CurrentPic_Name;
	unsigned char* m_CurrentPic_Buffer;
	int m_CurrentPic_X;
	int m_CurrentPic_Y;
	int m_CurrentPic_XPos;
	int m_CurrentPic_YPos;
	int m_CurrentPic_XPan;
	int m_CurrentPic_YPan;
	
	unsigned char* m_busy_buffer;
	int m_busy_x;
	int m_busy_y;
	int m_busy_width;
	int m_busy_cpp;

	int m_startx;
	int m_starty;
	int m_endx;
	int m_endy;
	
	CFormathandler * fh_getsize(const char *name,int *x,int *y, int width_wanted, int height_wanted);
	void init_handlers(void);
	void add_format(int (*picsize)(const char *,int *,int*,int,int),int (*picread)(const char *,unsigned char **,int*,int*), int (*id)(const char*));

};


#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

#define dbout(fmt, args...) {struct timeval tv; gettimeofday(&tv,NULL); \
        printf( "PV[%ld|%02ld] " fmt, (long)tv.tv_sec, (long)tv.tv_usec/10000, ## args);}
#endif

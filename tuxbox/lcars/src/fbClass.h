#ifndef FB_H
#define FB_H

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <iostream.h>

#include <string>
#include <sstream>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

#define FB_DEV "/dev/fb/0"

class fbClass
{
	int fbfd;
	struct fb_var_screeninfo vinfo; // tats�chliche vinfo
	struct fb_fix_screeninfo finfo; // tats�chliche finfo
	struct fb_var_screeninfo old_vinfo; // gesicherte vinfo
	struct fb_fix_screeninfo old_finfo; // gesicherte finfo

	char *fbp; // Pointer auf memorymapped framebuffer

	int bytes_per_pixel;
	long int x_calc[720], y_calc[576];

	int transparent_color;

	__u16 r[256];
	__u16 g[256];
	__u16 b[256];
	__u16 tr[256];
	
	struct fb_cmap cmap;

	FT_Library  library;
	FT_Face      face;      
	FT_GlyphSlot  slot;
	
	FT_Face      face_vt;      
	FT_GlyphSlot  slot_vt;
	
	FT_UInt       glyph_index;
	float factor;

	struct font_cache
	{
		int size;
		
		int top;
		int left;
		int width;
		int rows;
		int pitch;
		int advancex;
		int bearingY;

		char bitmap[2000]; // very ugly, I know... :(
	};
	std::multimap<char, struct font_cache> cache;
	typedef std::multimap<char, struct font_cache>::iterator It;

	int fade_down[256]; // runtergerechnete farben
	char fades[10][10];
	std::multimap<float, int> ycorrector;

	std::multimap<std::string, std::string> variables;

public:
	fbClass(int x = 720, int y = 576, int bpp = 8);
	~fbClass();

	void addVariable(std::string, std::string);
	void addVariable(std::string, int);
	void clearVariables() { variables.clear(); }
	void runCommand(std::string command_string);

	// Paletten-Funktionen
	void setPalette(int color, int r, int g, int b, int transp);
	void setFade(int color, int r_start, int g_start, int b_start, int r_stop, int g_stop, int b_stop);
	void setTransparent(int color) { transparent_color = color; }
	void setMode(int x, int y, int bpp);

	// Mal-Funktionen
	void clearScreen();
	void fillScreen(int color);
	void setPixel(int x, int y, int color);
	void fillBox(int x1, int y1, int x2, int y2, int color);

	// Rueckgabe-Funktionen
	unsigned short int getPixel(int x, int y);
	int getMaxX();
	int getMaxY();
	int getHandle() { return fbfd; }
	int getWidth(char c);
	
	// Text Funktionen
	void loadFonts(std::string standardfont, std::string vtfont);
	void setTextSize(float setfactor);
	void draw_bitmap(font_cache font, int x, int y, int color);
	void putText(int xpos, int ypos, int color, int i, int max_size = -1, int alignment = 0);
	void putText(int xpos, int ypos, int color, char text[150], int max_size = -1, int alignment = 0);
	void putText(int xpos, int ypos, int color, std::string text, int max_size = -1, int alignment = 0);
};

#endif

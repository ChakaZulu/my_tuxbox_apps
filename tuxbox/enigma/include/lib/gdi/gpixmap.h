#ifndef __gpixmap_h
#define __gpixmap_h

#include <pthread.h>
#include <estring.h>
#include <erect.h>
#include "fb.h"
#include "elock.h"

struct gColor
{
	int color;
	gColor(int color): color(color)
	{
	}
	gColor(): color(0)
	{
	}
	operator int() const { return color; }
	int operator ==(const gColor &o) const { return o.color!=color; }
};

struct gRGB
{
	int b, g, r, a;
	gRGB(int r, int g, int b, int a=0): b(b), g(g), r(r), a(a)
	{
	}
	gRGB(unsigned long val): b(val&0xFF), g((val>>8)&0xFF), r((val>>16)&0xFF), a((val>>24)&0xFF)		// ARGB
	{
	}
	gRGB()
	{
	}
};

struct gPalette
{
	int start, len;
	gRGB *data;
};

/**
 * \brief A softreference to a font.
 *
 * The font is specified by a name and a size.
 * \c gFont is part of the \ref gdi.
 */
struct gFont
{
	eString family;
	int pointSize;
	
	/**
	 * \brief Constructs a font with the given name and size.
	 * \param family The name of the font, for example "NimbusSansL-Regular Sans L Regular".
	 * \param pointSize the size of the font in PIXELS.
	 */
	gFont(const eString &family, int pointSize):
			family(family), pointSize(pointSize)
	{
	}
	
	gFont()
	{
	}
};


struct gPixmap
{
	int x, y, bpp, bypp, stride;
	void *data;
	int colors;
	gRGB *clut;
	
	eLock contentlock;
	int final;
	
	gPixmap *lock();
	void unlock();
	
	eSize getSize() const { return eSize(x, y); }
	
	void fill(const eRect &area, const gColor &color);
	void blit(const gPixmap &src, ePoint pos, const eRect &clip=eRect());
	
	void mergePalette(const gPixmap &target);
	void line(ePoint start, ePoint end, gColor color);

	void finalLock();
	gPixmap();
	virtual ~gPixmap();
};

struct gImage: gPixmap
{
	gImage(eSize size, int bpp);
	~gImage();
};

#endif

#ifndef __FONT_H
#define __FONT_H

#include "fb.h"
#include <freetype/freetype.h>
#include <freetype/ftcache.h>
#include <freetype/cache/ftcchunk.h>
#include <freetype/cache/ftcglyph.h>
#include <freetype/cache/ftcimage.h>
#include <freetype/cache/ftcmanag.h>
#include <freetype/cache/ftcsbits.h>
#include <freetype/cache/ftlru.h>
#include <qsize.h>
#include <qpoint.h>
#include <qrect.h>
#include <qvector.h>
#include <qlist.h>

class FontRenderClass;
class Font;
class gPixmapDC;
class gFont;

class fontRenderClass
{ 
	friend class Font;
	friend class eTextPara;
	fbClass *fb;
	struct fontListEntry
	{
		char *filename, *face;
		fontListEntry *next;
		~fontListEntry();
	} *font;

	FT_Library library;
	FTC_Manager			cacheManager;				/* the cache manager							 */
	FTC_Image_Cache	imageCache;					/* the glyph image cache					 */
	FTC_SBit_Cache	 sbitsCache;					/* the glyph small bitmaps cache	 */

	int AddFont(const char *filename);
	FTC_FaceID getFaceID(const char *face);
	FT_Error getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit);
	static fontRenderClass *instance;
public:
	static fontRenderClass *getInstance();
	FT_Error FTC_Face_Requester(FTC_FaceID	face_id,
															FT_Face*		aface);
	Font *getFont(const char *face, int size, int tabwidth=-1);
	fontRenderClass();
	~fontRenderClass();
};

#define RS_WRAP		1
#define RS_DOT		2
#define RS_DIRECT	4
#define RS_FADE		8

#define GS_ISSPACE  1
#define GS_ISFIRST  2
#define GS_USED			4
#define GS_MYWRAP   8

struct pGlyph
{
	int x, y, w;
	Font *font;
	FT_ULong glyph_index;
	int flags;
};

class Font;
class eLCD;

class eTextPara
{
	Font *current_font;
	FT_Face current_face;
	int use_kerning;
	int previous;

	QRect area;
	QPoint cursor;
	QSize maximum;
	int left;
	QList<pGlyph> glyphs;
	int refcnt;

	int appendGlyph(FT_UInt glyphIndex, int flags);
	void newLine();
	void setFont(Font *font);
public:
	eTextPara(QRect area, QPoint start=QPoint(-1, -1))
		: area(area), cursor(start), maximum(0, 0), left(start.x())
	{
		current_font=0;
		current_face=0;
		refcnt=0;
	}
	~eTextPara();

	void destroy();
	eTextPara *grab();

	void setFont(const gFont &font);
	int renderString(const QString &string, int flags=0);

	void clear();

	void blit(gPixmapDC &dc, const QPoint &offset);

	enum
	{
		dirLeft, dirRight, dirCenter, dirBlock
	};
	void realign(int dir);
	QSize getExtend();
};

class Font
{
public:
	FTC_Image_Desc font;
	fontRenderClass *renderer;
	int ref;
	FT_Error getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit);
	FT_Face face;
	FT_Size size;
	
	int tabwidth;
	int height;
	Font(fontRenderClass *render, FTC_FaceID faceid, int isize, int tabwidth);
	~Font();
	
	void lock();
	void unlock();	// deletes if ref==0
};

extern fontRenderClass *font;

#endif

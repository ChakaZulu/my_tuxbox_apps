#ifndef __grc_h
#define __grc_h

/*
	gPainter ist die high-level version. die highlevel daten werden zu low level opcodes ueber
	die gRC-queue geschickt und landen beim gDC der hardwarespezifisch ist, meist aber auf einen
	gPixmap aufsetzt (und damit unbeschleunigt ist).
*/

#include <pthread.h>
#include <deque>
#include <stack>

#include <lib/base/estring.h>
#include <lib/base/erect.h>
#include <lib/system/elock.h>
#include <lib/gdi/gpixmap.h>


class eTextPara;

class gDC;
struct gOpcode
{
	enum Opcode
	{
		begin,
		
		renderText,
		renderPara,
		
		fill,
		blit,

		setPalette,
		mergePalette,
		
		line,
		
		clip,
		
		flush,
		end,
		
		shutdown
	} opcode;

	union para
	{
		struct pbegin
		{
			eRect area;
			pbegin(const eRect &area): area(area) { }
		} *begin;
		
		struct pfill
		{
			eRect area;
			gColor color;
			pfill(const eRect &area, gColor color): area(area), color(color) { }
		} *fill;

		struct prenderText
		{
			gFont font;
			eRect area;
			eString text;
			gRGB foregroundColor, backgroundColor;
			prenderText(const gFont &font, const eRect &area, const eString &text, const gRGB &foregroundColor, const gRGB &backgroundColor):
				font(font), area(area), text(text), foregroundColor(foregroundColor), backgroundColor(backgroundColor) { }
		} *renderText;

		struct prenderPara
		{
			ePoint offset;
			eTextPara *textpara;
			gRGB foregroundColor, backgroundColor;
			prenderPara(const ePoint &offset, eTextPara *textpara, const gRGB &foregroundColor, const gRGB &backgroundColor)
				: offset(offset), textpara(textpara), foregroundColor(foregroundColor), backgroundColor(backgroundColor) { }
		} *renderPara;

		struct psetPalette
		{
			gPalette *palette;
			psetPalette(gPalette *palette): palette(palette) { }
		} *setPalette;
		
		struct pblit
		{
			gPixmap *pixmap;
			ePoint position;
			eRect clip;
			pblit(gPixmap *pixmap, const ePoint &position, const eRect &clip)
				: pixmap(pixmap), position(position), clip(clip) { }
		} *blit;

		struct pmergePalette
		{
			gPixmap *target;
			pmergePalette(gPixmap *target): target(target) { }
		} *mergePalette;
		
		struct pline
		{
			ePoint start, end;
			gColor color;
			pline(const ePoint &start, const ePoint &end, gColor color): start(start), end(end), color(color) { }
		} *line;

		struct pclip
		{
			eRect clip;
			pclip(const eRect &clip): clip(clip) { }
		} *clip;
	} parm;

	int flags;
	
	gDC *dc;
};

class gRC
{
	static gRC *instance;
	
	static void *thread_wrapper(void *ptr);
	pthread_t the_thread;
	void *thread();
	
	eLock queuelock;
	std::deque<gOpcode> queue;
	
public:
	gRC();
	virtual ~gRC();

	void submit(const gOpcode &opcode);
	static gRC &getInstance();
};

class gPainter
{
	gDC &dc;
	gRC &rc;
	friend class gRC;

	gOpcode *beginptr;

			/* paint states */	
	std::stack<eRect> cliparea;
	gFont font;
	gColor foregroundColor, backgroundColor;
	ePoint logicalZero;
	void begin(const eRect &rect);
	void end();
public:
	gPainter(gDC &dc, eRect rect=eRect());
	virtual ~gPainter();

	void setBackgroundColor(const gColor &color);
	void setForegroundColor(const gColor &color);

	void setFont(const gFont &font);
	void renderText(const eRect &position, const std::string &string, int flags=0);
	void renderPara(eTextPara &para, ePoint offset=ePoint(0, 0));

	void fill(const eRect &area);
	
	void clear();
	
	void blit(gPixmap &src, ePoint pos, eRect clip=eRect(), int flags=0);

	void setPalette(gRGB *colors, int start=0, int len=256);
	void mergePalette(gPixmap &target);
	
	void line(ePoint start, ePoint end);

	void setLogicalZero(ePoint abs);
	void moveLogicalZero(ePoint rel);
	void resetLogicalZero();
	
	void clip(eRect clip);
	void clippop();

	void flush();
};

class gDC
{
protected:
	eLock dclock;
public:
	virtual void exec(gOpcode *opcode)=0;
	virtual gPixmap &getPixmap()=0;
	virtual eSize getSize()=0;
	virtual const eRect &getClip()=0;
	virtual gRGB getRGB(gColor col)=0;
	virtual ~gDC();
	void lock() { dclock.lock(1); }
	void unlock() { dclock.unlock(1); }
};

class gPixmapDC: public gDC
{
protected:
	gPixmap *pixmap;
	eRect clip;

	void exec(gOpcode *opcode);
	gPixmapDC();
public:
	gPixmapDC(gPixmap *pixmap);
	virtual ~gPixmapDC();
	gPixmap &getPixmap() { return *pixmap; }
	gRGB getRGB(gColor col);
	const eRect &getClip() { return clip; }
	virtual eSize getSize() { return eSize(pixmap->x, pixmap->y); }
};

#endif

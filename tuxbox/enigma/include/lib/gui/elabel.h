#ifndef __elabel_h
#define __elabel_h

#include <lib/gui/ewidget.h>
#include <lib/gdi/grc.h>

// Definition Blit Flags
#define BF_ALPHATEST 1

class eLabel: public eDecoWidget
{
protected:
	int blitFlags;
	int flags;
	eTextPara *para;
	gColor transparentBackgroundColor;
	int align;
	void validate( const eSize* s=0 );
	int eventHandler(const eWidgetEvent &event);
	void redrawWidget(gPainter *target, const eRect &area);
	int setProperty(const eString &prop, const eString &value);
	int yOffs;
	gPixmap *shortcutPixmap; // shortcut pixmap to be displayed right after description
public:
	void invalidate();
	enum { flagVCenter = 64 };
	eLabel(eWidget *parent, int flags=0 /* RS_WRAP */ , int takefocus=0, const char* deco="eLabel" );
	~eLabel();

	void setBlitFlags( int flags );
	void setFlags(int flags);
	void removeFlags(int flags);
	void setAlign(int align);

	eSize getExtend();
	ePoint getLeftTop();

	ePoint pixmap_position, text_position;
};

#endif

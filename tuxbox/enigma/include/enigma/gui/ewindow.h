#ifndef __ewindow_h
#define __ewindow_h

#include "ewidget.h"

/**
 * \brief A (decorated) top level widget.
 *
 * An eWindow is whats actually seen as a window. It's top level (thus you cannot specify a parent),
 * and may have a (skinned) decoration. It's clientrect is usually a bit smaller since it has a titlebar
 * and a border.
 *
 */
class eWindow: public eWidget
{
//	Q_OBJECT

	gPixmap *iTopLeft, *iTop, 
			*iTopRight, *iLeft, *iRight, 
			*iBottomLeft, *iBottom, *iBottomRight;
protected:
	int borderTop, borderLeft, borderBottom, borderRight, titleOffsetX, titleOffsetY, titleFontSize, titleHeight, titleBorderY;
	void redrawWidget(gPainter *target, const eRect &where);
	void drawTitlebar(gPainter *target);
  void OnFontSizeChanged(int NewFontSize);
	void recalcClientRect();
	int eventFilter(const eWidgetEvent &event);
	void willShow();
	void willHide();
public:

	/**
	 * \brief Constructs the window
	 *
	 * \arg takefocus the \c eWidget::eWidget takefocus parameter. You don't need to set it if just
	 * one widget \e inside the parent needs focus. That widget can apply for it by itself.
	 */
	eWindow(int takefocus=0);
	
	/**
	 * destructs the window.
	 */
	~eWindow();
};

#endif

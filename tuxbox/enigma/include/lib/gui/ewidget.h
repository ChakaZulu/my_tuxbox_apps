#ifndef __ewidget_h
#define __ewidget_h

//#include <qobject.h>
#include <ebase.h>
#include "epoint.h"
#include "esize.h"
#include "erect.h"
#include <qlist.h>
#include "grc.h"
#include <list>
#include <libsig_comp.h>

class eWidgetEvent
{
public:
	enum eventType
	{
		keyUp, keyDown,
		willShow, willHide,
		execBegin, execDone,
		gotFocus, lostFocus,
		
		changedText, changedFont, changedForegroundColor, changedBackgroundColor,
		changedSize, changedPosition, changedPixmap,
	} type;
	int parameter;
	eWidgetEvent(eventType type, int parameter=0): type(type), parameter(parameter) { }
};

/** \brief The main widget class. All widgets inherit this class.
 * eWidget handles focus management.
 */
class eWidget: public Object
{
//	Q_OBJECT
	enum
	{
		stateShow=1
	};

public:// slots:
	/**
	 * \brief Exits a (model) widget.
	 *
	 * Quit the local event loop, thus returning the control to the function which called \c exec.
	 * \sa eWidget::accept
	 * \sa eWidget::reject
	 */
	void close(int result);
	
	/**
	 * \brief Closes with a returncode of 0 (success).
	 *
	 * Synonym to \c close(0);. Useful to use as a slot.
	 * \sa eWidget::close
	 */
	void accept();

	/**
	 * \brief Closes with a returncode of -1 (failure).
	 *
	 * Synonym to \c close(-1);. Useful to use as a slot.
	 * \sa eWidget::close
	 */
	void reject();
	
	std::list<eWidget*> childlist;
	
protected:
	eWidget *parent;
	QString name;
	ePoint position;
	eSize size;
	eRect clientrect;
	eRect clientclip;
	QList<eWidget> _focusList;
	eWidget *oldfocus;
	int takefocus;
	int state;
	
	gDC *target;

	inline eWidget *getTLW()
	{
		return parent?parent->getTLW():this;
	}
	inline eWidget *getNonTransparentBackground()
	{
		if (getBackgroundColor()!=-1)
			return this;
		return parent?parent->getNonTransparentBackground():this;
	}
	int result, in_loop, have_focus, just_showing;
	void takeFocus();
	void releaseFocus();

	void _willShow();
	void _willHide();
	
	virtual void willShow();
	virtual void willHide();
	
	virtual void setPalette();

	void willShowChildren();
	void willHideChildren();
	
	virtual int eventFilter(const eWidgetEvent &event);	/** 0 for 'no action taken' */

	virtual void keyDown(int rc);
	virtual void keyUp(int rc);
	
	virtual void gotFocus();
	virtual void lostFocus();
	
	virtual void recalcClientRect();
	void recalcClip();
	void checkFocus();

			// generic properties
	gFont font;
	QString text;
	gColor backgroundColor, foregroundColor;
	
	gPixmap *pixmap;

public:
	eWidget *LCDTitle;
	eWidget *LCDElement;
	eWidget *LCDTmp;

	inline ePoint getAbsolutePosition()
	{
		return (parent?(parent->getAbsolutePosition()+parent->clientrect.topLeft()+position):position);
	}
	inline ePoint getTLWPosition()
	{
		return (parent?(parent->getTLWPosition()+parent->clientrect.topLeft()+position):ePoint(0,0));
	}
	virtual void redrawWidget(gPainter *target, const eRect &area);
	virtual void eraseBackground(gPainter *target, const eRect &area);

	/**
	 * \brief Constructs a new eWidget. 
	 * \param parent The parent widget. The widget gets automatically removed when the parent gets removed.
	 * \param takefocus Specifies if the widget should be appended to the focus list of the TLW, i.e. if it can
	          receive keys.
	 */
	eWidget(eWidget *parent=0, int takefocus=0);

	/**
	 * \brief Destructs an eWidget and all its childs.
	 *
	 * hide() is called when the widget is shown. The set ePixmap is \e not
	 * freed. If the widget acquired focus, it will be removed from the focuslist.
	 * \sa eWidget::setPixmap
	 */
	virtual ~eWidget();
	
	/**
	 * \brief Returns a pointer to the focus list.
	 *
	 * The focus list is the list of childs which have the \c takefocus flag set.
	 * This list is only maintained for TLWs.
	 */
	QList<eWidget> *focusList() { return &_focusList; }

	/**
	 * \brief Resizes the widget.
	 *
	 * Sets the size of the widget to the given size. The event \c changedSize event will be generated.
	 * \param size The new size, relative to the position.
	 */
	void resize(eSize size);
	
	/**
	 * \brief Moves the widget.
	 *
	 * Set the new position of the widget to the given position. The \c changedPosition event will be generated.
	 * \param position The new position, relative to the parent's \c clientrect.
	 */
	void move(ePoint position);
	
	/**
	 * \brief Returns the current size.
	 *
	 * \return Current size of the widget, relative to the position.
	 */
	eSize getSize() { return size; }
	
	/** 
	 * \brief Returns the current position.
	 *
	 * \return Current position, relative to the parent's \c clientrect.
	 */
	ePoint getPosition() { return position; }
	
	/**
	 * \brief Returns the size of the clientrect.
	 *
	 * \return The usable size for the childwidgets.
	 */
	eSize getClientSize() { return clientrect.size(); }
	
	/**
	 * \brief Returns the clientrect.
	 *
	 * \return The area usable for the childwidgets.
	 */
	eRect getClientRect() { return clientrect; }
	
	/**
	 * \brief Recursive redraw of a widget.
	 *
	 * All client windows get repaint too, but no widgets above. Unless you have a good reason, you shouldn't
	 * use this function and use \c invalidate().
	 * \param area The area which should be repaint. The default is to repaint the whole widget.
	 * \sa eWidget::invalidate
	 */
	void redraw(eRect area=eRect());
	
	/**
	 * \brief Recursive (complete) redraw of a widget.
	 *
	 * Redraws the widget including background. This is the function to use if you want to manually redraw something!
	 * \param area The area which should be repaint. The default is to repaint the whole widget.
	 * \sa eWidget::redraw
	 */
	void invalidate(eRect area=eRect());
	
	/**
	 * \brief Enters modal message loop.
	 *
	 * A new event loop will be launched. The function returns when \a close is called.
	 * \return The argument of \a close.
	 * \sa eWidget::close
	 */
	int exec();
	
	/**
	 * \brief Visually clears the widget.
	 *
	 * Clears the widget. This is done on \a hide().
	 * \sa eWidget::hide
	 */
	void clear();
	
	/**
	 * \brief Delivers a widget-event.
	 *
	 * \param event The event to deliver.
	 */
	void event(const eWidgetEvent &event);
	
	/**
	 * \brief Shows the widget.
	 *
	 * If necessary, the widget will be linked into the TLW's active focus list. The widget will
	 * visually appear.
	 * \sa eWidget::hide
	 */
	void show();
	
	/** 
	 * \brief Hides the widget.
	 *
	 * The widget will be removed from the screen. All childs will be hidden too.
	 * \sa eWidget::show
	 */
	void hide();
	
	/** 
	 * \brief Returns if the widget is vissible.
	 *
	 * \return If the widget and all parents are visible, \c true is returned, else false.
	 */
	int isVisible() { return (state&stateShow) && ((!parent) || parent->isVisible()); }
	
	/**
	 * \brief changes the focused widget.
	 *
	 * Focuses the next or previous widget of the \c focuslist. An \c gotFocus and \c lostFocus event will be
	 * generated.
	 * \param dir The direction. 0 is forward, 1 is backward.
	 */
	void focusNext(int dir=0);
	
	/**
	 * \brief Sets the widget font.
	 *
	 * The font is used for example by the \c eLabel.
	 * \sa eLabel
	 * \param font The new font used by widget-specific drawing code.
	 */
	void setFont(const gFont &font);
	
	/**
	 * \brief Sets the widget caption or text.
	 *
	 * \param label The text to assign to the widget.
	 */
	void setText(const QString &label);
	
	const	QString& getText() { return text; }
	void setBackgroundColor(gColor color);
	void setForegroundColor(gColor color);
	void setPixmap(gPixmap *pmap);
	void setTarget(gDC *target);
	void setLCD(eWidget *lcdtitle, eWidget *lcdelement);
	void setName(const char *name);
	
	gColor getBackgroundColor() { return backgroundColor; }
	gColor getForegroundColor() { return foregroundColor; }
	
	int width() { return getSize().width(); }
	int height() { return getSize().height(); }
	
	gPainter *getPainter(eRect area=eRect());
	
	/**
	 * \brief Sets a property.
	 *
	 * A property is a value/data pair which is used for serializing widgets (like in skinfiles).
	 * These properties are available to all \c "eWidget"-based classes.
	 * \arg \c position, the position of the widget, relative to the parent's childarea. Consider using csize for TLWs.
	 * Positions are specified in a "x:y" manner.
	 * \arg \c cposition, the position of the widget's clientrect (upper left). 
	 * This is useful for specifing a position independant of a decoration which might be
	 * different sized. The real position will be calculated to match the requested position.
	 * \arg \c size, the size of the widget. Consider using csize for TLWs. Sizes are specified in a "width:height" manner.
	 * \arg \c csize, the size of the clientrect. The real size will be calculated to match the requested size.
	 * \arg \c text, the text/caption of the widget.
	 * \arg \c font, the primary font used in the widget.
	 * \arg \c name, the name of the widget for referring them.
	 * \arg \c pixmap, an already loaded, named pixmap to be used as the widget's pixmap.
	 * \arg \c foregroundColor, a named color, which will be used for the widget's foreground color.
	 * \arg \c backgroundColor
	 * \param prop The property to be set.
	 * \param value The value to be set.
	 */
	virtual int setProperty(const QString &prop, const QString &value);
	
	eWidget *search(const QString &name);
};

#endif

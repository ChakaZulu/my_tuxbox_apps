#ifndef __listbox_h
#define __listbox_h

#include <sstream>

#include <core/driver/rc.h>
#include <core/gdi/grc.h>
#include <core/gdi/fb.h>
#include <core/gui/ewidget.h>
#include <core/gui/eskin.h>
#include <core/gui/ewindow.h>
#include <core/gui/guiactions.h>
#include <core/gui/decoration.h>
#include <core/gui/statusbar.h>

int calcFontHeight( const gFont& font );

class eListBoxBase: public eWidget
{
	eDecoration deco, deco_selected;	
	gPixmap *iArrowUpDown, *iArrowUp, *iArrowDown, *iArrowLeft, *iArrowRight;
protected:
	const eWidget* descr;
	eLabel* tmpDescr; // used for description Label in LCD
	gColor colorActiveB, colorActiveF;
	eRect crect, crect_selected;
	int MaxEntries, item_height, flags;
public:
	enum
	{
		flagNoUpDownMovement=1,
		flagNoPageMovement=2,
		flagLoadDeco=4
	};
	void setFlags(int);
protected:
	eListBoxBase(eWidget* parent, const eWidget* descr=0);
	eRect getEntryRect(int n);
	int setProperty(const eString &prop, const eString &value);
	int eventHandler(const eWidgetEvent &event);
	void recalcMaxEntries();
	void recalcClientRect();
	void redrawBorder(gPainter *target, eRect &area);
	void invalidateEntry(int n){	invalidate(getEntryRect(n));}
	int newFocus();
	void gotFocus();
	void lostFocus();
	void loadDeco();
};

template <class T>
class eListBox: public eListBoxBase
{
	typedef typename ePtrList<T>::iterator ePtrList_T_iterator;

	ePtrList<T> childs;
	ePtrList_T_iterator top, bottom, current;
	int recalced;
public:
	eListBox(eWidget *parent, const eWidget* descr=0 );
	~eListBox();

	void init();

	void append(T* e, bool holdCurrent=false);
	void remove(T* e, bool holdCurrent=false);
	void clearList();

	Signal1<void, T*> selected;	
	Signal1<void, T*> selchanged;

	void setCurrent(const T *c);
	T* getCurrent()	{ return current != childs.end() ? *current : 0; }
	T* goNext();
	T* goPrev();

	void sort();

	template <class Z>
	void forEachEntry(Z ob)
	{
		for (ePtrList_T_iterator i(childs.begin()); i!=childs.end(); ++i)
			if (ob(**i))
				break;
	}

	enum
	{
		dirPageDown, dirPageUp, dirDown, dirUp, dirFirst
	};

	int moveSelection(int dir);
	void setActiveColor(gColor back, gColor front);
	void redrawWidget(gPainter *target, const eRect &area);
	int eventHandler(const eWidgetEvent &event);
	void lostFocus();
	void gotFocus();
};

class eListBoxEntry: public Object
{
	friend class eListBox<eListBoxEntry>;
protected:
	eListBox<eListBoxEntry>* listbox;
public:
	eListBoxEntry(eListBox<eListBoxEntry>* parent)
		:listbox(parent)
	{	
		if (listbox)
			listbox->append(this);
	}
	virtual ~eListBoxEntry()
	{
		if (listbox)
			listbox->remove(this);
	}

	void drawEntryRect( gPainter* rc, const eRect& where, const gColor& coActiveB, const gColor& coActiveF, const gColor& coNormalB, const gColor& coNormalF, int state );
};

class eListBoxEntryText: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryText>;
protected:
	eString text;
	void *key;
	int align;
	eTextPara *para;
	int yOffs;
	static gFont font;
public:
	static int getEntryHeight();

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const char* txt=0, void *key=0, int align=0 )
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb ), text(txt), key(key), align(align), para(0)
	{
	}

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const eString& txt, void* key=0, int align=0 )
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb ), text(txt), key(key), align(align), para(0)
	{
	}

	~eListBoxEntryText();
	
	bool operator < ( const eListBoxEntryText& e) const
	{
		if (key == e.key)
			return text < e.text;	
		else
			return key < e.key;
	}
	
	void *& getKey() { return key; }
	const eString& getText() { return text; }

protected:
	const eString& redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryTextStream: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTextStream>;
protected:
	std::stringstream text;
	static gFont font;
public:
	static int getEntryHeight();

	eListBoxEntryTextStream(eListBox<eListBoxEntryTextStream>* lb)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb)
	{		
	}

	bool operator < ( const eListBoxEntryTextStream& e) const
	{
		return text.str() < e.text.str();	
	}

protected:
	eString redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryMenu: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryMenu>;
	eString helptext;
public:
	const eString &getHelpText() const { return helptext; }
	Signal0<void> selected;

	eListBoxEntryMenu(eListBox<eListBoxEntryMenu>* lb, const char* txt, const char* hlptxt=0, int align=0 )
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, txt, 0, align), helptext(hlptxt?hlptxt:_("no description avail") )
	{
		if (listbox)
			CONNECT(listbox->selected, eListBoxEntryMenu::LBSelected);
	}
	void LBSelected(eListBoxEntry* t)
	{
		if (t == this)
			/* emit */ selected();
	}
};

////////////////////////////////////// inline Methoden eListBox //////////////////////////////////////////


template <class T>
inline void eListBox<T>::append(T* entry, bool holdCurrent)
{
	T* cur = 0;
	if (holdCurrent)
		cur = current;

	childs.push_back(entry);
	init();

	if (cur)
		setCurrent(cur);
}

template <class T>
inline void eListBox<T>::remove(T* entry, bool holdCurrent)
{
	T* cur = 0;

	if (holdCurrent && current != entry)
		cur = current;

	childs.take(entry);
	init();

	if (cur)
		setCurrent(cur);
}

template <class T>
inline void eListBox<T>::clearList()
{
	while (!childs.empty())
		delete childs.first();
}

template <class T>
inline void eListBox<T>::sort()
{
	T* cur = current;
	childs.sort();
	init();
	if (cur)
		setCurrent(cur);
}

template <class T>
inline T* eListBox<T>::goNext()
{
	moveSelection(dirDown);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline T* eListBox<T>::goPrev()
{
	moveSelection(dirUp);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline eListBox<T>::eListBox(eWidget *parent, const eWidget* descr)
	 :eListBoxBase(parent, descr),
		top(childs.end()), bottom(childs.end()), current(childs.end()), recalced(0)
{
	childs.setAutoDelete(false);	// machen wir selber

	addActionMap(&i_cursorActions->map);
	addActionMap(&i_listActions->map);
	item_height = T::getEntryHeight();

}

template <class T>
inline eListBox<T>::~eListBox()
{
	while (childs.begin() != childs.end())
		delete childs.front();
}

template <class T>
inline void eListBox<T>::redrawWidget(gPainter *target, const eRect &where)
{
	if ( !isVisible() )
		return;

	eRect rc = where;

	eListBoxBase::redrawBorder(target, rc);

	// rc wird in eListBoxBase ggf auf den neuen Client Bereich ohne Rand verkleinert
	
	int i=0;
	for (ePtrList_T_iterator entry(top); (entry != bottom) && (entry != childs.end()); ++entry)
	{
		eRect rect = getEntryRect(i);

		eString s;

		if ( rc.contains(rect) )
			if ( entry == current )
			{
				if ( LCDTmp )
					LCDTmp->setText( entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), ( have_focus ? 1 : ( MaxEntries > 1 ? 2 : 0 ) ) ) );
				else if ( parent->LCDElement )
					parent->LCDElement->setText( entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), ( have_focus ? 1 : ( MaxEntries > 1 ? 2 : 0 ) ) ) );
				else
					entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), ( have_focus ? 1 : ( MaxEntries > 1 ? 2 : 0 ) )	);		
			}
			else
				entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), ( have_focus ? 0 : ( MaxEntries > 1 ? 2 : 0 ) )	);

		i++;
	}

	target->flush();
}

template <class T>
inline void eListBox<T>::gotFocus()
{
	eListBoxBase::gotFocus();

	have_focus++;

	if (!childs.empty())
		if ( eListBoxBase::newFocus() )   // recalced ?
		{
			ePtrList_T_iterator it = current;
			init();	
			setCurrent(it);
		}
		else if ( isVisible() )
		{
			int i=0;	
			for ( ePtrList_T_iterator entry(top); entry != bottom; i++, ++entry)
				if (*entry == *current)
					invalidateEntry(i);
		}
}

template <class T>
inline void eListBox<T>::lostFocus()
{	
	eListBoxBase::lostFocus();

	have_focus--;

	if (!childs.empty())
		if ( eListBoxBase::newFocus() ) 	//recalced ?
		{
			ePtrList_T_iterator it = current;
			init();	
			setCurrent(it);
		}
		else if ( isVisible() )
		{
			int i = 0;
			for (ePtrList_T_iterator entry(top); entry != bottom; i++, ++entry)
				if (*entry == *current)
					invalidateEntry(i);
		}

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");

}

template <class T>
inline void eListBox<T>::init()
{
	current = top = bottom = childs.begin();

	for (int i = 0; i < MaxEntries; i++, bottom++)
		if (bottom == childs.end() )
			break;	
}

template <class T>
inline int eListBox<T>::moveSelection(int dir)
{
	if (childs.empty())
		return 0;
		
	T *oldptr = *current,
		*oldtop = *top;
	switch (dir)
	{
		case dirPageDown:
			if (bottom == childs.end())
			{
				current = bottom;		// --bottom always valid because !childs.empty()
				--current;
			} else
				for (int i = 0; i < MaxEntries; i++)
				{
					if (bottom == childs.end())
						break;
					bottom++;
					top++;
					current++;
				}
		break;

		case dirPageUp:
			if (top == childs.begin())
				current = top;
			else
				for (int i = 0; i < MaxEntries; i++)
				{	
					if (top == childs.begin())
						break;
					bottom--;
					top--;
					current--;
				}
		break;
		
		case dirUp:
			if ( current == childs.begin() )				// wrap around?
			{
				current = --childs.end();					// select last
				top = bottom = childs.end();
				for (int i = 0; i < MaxEntries; i++, top--)
					if (top == childs.begin())
						break;
			}
			else
				if (current-- == top) // new top must set
				{
					for (int i = 0;i < MaxEntries; i++, top--, bottom--)
						if (top == childs.begin())
							break;
				}
		break;

		case dirDown:
			if ( current == --childs.end() )				// wrap around?
			{
				top = current = bottom = childs.begin(); 	// goto first;
				for (int i = 0; i < MaxEntries; i++, bottom++)
					if ( bottom == childs.end() )
						break;
			}
			else
			{
				if (++current == bottom)   // ++current ??
				{
					for (int i = 0; i<MaxEntries; i++, top++, bottom++)
						if ( bottom == childs.end() )
							break;
				}
			}
			break;
		case dirFirst:
			top = current = bottom = childs.begin(); 	// goto first;
			for (int i = 0; i < MaxEntries; i++, bottom++)
				if ( bottom == childs.end() )
					break;
			break;
		default:
			return 0;
	}
	
	if (*current != oldptr)  // current has changed
		/*emit*/ selchanged(*current);

	if (isVisible())
	{
		if (oldtop != *top)
			invalidate();
		else if ( *current != oldptr)
		{
			int i=0;
			int old=-1, cur=-1;
			
			for (ePtrList_T_iterator entry(top); entry != bottom; i++, ++entry)
				if ( *entry == oldptr)
					old=i;
				else if ( *entry == *current )
					cur=i;
			
			if (old != -1)
				invalidateEntry(old);

			if ( (cur != -1) )
				invalidateEntry(cur);
		}
	}
	return 1;
}

template <class T>
inline int eListBox<T>::eventHandler(const eWidgetEvent &event)
{
	eListBoxBase::eventHandler(event);  // this calls not eWidget::eventhandler...

	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			init();
		break;

		case eWidgetEvent::evtAction:
			if ((event.action == &i_listActions->pageup) && !(flags & flagNoPageMovement))
				moveSelection(dirPageUp);
			else if ((event.action == &i_listActions->pagedown) && !(flags & flagNoPageMovement))
				moveSelection(dirPageDown);
			else if ((event.action == &i_cursorActions->up) && !(flags & flagNoUpDownMovement))
				moveSelection(dirUp);
			else if ((event.action == &i_cursorActions->down) && !(flags & flagNoUpDownMovement))
				moveSelection(dirDown);
			else if (event.action == &i_cursorActions->ok)
			{
				if ( current == childs.end() )
					/*emit*/ selected(0);
				else
					/*emit*/ selected(*current);
			}
			else if (event.action == &i_cursorActions->cancel)
				/*emit*/ selected(0);
			else
				break;
		return 1;
		default:
		break;
	}
	return eWidget::eventHandler(event);
}

template <class T>
inline void eListBox<T>::setCurrent(const T *c)
{
	if (childs.empty() || *current == c)  // no entries or current is equal the entry to search
		return;	// do nothing

	ePtrList_T_iterator it(childs.begin());

	for ( ; it != childs.end(); it++)
		if ( *it == c )
			break;

	if ( it == childs.end() ) // entry not in listbox... do nothing
		return;

	int newCurPos=-1;
	int oldCurPos=-1;
	ePtrList_T_iterator oldCur(current);

	int i = 0;

	for (it=top; it != bottom; it++, i++ )  // check if entry to set between bottom and top
	{
		if ( *it == c)
		{
			newCurPos=i;
			current = it;
		}
		if ( *it == *oldCur)
			oldCurPos=i;
	}

	if (newCurPos != -1) // the we start to search from begin
	{
		if (isVisible())
		{			
			invalidateEntry(newCurPos);
			invalidateEntry(oldCurPos);
		}
	}	
	else
	{
		bottom = childs.begin();
						
		while (newCurPos == -1 && MaxEntries )  // MaxEntries is already checked above...
		{
			if ( bottom != childs.end() )
				top = bottom;		// n�chster Durchlauf

			for (	i = 0; i < MaxEntries && bottom != childs.end(); bottom++, i++)
			{
				if ( *bottom == c )		// das suckt... hier werden Zeiger verglichen ! Hier wird nicht der Operator== von T benutzt !
				{
					current = bottom;  // we have found
					newCurPos++;
				}
      }
		}
		if (isVisible())
			invalidate();   // Draw all
  }
	return;
}

template <class T>
void eListBox<T>::setActiveColor(gColor back, gColor front)
{
	colorActiveB=back;
	colorActiveF=front;

	if (current != childs.end())
	{
		int i = 0;
		for (ePtrList_T_iterator it(top); it != bottom; i++, it++)
		{
			if (it == current)
			{
				invalidateEntry(i);
				break;
			}
		}
	}
}

template <class T>
class eListBoxWindow: public eWindow
{
protected:
	int Entrys;
	int width;
	eListBox<T> list;
	eStatusBar *statusbar;
public:
	eListBoxWindow(eString Title="", int Entrys=0, int width=400, bool sbar=0);
};

template <class T>
inline eListBoxWindow<T>::eListBoxWindow(eString Title, int Entrys, int width, bool sbar)
	: eWindow(0), Entrys(Entrys), width(width), list(this), statusbar(sbar?new eStatusBar(this):0)
{
	setText(Title);
	cresize( eSize(width, (sbar?40:10)+Entrys*T::getEntryHeight() ) );
	list.move(ePoint(10, 5));
	list.resize(eSize(getClientSize().width()-20, getClientSize().height()-(sbar?35:5) ));
	if (sbar)
	{
		statusbar->move( ePoint(0, getClientRect().bottom()-30) );
		statusbar->resize( eSize( size.width(), 30) );
		statusbar->setFlags( eStatusBar::flagLoadDeco );
	}
}

#endif

#ifndef __listbox_h
#define __listbox_h

#include <core/driver/rc.h>
#include <core/gdi/grc.h>
#include <core/gdi/fb.h>
#include <core/gui/ewidget.h>
#include <core/gui/eskin.h>
#include <core/gui/ewindow.h>
#include <core/gui/guiactions.h>
#include <sstream>

template <class T>
class eListBox: public eWidget
{
	typedef typename ePtrList<T>::iterator ePtrList_T_iterator;
	void redrawWidget(gPainter *target, const eRect &area);
	ePtrList<T> childs;
	ePtrList_T_iterator top, bottom, current;

	int MaxEntries, item_height, flags;
	gPixmap *iArrowUpDown, *iArrowUp, *iArrowDown, *iArrowLeft, *iArrowRight;
	gColor colorActiveB, colorActiveF;

	void gotFocus();
	void lostFocus();
	eRect getEntryRect(int n);
	void invalidateEntry(int n);
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	void append(T* e);
	void remove(T* e);
	eListBox(eWidget *parent);
	~eListBox();

	Signal1<void, T*> selected;	
	Signal1<void, T*> selchanged;

	void init();
	void clearList();
	void setCurrent(const T *c);
	T* getCurrent()	{ return current != childs.end() ? *current : 0; }
	void sort();
	T* goNext();
	T* goPrev();
	int setProperty(const eString &prop, const eString &value);
	void eraseBackground(gPainter *target, const eRect &clip);

	template <class Z>
	void forEachEntry(Z ob)
	{
		for (ePtrList_T_iterator i(childs.begin()); i!=childs.end(); ++i)
			if (ob(**i))
				break;
	}

	int have_focus;
	void setActiveColor(gColor back, gColor front);
	enum
	{
		dirPageDown, dirPageUp, dirDown, dirUp, dirFirst
	};
	int moveSelection(int dir);
	
	enum
	{
		flagNoUpDownMovement=1,
		flagNoPageMovement=2,
		flagNewStyle=4
	};
	
	void setFlags(int flags);
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
	~eListBoxEntry()
	{
		if (listbox)
			listbox->remove(this);
	}
	virtual eSize getExtend()	{ return eSize(); }
};

class eListBoxEntryText: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryText>;
protected:
	eString text;
	void *key;
	int align;
	eTextPara *para;
	gFont font;
public:
	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const char* txt=0, void *key=0, int align=0)
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb ), text(txt), key(key), align(align), para(0)
	{
			font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");
	}

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const eString& txt, void* key=0, int align=0)
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb ), text(txt), key(key), align(align), para(0)
	{
			font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");
	}

	virtual ~eListBoxEntryText();
	
	bool operator < ( const eListBoxEntryText& e) const
	{
		if (key == e.key)
			return text < e.text;	
		else
			return key < e.key;
	}
	
	void *getKey() { return key; }
	const eString& getText() { return text; }

	eSize getExtend();

protected:
	void redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryTextStream: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTextStream>;
protected:
	std::stringstream text;
	gFont font;
public:
	eListBoxEntryTextStream(eListBox<eListBoxEntryTextStream>* lb)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb)
	{		
			font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");	
	}

	bool operator < ( const eListBoxEntryTextStream& e) const
	{
		return text.str() < e.text.str();	
	}

protected:
	void redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryMenu: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryMenu>;
public:
	Signal0<void> selected;

	eListBoxEntryMenu(eListBox<eListBoxEntryMenu>* lb, const char* txt)
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, txt)
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
inline void eListBox<T>::append(T* entry)
{
	childs.push_back(entry);
	
	init();
}

template <class T>
inline void eListBox<T>::remove(T* entry)
{
	childs.take(entry);

	init();
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
	childs.sort();
	init();
}

template <class T>
inline eRect eListBox<T>::getEntryRect(int pos)
{
	return eRect(ePoint(0, pos*item_height), eSize(size.width(), item_height));
}

template <class T>
inline void eListBox<T>::invalidateEntry(int n)
{
	invalidate(getEntryRect(n));
}

template <class T>
inline int eListBox<T>::setProperty(const eString &prop, const eString &value)
{
	if (prop=="activeForegroundColor")
		colorActiveF=eSkin::getActive()->queryScheme(value);
	else if (prop=="activeBackgroundColor")
		colorActiveB=eSkin::getActive()->queryScheme(value);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

template <class T>
inline void eListBox<T>::setActiveColor(gColor back, gColor front)
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
inline eListBox<T>::eListBox(eWidget *parent)
	 :eWidget(parent, 1),
		top(childs.end()), bottom(childs.end()), current(childs.end()),
		item_height(font.pointSize+2),
		flags(0),
		iArrowUpDown(eSkin::getActive()->queryImage("eListBox.arrow.updown")),
		iArrowUp(eSkin::getActive()->queryImage("eListBox.arrow.up")),
		iArrowDown(eSkin::getActive()->queryImage("eListBox.arrow.down")),
		iArrowLeft(eSkin::getActive()->queryImage("eListBox.arrow.left")),
		iArrowRight(eSkin::getActive()->queryImage("eListBox.arrow.right")),
		colorActiveB(eSkin::getActive()->queryScheme("global.selected.background")),
		colorActiveF(eSkin::getActive()->queryScheme("global.selected.foreground")),
		have_focus(0)
{
	childs.setAutoDelete(false);	// machen wir selber

	addActionMap(&i_cursorActions->map);
	addActionMap(&i_listActions->map);
}

template <class T>
inline eListBox<T>::~eListBox()
{
	while (childs.begin() != childs.end())
	{
		T* l=childs.front();
		delete l;
	}
}

template <class T>
void eListBox<T>::eraseBackground(gPainter *target, const eRect &clip)
{
}

template <class T>
inline void eListBox<T>::redrawWidget(gPainter *target, const eRect &where)
{
	// letzter Parameter redraw
	// 	0 not selected
	//  1 selected with focus
	//  2 selcted without focus
	int i=0;

	for (ePtrList_T_iterator entry(top); (entry != bottom) && (entry != childs.end()); ++entry)
	{
		eRect rect = getEntryRect(i);

		if ( where.contains(rect) )
			entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), (entry == current)?(have_focus?1:2):0 );

		i++;
	}

	target->flush();
}

template <class T>
inline void eListBox<T>::gotFocus()
{
	have_focus++;

	if (childs.empty())
		return;

	int i=0;	
	for ( ePtrList_T_iterator entry(top); entry != bottom; i++, ++entry)
		if (*entry == *current)
			invalidateEntry(i);
}

template <class T>
inline void eListBox<T>::lostFocus()
{	
	have_focus--;

	if (childs.empty())
		return;

/*	
	int i = 0;
	if (isVisible())
	{
		for (ePtrList_T_iterator entry(top); top != bottom; i++, ++entry)
			if (*entry == *current)
				invalidateEntry(i);
	}*/

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");
}

template <class T>
inline void eListBox<T>::init()
{
	MaxEntries = size.height() / item_height;

	current = top = bottom = childs.begin();

	for (int i = 0; i < MaxEntries; i++, bottom++)
	{
		if (bottom == childs.end() )
			break;	
	}
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
inline void eListBox<T>::setFlags(int _flags)
{
	flags=_flags;
}

template <class T>
inline int eListBox<T>::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
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
		case eWidgetEvent::changedSize:
			init();
		break;
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
		
		while (newCurPos == -1)
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
class eListBoxWindow: public eWindow
{
protected:
	int Entrys;
	int width;
public:
	eListBox<T> list;
	eListBoxWindow(eString Title="", int Entrys=0, int width=400);
};

template <class T>
inline eListBoxWindow<T>::eListBoxWindow(eString Title, int Entrys, int width)
	: eWindow(0), Entrys(Entrys), width(width), list(this)
{
	setText(Title);
	cresize(eSize(width, 10+Entrys*(list.getFont().pointSize+4)));
	
	list.move(ePoint(10, 5));
	eSize size = getClientSize();
	size.setWidth(size.width()-20);
	size.setHeight(size.height()-10);
	list.resize(size);
}

#endif

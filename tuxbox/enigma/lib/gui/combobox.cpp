#include <lib/gui/combobox.h>
#include <lib/gdi/font.h>

eComboBox::eComboBox( eWidget* parent, int OpenEntries, eLabel* desc, int takefocus, const char *deco )
:eButton(parent, desc, takefocus, deco), listbox(0, 0, takefocus), button( this, 0, 0, eSkin::getActive()->queryValue("eComboBox.smallButton.decoWidth",0)?"eButton":""), pm(0), entries(OpenEntries)
{
	align=eTextPara::dirLeft;
	if ( eSkin::getActive()->queryValue("eComboBox.smallButton.decoWidth",0) )
		button.loadDeco();
	button.setBlitFlags(BF_ALPHATEST);
	pm=eSkin::getActive()->queryImage("eComboBox.arrow");
	button.setPixmap(pm);
	listbox.hide();
	listbox.setDeco("eComboBox.listbox");
	listbox.loadDeco();
	CONNECT( selected, eComboBox::onOkPressed );
	CONNECT( listbox.selected, eComboBox::onEntrySelected );
	CONNECT( listbox.selchanged, eComboBox::onSelChanged );
	this->zOrderRaise();
	listbox.zOrderRaise();
	addActionMap(&i_cursorActions->map);
}

void eComboBox::redrawWidget(gPainter *target, const eRect &rc)
{
//	target->clip( eRect( rc.left(), rc.top(), rc.width()-button.width(), rc.bottom() ) );
	eLabel::redrawWidget(target, rc);
//	target->clippop();
}

void eComboBox::onOkPressed()
{
	if ( flags & flagShowEntryHelp)
	{
		oldHelpText=helptext;
		setHelpText( listbox.getCurrent()->getHelpText() );
	}
	if ( flags & flagSorted )
		listbox.sort();
	parent->setFocus( &listbox );
	ePoint pt = getAbsolutePosition();
	if ( pt.y() + getSize().height() + listbox.getSize().height() > 520)
		listbox.move( ePoint( pt.x(), pt.y()-listbox.getSize().height() ) );
	else
		listbox.move( ePoint( pt.x(), pt.y()+getSize().height() ) );

	eWindow::globalCancel( eWindow::OFF );
	listbox.show();
}

int eComboBox::setProperty( const eString& prop, const eString& val )
{
	if ( prop == "sorted" )
		flags |= flagSorted;
	else if (prop == "openEntries" )
		entries = atoi( val.c_str() );
	else if (prop == "showEntryHelp" )
		flags |= flagShowEntryHelp;
	else if (prop == "openWidth" )
	{
		int width=listbox.getSize().width();
		width = atoi(val.c_str());
  setOpenWidth( width );
	}
	else
		return eButton::setProperty( prop, val);
	return 0;
}

int eComboBox::eventHandler( const eWidgetEvent& event )
{
	switch (event.type)
	{
		case eWidgetEvent::evtShortcut:
			onOkPressed();
			break;
		case eWidgetEvent::changedPosition:
		case eWidgetEvent::changedSize:
		{
			eListBoxEntryText* cur = listbox.getCurrent();
			listbox.resize( eSize( getSize().width(), eListBoxEntryText::getEntryHeight()*entries+listbox.getDeco().borderBottom+listbox.getDeco().borderTop ) );
			int smButtonDeco = eSkin::getActive()->queryValue("eComboBox.smallButton.decoWidth", pm?pm->x:0 );
			if (deco)
			{
				button.resize( eSize(smButtonDeco, crect.height()) );
				button.move( ePoint( crect.right()-smButtonDeco, crect.top() ) );
			}
			else
			{
				button.resize( eSize(smButtonDeco, height()) );
				button.move( ePoint( position.x()+size.width()-smButtonDeco, 0 ) );
			}
			if (pm)
				button.pixmap_position = ePoint( (button.getSize().width() - pm->x) / 2, (button.getSize().height() - pm->y) / 2 );
			if (cur)
				listbox.setCurrent(cur);
		}
		default:
			return eButton::eventHandler( event );	
	}
	return 1;
}

int eComboBox::moveSelection ( int dir )
{
	int ret = listbox.moveSelection( dir );
	eListBoxEntryText *cur = listbox.getCurrent();
	if ( cur )
	{
		setText( cur->getText() );
		current = cur;
	}
	return ret;
}


void eComboBox::onEntrySelected( eListBoxEntryText* e)
{
	listbox.hide();
	if (flags & flagShowEntryHelp)
		setHelpText( oldHelpText );

	if (e && button.getText() != e->getText() )
	{
		setText( e->getText(), false );
		setFocus( this );
		if ( parent->LCDElement )
			parent->LCDElement->setText("");
		current = e;
		/* emit */ selchanged_id(this, current);
		/* emit */ selchanged(current);
	}
	else
		setFocus( this );

	eWindow::globalCancel( eWindow::ON );
}

void eComboBox::onSelChanged(eListBoxEntryText* le)
{
	if (flags & flagShowEntryHelp )
		setHelpText( le->getHelpText() );
	if ( parent->getFocus() == &listbox )
	{
		if ( LCDTmp )
			LCDTmp->setText( le->getText() );
		else if ( parent->LCDElement )
			parent->LCDElement->setText( le->getText() );
	}
}

void eComboBox::removeEntry( eListBoxEntryText* le )
{
	if (le)
	{
		listbox.remove(le);
		if ( flags & flagSorted )
			listbox.sort();
	}
}

void eComboBox::removeEntry( int num )
{
	if ( listbox.getCount() >= num)
	{
		setCurrent(	num );
	  listbox.remove( listbox.getCurrent() );
		if ( flags & flagSorted )
			listbox.sort();
	}
}

void eComboBox::removeEntry( void* key )
{
	setCurrent(key);
	if (listbox.getCurrent() && key == listbox.getCurrent()->getKey() )
	{
		listbox.remove( listbox.getCurrent() );
		if ( flags & flagSorted )
			listbox.sort();
	}
}

int eComboBox::setCurrent( eListBoxEntryText* le )
{
	if (!le)
		return E_INVALID_ENTRY;

	int err = listbox.setCurrent( le );
	if( err && err != E_ALLREADY_SELECTED )
		return err;

	setText( listbox.getCurrent()->getText() );
	current = listbox.getCurrent();

	return OK;
}

struct selectEntryByNum: public std::unary_function<const eListBoxEntryText&, void>
{
	int num;
	eListBox<eListBoxEntryText>* lb;

	selectEntryByNum(int num, eListBox<eListBoxEntryText> *lb): num(num), lb(lb)
	{
	}

	bool operator()(const eListBoxEntryText& le)
	{
		if (!num--)
		{
			lb->setCurrent(&le);
	 		return 1;
		}
		return 0;
	}
};

int eComboBox::setCurrent( int num )
{
	if ( num > listbox.getCount() )
		return E_INVALID_ENTRY;

	int err = listbox.forEachEntry( selectEntryByNum(num, &listbox ) );
	if ( err )
		return E_COULDNT_FIND;

	setText( listbox.getCurrent()->getText() );
	current = listbox.getCurrent();

	return OK;
}

struct selectEntryByKey: public std::unary_function<const eListBoxEntryText&, void>
{
	void* key;
	eListBox<eListBoxEntryText>* lb;

	selectEntryByKey(void *key, eListBox<eListBoxEntryText> *lb):key(key), lb(lb)
	{
	}

	bool operator()(const eListBoxEntryText& le)
	{
		if ( le.getKey() == key )
		{
			lb->setCurrent(&le);
			return 1;
		}

		return 0;
	}
};

int eComboBox::setCurrent( void* key )
{
	if (!listbox.getCount())
		return E_INVALID_ENTRY;

	eListBoxEntryText* cur = listbox.getCurrent();

  if ( cur && cur->getKey() == key )
		goto ok;
	
	int err;
	if ( (err=listbox.forEachEntry( selectEntryByKey(key, &listbox ) ) ) )
		return E_COULDNT_FIND;

ok:
	setText( listbox.getCurrent()->getText() );
	current = listbox.getCurrent();

	return OK; 
}

eListBoxEntryText* eComboBox::getCurrent()
{
	return current;
}

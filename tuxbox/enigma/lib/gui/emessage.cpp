#include "enigma.h"
#include "elabel.h"
#include "emessage.h"
#include "ebutton.h"

eMessageBox::eMessageBox(QString message, QString caption): eWindow(0)
{
	setText(caption);
	move(QPoint(100, 70));
	resize(QSize(520, 430));

	text=new eLabel(this);
	text->setText(message);
	text->move(QPoint(0, 0));
	
	text->resize(QSize(clientrect.width()-20, clientrect.height()));
	text->setFlags(RS_WRAP);

	QSize ext=text->getExtend();
	if (ext.width()<150)
		ext.setWidth(150);
	text->resize(ext); 

	resize(QSize(ext.width()+20+size.width()-clientrect.width(), ext.height()+size.height()-clientrect.height() + eZap::FontSize+14));

	eButton *b=new eButton(this);
	b->resize(QSize(size.width()-20, eZap::FontSize+4));
	b->setText("...OK!");
	ext=b->getExtend();
	b->resize(ext);
//	b->move(QPoint((clientrect.width()-ext.width())/2, clientrect.height()-eZap::FontSize-14));	// center
	b->move(QPoint(clientrect.width()-ext.width(), clientrect.height()-eZap::FontSize-14));	// right align
	connect(b, SIGNAL(selected()), SLOT(okPressed()));
}

eMessageBox::~eMessageBox()
{
}

void eMessageBox::okPressed()
{
  close(0);
}

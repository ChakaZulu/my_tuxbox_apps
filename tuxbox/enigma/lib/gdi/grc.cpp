// for debugging use: 
// #define SYNC_PAINT
#include "grc.h"
#include "font.h"
#include <unistd.h>
#include <pthread.h>
#include "init.h"

void *gRC::thread_wrapper(void *ptr)
{
	nice(1);
	return ((gRC*)ptr)->thread();
}

gRC *gRC::instance=0;

gRC::gRC()
{
	ASSERT(!instance);
	instance=this;
	if (pipe(fd)<0)
	{
		perror("pipe");
	}	
	pthread_create(&the_thread, 0, thread_wrapper, this);
}

gRC::~gRC()
{
	gOpcode o;
	o.dc=0;
	o.opcode=gOpcode::shutdown;
	submit(o);
	close(fd[1]);
	instance=0;
}

void *gRC::thread()
{
	int rptr=0;
	gOpcode o;
	while (1)
	{
		read(fd[0], &o, sizeof(o));
		if (o.opcode==gOpcode::shutdown)
		{
			close(fd[0]);
			break;
		}
		o.dc->exec(&o);
	}
	pthread_exit(0);
}

gRC &gRC::getInstance()
{
	return *instance;
}

void gRC::submit(const gOpcode &o)
{
	write(fd[1], &o, sizeof(o));
}

static int gPainter_instances;

gPainter::gPainter(gDC &dc, QRect rect): dc(dc), rc(gRC::getInstance()), foregroundColor(0), backgroundColor(0)
{
	if (rect.isNull())
		rect=QRect(QPoint(0, 0), dc.getSize());
	ASSERT(!gPainter_instances);
	gPainter_instances++;
	begin(rect);
}

gPainter::~gPainter()
{
	end();
	gPainter_instances--;
}

void gPainter::begin(const QRect &rect)
{
	gOpcode o;
	dc.lock();
	o.dc=&dc;
	o.opcode=gOpcode::begin;
	o.parm.begin.area=new QRect(rect);
	
	cliparea=rect;
	setLogicalZero(cliparea.topLeft());
	rc.submit(o);
}

void gPainter::setBackgroundColor(const gColor &color)
{
	backgroundColor=color;
}

void gPainter::setForegroundColor(const gColor &color)
{
	foregroundColor=color;
}

void gPainter::setFont(const gFont &mfont)
{
	font=mfont;
}

void gPainter::renderText(const QRect &pos, const QString &string, int flags)
{
	QRect area=pos;
	area.moveBy(logicalZero.x(), logicalZero.y());

	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::renderText;
	o.parm.renderText.text=new QString(string);
	o.parm.renderText.area=new QRect(area);
	o.parm.renderText.font=new gFont(font);
	o.flags=flags;
	rc.submit(o);
}

void gPainter::renderPara(eTextPara &para)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::renderPara;
	o.parm.renderPara.textpara=para.grab();
	o.parm.renderPara.offset=new QPoint(logicalZero);
	rc.submit(o);
}

void gPainter::fill(const QRect &area)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::fill;
	o.parm.fill.area=new QRect(area);
	o.parm.fill.area->moveBy(logicalZero.x(), logicalZero.y());
	(*o.parm.fill.area)&=cliparea;
	o.parm.fill.color=new gColor(foregroundColor);
	rc.submit(o);
}

void gPainter::blit(gPixmap &pixmap, QPoint pos, QRect clip)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::blit;
	o.parm.blit.position=new QPoint(pos);
	(*o.parm.blit.position)+=logicalZero;
	o.parm.blit.clip=new QRect(clip);
	(*o.parm.blit.clip).moveBy(logicalZero.x(), logicalZero.y());
	o.parm.blit.pixmap=pixmap.lock();
	rc.submit(o);
}

void gPainter::clear()
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::fill;
	o.parm.fill.area=new QRect(cliparea);
	o.parm.fill.color=new gColor(backgroundColor);
	rc.submit(o);
}

void gPainter::setPalette(gRGB *colors, int start=0, int len=256)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::setPalette;
	o.parm.setPalette.palette=new gPalette;
	o.parm.setPalette.palette->data=new gRGB[len];
	memcpy(o.parm.setPalette.palette->data, colors, len*sizeof(gRGB));
	o.parm.setPalette.palette->start=start;
	o.parm.setPalette.palette->len=len;
	rc.submit(o);
}

void gPainter::mergePalette(gPixmap &target)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::mergePalette;
	o.parm.mergePalette.target=target.lock();
	rc.submit(o);
}

void gPainter::line(QPoint start, QPoint end)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::line;
	o.parm.line.start=new QPoint(start+logicalZero);
	o.parm.line.end=new QPoint(end+logicalZero);
	o.parm.line.color=new gColor(foregroundColor);
	rc.submit(o);
}

void gPainter::setLogicalZero(QPoint rel)
{
	logicalZero=rel;
}

void gPainter::moveLogicalZero(QPoint rel)
{
	logicalZero+=rel;
}

void gPainter::resetLogicalZero()
{
	logicalZero.setX(0);
	logicalZero.setY(0);
}

void gPainter::clip(QRect clip)
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::clip;
	o.parm.clip.clip=new QRect(clip);
	o.parm.clip.clip->moveBy(logicalZero.x(), logicalZero.y());
	cliparea&=*o.parm.clip.clip;
	rc.submit(o);
}

void gPainter::flush()
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::flush;
	rc.submit(o);
}

void gPainter::end()
{
	gOpcode o;
	o.dc=&dc;
	o.opcode=gOpcode::end;
	rc.submit(o);
}

gDC::~gDC()
{
}

gPixmapDC::gPixmapDC(): pixmap(0)
{
}

gPixmapDC::gPixmapDC(gPixmap *pixmap): pixmap(pixmap)
{
}

gPixmapDC::~gPixmapDC()
{
	dclock.lock();
}

void gPixmapDC::exec(gOpcode *o)
{
	switch(o->opcode)
	{
	case gOpcode::begin:
		clip=*o->parm.begin.area;
		delete o->parm.begin.area;
		break;
	case gOpcode::renderText:
	{
		eTextPara *para=new eTextPara(*o->parm.renderText.area);
		para->setFont(*o->parm.renderText.font);
		para->renderString(*o->parm.renderText.text, o->flags);
		para->blit(*this, QPoint(0, 0));
		para->destroy();
		delete o->parm.renderText.text;
		delete o->parm.renderText.area;
		delete o->parm.renderText.font;
		break;
	}
	case gOpcode::renderPara:
	{
		o->parm.renderPara.textpara->blit(*this, *o->parm.renderPara.offset);
		o->parm.renderPara.textpara->destroy();
		delete o->parm.renderPara.offset;
		break;
	}
	case gOpcode::fill:
		pixmap->fill(*o->parm.fill.area, *o->parm.fill.color);
		delete o->parm.fill.area;
		delete o->parm.fill.color;
		break;
	case gOpcode::blit:
	{
		if (o->parm.blit.clip->isNull())
			*o->parm.blit.clip=clip;
		else
			(*o->parm.blit.clip)&=clip;
		pixmap->blit(*o->parm.blit.pixmap, *o->parm.blit.position, *o->parm.blit.clip);
		o->parm.blit.pixmap->unlock();
		delete o->parm.blit.position;
		delete o->parm.blit.clip;
		break;
	}
	case gOpcode::setPalette:
		if (o->parm.setPalette.palette->start>pixmap->colors)
			o->parm.setPalette.palette->start=pixmap->colors;
		if (o->parm.setPalette.palette->len>(pixmap->colors-o->parm.setPalette.palette->start))
			o->parm.setPalette.palette->len=pixmap->colors-o->parm.setPalette.palette->start;
		if (o->parm.setPalette.palette->len)
			memcpy(pixmap->clut+o->parm.setPalette.palette->start, o->parm.setPalette.palette->data, o->parm.setPalette.palette->len*sizeof(gRGB));
		delete[] o->parm.setPalette.palette->data;
		delete o->parm.setPalette.palette;
		break;
	case gOpcode::mergePalette:
		pixmap->mergePalette(*o->parm.blit.pixmap);
		o->parm.blit.pixmap->unlock();
		break;
	case gOpcode::line:
		pixmap->line(*o->parm.line.start, *o->parm.line.end, *o->parm.line.color);
		delete o->parm.line.start;
		delete o->parm.line.end;
		delete o->parm.line.color;
		break;
	case gOpcode::clip:
		clip&=*o->parm.clip.clip;
		delete o->parm.clip.clip;
		break;
	case gOpcode::end:
		unlock();
	case gOpcode::flush:
		break;
	default:
		qFatal("illegal opcode %d. expect memory leak!", o->opcode);
	}
}

eAutoInitP0<gRC, 1> init_grc("gRC");

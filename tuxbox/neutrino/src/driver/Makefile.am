SUBDIRS = pictureviewer audiodec

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@MPEGTOOLS_CFLAGS@ \
	-I$(top_srcdir)/lib

noinst_LIBRARIES = libneutrino_driver.a libneutrino_driver_netfile.a

libneutrino_driver_a_SOURCES = \
	ringbuffer.c \
	encoding.cpp \
	fontrenderer.cpp \
	framebuffer.cpp \
	fb_window.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	audioplay.cpp \
	lcdd.cpp \
	bigclock.cpp \
	newclock.cpp \
	pig.cpp \
	genpsi.c \
	stream2file.cpp \
	screen_max.cpp \
	slotbuffer.c \
	file.cpp \
	audiometadata.cpp \
	audiofile.cpp \
	shutdown_count.cpp \
	BitrateCalculator.cpp

libneutrino_driver_netfile_a_SOURCES = netfile.cpp

#AM_CPPFLAGS = -DSKIP_CA_STATUS

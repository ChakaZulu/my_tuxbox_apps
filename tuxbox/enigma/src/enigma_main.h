#ifndef __enigma_main_h
#define __enigma_main_h

#include "elistbox.h"
#include "ewindow.h"
#include "epixmap.h"
#include "si.h"
#include "dvb.h"
#include "enigma_lcd.h"
#include "multipage.h"

class eListbox;
class eLabel;
class eListboxEntry;
class eProgress;
class EIT;
class SDT;
class SDTEntry;
class PMT;
class PMTEntry;
class eNumber;
class gPainter;

class NVODStream: public eListboxEntry
{
	Q_OBJECT
private slots:
	void EITready(int error);
public:
	NVODStream(eListbox *listbox, int transport_stream_id, int original_network_id, int service_id);
	QString getText(int col=0) const;
	int transport_stream_id, original_network_id, service_id;
	EIT eit;
};

class NVODReferenceEntry;

class eNVODSelector: public eWindow
{
	Q_OBJECT
	eListbox *list;
private slots:
	void selected(eListboxEntry *);
public:
	eNVODSelector();
	void clear();
	void add(NVODReferenceEntry *ref);
};

class AudioStream: public eListboxEntry
{
	Q_OBJECT
public:
	enum
	{
		audioMPEG, audioAC3
	};
	AudioStream(eListbox *listbox, PMTEntry *stream);
	QString getText(int col=0) const;
	PMTEntry *stream;
};

class eAudioSelector: public eWindow
{
	Q_OBJECT
	eListbox *list;
private slots:
	void selected(eListboxEntry *);
public:
	eAudioSelector();
	void clear();
	void add(PMTEntry *);
};

class LinkageDescriptor;

class SubService: public eListboxEntry
{
	Q_OBJECT
public:
	SubService(eListbox *listbox, LinkageDescriptor *descr);
	QString getText(int col=0) const;

	int transport_stream_id, original_network_id, service_id;
	QString name;
};

class eSubServiceSelector: public eWindow
{
	Q_OBJECT
	eListbox *list;
private slots:
	void selected(eListboxEntry *);
public:
	eSubServiceSelector();
	void clear();
	void add(LinkageDescriptor *ref);
};

class eServiceNumberWidget: public eWindow
{
	Q_OBJECT
	eNumber *number;
	int chnum;
	QTimer *timer;
private slots:
	void selected(int*);
	void timeout();
public:
	eServiceNumberWidget(int initial);
	~eServiceNumberWidget();
};

#define ENIGMA_NVOD		1	
#define ENIGMA_AUDIO	2
#define ENIGMA_SUBSERVICES	4

class eEventDisplay;

class eZapMain: public eWidget
{
	Q_OBJECT;
	
	eLabel 	*ChannelNumber, *ChannelName, *Clock, *EINow, *EINext, 
		*EINowDuration, *EINextDuration, *EINowTime, *EINextTime,
		*Description,
		*ButtonRedEn, *ButtonRedDis, 
		*ButtonGreenEn, *ButtonGreenDis, 
		*ButtonYellowEn, *ButtonYellowDis,
		*ButtonBlueEn, *ButtonBlueDis;
	
	ePixmap *DolbyOn, *DolbyOff, *CryptOn, *CryptOff, *WideOn, *WideOff;
	
	eProgress *Progress, *VolumeBar;

	QTimer timeout, clocktimer;

	int cur_start, cur_duration;
	
	eNVODSelector nvodsel;
	eSubServiceSelector subservicesel;
	eAudioSelector audiosel;
	eEventDisplay *actual_eventDisplay;
	int flags;
	
	eZapLCD lcdmain;
	
	void redrawWidget(gPainter *, const QRect &where);
	void eraseBackground(gPainter *, const QRect &where);
	void setEIT(EIT *);
	void handleNVODService(SDTEntry *sdtentry);
protected:
	void keyDown(int);
	void keyUp(int);
private slots:
	void serviceChanged(eService *, int);
	void gotEIT(EIT *, int);
	void gotSDT(SDT *);
	void gotPMT(PMT *);
	void timeOut();
	void leaveService(eService *);
	void clockUpdate();
	void updateVolume(int vol);
	void aspectRatioChanged(int aspect);
	void scrambled(bool b);
	void isAC3(bool b);
public:
	eZapMain();
	~eZapMain();
signals: 
  void AC3detected(bool);
};

#endif /* __enigma_main_h */

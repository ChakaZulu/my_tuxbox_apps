#ifndef __edvb_h
#define __edvb_h

#include <stdio.h>
#include <list>

#include <include/libsig_comp.h>
#include <core/dvb/esection.h>
#include <core/system/econfig.h>
#include <core/base/estring.h>
#include <core/base/eptrlist.h>
#include <core/dvb/settings.h>

class eService;
class eTransponder;
class eTransponderList;
class eDVBServiceController;

class PAT;
class PMT;
class PMTEntry;
class SDT;
class NIT;
class EIT;
class TDT;
class BAT;
class Descriptor;

#define ENOCASYS	1000	/// service is not free and no valid caid
#define ENOSTREAM 1001  /// no video or audio stream
#define ENVOD			1002	/// nvod stream has to be selected

class eBouquet;
class eAVSwitch;
class eStreamWatchdog;
class MHWEIT;
class eDVBRecorder;
class eDVBScanController;
class eDVB;

class eTransponder;

class eDVBEvent
{
public:
	int type;
	enum
	{
		eventTunedIn, 
		eventUser,
	};
	int err;
	eTransponder *transponder;
	
	eDVBEvent(int type): type(type) { }
	eDVBEvent(int type, int err, eTransponder *transponder): type(type), err(err), transponder(transponder) { }
};

class eDVBState
{
public:
	int state;
	enum
	{
		stateIdle,
		stateUser
	};
	eDVBState(int state): state(state) { }
	operator int () const { return state; }
};

class eDVBController
{
protected:
	eDVB &dvb;
public:
	eDVBController(eDVB &dvb): dvb(dvb) { }
	virtual ~eDVBController()=0;
	virtual void handleEvent(const eDVBEvent &event)=0;
};

/**
 * \brief High level DVB class.
 *
 * eDVB contains high-level dvb-functions like 
 * "switchService" which can be called almost 
 * stateless from everywhere...
 */
class eDVB: public Object
{
	static eDVB *instance;
public:
		/** tables for current service/transponder */
	eAUTable<PAT> tPAT;
	eAUTable<PMT> tPMT;
	eAUTable<SDT> tSDT;
	eAUTable<NIT> tNIT, tONIT;
	eAUTable<EIT> tEIT;
	eAUTable<BAT> tBAT;
	
	eDVBRecorder *recorder;

public:
	enum
	{
		controllerNone,
		controllerScan,
		controllerService
	};
	
	
protected:
  int controllertype;
	eDVBController *controller;

private:
	void tunedIn(eTransponder*, int);
	eDVBState state;
public:
	
	void setMode(int mode);

	const eDVBState &getState() const { return state; }
	void setState(const eDVBState &newstate) { /*emit*/ stateChanged(state=newstate); }
	void event(const eDVBEvent &event);

	Signal1<void, const eDVBState&> stateChanged;
	Signal1<void, const eDVBEvent&> eventOccured;

		// -> noch woanders hin
	Signal1<void, int> volumeChanged;
	Signal0<void> timeUpdated;
	
public:
	eString getVersion();
	eDVB();
	~eDVB();
	static eDVB *getInstance()
	{
		return instance;
	}

	eString getInfo(const char *info);
	
	PMT *getPMT();
	EIT *getEIT();

		// -> decoder
	int volume, mute;	
	/**
	 * \brief Changes the volume.
	 *
	 * \param abs What to change:
	 * \arg \c 0 Volume, relative
	 * \arg \c 1 Volume, absolute
	 * \arg \c 2 Mute, set
	 * \arg \c 3 Mute, change
	 * \param vol The volume/muteflag to set. In case of volume, 0 means max and 63 means min.
	 */
	void changeVolume(int abs, int vol);
	
	/**
	 * \brief Configures the network.
	 *
	 * Configures the network according to the configuration stored in the registry.
	 */
	void configureNetwork();
	
			// recording
		/// starts a new recording
	void recBegin(const char *filename); 
		/// pauses a recording
	void recPause();
		/// resumes a recording
	void recResume();
		/// closes a recording
	void recEnd();
	
	int time_difference;
	
	/* container for settings */
	eDVBSettings *settings;

	eDVBServiceController *getServiceAPI();
	eDVBScanController *getScanAPI();

	Signal1<void, bool> scrambled;
	Signal0<void> serviceListChanged;
	Signal0<void> bouquetListChanged;
	Signal1<void, eService*> leaveService;
	Signal1<void, eService*> enterService;
	Signal1<void, eTransponder*> leaveTransponder;
	Signal1<void, eTransponder*> enterTransponder;
	Signal2<void, eTransponder*, int> switchedTransponder;
	Signal2<void, eService*, int> switchedService;
	Signal2<void, EIT*, int> gotEIT;
	Signal1<void, SDT*> gotSDT;
	Signal1<void, PMT*> gotPMT;
};

#endif

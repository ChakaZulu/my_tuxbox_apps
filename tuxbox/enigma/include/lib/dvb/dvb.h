#ifndef __dvb_h
#define __dvb_h

#include <stdio.h>
#include "si.h"

/** reihenfolge: transport_stream_id, original_network_id, service_id
    bei services wird die transport_stream_id evtl. ignoriert */

class eDVB;

#include <list>
#include <map>
#include <utility>
#include <functional>
#include <string>
#include <set>
#include <stack>
#include <xmltree.h>

#include <lib/system/elock.h>

#ifndef MIN
	#define MIN(a,b) (a < b ? a : b)
#endif

#ifndef MAX
	#define MAX(a,b) (a > b ? a : b)
#endif

class eTransponderList;
class eServiceReference;
class eLNB;
class eSatellite;

		// bitte KEINE operator int() definieren, sonst bringt das ganze nix!
struct eTransportStreamID
{
private:
	int v;
public:
	int get() const { return v; }
	eTransportStreamID(int i): v(i) { }
	eTransportStreamID(): v(-1) { }
	bool operator == (const eTransportStreamID &c) const { return v == c.v; }
	bool operator != (const eTransportStreamID &c) const { return v != c.v; }
	bool operator < (const eTransportStreamID &c) const { return v < c.v; }
	bool operator > (const eTransportStreamID &c) const { return v > c.v; }
};

struct eServiceID
{
private:
	int v;
public:
	int get() const { return v; }
	eServiceID(int i): v(i) { }
	eServiceID(): v(-1) { }
	bool operator == (const eServiceID &c) const { return v == c.v; }
	bool operator != (const eServiceID &c) const { return v != c.v; }
	bool operator < (const eServiceID &c) const { return v < c.v; }
	bool operator > (const eServiceID &c) const { return v > c.v; }
};

struct eOriginalNetworkID
{
private:
	int v;
public:
	int get() const { return v; }
	eOriginalNetworkID(int i): v(i) { }
	eOriginalNetworkID(): v(-1) { }
	bool operator == (const eOriginalNetworkID &c) const { return v == c.v; }
	bool operator != (const eOriginalNetworkID &c) const { return v != c.v; }
	bool operator < (const eOriginalNetworkID &c) const { return v < c.v; }
	bool operator > (const eOriginalNetworkID &c) const { return v > c.v; }
};

struct eDVBNamespace
{
private:
	int v;
public:
	int get() const { return v; }
	eDVBNamespace(int i): v(i) { }
	eDVBNamespace(): v(-1) { }
	bool operator == (const eDVBNamespace &c) const { return v == c.v; }
	bool operator != (const eDVBNamespace &c) const { return v != c.v; }
	bool operator < (const eDVBNamespace &c) const { return v < c.v; }
	bool operator > (const eDVBNamespace &c) const { return v > c.v; }
};

struct tsref
{
	eDVBNamespace dvbnamespace;
	eTransportStreamID tsid;
	eOriginalNetworkID onid;
	bool operator<(const tsref &c) const
	{
		if (dvbnamespace < c.dvbnamespace)
			return 1;
		else if (dvbnamespace == c.dvbnamespace)
		{
			if (onid < c.onid)
				return 1;
			else if (onid == c.onid)
				if (tsid < c.tsid)
					return 1;
		}
		return 0;
	}
	tsref(eDVBNamespace dvbnamespace, eTransportStreamID tsid, eOriginalNetworkID onid): 
			dvbnamespace(dvbnamespace), tsid(tsid), onid(onid)
	{
	}
};

class eTransponder
{
	eTransponderList &tplist;
//	friend struct eTransponder::satellite;
public:
	struct cable
	{
		int valid;
		int frequency, symbol_rate;
		int modulation;
		int inversion, fec_inner;
		void set(const CableDeliverySystemDescriptor *descriptor);
		int tune(eTransponder *);
		int isValid() { return valid; }
		bool operator == (const cable &c) const
		{
			if (valid != c.valid)
				return 0;
			if (frequency != c.frequency)
				return 0;
			if (symbol_rate != c.symbol_rate)
				return 0;
			if (modulation != c.modulation)
				return 0;
			if (inversion != c.inversion)
				return 0;
			if (fec_inner != c.fec_inner)
				return 0;
			return 1;
		}
		bool operator<(const cable &c) const
		{
			if ( frequency == c.frequency )
			{
				if ( symbol_rate == c.symbol_rate )
					return fec_inner < c.fec_inner;
				else
					return symbol_rate < c.symbol_rate;
			}
			return frequency < c.frequency;
		}
	} cable;
	struct satellite
	{
		int valid;
		unsigned int frequency, symbol_rate;
		int polarisation, fec, inversion, orbital_position;
		void set(const SatelliteDeliverySystemDescriptor *descriptor);
		int tune(eTransponder *);
		int isValid() const { return valid; }
		bool operator == (const satellite &c) const
		{
			if (valid != c.valid)
				return 0;
//   		eDebug("frequency %i - %i = %i", frequency, c.frequency, MAXDIFF(frequency,c.frequency) );
			if ( abs( frequency-c.frequency ) > 1000 )
				return 0;
//   		eDebug("symbol_rate -> %i != %i", symbol_rate, c.symbol_rate );
			if ( abs(symbol_rate-c.symbol_rate) > 2000 )
				return 0;
//   		eDebug("polarisation -> %i != %i", polarisation, c.polarisation );
			if (polarisation != c.polarisation)
				return 0;
//   		eDebug("fec -> %i != %i", fec, c.fec );
			if (fec != c.fec)
				return 0;
//   		eDebug("inversion -> %i != %i", inversion, c.inversion );
			// dont compare inversion when one have AUTO
			if (inversion != 2 && c.inversion != 2 && inversion != c.inversion)
				return 0;
//			eDebug("orbital_position -> %i != %i", orbital_position, c.orbital_position);
			if (orbital_position != c.orbital_position)
				return 0;
//			eDebug("Satellite Data is equal");
			return 1;
		}
		bool operator<(const satellite &s) const
		{
			if ( frequency == s.frequency )
			{
				if ( symbol_rate == s.symbol_rate )
					return orbital_position < s.orbital_position;
				else
					return symbol_rate < s.symbol_rate;
			}
			return frequency < s.frequency;
		}
	} satellite;
	eTransponder(eTransponderList &tplist, eDVBNamespace dvbnamespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id);
	eTransponder(eTransponderList &tplist);
	void setSatellite(SatelliteDeliverySystemDescriptor *descr) { satellite.set(descr); }
	void setCable(CableDeliverySystemDescriptor *descr) { cable.set(descr); }
	void setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int orbital_position, int inversion);
	void setCable(int frequency, int symbol_rate, int inversion, int modulation );
	
	eTransponder &operator=(const eTransponder &ref)
	{
		cable=ref.cable;
		satellite=ref.satellite;
		state=ref.state;
		transport_stream_id=ref.transport_stream_id;
		original_network_id=ref.original_network_id;
		dvb_namespace=ref.dvb_namespace;
		return *this;
	}
	int tune();
	int isValid();
		
	eDVBNamespace dvb_namespace;
	eTransportStreamID transport_stream_id;
	eOriginalNetworkID original_network_id;
	
	static eDVBNamespace buildNamespace(eOriginalNetworkID onid, eTransportStreamID tsid, int orbital_position, int freq);

	enum
	{
		stateToScan, stateError, stateOK
	};
	int state;

	operator tsref()
	{
		return tsref(
			dvb_namespace,
			transport_stream_id,
			original_network_id );
	}

	bool operator==(const eTransponder &c) const
	{
//		eDebug("onid = %i, c.onid = %i, tsid = %i, c.tsid = %i", original_network_id.get(), transport_stream_id.get(), c.original_network_id.get(), c.transport_stream_id.get() );
		if ( original_network_id != -1 && c.original_network_id != -1 && transport_stream_id != -1 && c.transport_stream_id != -1)
		{
//	  	eDebug("TSID / ONID Vergleich");
				return ( original_network_id == c.original_network_id &&
										transport_stream_id == c.transport_stream_id );
		}
		else
		{
			if (satellite.valid && c.satellite.valid)
				return satellite == c.satellite;
			if (cable.valid && c.cable.valid)
				return cable == c.cable;
		}
		return 1;
	}

	bool operator<(const eTransponder &c) const
	{
		if ((original_network_id == -1) && (transport_stream_id == -1))
		{
			if ((c.original_network_id == -1) && (c.transport_stream_id == -1))
				return cable.valid?cable<c.cable:satellite<c.satellite;
			else
				return 1;
		}
		if (original_network_id < c.original_network_id)
			return 1;
		else if (original_network_id == c.original_network_id)
			if (transport_stream_id < c.transport_stream_id)
				return 1;
		return 0;
	}

};

class eService
{
public:
	eService(const eString &service_name, int spflags=0);
	virtual ~eService();

	eString service_name;
		// flags used for display in the service menu.
	enum
	{
		spfOwnerdraw=1,
		spfColDontChange=0,		// don't change
		spfColSingle=2,				// i.e. 1 column only
		spfColMulti=4,				// i.e. 3 columns
		spfColCombi=6,				// i.e. 1 column "bouquets", 2 columns services
		spfColMask=6
	};
	int spflags;

	class eServiceDVB *dvb;
#ifndef DISABLE_FILE
	class eServiceID3 *id3;
#endif
};

class eServiceDVB: public eService
{
public:
	enum cacheID
	{
		cVPID, cAPID, cTPID, cPCRPID, cAC3PID, cacheMax
	};
	eServiceDVB(eDVBNamespace dvb_namespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, const SDTEntry *sdtentry, int service_number=-1);
	eServiceDVB(eDVBNamespace dvb_namespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, eServiceID service_id, int service_number=-1);
	eServiceDVB(eServiceID service_id, const char *name);
	eServiceDVB(const eServiceDVB &c);

	void update(const SDTEntry *sdtentry);

	eDVBNamespace dvb_namespace;
	eTransportStreamID transport_stream_id;
	eOriginalNetworkID original_network_id;
	eServiceID service_id;
	int service_type;

	eString service_provider;

	int service_number;		// nur fuer dvb, gleichzeitig sortierkriterium...

	enum
	{
		dxNoDVB=1,
		dxDontshow=2,
		dxNoPMT=4
	};
	int dxflags;

	int cache[cacheMax];

	void set(cacheID c, int v)
	{
		cache[c]=v;
	}

	int get(cacheID c)
	{
		return cache[c];
	}

	void clearCache()
	{
		for (int i=0; i<cacheMax; i++)
			cache[i]=-1;
	}

	bool operator<(const eServiceDVB &c) const
	{
		if (original_network_id < c.original_network_id)
			return 1;
		else if (original_network_id == c.original_network_id)
			if (service_id < c.service_id)
				return 1;
		return 0;
	}
};

class eServiceReference
{
	static std::set<eServiceReference> locked;
	static bool lockedListChanged;
public:
	static void loadLockedList( const char* filename );
	static void saveLockedList( const char* filename );
	bool isLocked() const { return locked.find( *this ) != locked.end(); }
	void lock() const { locked.insert( *this );lockedListChanged=true; }
	void unlock() const { locked.erase( *this );lockedListChanged=true; }
	enum
	{
		idInvalid=-1,
		idStructure, // service_id == 0 is root
		idDVB,
		idFile,
		idUser=0x1000
	};
	int type;

	eString descr;

	int flags; // flags will NOT be compared.
	enum
	{
		isDirectory=1,		// SHOULD enter  (implies mustDescent)
		mustDescent=2,		// cannot be played directly - often used with "isDirectory" (implies canDescent)
		/*
			for example:
				normal services have none of them - they can be fed directly into the "play"-handler.
				normal directories have both of them set - you cannot play a directory directly and the UI should descent into it.
				playlists have "mustDescent", but not "isDirectory" - you don't want the user to browse inside the playlist (unless he really wants)
				services with sub-services have none of them, instead the have the "canDecsent" flag (as all of the above)
		*/
		canDescent=4,			// supports enterDirectory/leaveDirectory
		flagDirectory=isDirectory|mustDescent|canDescent,
		shouldSort=8,			// should be ASCII-sorted according to service_name. great for directories.
		hasSortKey=16,		// has a sort key in data[3]. not having a sort key implies 0.
		sort1=32,					// sort key is 1 instead of 0
		isMarker=64
	};

	inline int getSortKey() const { return (flags & hasSortKey) ? data[3] : ((flags & sort1) ? 1 : 0); }

	int data[8];
	eString path;

	eServiceReference()
		: type(idInvalid), flags(0)
	{
	}

	eServiceReference(int type, int flags)
		: type(type), flags(flags)
	{
		memset(data, 0, sizeof(data));
	}
	eServiceReference(int type, int flags, int data0)
		: type(type), flags(flags)
	{
		memset(data, 0, sizeof(data));
		data[0]=data0;
	}
	eServiceReference(int type, int flags, int data0, int data1)
		: type(type), flags(flags)
	{
		memset(data, 0, sizeof(data));
		data[0]=data0;
		data[1]=data1;
	}
	eServiceReference(int type, int flags, int data0, int data1, int data2)
		: type(type), flags(flags)
	{
		memset(data, 0, sizeof(data));
		data[0]=data0;
		data[1]=data1;
		data[2]=data2;
	}
	eServiceReference(int type, int flags, int data0, int data1, int data2, int data3)
		: type(type), flags(flags)
	{
		memset(data, 0, sizeof(data));
		data[0]=data0;
		data[1]=data1;
		data[2]=data2;
		data[3]=data3;
	}
	eServiceReference(int type, int flags, int data0, int data1, int data2, int data3, int data4)
		: type(type), flags(flags)
	{
		memset(data, 0, sizeof(data));
		data[0]=data0;
		data[1]=data1;
		data[2]=data2;
		data[3]=data3;
		data[4]=data4;
	}
	eServiceReference(int type, int flags, const eString &path)
		: type(type), flags(flags), path(path)
	{
		memset(data, 0, sizeof(data));
	}
	eServiceReference(const eString &string);
	eString toString() const;
	bool operator==(const eServiceReference &c) const
	{
		if (type != c.type)
			return 0;
		return /* (flags == c.flags) && */ (memcmp(data, c.data, sizeof(int)*8)==0) && (path == c.path);
	}
	bool operator!=(const eServiceReference &c) const
	{
		return !(*this == c);
	}
	bool operator<(const eServiceReference &c) const
	{
		if (type < c.type)
			return 1;

		if (type > c.type)
			return 0;
			
/*		if (flags < c.flags)
			return 1;
		if (flags > c.flags)
			return 0; */

		int r=memcmp(data, c.data, sizeof(int)*8);
		if (r)
			return r < 0;
		return path < c.path;
	}
	operator bool() const
	{
		return type != idInvalid;
	}
};

class eServicePath
{
	std::list<eServiceReference> path;
public:
	eServicePath()	{	}
	eServicePath( const eString& data );
	eServicePath(const eServiceReference &ref);
	void setString( const eString& data );
	eString toString();
	bool up();
	void down(const eServiceReference &ref);
	eServiceReference current() const;
	eServiceReference top() const { return current(); }
	eServiceReference bottom() const;
	int size() const;
};

struct eServiceReferenceDVB: public eServiceReference
{
	int getServiceType() const { return data[0]; }
	void setServiceType(int service_type) { data[0]=service_type; }

	eServiceID getServiceID() const { return eServiceID(data[1]); }
	void setServiceID(eServiceID service_id) { data[1]=service_id.get(); }

	eTransportStreamID getTransportStreamID() const { return eTransportStreamID(data[2]); }
	void setTransportStreamID(eTransportStreamID transport_stream_id) { data[2]=transport_stream_id.get(); }

	eOriginalNetworkID getOriginalNetworkID() const { return eOriginalNetworkID(data[3]); }
	void setOriginalNetworkID(eOriginalNetworkID original_network_id) { data[3]=original_network_id.get(); }

	eDVBNamespace getDVBNamespace() const { return eDVBNamespace(data[4]); }
	void setDVBNamespace(eDVBNamespace dvbnamespace) { data[4]=dvbnamespace.get(); }

	eServiceReferenceDVB(eDVBNamespace dvbnamespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, eServiceID service_id, int service_type)
		:eServiceReference(eServiceReference::idDVB, 0)
	{
		setTransportStreamID(transport_stream_id);
		setOriginalNetworkID(original_network_id);
		setDVBNamespace(dvbnamespace);
		setServiceID(service_id);
		setServiceType(service_type);
	}

	eServiceReferenceDVB()
	{
	}
};

class eBouquet
{
public:
	int bouquet_id;
	eString bouquet_name;
	std::list<eServiceReferenceDVB> list;

	inline eBouquet(int bouquet_id, eString& bouquet_name)
		: bouquet_id(bouquet_id), bouquet_name(bouquet_name)
	{
	}
	template <class T>
	void forEachServiceReference(T ob)
	{
		for (std::list<eServiceReferenceDVB>::iterator i(list.begin()); i!=list.end(); ++i)
			ob(*i);
	}
	void add(const eServiceReferenceDVB &);
	int remove(const eServiceReferenceDVB &);
	bool operator == (const eBouquet &c) const
	{
		return bouquet_id==c.bouquet_id;
	}
	bool operator < (const eBouquet &c) const
	{
		return (bouquet_name.compare(c.bouquet_name));
	}
};

struct eSwitchParameter
{
	enum SIG22	{	HILO=0, ON=1, OFF=2	}; // 22 Khz
	enum VMODE	{	HV=0, _14V=1, _18V=2, _0V=3 }; // 14/18 V
	VMODE VoltageMode;
	SIG22 HiLoSignal;
};

class eSatellite
{
	eTransponderList &tplist;
	int orbital_position;
	eString description;
	eSwitchParameter switchParams;
	eLNB *lnb;
	std::map<int, eSatellite*>::iterator tpiterator;
	friend class eLNB;
public:
	eSatellite(eTransponderList &tplist, int orbital_position, eLNB &lnb);
	~eSatellite();
	
	const eString &getDescription() const
	{
		return description;
	}
	
	void setDescription(const eString &description)
	{
		this->description=description;
	}
	
	int getOrbitalPosition() const
	{
		return orbital_position;
	}

	eSwitchParameter &getSwitchParams()
	{
		return switchParams;
	}	

	eLNB *getLNB() const
	{
		return lnb;
	}

	void setLNB( eLNB* _lnb )
	{
		lnb = _lnb;
	}
	
	void setOrbitalPosition(int orbital_position);

	bool operator<(const eSatellite &sat) const
	{
		return orbital_position < sat.orbital_position;
	}

	bool operator==(const eSatellite &sat) const
	{
		return orbital_position == sat.orbital_position;
	}
};                     

struct eDiSEqC
{
	enum { AA=0, AB=1, BA=2, BB=3, SENDNO=4 /* and 0xF0 .. 0xFF*/  }; // DiSEqC Parameter
	int DiSEqCParam;
  
	enum tDiSEqCMode	{	NONE=0, V1_0=1, V1_1=2, V1_2=3, SMATV=4 }; // DiSEqC Mode
	tDiSEqCMode DiSEqCMode;
  
	enum tMiniDiSEqCParam  { NO=0, A=1, B=2 };
	tMiniDiSEqCParam MiniDiSEqCParam;

	std::map< int, int > RotorTable; // used for Rotors does not support gotoXX Cmd
	int DiSEqCRepeats;      // for cascaded switches
	int FastDiSEqC;         // send no DiSEqC on H/V or Lo/Hi change
	int SeqRepeat;          // send the complete DiSEqC Sequence twice...
	int SwapCmds;           // swaps the committed & uncommitted cmd
	int uncommitted_cmd;    // state of the 4 uncommitted switches..
	int useGotoXX;          // Rotor Support gotoXX Position ?
	int useRotorInPower;    // can we use Rotor Input Power to detect Rotor state ?
	double DegPerSec;       // degress per Second.. used when no Input Power can used
	enum { NORTH, SOUTH, EAST, WEST };
	int gotoXXLoDirection;  // EAST, WEST
	int gotoXXLaDirection;  // NORT, SOUTH
	double gotoXXLongitude; // Longitude for gotoXX� Function
	double gotoXXLatitude;  // Latitude for gotoXX� Function
	void setRotorDefaultOptions(); // set default rotor options
};

class eLNB
{
	unsigned int lof_hi, lof_lo, lof_threshold;
	int increased_voltage;
	ePtrList<eSatellite> satellites;
	eTransponderList &tplist;
	eDiSEqC DiSEqC;
public:
	eLNB(eTransponderList &tplist): tplist(tplist)
	{
		satellites.setAutoDelete(true);
	}
	void setLOFHi(unsigned int lof_hi) { this->lof_hi=lof_hi; }
	void setLOFLo(unsigned int lof_lo) { this->lof_lo=lof_lo; }
	void setLOFThreshold(unsigned int lof_threshold) { this->lof_threshold=lof_threshold; }
	void setIncreasedVoltage( int inc ) { increased_voltage = inc; }
	unsigned int getLOFHi() const { return lof_hi; }
	unsigned int getLOFLo() const { return lof_lo; }
	unsigned int getLOFThreshold() const { return lof_threshold; }
	int getIncreasedVoltage() const { return increased_voltage; }
	eDiSEqC& getDiSEqC() { return DiSEqC; }	
	eSatellite *addSatellite(int orbital_position);
	void deleteSatellite(eSatellite *satellite);
	void addSatellite( eSatellite *satellite);
	void setDefaultOptions();
	eSatellite* takeSatellite( eSatellite *satellite);
	bool operator==(const eLNB& lnb) { return this == &lnb; }
	ePtrList<eSatellite> &getSatelliteList() { return satellites; }
};

class tpPacket
{
public:
	bool operator==(const tpPacket& p)
	{
		// this do only compare the adresses.. to find a tpPacket
		// in a std::list<tpPacket>.. but it is fast !!
		return &possibleTransponders == &p.possibleTransponders;
	}
	std::string name;
	int scanflags;
	int orbital_position;
	std::list<eTransponder> possibleTransponders;
};

class existNetworks
{
	bool networksLoaded;
public:
	std::list<tpPacket>& getNetworks();
	std::map<int,tpPacket>& getNetworkNameMap();
	int reloadNetworks();
	int saveNetworks();
	void invalidateNetworks() { networksLoaded=false; }
protected:
	int fetype;
	existNetworks();
	std::list<tpPacket> networks;
	std::map<int, tpPacket> names;
	int addNetwork(tpPacket &p, XMLTreeNode *node, int type);
};

class eTransponderList: public existNetworks
{
	static eTransponderList* instance;
	std::map<tsref,eTransponder> transponders;
	std::map<eServiceReferenceDVB,eServiceDVB> services;
	std::multimap<int,eSatellite*> satellites;
	std::list<eLNB> lnbs;
	std::map<int,eServiceReferenceDVB> channel_number;
	friend class eLNB;
	friend class eSatellite;
//	ePlaylist *newServices;
//	eServiceReference newServicesRef;
public:
	std::map<tsref,int> TimeOffsetMap;
	void readTimeOffsetData( const char* filename );
	void writeTimeOffsetData( const char* filename );

	void clearAllServices()	{	services.clear(); }
	void clearAllTransponders()	{	transponders.clear(); }
	
	void removeOrbitalPosition(int orbital_position);

	static eTransponderList* getInstance()	{ return instance; }
	eTransponderList();

	~eTransponderList()
	{
		writeLNBData();  // write Data to registry
/*		newServices->save();
		eServiceInterface::getInstance()->removeRef(newServicesRef);*/

		if (instance == this)
			instance = 0;
	}

	void readLNBData();
	void writeLNBData();

	eTransponder &createTransponder(eDVBNamespace dvb_namespace,eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id);
	eServiceDVB &createService(const eServiceReferenceDVB &service, int service_number=-1, bool *newService=0);
	int handleSDT(const SDT *sdt, eDVBNamespace dvb_namespace, eOriginalNetworkID onid=-1, eTransportStreamID tsid=-1);
	Signal1<void, eTransponder*> transponder_added;
	Signal2<void, const eServiceReferenceDVB &, bool> service_found;

	eTransponder *searchTS(eDVBNamespace dvbnamespace, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id);
	eServiceDVB *searchService(const eServiceReference &service);
	const eServiceReferenceDVB *searchService(eDVBNamespace dvbnamespace, eOriginalNetworkID original_network_id, eServiceID service_id);
	eServiceReferenceDVB searchServiceByNumber(int channel_number);
	
	template <class T> 
	void forEachService(T ob)
	{
		for (std::map<eServiceReferenceDVB,eServiceDVB>::iterator i(services.begin()); i!=services.end(); ++i)
			ob(i->second);
	}
	template <class T> 
	void forEachServiceReference(T ob)
	{
		for (std::map<eServiceReferenceDVB,eServiceDVB>::iterator i(services.begin()); i!=services.end(); ++i)
			ob(i->first);
	}
	template <class T> void forEachTransponder(T ob)
	{
		for (std::map<tsref,eTransponder>::iterator i(transponders.begin()); i!=transponders.end(); ++i)
			ob(i->second);
	}
	template <class T> void forEachChannel(T ob)
	{
		for (std::map<int,eServiceDVB*>::iterator i(channel_number.begin()); i!=channel_number.end(); ++i)
			ob(*i->second);
	}

	eTransponder *getFirstTransponder(int state);
	eSatellite *findSatellite(int orbital_position);
  std::multimap< int, eSatellite*>::iterator begin() { return satellites.begin(); }
  std::multimap< int, eSatellite*>::iterator end() { return satellites.end(); }
	std::list<eLNB>& getLNBs()	{	return lnbs;	}
};

#endif

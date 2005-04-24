#ifdef ENABLE_DYN_MOUNT

#ifndef __enigma_mount_h__
#define __enigma_mount_h__

#include <configfile.h>
#include <string.h>
#include <lib/gui/listbox.h>

#define	MOUNTCONFIGFILE	"/var/tuxbox/config/enigma/mount.conf"

typedef struct
{
	int id;			// sequential number
	eString	userName;	// username, only for CIFS
	eString	password;	// password, only for CIFS
	eString	localDir;	// local mount dir
	eString	mountDir;	// directory which should be mounted
	int ip[4];		// ip address from machine
	int fstype;		// 0 = NFS, 1 = CIFS, 2 = DEV, 3 = SMBFS
	int automount;		// mount at startup
	eString options;	// rw, intr, soft, udp, nolock
	bool mounted;		// if already mounted or not
	eString description;    // description
} t_mount;

class eMountPoint
{
private:
	bool fileSystemIsSupported(eString);
	bool isMounted(void);
public:
	
	t_mount mp;
	eMountPoint(CConfigFile *, int);
	eMountPoint(t_mount);
	~eMountPoint();
	
	void save(FILE *, int);
	int mount(void);
	int unmount(void);
	bool isIdentical(eString, eString);
};

class eMountMgr
{
private:
	static eMountMgr *instance;
	std::vector <eMountPoint> mountPoints;
	std::vector <eMountPoint>::iterator mp_it;
	void addMountedFileSystems(void);
public:
	eString listMountPoints(eString); // webif
	eString listMovieSources(); // webif
	void removeMountPoint(int);
	int addMountPoint(t_mount);
	void changeMountPoint(int, t_mount);
	t_mount getMountPointData(int);
	int mountMountPoint(int);
	int unmountMountPoint(int);
	void automountMountPoints(void);
	void unmountAllMountPoints(void);
	int selectMovieSource(int);
	void save();
	void init();

	static eMountMgr *getInstance() {return (instance) ? instance : instance = new eMountMgr();}

	eMountMgr();
	~eMountMgr();
};
#endif

#endif

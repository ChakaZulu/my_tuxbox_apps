#ifndef __enigma_osd_mount__
#define __enigma_osd_mount__

#include <configfile.h>
#include <string.h>

#include <lib/gui/textinput.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/combobox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/listbox.h>
#include <enigma_mount.h>

class eListBoxEntryMountOSD: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryMountOSD>;
	friend class eTimerListView;
	friend struct _selectEvent;
	static gFont LocalFont, ServerFont;
	static gPixmap *ok, *failed;
	static int ServerXSize, LocalXSize;
	int LocalYOffs, ServerYOffs;

	eTextPara *paraLocal, *paraServer, *paraFS, *paraAuto;
	eString dummy2;

	const eString &redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited);
	static int getEntryHeight();
public:
	int id;
	void redrawEntry();
	eListBoxEntryMountOSD(eListBox<eListBoxEntryMountOSD> *listbox, int id);
	~eListBoxEntryMountOSD();
};

class eMountOSD: public eWindow
{
private:
	static eMountOSD *instance;

	eListBox<eListBoxEntryMountOSD> *mountList;
	eButton	*newmount, *remmount, *unmount, *mountnow;
	eStatusBar *statusbar;

	void mountSelected(eListBoxEntryMountOSD *sel);
	void fieldSelected(int *number);
	void addMount();
	void removeMount();
	void createDirectory(eString directory);

public:
	void mountNow();
	void unmountNow();
	void updateList();
	static eMountOSD *getInstance() {return (instance) ? instance : instance = new eMountOSD();}

	eMountOSD();
	~eMountOSD();
};

class eMountOSDWindow: public eWindow
{
private:
	t_mount mp;
	eComboBox *fstype;
	eNumber	*ip;
	eTextInputField	*localdir, *mountdir, *tusername, *tpassword;
	eButton	*editoptions, *mountnow, *savemount, *abort;
	eCheckbox *automount;
	eStatusBar *statusbar;
	eLabel *lusername, *lpassword, *loptions, *lserver;

	void mountFSChanged(eListBoxEntryText *sel);
	void editOptions();
	void saveMount();
	void fieldSelected(int *number);
	bool checkMounted();
	void passwordSelected();

public:
	eMountOSDWindow(int id);
	~eMountOSDWindow();
};

class eMountOSDOptionsWindow: public eWindow
{
private:
	eCheckbox *async, *sync, *atime, *autom, *execm, *noexec, *ro, *rw, *users, *nolock, *intr, *soft, *udp;
	eButton	*newoptions, *restore, *save, *abort;
	eStatusBar *statusbar;
	eLabel *ownopt;
	t_mount mp;
	int checkOptions(eString options, eString option);
	void enterOptions();
	void setDefaultOptions();
	void saveOptions();

public:
	eMountOSDOptionsWindow(int id);
	~eMountOSDOptionsWindow();
};

class eTextInput
{
public:
	eTextInput();
	~eTextInput();
	eString	showTextInput(eString title, eString helptext, eButton *button);
};

#endif


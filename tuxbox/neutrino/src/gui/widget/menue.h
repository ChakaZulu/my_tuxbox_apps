#ifndef __MENU__
#define __MENU__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"
#include "../daemonc/lcdd.h"
#include "color.h"


#include <string>
#include <vector>

using namespace std;


class CChangeObserver
{
	public:
		virtual ~CChangeObserver(){};
		virtual bool changeNotify(string OptionName){return false;};
};

class CMenuTarget
{
	public:
		enum
		{
			RETURN_REPAINT = 1,
			RETURN_EXIT = 2,
			RETURN_EXIT_ALL = 4
		};
		
	CMenuTarget(){};
	virtual ~CMenuTarget(){};
	virtual void hide(){};
	virtual int exec(CMenuTarget* parent, string actionKey){return 0;};
};


class CMenuItem
{
	protected:
		int x, y, dx;
	public:

		enum
		{
			RETURN_REPAINT = 1,
			RETURN_EXIT = 2,
			RETURN_EXIT_ALL = 4
		};

		CMenuItem(){};
		virtual ~CMenuItem(){};

		virtual void init(int X, int Y, int DX){x=X;y=Y;dx=DX;}
		virtual int paint(bool selected=false){return -1;};
		virtual int getHeight(){return -1;};
		virtual bool isSelectable(){return false;};

		virtual int exec(CMenuTarget* parent){return 0;};
};

class CMenuSeparator : public CMenuItem
{
	int		height;
	int		type;
	string		text;

	public:
		enum
		{
			EMPTY = 0,
			LINE = 1,
			STRING = 2,
			ALIGN_CENTER = 4,
			ALIGN_LEFT = 8,
			ALIGN_RIGHT = 16
		};

		
		CMenuSeparator(int Type=0, string Text="");

		int paint(bool selected=false);
		int getHeight(){return height;};
};

class CMenuForwarder : public CMenuItem
{
	int		height;
	string		text;
	char*		option;
	bool		active;
	CMenuTarget*	jumpTarget;
	string		actionKey;
    bool        localizing;
	public:
		
		CMenuForwarder(string Text, bool Active=true, char *Option=NULL, CMenuTarget* Target=NULL, string ActionKey="", bool Localizing= true);
		int paint(bool selected=false);
		int getHeight(){return height;};
		int exec(CMenuTarget* parent);
		bool isSelectable(){return active;};
};

class CMenuOptionChooser : public CMenuItem
{
	struct keyval
	{
		int key;
		string value;
	};

	vector<keyval*>    options;
	int                height;
	string             optionName;
	bool               active;
	int*               optionValue;
	CChangeObserver*   observ;
    bool               localizing;

	public:
		CMenuOptionChooser(){};
		CMenuOptionChooser(string OptionName, int* OptionValue, bool Active = false, CChangeObserver* Observ = NULL, bool Localizing= true);
		~CMenuOptionChooser();

		void addOption(int key, string value);
		int paint(bool selected);
		int getHeight(){return height;};
		bool isSelectable(){return active;};

		int exec(CMenuTarget* parent);
};

class CMenuOptionStringChooser : public CMenuItem
{
	vector<string>	options;
	int				height;
	string			optionName;
	bool			active;
	char*			optionValue;
	CChangeObserver*	observ;
    bool               localizing;

	public:
		CMenuOptionStringChooser(){};
		CMenuOptionStringChooser(string OptionName, char* OptionValue, bool Active = false, CChangeObserver* Observ = NULL, bool Localizing= true);
		~CMenuOptionStringChooser();

		void addOption( string value);
		int paint(bool selected);
		int getHeight(){return height;};
		bool isSelectable(){return active;};

		int exec(CMenuTarget* parent);
};


class CMenuWidget : public CMenuTarget
{
	protected:
		vector<CMenuItem*>	items;
		string			name;
		string			iconfile;

		int			width;
		int			height;
		int			x;
		int			y;
		int			selected;

	public:
		CMenuWidget(){name="";iconfile="";selected=-1;};
		CMenuWidget(string Name, string Icon="");
		~CMenuWidget();

		virtual void addItem(CMenuItem* menuItem, bool defaultselected=false);
		virtual void paint();
		virtual void hide();
		virtual int exec(CMenuTarget* parent, string actionKey);

		void setName(string Name){name=Name;};
		void setIcon(string Icon){iconfile=Icon;};
};


#endif



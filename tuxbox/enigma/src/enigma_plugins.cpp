#include <enigma_plugins.h>

#include <config.h>

#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <enigma.h>
#include <enigma_lcd.h>
#include <enigma_main.h>
#include <lib/base/eerror.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/font.h>
#include <lib/gdi/grc.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/decoder.h>
#include <lib/gui/eskin.h>
#include <lib/gui/eprogress.h>
#include <lib/system/info.h>

ePluginThread *ePluginThread::instance = NULL;

ePlugin::ePlugin(eListBox<ePlugin> *parent, const char *cfgfile, eSimpleConfigFile &config, const char* descr)
	: eListBoxEntryText((eListBox<eListBoxEntryText>*)parent)
{
	eDebug(cfgfile);

	name = text = config.getInfo("name");

	if (text.isNull())
		text="(" + eString(cfgfile) + " is invalid)";

	desc = config.getInfo("desc");

	if (desc)
	{
		helptext = desc;
	}

	depend = config.getInfo("depend");
	cfgname = cfgfile;
	requires = config.getInfo("requires");
	needfb = atoi(config.getInfo("needfb").c_str());
	needlcd = atoi(config.getInfo("needlcd").c_str());
	needrc = atoi(config.getInfo("needrc").c_str());
	needvtxtpid = atoi(config.getInfo("needvtxtpid").c_str());
	needoffsets = atoi(config.getInfo("needoffsets").c_str());
	version = atoi(config.getInfo("pluginversion").c_str());
	type = atoi(config.getInfo("type").c_str());
	showpig = atoi(config.getInfo("pigon").c_str());
	
	if (type == eZapPlugins::ScriptPlugin)
		sopath = eString(cfgfile).left(strlen(cfgfile) - 4) + ".sh";
	else 
		sopath = eString(cfgfile).left(strlen(cfgfile) - 4) + ".so";

	pluginname = eString(cfgfile).mid(eString(cfgfile).rfind('/') + 1);

	pluginname = pluginname.left(pluginname.length() - 4);
	sortpos = 10000;
	eConfig::getInstance()->getKey(eString().sprintf("/enigma/plugins/sortpos/%s",pluginname.c_str()).c_str(), sortpos);

}

ePluginContextMenu::ePluginContextMenu(ePlugin* current_plugin, int reordering)
: eListBoxWindow<eListBoxEntryText>(_("Plugin Menu"), 6, 400, true)
{
	init_ePluginContextMenu(current_plugin, reordering);
}
void ePluginContextMenu::init_ePluginContextMenu(ePlugin* current_plugin, int reordering)
{
	eListBoxEntry *prev=0;

	move(ePoint(150, 80));
	if ( reordering )
		prev = new eListBoxEntryText(&list, _("disable move mode"), (void*)1, 0, _("switch move mode off"));
	else
		prev = new eListBoxEntryText(&list, _("enable move mode"), (void*)1, 0, _("activate mode to simply change the entry order"));
	struct stat64 s;
	if (!::stat64(current_plugin->cfgname.c_str(),&s) && ((s.st_mode & S_IWUSR) == S_IWUSR))
	{
		prev = new eListBoxEntryText(&list, _("rename"), (void*)2, 0, _("rename the selected plugin"));
	}
	list.setFlags(eListBoxBase::flagHasShortcuts);
	CONNECT(list.selected, ePluginContextMenu::entrySelected);
}

void ePluginContextMenu::entrySelected(eListBoxEntryText *test)
{
	if (!test)
		close(0);
	else
		close((int)test->getKey());
}

const char *eZapPlugins::PluginPath[] = { "/var/tuxbox/plugins/", PLUGINDIR "/", "" };

eZapPlugins::eZapPlugins(Types type, eWidget* lcdTitle, eWidget* lcdElement)
	:eListBoxWindow<ePlugin>(type == StandardPlugin ? _("Plugins") : _("Games"), 8, 400,true), type(type),reordering(0)
{
	setHelpText(_("select plugin and press ok"));
#ifndef DISABLE_LCD
	setLCD(lcdTitle, lcdElement);
#endif
	list.setFlags(eListBoxBase::flagHasShortcuts);
	CONNECT(list.selected, eZapPlugins::selected);
	valign();
}

int eZapPlugins::listPlugins(Types type, std::vector<eString> &list)
{
	int cnt = 0;
	for (int i = 0; i < 2; i++)
	{
		DIR *d = opendir(PluginPath[i]);
		if (!d)
		{
			continue;
		}
		while (struct dirent *e = readdir(d))
		{
			eString FileName = e->d_name;
			if (FileName.find(".cfg") == FileName.size() - 4)
			{
				eString cfgname = (eString(PluginPath[i]) + FileName).c_str();
				eSimpleConfigFile config(cfgname.c_str());
				if (atoi(config.getInfo("type").c_str()) == (int)type)
				{
					list.push_back(cfgname);
					++cnt;
				}
			}
		}
		closedir(d);
	}
	return cnt;
}

int eZapPlugins::getAutostartPlugins(std::vector<eString> &list)
{
	int cnt = listPlugins(AutostartPlugin, list);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		list[i] = list[i].left(list[i].length() - 4) + ".so";
	}
	return cnt;
}

int eZapPlugins::getFileExtensionPlugins(std::vector<FileExtensionScriptInfo> &list)
{
	std::vector<eString> cfgfilelist;
	int cnt = listPlugins(FileExtensionScriptPlugin, cfgfilelist);
	for (unsigned int i = 0; i < cfgfilelist.size(); i++)
	{
		FileExtensionScriptInfo info;
		eSimpleConfigFile config(cfgfilelist[i].c_str());
		info.file_pattern = config.getInfo("pattern");
		info.directory_pattern = config.getInfo("dirpattern");
		info.needfb = atoi(config.getInfo("needfb").c_str());
		info.needrc = atoi(config.getInfo("needrc").c_str());
		info.needlcd = atoi(config.getInfo("needlcd").c_str());
		info.command = config.getInfo("command");
		list.push_back(info);
	}
	return cnt;
}

int eZapPlugins::find(bool ignore_requires)
{
	int cnt=0;
	ePlugin *plg=0;
	std::set<eString> exist;
	int connType=0;
	eConfig::getInstance()->getKey("/elitedvb/network/connectionType", connType);
	bool hasNetwork = eSystemInfo::getInstance()->hasNetwork();


	for ( int i = 0; i < 2; i++ )
	{
		DIR *d=opendir(PluginPath[i]);
		if (!d)
		{
			eString err;
			err.sprintf(_("Couldn't read plugin directory %s"), PluginPath[i]);
			eDebug(err.c_str());
			if ( i )
			{
				eMessageBox msg(err, _("Error"), eMessageBox::iconError|eMessageBox::btOK );
				msg.show();
				msg.exec();
				msg.hide();
				return -1;
			}
			continue;
		}
		while (struct dirent *e=readdir(d))
		{
			eString FileName = e->d_name;
			if (FileName.find(".cfg") == FileName.size() - 4)
			{
				eString cfgname = (eString(PluginPath[i]) + FileName).c_str();
				eSimpleConfigFile config(cfgname.c_str());

				int current_type = (Types)atoi(config.getInfo("type").c_str());
				
				//Scripts should be treated as normal plugins in this context
				if (current_type == ScriptPlugin)
					current_type = StandardPlugin;
					
				if ((type == AnyPlugin) || (type == current_type))
				{
					// do not add existing plugins twice
					if (exist.find(FileName) != exist.end())
						continue;

					exist.insert(FileName);
					// check for required specifications
					eString requires = config.getInfo("requires");
					if (!ignore_requires)
					{
						if ((!hasNetwork) && (requires.find("network") != eString::npos))
							continue;
						if ((!connType) && (requires.find("dsl") != eString::npos))
							continue;
					}
					plg = new ePlugin(&list, cfgname.c_str(), config);
					++cnt;
				}
			}
		}
		closedir(d);
	}
	list.sort();
	return cnt;
}

int eZapPlugins::execSelectPrevious(eString &previous)
{
	int cnt=0;
	int res = 0;

	previousPlugin = previous;

	cnt = find();
	if ((type == StandardPlugin) && (cnt == GamePlugin))
	{
		selected(list.getFirst());
	}
	else
	{
		ePlugin *it = list.getFirst();
		list.setCurrent(it);
		if (!it) return res;
		while (it->pluginname != previous)
		{
			it = list.goNext();
			if (it == list.getFirst()) break;
		}
		list.setCurrent(it);
		show();
		res=eListBoxWindow<ePlugin>::exec();
		hide();
	}
	previous = previousPlugin;
	return res;
}

int eZapPlugins::exec()
{
	int cnt=0;
	int res = 0;

	cnt = find();
	if ((type == StandardPlugin) && (cnt == GamePlugin))
	{
		selected(list.getFirst());
	} else {
		list.setCurrent(list.getFirst());
		show();
		res=eListBoxWindow<ePlugin>::exec();
		hide();
	}
	return res;
}

eString eZapPlugins::execPluginByName(const char* name, bool onlySearch)
{
	if ( name )
	{
		eString Path;
		for ( int i = 0; i < 3; i++ )
		{
			Path=PluginPath[i];
			Path+=name;
			FILE *fp=fopen(Path.c_str(), "rb");
			if ( fp )
			{
				fclose(fp);
				eSimpleConfigFile config(Path.c_str());
				ePlugin p(0, Path.c_str(), config);
				if (ePluginThread::getInstance())
				{
					eDebug("currently one plugin is running.. dont start another one!!");
					return _("E: currently another plugin is running...");
				}
				if ( !onlySearch )
					execPlugin(&p);
				return "OK";
			}
			else if ( i == 2)
				return eString().sprintf(_("plugin '%s' not found"), name );
		}
	}
	return _("E: no name given");
}

void eZapPlugins::execPlugin(ePlugin* plugin)
{
	if (plugin->type == ScriptPlugin)
	{
			//The current plugin is a script
			if ((access(plugin->sopath.c_str(), X_OK) == 0))
			{
				hide();
 				eScriptOutputWindow wnd(plugin);
 				wnd.show();
 				wnd.exec();
 				wnd.hide();
 				show();
			}
			else
			{
				eDebug("can't execute %s",plugin->sopath.c_str());
				eMessageBox mbox(eString().sprintf(_("Cannot execute %s (check rights)"), plugin->sopath.c_str()), (_("Error")), eMessageBox::iconError | eMessageBox::btOK, eMessageBox::btOK, 5);
  	 			mbox.show();
   				mbox.exec();
   				mbox.hide();
			}
	}
	else
	{
			ePluginThread *p = new ePluginThread(plugin, PluginPath, in_loop?this:0);
			p->start();
	}
}
class eSetPluginSortOrder
{
	int n;
public:
	eSetPluginSortOrder(): n(0) { }
	bool operator()(ePlugin &e)
	{
		e.sortpos = ++n;
		eConfig::getInstance()->setKey(eString().sprintf("/enigma/plugins/sortpos/%s",e.pluginname.c_str()).c_str(), e.sortpos);
		return 0;
	}
};

void eZapPlugins::selected(ePlugin *plugin)
{
	switch (reordering)
	{
		case 1:
			list.setMoveMode(1);
			list.setActiveColor(eSkin::getActive()->queryColor("eServiceSelector.entrySelectedToMove"),gColor(0));
			reordering = 2;	
			return;
		case 2:
			list.setMoveMode(0);
			list.setActiveColor(selectedBackColor,gColor(0));
			reordering = 1;	
			list.forEachEntry(eSetPluginSortOrder());
			return;
	}

	if (!plugin || !plugin->pluginname )
	{
		close(0);
		return;
	}
	execPlugin(plugin);
	previousPlugin = plugin->pluginname;
}
void eZapPlugins::toggleMoveMode()
{
	if (!reordering)
	{
		selectedBackColor = list.getActiveBackColor();
		reordering = 1;
	}
	else
		reordering= 0;
}
void eZapPlugins::renamePlugin()
{
	ePlugin* plugin = (ePlugin*)list.getCurrent();
	eSimpleConfigFile config(plugin->cfgname.c_str());
	TextEditWindow wnd(_("Enter new name for the plugin:"));
	wnd.setText(_("Rename plugin"));
	wnd.show();
	wnd.setEditText(config.getInfo("name"));
	int ret = wnd.exec();
	wnd.hide();
	if ( !ret && config.getInfo("name") != wnd.getEditText())
	{
		config.setInfo("name",wnd.getEditText().c_str());
		config.Save(plugin->cfgname.c_str());
		plugin->SetText(wnd.getEditText());
	}
}

void eZapPlugins::showContextMenu()
{
	ePluginContextMenu m((ePlugin*)list.getCurrent(), reordering);
	hide();
	m.show();
	int res=m.exec();
	m.hide();
	switch (res)
	{
	case 1: // enable/disable movemode
		toggleMoveMode();
		break;
	case 2: // rename plugin
		renamePlugin();
		break;
	default:
		break;
	}
	show();
}

int eZapPlugins::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (list.eventHandlerShortcuts(event))
			return 1;
		else if (event.action == &i_cursorActions->cancel)
			close(0);
		else if (event.action == &i_shortcutActions->menu)
			showContextMenu();
		else
			break;
		return 1;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}


PluginParam *ePluginThread::first = NULL, *ePluginThread::tmp = NULL;

void ePluginThread::MakeParam(const char * const id, int val)
{
	PluginParam* p = new PluginParam;

	if (tmp)
		tmp->next = p;

	p->id = id;
	char buf[10];
	sprintf(buf, "%i", val);
	p->val = new char[strlen(buf)+1];
	strcpy(p->val, buf);

	if (!first)
		first = p;

	p->next=0;
	tmp = p;
}

void ePluginThread::start()
{
	wasVisible = wnd ? wnd->isVisible() : 0;

	if (!thread_running())
	{
		argc=0;
		eString argv[20];

		if (depend)
		{
			char depstring[129];
			char *p;
			char *np;

			strcpy(depstring, depend.c_str());

			p=depstring;

			while(p)
			{
				np=strchr(p,',');
				if ( np )
					*np=0;

				for ( int i=0; i < 3; i++ )
				{
					eString str;
					if (np)
						str.assign( p, np-p );
					else
						str.assign( p );

					FILE *fp=fopen((eString(PluginPath[i])+str).c_str(), "rb");
					if ( fp )
					{
						fclose(fp);
						argv[argc++] = eString(PluginPath[i])+str;
						break;
					}
				}
				p=np?np+1:0;
			}
		}

		argv[argc++]=sopath;

		int i;
		eDebug("pluginname is %s %d", pluginname.c_str(), wasVisible);

		for (i=0; i<argc; i++)
		{
			eDebug("loading %s" , argv[i].c_str());
			libhandle[i]=dlopen(argv[i].c_str(), RTLD_GLOBAL|RTLD_NOW);
			if (!libhandle[i])
			{
				const char *de=dlerror();
				eDebug(de);
				eMessageBox msg(de, "plugin loading failed", eMessageBox::btOK, eMessageBox::btOK, 5 );
				msg.show();
				msg.exec();
				msg.hide();
				break;
			}
		}
		if (i<argc)  // loading of one dependencie failed... close the other
		{
			while(i)
				dlclose(libhandle[--i]);
			if (wasVisible)
				wnd->show();
		}
		else
		{
// this is ugly code.. but i have no other idea to detect enigma plugins..
			bool isEnigmaPlugin=false;
			int fd = open(sopath.c_str(), O_RDONLY);
			if ( fd >= 0 )
			{
				char buf[8192];
				while(!isEnigmaPlugin)
				{
					int rd = ::read(fd, buf, 8192);
					for (int i=0; i < rd-15; ++i )
					{
						if (!strcmp(buf+i, "_ZN7eWidgetD0Ev"))
							isEnigmaPlugin=true;
					}
					if ( rd < 8192 )
						break;
				}
				close(fd);
			}

			eDebug("would exec (%s) plugin %s",
				isEnigmaPlugin ? "ENIGMA" : "NORMAL",
				sopath.c_str());

			PluginExec execPlugin = (PluginExec) dlsym(libhandle[i-1], "plugin_exec");
			if (!execPlugin)
				// show messagebox.. and close after 5 seconds...
			{
				eMessageBox msg("The symbol plugin_exec was not found. sorry.", "plugin executing failed", eMessageBox::btOK, eMessageBox::btOK, 5 );
				msg.show();
				msg.exec();
				msg.hide();
			}
			else
			{
				if (needrc)
					MakeParam(P_ID_RCINPUT, eRCInput::getInstance()->lock());

				if ( wasVisible )
					wnd->hide();

				if (needfb)
					MakeParam(P_ID_FBUFFER, fbClass::getInstance()->lock());

#ifndef DISABLE_LCD
				if (needlcd && eSystemInfo::getInstance()->hasLCD())
					MakeParam(P_ID_LCD, eDBoxLCD::getInstance()->lock() );
#endif

				if (needvtxtpid)
				{
					if(Decoder::current.tpid==-1)
						MakeParam(P_ID_VTXTPID, 0);
					else
						MakeParam(P_ID_VTXTPID, Decoder::current.tpid);
			// stop vtxt reinsertion
					tpid = Decoder::current.tpid;
					if (tpid != -1)
					{
						eDebug("stop vtxt reinsertion");
						Decoder::parms.tpid=-1;
						Decoder::Set();
					}
				}

				if (needoffsets)
				{
					int left=20, top=20, right=699, bottom=555;
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/left", left);
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/top", top);
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/right", right);
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/bottom", bottom);
					MakeParam(P_ID_OFF_X, left);
					MakeParam(P_ID_OFF_Y, top);
					MakeParam(P_ID_END_X, right);
					MakeParam(P_ID_END_Y, bottom);
				}

/*				for(PluginParam *par = first; par; par=par->next )
				{
					printf ("id: %s - val: %s\n", par->id, par->val);
					printf("%p\n", par->next);
				}*/

				if ( isEnigmaPlugin )
				{
					eDebug("start plugin in current thread");
					thread();
					finalize_plugin();
				}
				else
				{
					eDebug("start plugin thread...");
					run();  // start thread
				}
			}
		}
	}
	else
		eDebug("don't start plugin.. another one is running");
}

void ePluginThread::thread()
{
	if ( thread_running() )
		eDebug("plugin thread running.. execute plugin now");
	else
		eDebug("execute plugin now");
	PluginExec execPlugin = (PluginExec) dlsym(libhandle[argc-1], "plugin_exec");
	execPlugin(first);
	eDebug("execute plugin finished");
}

void ePluginThread::recv_msg(const int &)
{
	finalize_plugin();
}

void ePluginThread::thread_finished()
{
	message.send(1);
}

void ePluginThread::finalize_plugin()
{
	while (argc)
		dlclose(libhandle[--argc]);

	while (first)  // Parameter Liste freigegeben
	{
		tmp = first->next;
		delete [] first->val;
		delete first;
		first = tmp;
	}

	if (needfb)
		fbClass::getInstance()->unlock();

#ifndef DISABLE_LCD
	if (needlcd && eSystemInfo::getInstance()->hasLCD())
	{
		eDBoxLCD::getInstance()->unlock();
		eZapLCD::getInstance()->invalidate();
	}
#endif

	if ( wasVisible )
		wnd->show();

	if (needrc)
		eRCInput::getInstance()->unlock();

	if (needvtxtpid)
	{
		// start vtxt reinsertion
		if (tpid != -1 && Decoder::current.tpid == -1)
		{
			eDebug("restart vtxt reinsertion");
			Decoder::parms.tpid = tpid;
			Decoder::Set();
		}
	}
	delete this;
}

eScriptOutputWindow::eScriptOutputWindow(ePlugin *plugin):
eWindow(1)
{
	cresize(eSize(580, 420));

	setText(eString().sprintf(_("Output from %s"), plugin->sopath.c_str()));

	scrollbar = new eProgress(this);
	scrollbar->setName("scrollbar");
	scrollbar->setStart(0);
	scrollbar->setPerc(100);
	scrollbar->move(ePoint(width() - 30, 5));
	scrollbar->resize(eSize(20, height() - 100));
	scrollbar->setProperty("direction", "1");

	visible = new eWidget(this);
	visible->setName("visible");
	visible->move(ePoint(10, 5));
	visible->resize(eSize(width() - 40, height() - 100));

	label = new eLabel(visible);
	label->setFlags(RS_WRAP);
	float lineheight = fontRenderClass::getInstance()->getLineHeight(label->getFont());
	int lines = (int) (visible->getSize().height() / lineheight);
	pageHeight = (int) (lines * lineheight);
	visible->resize(eSize(visible->getSize().width(), pageHeight + (int) (lineheight / 6)));
	label->resize(eSize(visible->getSize().width(), pageHeight * 16));

	label->hide();
	label->move(ePoint(0, 0));
	label->setText(eString().sprintf(_("Executing %s. Please wait..."), plugin->sopath.c_str()));
	script = new eConsoleAppContainer(plugin->sopath);
	if (!script->running())
		label->setText(eString().sprintf(_("Could not execute %s"), plugin->sopath.c_str()));
	else
	{
		eDebug("%s started", plugin->sopath.c_str());
		CONNECT(script->dataAvail, eScriptOutputWindow::getData);
		CONNECT(script->appClosed, eScriptOutputWindow::scriptClosed);
	}
	updateScrollbar();
	label->show();

	valign();
}

int eScriptOutputWindow::eventHandler(const eWidgetEvent & event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (total && event.action == &i_cursorActions->up)
		{
			ePoint curPos = label->getPosition();
			if (curPos.y() < 0)
			{
					label->move(ePoint(curPos.x(), curPos.y() + pageHeight));
					updateScrollbar();
			}
		}
		else if (total && event.action == &i_cursorActions->down)
		{
			ePoint curPos = label->getPosition();
			if ((total - pageHeight) >= abs(curPos.y() - pageHeight))
			{
					label->move(ePoint(curPos.x(), curPos.y() - pageHeight));
					updateScrollbar();
			}
		}
		else if (event.action == &i_cursorActions->cancel)
			close(0);
		else
			break;
		return 1;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}

void eScriptOutputWindow::updateScrollbar()
{
	total = pageHeight;
	int pages = 1;
	while (total < label->getExtend().height())
	{
		total += pageHeight;
		pages++;
	}

	int start = -label->getPosition().y() * 100 / total;
	int vis = pageHeight * 100 / total;
	scrollbar->setParams(start, vis);
	scrollbar->show();
	if (pages == 1)
		total = 0;
}


void eScriptOutputWindow::getData(eString str)
{
	scriptOutput += str;
}

void eScriptOutputWindow::scriptClosed(int state)
{
	if (script)
	{
		delete script;
		script = 0;
	}
	label->hide();
	label->move(ePoint(0, 0));
	label->setText(scriptOutput);
	updateScrollbar();
	label->show();
	scriptOutput.clear();
}

eScriptOutputWindow::~eScriptOutputWindow()
{
	if (script)
	{
		if (script->running())
			script->kill();
		delete script;
		script = 0;
	}
}



/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/plugins.h>

#include <sstream>
#include <fstream>
#include <iostream>

#include <dirent.h>
#include <dlfcn.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <global.h>
#include <neutrino.h>

#include <zapit/client/zapittools.h>

/* for alexW images with old drivers: 
 * #define USE_VBI_INTERFACE 1
 */

#ifdef USE_VBI_INTERFACE
 #define AVIA_VBI_START_VTXT	1
 #define AVIA_VBI_STOP_VTXT	2 
#endif

#include <daemonc/remotecontrol.h>
extern CPlugins       * g_PluginList;    /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

bool CPlugins::plugin_exists(const std::string & filename)
{
	return (find_plugin(filename) >= 0);
}

int CPlugins::find_plugin(const std::string & filename)
{
	for(int i = 0; i <  (int) plugin_list.size();i++)
	{
		if( (filename.compare(plugin_list[i].filename) == 0) || (filename.compare(plugin_list[i].filename + ".cfg") == 0) )
			return i;
	}
	return -1;
}

void CPlugins::scanDir(const char *dir)
{
	struct dirent **namelist;
	std::string fname;

	int number_of_files = scandir(dir, &namelist, 0, alphasort);

	for (int i = 0; i < number_of_files; i++)
	{
		std::string filename;

		filename = namelist[i]->d_name;
		int pos = filename.find(".cfg");
		if (pos > -1)
		{
			plugin new_plugin;
			new_plugin.filename = filename.substr(0, pos);
			fname = dir;
			fname += '/';
			new_plugin.cfgfile = fname.append(new_plugin.filename);
			new_plugin.cfgfile.append(".cfg");
			parseCfg(&new_plugin);
			new_plugin.pluginfile = fname;
			if (new_plugin.type == PLUGIN_TYPE_SCRIPT)
				new_plugin.pluginfile.append(".sh");
			else
				new_plugin.pluginfile.append(".so");
			if(!plugin_exists(new_plugin.filename))
			{
				plugin_list.push_back(new_plugin);
				number_of_plugins++;
			}
		}
	}
}

void CPlugins::loadPlugins()
{
	frameBuffer = CFrameBuffer::getInstance();
	number_of_plugins = 0;
	plugin_list.clear();

	scanDir("/var/tuxbox/plugins");
	scanDir(PLUGINDIR);
	sort(plugin_list.begin(), plugin_list.end());
}

CPlugins::~CPlugins()
{
	plugin_list.clear();
}

void CPlugins::parseCfg(plugin *plugin_data)
{
//	FILE *fd;

	std::ifstream inFile;
	std::string line[20];
	int linecount = 0;

	inFile.open(plugin_data->cfgfile.c_str());

	while(linecount < 20 && getline(inFile, line[linecount++]));

	plugin_data->fb = false;
	plugin_data->rc = false;
	plugin_data->lcd = false;
	plugin_data->vtxtpid = false;
	plugin_data->showpig = false;
	plugin_data->needoffset = false;
	plugin_data->hide = false;
	plugin_data->type = PLUGIN_TYPE_DISABLED;

	for (int i = 0; i < linecount; i++)
	{
		std::istringstream iss(line[i]);
		std::string cmd;
		std::string parm;

		getline(iss, cmd, '=');
		getline(iss, parm, '=');

		if (cmd == "pluginversion")
		{
			plugin_data->version = atoi(parm.c_str());
		}
		else if (cmd == "name")
		{
			plugin_data->name = parm;
		}
		else if (cmd == "desc")
		{
			plugin_data->description = parm;
		}
		else if (cmd == "depend")
		{
			plugin_data->depend = parm;
		}
		else if (cmd == "type")
		{
			plugin_data->type = (plugin_type_t)atoi(parm.c_str());
		}
		else if (cmd == "needfb")
		{
			plugin_data->fb = ((parm == "1")?true:false);
		}
		else if (cmd == "needrc")
		{
			plugin_data->rc = ((parm == "1")?true:false);
		}
		else if (cmd == "needlcd")
		{
			plugin_data->lcd = ((parm == "1")?true:false);
		}
		else if (cmd == "needvtxtpid")
		{
			plugin_data->vtxtpid = ((parm == "1")?true:false);
		}
		else if (cmd == "pigon")
		{
			plugin_data->showpig = ((parm == "1")?true:false);
		}
		else if (cmd == "needoffsets")
		{
			plugin_data->needoffset = ((parm == "1")?true:false);
		}
		else if (cmd == "hide")
		{
			plugin_data->hide = ((parm == "1")?true:false);
		}

	}

	inFile.close();
}

PluginParam * CPlugins::makeParam(const char * const id, const char * const value, PluginParam * const next)
{
	PluginParam * startparam = new PluginParam;

	startparam->next = next;
	startparam->id   = id;
	startparam->val  = strdup(value);

	return startparam;
}

PluginParam * CPlugins::makeParam(const char * const id, const int value, PluginParam * const next)
{
	char aval[10];

	sprintf(aval, "%d", value);

	return makeParam(id, aval, next);
}

void CPlugins::startPlugin(const char * const name)
{
	int pluginnr = find_plugin(name);
	if (pluginnr > -1)
		startPlugin(pluginnr);
}

void CPlugins::startScriptPlugin(int number)
{

	const char *script = plugin_list[number].pluginfile.c_str();
	FILE *f = fopen(script,"r");
	if (f != NULL)
	{
		fclose(f);
		printf("executing %s\n",script);
		f = popen(script,"r");
		if (f != NULL)
		{
			char output[1024];
			char o[1024];
			while (fgets(output,1024,f))
			{
				if (strcmp(output,o) == 0)
					printf("same: %s\n",o);
				strcpy(o,output);
				scriptOutput += output; 
			}
			pclose(f);
		} 
		else 
		{	
			printf("can't execute %s\n",script);
		}
	} 
	else
	{
		printf("file not found %s\n",script);
	}
}


void CPlugins::startPlugin(int number)
{
	delScriptOutput();

	if (plugin_list[number].type == PLUGIN_TYPE_SCRIPT)
	{
		startScriptPlugin(number);
		return;
	}

	PluginExec execPlugin;
	char depstring[129];
	char			*argv[20];
	void			*libhandle[20];
	int			argc, i, lcd_fd=-1;
	char			*p;
	char			*np;
	void			*handle;
	char *        error;
	int           vtpid      =  0;
	PluginParam * startparam =  0;

	if (plugin_list[number].fb)
	{
		startparam = makeParam(P_ID_FBUFFER  , frameBuffer->getFileHandle()    , startparam);
	}
	if (plugin_list[number].rc)
	{
		startparam = makeParam(P_ID_RCINPUT  , g_RCInput->getFileHandle()      , startparam);
		startparam = makeParam(P_ID_RCBLK_ANF, g_settings.repeat_genericblocker, startparam);
		startparam = makeParam(P_ID_RCBLK_REP, g_settings.repeat_blocker       , startparam);
	}
	else
	{
		g_RCInput->stopInput();
	}
	if (plugin_list[number].lcd)
	{
		CLCD::getInstance()->pause();

		lcd_fd = open("/dev/dbox/lcd0", O_RDWR);

		startparam = makeParam(P_ID_LCD      , lcd_fd                          , startparam);
	}
	if (plugin_list[number].vtxtpid)
	{
		vtpid = g_RemoteControl->current_PIDs.PIDs.vtxtpid;
#ifdef USE_VBI_INTERFACE
		int fd = open("/dev/dbox/vbi0", O_RDWR);
		if (fd > 0)
		{
			ioctl(fd, AVIA_VBI_STOP_VTXT, 0);
			close(fd);
		}
#endif
		startparam = makeParam(P_ID_VTXTPID, vtpid, startparam);
	}
	if (plugin_list[number].needoffset)
	{
		startparam = makeParam(P_ID_VFORMAT  , g_settings.video_Format         , startparam);
		startparam = makeParam(P_ID_OFF_X    , g_settings.screen_StartX        , startparam);
		startparam = makeParam(P_ID_OFF_Y    , g_settings.screen_StartY        , startparam);
		startparam = makeParam(P_ID_END_X    , g_settings.screen_EndX          , startparam);
		startparam = makeParam(P_ID_END_Y    , g_settings.screen_EndY          , startparam);
	}

	PluginParam *par = startparam;
	for( ; par; par=par->next )
	{
		printf("[gamelist.cpp] (id,val):(%s,%s)\n", par->id, par->val);
	}

	std::string pluginname = plugin_list[number].filename;

	strcpy(depstring, plugin_list[number].depend.c_str());

	argc=0;
	if ( depstring[0] )
	{
		p=depstring;
		while( 1 )
		{
			argv[ argc ] = p;
			argc++;
			np = strchr(p,',');
			if ( !np )
				break;

			*np=0;
			p=np+1;
			if ( argc == 20 )	// mehr nicht !
				break;
		}
	}

	for( i=0; i<argc; i++ )
	{
		std::string libname = argv[i];
		printf("[CPlugins] try load shared lib : %s\n",argv[i]);
		libhandle[i] = dlopen ( *argv[i] == '/' ?
			argv[i] : (PLUGINDIR "/"+libname).c_str(),
			RTLD_NOW | RTLD_GLOBAL );
		if ( !libhandle )
		{
			fputs (dlerror(), stderr);
			break;
		}
	}
	while ( i == argc )		// alles geladen
	{
		handle = dlopen ( plugin_list[number].pluginfile.c_str(), RTLD_NOW);
		if (!handle)
		{
			fputs (dlerror(), stderr);
			//should unload libs!
			break;
		}
		execPlugin = (PluginExec) dlsym(handle, "plugin_exec");
		if ((error = dlerror()) != NULL)
		{
			fputs(error, stderr);
			dlclose(handle);
			//should unload libs!
			break;
		}
		printf("[CPlugins] try exec...\n");
		execPlugin(startparam);
		dlclose(handle);
		printf("[CPlugins] exec done...\n");
		//restore framebuffer...


		if (!plugin_list[number].rc)
			g_RCInput->restartInput();
		g_RCInput->clearRCMsg();

		if (plugin_list[number].lcd)
		{       
			if(lcd_fd != -1)
				close(lcd_fd);
			CLCD::getInstance()->resume();
		}       

		if (plugin_list[number].fb)
		{
			frameBuffer->paletteSet();
			frameBuffer->paintBackgroundBox(0,0,720,576);
		}

#ifdef USE_VBI_INTERFACE
		if (plugin_list[number].vtxtpid)
		{
			if (vtpid != 0)
			{
				// versuche, den gtx/enx_vbi wieder zu starten
				int fd = open("/dev/dbox/vbi0", O_RDWR);
				if (fd > 0)
				{
					ioctl(fd, AVIA_VBI_START_VTXT, vtpid);
					close(fd);
				}
			}
		}
#endif
		//redraw menue...
		break;	// break every time - never loop - run once !!!
	}

	/* unload shared libs */
	for( i=0; i<argc; i++ )
	{
		if ( libhandle[i] )
			dlclose(libhandle[i]);
		else
			break;
	}

	for(par = startparam ; par; )
	{
		/* we must not free par->id, since it is the original */
		free(par->val);
		PluginParam * tmp = par;
		par = par->next;
		delete tmp;
	}
}

bool CPlugins::hasPlugin(plugin_type_t type)
{
	for (std::vector<plugin>::iterator it=plugin_list.begin();
		 it!=plugin_list.end();it++)
	{
		if (it->type == type && !it->hide)
			return true;
	}
	return false;
}

const std::string& CPlugins::getScriptOutput() const
{
	return scriptOutput;
}

void CPlugins::delScriptOutput()
{
	scriptOutput.clear();
}


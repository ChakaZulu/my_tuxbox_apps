/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: yapi.h,v 1.8 2006/06/17 17:24:10 yjogol Exp $

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

#ifndef __nhttpd_yapi_h__
#define __nhttpd_yapi_h__

// c++
#include <map>
#include <string>

// tuxbox
//#include <driver/yd.h>
#include <global.h>
#include <neutrino.h>
#include <system/settings.h>
#include <system/fsmounter.h>
//#include <ydisplay/ydisplay.h>
//#include <ydisplay/fontrenderer.h>

#include <dbox/fp.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <unistd.h>

// nhttpd
#include "helper.h"
#include "request.h"

class CWebserver;
class CWebserverRequest;
class CControlAPI;


#include "controlapi.h"

//-------------------------------------------------------------------------
// ycgi : command escapes
//-------------------------------------------------------------------------
#define YCGI_ESCAPE_START "{="
#define YCGI_ESCAPE_END "=}"

#define yHTML_SEARCHFOLDER_MOUNT "/mnt/httpd"

//-------------------------------------------------------------------------

class CyAPI
{
private:
	CWebDbox	*Parent;

	// ycgi globals 
	std::map<std::string, std::string> ycgi_global_vars;
	
	// caching
	struct stat yCached_blocks_attrib;
	std::string yCached_blocks_filename;
	std::string yCached_blocks_content;
	
	// parsing engine
	std::string cgi_file_parsing(CWebserverRequest *request, std::string htmlfilename, bool ydebug);
	std::string cgi_cmd_parsing(CWebserverRequest* request, std::string html_template, bool ydebug);
	std::string YWeb_cgi_cmd(CWebserverRequest* request, std::string ycmd);
	std::string YWeb_cgi_func(CWebserverRequest* request, std::string ycmd);

	// func
	std::string func_mount_get_list();
	std::string func_mount_set_values(CWebserverRequest* request);
	std::string func_get_bouquets_as_dropdown(std::string para);
	std::string func_get_actual_bouquet_number();
	std::string func_get_channels_as_dropdown(std::string para);
	std::string func_get_actual_channel_id();
	std::string func_get_mode();
	std::string func_get_video_pids(std::string para);
	std::string func_get_radio_pid();
	std::string func_get_audio_pids_as_dropdown(std::string para);
	std::string func_get_request_data(CWebserverRequest* request, std::string para);
	std::string func_unmount_get_list();
	std::string func_do_reload_nhttpd_config(CWebserverRequest* request);
	std::string func_get_partition_list();
	std::string func_get_boxtype();
	std::string func_change_nhttpd(CWebserverRequest* request, std::string para);

	
	// helpers
	std::string YWeb_cgi_get_ini(CWebserverRequest *request, std::string filename, std::string varname, std::string yaccess);
	void YWeb_cgi_set_ini(CWebserverRequest *request, std::string filename, std::string varname, std::string varvalue, std::string yaccess);
	std::string YWeb_cgi_include_block(std::string filename, std::string blockname, std::string ydefault);

public:

//-------------------------------------------------------------------------
// Search folders for html files
//-------------------------------------------------------------------------
	static const unsigned int HTML_DIR_COUNT = 3;
	std::string HTML_DIRS[HTML_DIR_COUNT];

	CyAPI(CWebDbox *webdbox);
	~CyAPI(void);
	// Execute calls
	bool Execute(CWebserverRequest* request);
	bool cgi(CWebserverRequest *request);
	bool ParseAndSendFile(CWebserverRequest *request);

	friend class CControlAPI;
};

#endif /* __nhttpd_yapi_h__ */

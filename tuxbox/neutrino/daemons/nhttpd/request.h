/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: request.h,v 1.27 2006/04/08 16:20:42 yjogol Exp $

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


#ifndef __nhttpd_request_h__
#define __nhttpd_request_h__

#include <string>
#include <map>

#include "webserver.h"

typedef std::map<std::string, std::string> CStringList;

enum Method_Typ
{
	M_UNKNOWN = 0,
	M_POST = 1,
	M_GET = 2,
	M_PUT = 3,
	M_HEAD = 4
};

class CWebserverRequest
{
private:
	int UploadAlreadyRead(char *UploadBuffer);
	int UploadReadFromSocket(char *UploadBuffer, long contentsize, long readbytes);
	char* UploadExtractUploadFile(char *UploadBuffer, long contentsize, long& UploadFileLength);
	bool HandleUpload(void);
	bool ParseBoundaries(std::string bounds);	

protected:
	bool RequestCanceled;
	std::string rawbuffer;
	int rawbuffer_len;
	char *outbuf;
	std::string Boundary;
	
	long tmplong;
	int tmpint;
	std::string tmpstring;

	bool CheckAuth(void);
	std::string GetContentType(std::string ext);
	std::string GetFileName(std::string path, std::string filename);
	void SplitParameter(char *param_str);
	void RewriteURL(void);
	int OpenFile(std::string path, std::string filename);
	long ParseBuffer(char *file_buffer, long file_length, char *out_buffer, long out_buffer_size, CStringList &params);
	bool ParseFirstLine(std::string zeile);
	bool ParseParams(std::string param_string);
	bool ParseHeader(std::string header);
	std::string GetRawLoopingRequest(void);


public:
	class CWebserver *Parent;
	friend class TWebDbox;
	int Socket;
	unsigned long RequestNumber;
	int Method;
	int HttpStatus;

	std::string Host;
	std::string URL;
	std::string Path;
	std::string Filename;
	std::string FileExt;
	std::string Param_String;
	std::string Client_Addr;

	CStringList ParameterList;
	CStringList HeaderList;
	
	std::map<int, std::string> boundaries;

	CWebserverRequest(CWebserver *server);
	~CWebserverRequest(void);
	
	// output methods
	void printf(const char *fmt, ...);
	bool SocketWrite(char const *text);
	bool SocketWriteLn(char const *text);
	bool SocketWriteData(char const *data, long length);
	bool SocketWrite(const std::string text) { return SocketWrite(text.c_str()); }
	bool SocketWriteLn(const std::string text) { return SocketWriteLn(text.c_str()); }
	bool SendFile(const std::string path, const std::string filename);

	void SendHTMLFooter(void);
	void SendHTMLHeader(std::string Titel);
	void SendPlainHeader(std::string contenttype = "text/plain");
	void Send302(char const *URI);
	void Send404Error(void);
	void Send500Error(void);
	void SendOk(void);
	void SendError(void);

	// request control
	bool Authenticate(void);
	static void URLDecode(std::string &encodedString);
	bool ParseFile(const std::string filename, CStringList &params);
	void PrintRequest(void);
	bool SendResponse(void);
	bool EndRequest(void);
	bool GetRawRequest(void);
	bool ParseRequest(void);
};

#endif /* __nhttpd_request_h__ */

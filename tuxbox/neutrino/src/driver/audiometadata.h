/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
	Copyright (C) 2002,2003 Dirch
	Copyright (C) 2002,2003,2004 Zwen
	
	libmad MP3 low-level core
	Homepage: http://www.dbox2.info/

	Kommentar:

	based on
	************************************
	*** madlld -- Mad low-level      ***  v 1.0p1, 2002-01-08
	*** demonstration/decoder        ***  (c) 2001, 2002 Bertrand Petit
	************************************

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
#include <string>
#include <mad.h>


#ifndef __AUDIO_METADATA__
#define __AUDIO_METADATA__

class CAudioMetaData
{
public:
	enum AudioType
   {
		NONE,
		CDR,
		MP3,
		OGG,
		WAV
	};
	AudioType type;
	std::string type_info;

	long filesize; /* filesize in bits */

	unsigned int bitrate; /* overall bitrate, vbr file: current bitrate */
	unsigned int avg_bitrate; /* average bitrate in case of vbr file */
	unsigned int samplerate;
	enum mad_layer layer;
	enum mad_mode mode;
	time_t total_time;
	bool vbr;
	/* if the variable hasInfoOrXingTag is true, this means the values of
	   VBR and Duration are correct and should not be changed by the
	   decoder */
	bool hasInfoOrXingTag;

	std::string artist;
	std::string title;
	std::string album;
	std::string sc_station;
	std::string date;
	std::string genre;
	std::string track;
	bool changed;

	// constructor
	CAudioMetaData()
		{
			clear();
		}

	// copy constructor
	CAudioMetaData( const CAudioMetaData& src )
		: type( src.type ), type_info( src.type_info ),
		filesize( src.filesize ), bitrate( src.bitrate ),
		avg_bitrate( src.avg_bitrate ), samplerate( src.samplerate ),
		layer( src.layer ), mode( src.mode ), total_time( src.total_time ),
		vbr( src.vbr ), hasInfoOrXingTag( src.hasInfoOrXingTag ),
		artist( src.artist ), title( src.title ), album( src.album ),
		sc_station( src.sc_station ), date( src.date ),
		genre( src.genre ), track( src.track ), changed( src.changed )
		{
		}

	// assignment operator
	void operator=( const CAudioMetaData& src )
		{
			// self assignment check
			if ( &src == this )
				return;

			type = src.type;
			type_info = src.type_info;
			filesize = src.filesize;
			bitrate = src.bitrate;
			avg_bitrate = src.avg_bitrate;
			samplerate = src.samplerate;
			layer = src.layer;
			mode = src.mode;
			total_time = src.total_time;
			vbr = src.vbr;
			hasInfoOrXingTag = src.hasInfoOrXingTag;
			artist = src.artist;
			title = src.title;
			album = src.album;
			date = src.date;
			genre = src.genre;
			track = src.track;
			sc_station = src.sc_station;
			changed = src.changed;
		}

	void clear()
	{
		type=NONE;
		type_info="";
			filesize=0;
		bitrate=0;
			avg_bitrate=0;
		samplerate=0;
		total_time=0;
		vbr=false;
			hasInfoOrXingTag=false;
		artist="";
		title="";
		album="";
		sc_station="";
		date="";
		genre="";
		track="";
		changed=false;
	}
};
#endif

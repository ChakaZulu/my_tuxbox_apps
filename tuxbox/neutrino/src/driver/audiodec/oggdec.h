/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002,2003,2004 Sania,Zwen
	
	ogg vorbis audio decoder
	Homepage: http://www.dbox2.info/

	Kommentar:

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


#ifndef __OGG_DEC__
#define __OGG_DEC__

#include <mad.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <driver/audioplay.h>
#include <tremor/ogg.h>
#include <tremor/ivorbisfile.h>
#define DECODE_SLOTS 30
class COggDec : public CBaseDec
{

public:
	static COggDec* getInstance();
	virtual int Decoder(FILE *,int , State* );
	bool GetMetaData(FILE *in, bool nice, CAudioMetaData* m);
	COggDec(){};
private:
	void ParseUserComments(vorbis_comment*, CAudioMetaData*);
	bool Open(FILE* , OggVorbis_File*);
	void SetMetaData(OggVorbis_File*, CAudioMetaData*);
	int Status;
	char* mPcmSlots[DECODE_SLOTS];
	ogg_int64_t mSlotTime[DECODE_SLOTS];
	int mWriteSlot;
	int mReadSlot;
	int mSlotSize;
	int mOutputFd;
	bool mSeekable;
	static void* OutputDsp(void *);
};


#endif


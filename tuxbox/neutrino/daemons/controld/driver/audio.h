/*
	Control-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

#ifndef __audiocontrol__
#define __audiocontrol__

#include "ost/audio.h"

class audioControl
{

	public:

		enum 
		{	
			STEREO = AUDIO_STEREO,
			MONO_LEFT = AUDIO_MONO_LEFT,
			MONO_RIGHT = AUDIO_MONO_RIGHT
		};

		static void setAudioMode(int mode);
		static void setVolume(char volume);
		static void setMute(bool mute);

};

#endif

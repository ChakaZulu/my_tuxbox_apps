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

#ifndef __MOD_rcinput__
#define __MOD_rcinput__

#include <linux/input.h>
#include <string>
#include <vector>

#ifndef KEY_OK
#define KEY_OK           0x160
#endif

#ifndef KEY_RED
#define KEY_RED          0x18e
#endif

#ifndef KEY_GREEN
#define KEY_GREEN        0x18f
#endif

#ifndef KEY_YELLOW
#define KEY_YELLOW       0x190
#endif

#ifndef KEY_BLUE
#define KEY_BLUE         0x191
#endif

/* SAGEM remote controls have the following additional keys */

#ifndef KEY_TOPLEFT
#define KEY_TOPLEFT      0x1a2
#endif

#ifndef KEY_TOPRIGHT
#define KEY_TOPRIGHT     0x1a3
#endif

#ifndef KEY_BOTTOMLEFT
#define KEY_BOTTOMLEFT   0x1a4
#endif

#ifndef KEY_BOTTOMRIGHT
#define KEY_BOTTOMRIGHT  0x1a5
#endif




#define NEUTRINO_UDS_NAME "/tmp/neutrino.sock"


class CRCInput
{
	private:
		struct event
		{
			uint	msg;
			uint	data;
		};

		struct timer
		{
			uint			id;
			unsigned long long	interval;
			unsigned long long	times_out;
			bool			correct_time;
		};

		uint               timerid;
		std::vector<timer> timers;

		int 		fd_pipe_high_priority[2];
		int 		fd_pipe_low_priority[2];
#ifdef OLD_RC_API
#define NUMBER_OF_EVENT_DEVICES 1
#else /* OLD_RC_API */
#define NUMBER_OF_EVENT_DEVICES 2
#endif /* OLD_RC_API */
		int         	fd_rc[NUMBER_OF_EVENT_DEVICES];
		int		fd_keyb;
		int		fd_event;

		int		fd_max;

		void open();
		void close();
		int translate(int code);

		void calculateMaxFd(void);

		int checkTimers();

	public:
		//rc-code definitions
		static const uint RC_MaxRC    = KEY_MAX;    /* /include/linux/input.h: #define KEY_MAX                 0x1ff */
		static const uint RC_KeyBoard = 0x4000;
		static const uint RC_Events   = 0x80000000;
		static const uint RC_Messages = 0x90000000;
		static const uint RC_WithData = 0xA0000000;
		enum
		{
			RC_0            = KEY_0,            /* /include/linux/input.h: #define KEY_0			11   */
			RC_1            = KEY_1,            /* /include/linux/input.h: #define KEY_1			 2   */
			RC_2            = KEY_2,            /* /include/linux/input.h: #define KEY_2			 3   */
			RC_3            = KEY_3,            /* /include/linux/input.h: #define KEY_3			 4   */
			RC_4            = KEY_4,            /* /include/linux/input.h: #define KEY_4			 5   */
			RC_5            = KEY_5,            /* /include/linux/input.h: #define KEY_5			 6   */
			RC_6            = KEY_6,            /* /include/linux/input.h: #define KEY_6			 7   */
			RC_7            = KEY_7,            /* /include/linux/input.h: #define KEY_7			 8   */
			RC_8            = KEY_8,            /* /include/linux/input.h: #define KEY_8			 9   */
			RC_9            = KEY_9,            /* /include/linux/input.h: #define KEY_9			10   */
			RC_backspace    = KEY_BACKSPACE,    /* /include/linux/input.h: #define KEY_BACKSPACE            14   */
			RC_home         = KEY_HOME,         /* /include/linux/input.h: #define KEY_HOME                102   */
			RC_up           = KEY_UP,           /* /include/linux/input.h: #define KEY_UP                  103   */
			RC_page_up      = KEY_PAGEUP,       /* /include/linux/input.h: #define KEY_PAGEUP              104   */
			RC_left         = KEY_LEFT,         /* /include/linux/input.h: #define KEY_LEFT                105   */
			RC_right        = KEY_RIGHT,        /* /include/linux/input.h: #define KEY_RIGHT               106   */
			RC_down         = KEY_DOWN,         /* /include/linux/input.h: #define KEY_DOWN                108   */
			RC_page_down    = KEY_PAGEDOWN,     /* /include/linux/input.h: #define KEY_PAGEDOWN            109   */
			RC_spkr         = KEY_MUTE,         /* /include/linux/input.h: #define KEY_MUTE                113   */
			RC_minus        = KEY_VOLUMEDOWN,   /* /include/linux/input.h: #define KEY_VOLUMEDOWN          114   */
			RC_plus         = KEY_VOLUMEUP,     /* /include/linux/input.h: #define KEY_VOLUMEUP            115   */
			RC_standby      = KEY_POWER,        /* /include/linux/input.h: #define KEY_POWER               116   */
			RC_help         = KEY_HELP,         /* /include/linux/input.h: #define KEY_HELP                138   */
			RC_setup        = KEY_SETUP,        /* /include/linux/input.h: #define KEY_SETUP               141   */
			RC_ok           = KEY_OK,           /* /include/linux/input.h: #define KEY_OK           0x160        */ /* in patched input.h */
			RC_red          = KEY_RED,          /* /include/linux/input.h: #define KEY_RED          0x18e        */ /* in patched input.h */
			RC_green        = KEY_GREEN,        /* /include/linux/input.h: #define KEY_GREEN        0x18f        */ /* in patched input.h */
			RC_yellow       = KEY_YELLOW,       /* /include/linux/input.h: #define KEY_YELLOW       0x190        */ /* in patched input.h */
			RC_blue         = KEY_BLUE,         /* /include/linux/input.h: #define KEY_BLUE         0x191        */ /* in patched input.h */
			RC_top_left     = KEY_TOPLEFT,      /* /include/linux/input.h: #define KEY_TOPLEFT      0x1a2        */ /* in patched input.h */
			RC_top_right    = KEY_TOPRIGHT,     /* /include/linux/input.h: #define KEY_TOPRIGHT     0x1a3        */ /* in patched input.h */
			RC_bottom_left  = KEY_BOTTOMLEFT,   /* /include/linux/input.h: #define KEY_BOTTOMLEFT   0x1a4        */ /* in patched input.h */
			RC_bottom_right = KEY_BOTTOMRIGHT,  /* /include/linux/input.h: #define KEY_BOTTOMRIGHT  0x1a5        */ /* in patched input.h */

			RC_timeout	= 0xFFFFFFFF,
			RC_nokey	= 0xFFFFFFFE
		};

		inline int getFileHandle(void) /* used for plugins (i.e. games) only */
		{
			return fd_rc[0];
		}
		void stopInput();
		void restartInput();

		unsigned long long repeat_block;
		unsigned long long repeat_block_generic;
		CRCInput();      //constructor - opens rc-device and starts needed threads
		~CRCInput();     //destructor - closes rc-device


		static bool isNumeric(const unsigned int key);
		static int getNumericValue(const unsigned int key);
		static unsigned int convertDigitToKey(const unsigned int digit);
		static int getUnicodeValue(const unsigned int key);

		static const char * getSpecialKeyName(const unsigned int key);
		static std::string getKeyName(const unsigned int key);

		int addTimer(unsigned long long Interval, bool oneshot= true, bool correct_time= true );
		int addTimer(struct timeval Timeout);
		int addTimer(const time_t *Timeout);

		void killTimer(uint id);

		static long long calcTimeoutEnd_MS(const int timeout_in_milliseconds);
		static long long calcTimeoutEnd(const int timeout_in_seconds);

		void getMsgAbsoluteTimeout(uint *msg, uint* data, unsigned long long *TimeoutEnd, bool bAllowRepeatLR= false);
		void getMsg(uint *msg, uint * data, int Timeout, bool bAllowRepeatLR= false);     //get message, timeout in 1/10 secs :)
		void getMsg_ms(uint *msg, uint* data, int Timeout, bool bAllowRepeatLR= false);     //get message, timeout in msecs :)
		void getMsg_us(uint *msg, uint* data, unsigned long long Timeout, bool bAllowRepeatLR= false);     //get message, timeout in �secs :)
		void postMsg(uint msg, uint data, bool Priority = true );  // push message back into buffer
		void clearRCMsg();

		int messageLoop( bool anyKeyCancels = false, int timeout= -1 );
};


#endif

#ifndef RC_INPUT_H
#define RC_INPUT_H

#include <linux/input.h>

#define RC_0			KEY_0
#define RC_1			KEY_1
#define RC_2			KEY_2
#define RC_3			KEY_3
#define RC_4			KEY_4
#define RC_5			KEY_5
#define RC_6			KEY_6
#define RC_7			KEY_7
#define RC_8			KEY_8
#define RC_9			KEY_9
#define RC_RIGHT		KEY_RIGHT
#define RC_LEFT			KEY_LEFT
#define RC_UP			KEY_UP
#define RC_DOWN			KEY_DOWN
#define RC_OK			KEY_OK
#define RC_SPKR			KEY_MUTE
#define RC_STANDBY		KEY_POWER
#define RC_GREEN		KEY_GREEN
#define RC_YELLOW		KEY_YELLOW
#define RC_RED			KEY_RED
#define RC_BLUE			KEY_BLUE
#define RC_PLUS			KEY_VOLUMEUP
#define RC_MINUS		KEY_VOLUMEDOWN
#define RC_HELP			KEY_HELP
#define RC_SETUP		KEY_SETUP
#define RC_HOME			KEY_HOME
#define RC_PAGE_DOWN	KEY_PAGEUP
#define RC_PAGE_UP		KEY_PAGEDOWN

extern	void			RcGetActCode( void );
extern	int				RcInitialize( int extfd );
extern	void			RcClose( void );

#endif

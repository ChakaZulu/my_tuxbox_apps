/******************************************************************************
 *                        <<< TuxMail - Mail Plugin >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmail.h,v $
 * Revision 1.17  2005/05/12 22:24:23  lazyt
 * - PIN-Fix
 * - add Messageboxes for send Mail done/fail
 *
 * Revision 1.16  2005/05/12 14:28:28  lazyt
 * - PIN-Protection for complete Account
 * - Preparation for sending Mails ;-)
 *
 * Revision 1.15  2005/05/11 19:00:38  robspr1
 * minor Mailreader changes / add to Spamlist undo
 *
 * Revision 1.14  2005/05/11 12:01:21  lazyt
 * Protect Mailreader with optional PIN-Code
 *
 * Revision 1.13  2005/05/10 12:55:15  lazyt
 * - LCD-Fix for DM500
 * - Autostart for DM7020 (use -DOE, put Init-Script to /etc/init.d/tuxmail)
 * - try again after 10s if first DNS-Lookup failed
 * - don't try to read Mails on empty Accounts
 *
 * Revision 1.12  2005/05/09 19:32:32  robspr1
 * support for mail reading
 *
 * Revision 1.11  2005/04/29 17:24:00  lazyt
 * use 8bit audiodata, fix skin and osd
 *
 * Revision 1.10  2005/03/28 14:14:14  lazyt
 * support for userdefined audio notify (put your 12/24/48KHz pcm wavefile to /var/tuxbox/config/tuxmail/tuxmail.wav)
 *
 * Revision 1.9  2005/03/24 13:15:21  lazyt
 * cosmetics, add SKIN=0/1 for different osd-colors
 *
 * Revision 1.8  2005/03/22 13:31:46  lazyt
 * support for english osd (OSD=G/E)
 *
 * Revision 1.7  2005/03/22 09:35:20  lazyt
 * lcd support for daemon (LCD=Y/N, GUI should support /tmp/lcd.locked)
 *
 * Revision 1.6  2005/02/26 10:23:48  lazyt
 * workaround for corrupt mail-db
 * add ADMIN=Y/N to conf (N to disable mail deletion via plugin)
 * show versioninfo via "?" button
 * limit display to last 100 mails (increase MAXMAIL if you need more)
 *
 * Revision 1.5  2004/07/10 11:38:14  lazyt
 * use -DOLDFT for older FreeType versions
 * replaced all remove() with unlink()
 *
 * Revision 1.4  2004/06/23 11:05:04  obi
 * compile fix
 *
 * Revision 1.3  2004/04/24 22:24:23  carjay
 * fix compiler warnings
 *
 * Revision 1.2  2003/05/16 15:07:23  lazyt
 * skip unused accounts via "plus/minus", add mailaddress to spamlist via "blue"
 *
 * Revision 1.1  2003/04/21 09:24:52  lazyt
 * add tuxmail, todo: sync (filelocking?) between daemon and plugin
 ******************************************************************************/

#include "config.h"

#if !defined(HAVE_DVB_API_VERSION) && defined(HAVE_OST_DMX_H)
#define HAVE_DVB_API_VERSION 1
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#if HAVE_DVB_API_VERSION == 3
#include <linux/input.h>
#endif
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

#include <plugin.h>

#define SCKFILE "/tmp/tuxmaild.socket"
#define LCKFILE "/tmp/lcd.locked"
#define RUNFILE "/var/etc/.tuxmaild"
#define CFGPATH "/var/tuxbox/config/tuxmail/"
#define CFGFILE "tuxmail.conf"
#define SPMFILE "spamlist"
#define POP3FILE "/tmp/tuxmail.pop3"
#define SMTPFILE "/tmp/tuxmail.smtp"

#define OE_START "/etc/rc2.d/S99tuxmail"
#define OE_KILL0 "/etc/rc0.d/K00tuxmail"
#define OE_KILL6 "/etc/rc6.d/K00tuxmail"

#define MAXMAIL 100

// rc codes

#if HAVE_DVB_API_VERSION == 1

#define	RC1_0		0x5C00
#define	RC1_1		0x5C01
#define	RC1_2		0x5C02
#define	RC1_3		0x5C03
#define	RC1_4		0x5C04
#define	RC1_5		0x5C05
#define	RC1_6		0x5C06
#define	RC1_7		0x5C07
#define	RC1_8		0x5C08
#define	RC1_9		0x5C09
#define	RC1_STANDBY	0x5C0C
#define	RC1_UP		0x5C0E
#define	RC1_DOWN	0x5C0F
#define	RC1_PLUS	0x5C16
#define	RC1_MINUS	0x5C17
#define	RC1_HOME	0x5C20
#define	RC1_DBOX	0x5C27
#define	RC1_MUTE	0x5C28
#define	RC1_RED		0x5C2D
#define	RC1_RIGHT	0x5C2E
#define	RC1_LEFT	0x5C2F
#define	RC1_OK		0x5C30
#define	RC1_BLUE	0x5C3B
#define	RC1_YELLOW	0x5C52
#define	RC1_GREEN	0x5C55
#define	RC1_HELP	0x5C82

#endif

#define	RC_0		0x00
#define	RC_1		0x01
#define	RC_2		0x02
#define	RC_3		0x03
#define	RC_4		0x04
#define	RC_5		0x05
#define	RC_6		0x06
#define	RC_7		0x07
#define	RC_8		0x08
#define	RC_9		0x09
#define	RC_RIGHT	0x0A
#define	RC_LEFT		0x0B
#define	RC_UP		0x0C
#define	RC_DOWN		0x0D
#define	RC_OK		0x0E
#define	RC_MUTE		0x0F
#define	RC_STANDBY	0x10
#define	RC_GREEN	0x11
#define	RC_YELLOW	0x12
#define	RC_RED		0x13
#define	RC_BLUE		0x14
#define	RC_PLUS		0x15
#define	RC_MINUS	0x16
#define	RC_HELP		0x17
#define	RC_DBOX		0x18
#define	RC_HOME		0x1F

// defines for mail-reading
 
#define BORDERSIZE 2
#define FONTHEIGHT_BIG 40
#define FONTHEIGHT_SMALL 24
#define FONT_OFFSET_BIG 10
#define FONT_OFFSET 5
#define VIEWX  619
#define VIEWY  504
#define FRAMEROWS  15

// functions

void ShowMessage(int message);
int CheckPIN(int Account);

// freetype stuff

#define FONT FONTDIR "/pakenham.ttf"

enum {LEFT, CENTER, RIGHT};
enum {SMALL, BIG};

FT_Library		library;
FTC_Manager		manager;
FTC_SBitCache		cache;
FTC_SBit		sbit;
#ifdef OLDFT
FTC_ImageDesc		desc;
#else
FTC_ImageTypeRec	desc;
#endif
FT_Face			face;
FT_UInt			prev_glyphindex;
FT_Bool			use_kerning;

char versioninfo_p[12], versioninfo_d[12] = "?.??";

// config

char admin = 'Y';
char osd = 'G';
int skin = 1;

// mail database

struct mi
{
	char type[2];	/* N=new, O=old, D=del */
	char save[2];	/* save type for restore */
	char uid[80];
	char date[8];	/* 01.Jan */
	char time[6];	/* 00:00 */
	char from[256];
	char subj[256];
};

struct
{
	int inactive;
	int mails;
	int mark_red;	/* del all */
	int mark_green;	/* del new */
	int mark_yellow;/* del old */
	char nr[2];	/* 0...9 */
	char time[6];	/* 00:00 */
	char name[32];
	char status[8];	/* 000/000 */
	int pincount;
	char code[5];
	struct mi mailinfo[MAXMAIL];

}maildb[10];

// devs

int fb, rc, lcd;

// daemon commands

enum {GET_STATUS, SET_STATUS, RELOAD_SPAMLIST, GET_VERSION, GET_MAIL, SEND_MAIL};

// framebuffer stuff

enum {FILL, GRID};
enum {TRANSP, WHITE, SKIN0, SKIN1, SKIN2, ORANGE, GREEN, YELLOW, RED};
enum {NODAEMON, STARTDONE, STARTFAIL, STOPDONE, STOPFAIL, BOOTON, BOOTOFF, ADD2SPAM, DELSPAM, SPAMFAIL, INFO, GETMAIL, GETMAILFAIL, SENDMAILDONE, SENDMAILFAIL};

unsigned char *lfb = 0, *lbb = 0;

struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;

unsigned short rd1[] = {0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0xFF<<8};
unsigned short gn1[] = {0xFF<<8, 0x00<<8, 0x40<<8, 0x80<<8, 0xC0<<8, 0xFF<<8, 0xFF<<8, 0x00<<8};
unsigned short bl1[] = {0xFF<<8, 0x80<<8, 0x80<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short tr1[] = {0x0000,  0x0A00,  0x0A00,  0x0A00,  0x0000,  0x0000,  0x0000,  0x0000 };

unsigned short rd2[] = {0xFF<<8, 0x25<<8, 0x4A<<8, 0x97<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0xFF<<8};
unsigned short gn2[] = {0xFF<<8, 0x3A<<8, 0x63<<8, 0xAC<<8, 0xC0<<8, 0xFF<<8, 0xFF<<8, 0x00<<8};
unsigned short bl2[] = {0xFF<<8, 0x4D<<8, 0x77<<8, 0xC1<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short tr2[] = {0x0000,  0x0A00,  0x0A00,  0x0A00,  0x0000,  0x0000,  0x0000,  0x0000 };

struct fb_cmap colormap1 = {1, 8, rd1, gn1, bl1, tr1};
struct fb_cmap colormap2 = {1, 8, rd2, gn2, bl2, tr2};

int startx, starty, sx, ex, sy, ey;
char online;
char mailfile;

#if HAVE_DVB_API_VERSION == 3

struct input_event ev;

#endif

unsigned short rccode;
int sim_key = 0;

char scroll_up[] =
{
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0,
	SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0,
	SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0,
	SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0,
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};

char scroll_dn[] =
{
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0,
	SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0,
	SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0,
	SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0,
	SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, WHITE, WHITE, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0, SKIN0
};

char circle[] =
{
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0
};

// lcd stuff

unsigned char lcd_buffer[] =
{
	0xE0, 0xF8, 0xFC, 0xFE, 0xFE, 0xFF, 0x7F, 0x7F, 0x7F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0xFF, 0xFF, 0xFF, 0xC7, 0xBB, 0x3B, 0xFB, 0xFB, 0x3B, 0xBB, 0xC7, 0xFF, 0x07, 0xFB, 0xFB, 0x07, 0xFB, 0xFB, 0xFB, 0x07, 0xFF, 0x87, 0x7B, 0xFB, 0xC7, 0xFB, 0x7B, 0x7B, 0x87, 0xFF, 0x07, 0xFB, 0xFB, 0x3B, 0xFB, 0xFB, 0xFB, 0x3B, 0xFB, 0xFB, 0x07, 0xFF, 0x07, 0xFB, 0xFB, 0xBB, 0xFB, 0xFB, 0xFB, 0x07, 0xFF, 0x27, 0xDB, 0xDB, 0x27, 0xFF, 0x07, 0xFB, 0xFB, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x7F, 0x7F, 0x7F, 0xFF, 0xFE, 0xFE, 0xFC, 0xF8, 0xE0,
	0xFF, 0x7F, 0x7F, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x70, 0x6F, 0x6F, 0x70, 0x7F, 0x7F, 0x7F, 0x70, 0x6F, 0x6F, 0x6C, 0x6F, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x71, 0x6F, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x70, 0x6F, 0x6F, 0x6F, 0x70, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x71, 0x6F, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x6C, 0x6D, 0x6D, 0x6D, 0x73, 0x7F, 0x7F, 0x7F, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7F, 0x7F, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0xF8, 0xFC, 0x04, 0xFC, 0xF8, 0x00, 0xFC, 0xFC, 0x30, 0x60, 0xFC, 0xFC, 0x00, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x40, 0xC0, 0x00, 0xC0, 0x40, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x20, 0x20, 0x20, 0xC0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0xC0, 0xE1, 0x31, 0x18, 0x09, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x77, 0x55, 0xDD, 0x00, 0xDD, 0x55, 0xDD, 0x00, 0xDD, 0x55, 0x77, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xE3, 0x14, 0x14, 0x14, 0x14, 0xE3, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x81, 0x83, 0x9F, 0x9E, 0x8F, 0x87, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0xFF, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x81, 0x82, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0xE0, 0x60, 0x60, 0xA0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xA0, 0x60, 0x60, 0xE0, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x10, 0x10, 0x10, 0x10, 0xE0, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0xE0, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x80, 0x40, 0x21, 0x11, 0x0A, 0x06, 0x04, 0x08, 0x08, 0x04, 0x06, 0x0A, 0x11, 0x21, 0x40, 0x80, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x3E, 0x01, 0x00, 0x00, 0xC0, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF,
	0x07, 0x18, 0x20, 0x40, 0x43, 0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x80, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x80, 0x83, 0x84, 0x84, 0x84, 0x84, 0x83, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x83, 0x80, 0x80, 0x80, 0x83, 0x86, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x86, 0x43, 0x40, 0x20, 0x18, 0x07
};

char lcd_status[] =
{
	1,1,1,1,0,0,1,1,1,1,1,0,1,1,0,1,1,	/* PAU */
	1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,
	1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,
	1,1,1,1,0,0,1,1,1,1,1,0,1,1,0,1,1,
	1,1,0,0,0,0,1,1,0,1,1,0,1,1,0,1,1,
	1,1,0,0,0,0,1,1,0,1,1,0,1,1,0,1,1,
	1,1,0,0,0,0,1,1,0,1,1,0,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
	0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,1,1,1,0,1,1,0,0,0,0,0,0,0,
	0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,

	0,1,1,1,0,0,1,1,0,0,1,1,0,1,1,0,0,	/* ONL */
	1,1,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,
	1,1,0,1,1,0,1,1,1,0,1,1,0,1,1,0,0,
	1,1,0,1,1,0,1,1,1,1,1,1,0,1,1,0,0,
	1,1,0,1,1,0,1,1,0,1,1,1,0,1,1,0,0,
	1,1,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,
	0,1,1,1,0,0,1,1,0,0,1,1,0,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,

	0,1,1,1,0,0,1,1,1,1,1,1,0,1,1,0,0,	/* OFL */
	1,1,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,
	1,1,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,
	1,1,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0,
	1,1,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,
	1,1,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,
	0,1,1,1,0,0,0,1,1,0,0,0,0,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,
	0,0,0,0,0,1,1,1,0,1,1,1,1,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char lcd_digits[] =
{
	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,0,0,1,1,1,1,0,0,0,
	0,0,1,1,0,0,1,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,1,0,0,1,1,0,0,
	0,0,0,1,1,1,1,0,0,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,0,1,1,1,1,0,
	1,1,0,1,1,1,0,0,1,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,1,0,0,1,1,
	0,0,0,0,0,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,1,0,0,1,1,
	0,0,0,0,0,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,0,0,0,0,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,
};

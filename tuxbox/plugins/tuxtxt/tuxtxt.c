/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    TOP-Text Support 2004 by Roland Meier <RolandMeier@Siemens.com>         *
 *    Info entnommen aus videotext-0.6.19991029,                              *
 *    Copyright (c) 1994-96 Martin Buck  <martin-2.buck@student.uni-ulm.de>   *
 *                                                                            *
 ******************************************************************************/

#include "tuxtxt.h"

/******************************************************************************
 * plugin_exec                                                                *
 ******************************************************************************/

#if HAVE_DVB_API_VERSION < 3
 #define dmx_pes_filter_params dmxPesFilterParams
 #define pes_type pesType
 #define dmx_sct_filter_params dmxSctFilterParams
#endif


void next_dec(int *i) /* skip to next decimal */
{
	(*i)++;

	if ((*i & 0x0F) > 0x09)
		*i += 0x06;

	if ((*i & 0xF0) > 0x90)
		*i += 0x60;

	if (*i > 0x899)
		*i = 0x100;
}

void prev_dec(int *i)           /* counting down */
{
	(*i)--;

	if ((*i & 0x0F) > 0x09)
		*i -= 0x06;

	if ((*i & 0xF0) > 0x90)
		*i -= 0x60;

	if (*i < 0x100)
		*i = 0x899;
}

int getIndexOfPageInHotlist()
{
	int i;
	for (i=0; i<=maxhotlist; i++)
	{
		if (page == hotlist[i])
			return i;
	}
	return -1;
}

void gethotlist()
{
	FILE *hl;
	char line[100];

	maxhotlist = -1;
	sprintf(line, CONFIGDIR "/tuxtxt/hotlist%d.conf", vtxtpid);
#if DEBUG
	printf("TuxTxt <gethotlist %s", line);
#endif
	if ((hl = fopen(line, "rb")) != 0)
	{
		do {
			if (!fgets(line, sizeof(line), hl))
				break;

			if (1 == sscanf(line, "%x", &hotlist[maxhotlist+1]))
			{
				if (hotlist[maxhotlist+1] >= 0x100 && hotlist[maxhotlist+1] <= 0x899)
				{
#if DEBUG
					printf(" %03x", hotlist[maxhotlist+1]);
#endif
					maxhotlist++;
					continue;
				}
			}
#if DEBUG
			else
				printf(" ?%s?", line);
#endif
		} while (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1));
		fclose(hl);
	}
#if DEBUG
	printf(">\n");
#endif
	if (maxhotlist < 0) /* hotlist incorrect or not found */
	{
		hotlist[0] = 0x100; /* create one */
		hotlist[1] = 0x303;
		maxhotlist = 1;
	}
}

void savehotlist()
{
	FILE *hl;
	char line[100];
	int i;

	sprintf(line, CONFIGDIR "/tuxtxt/hotlist%d.conf", vtxtpid);
#if DEBUG
	printf("TuxTxt <savehotlist %s", line);
#endif
	if (maxhotlist != 1 || hotlist[0] != 0x100 || hotlist[1] != 0x303)
	{
		if ((hl = fopen(line, "wb")) != 0)
		{
			for (i=0; i<=maxhotlist; i++)
			{
				fprintf(hl, "%03x\n", hotlist[i]);
#if DEBUG
				printf(" %03x", hotlist[i]);
#endif
			}
			fclose(hl);
		}
	}
	else
	{
		unlink(line); /* remove current hotlist file */
#if DEBUG
		printf(" (default - just deleted)");
#endif
	}
#if DEBUG
	printf(">\n");
#endif
}

void hex2str(char *s, unsigned int n)
{
	/* print hex-number into string, s points to last digit, caller has to provide enough space, no termination */
	do {
		char c = (n & 0xF);

		if (c > 9)
			c += 'A'-10;
		else
			c += '0';

		*s-- = c;
		n >>= 4;
	} while (n);
}

void decode_btt()
{
	/* basic top table */
	int i, current, b1, b2, b3, b4;

	current = 0x100;
#if DEBUG_BTT
	printf("TuxTxt <BTT %03x ", current);
#endif
	for (i=0; i<799; i++)
	{
		b1 = cachetable[0x1f0][0][40+i];
		if (b1 == ' ')
			b1 = 0;
		else
		{
			b1 = dehamming[b1];

			if (b1 == 0xFF)
			{
#if DEBUG_BTT
				printf("?%c?>\n", cachetable[0x1f0][40+0][i]);
				cachetable[0x1f0][0][40+799] = 0; /* mark btt as not received */
				return;
#endif
			}
		}
		basictop[current] = b1;
		next_dec(&current);
#if DEBUG_BTT
		printf("%x", b1);
		if ((current & 0x0FF) == 0)
			printf(">\nTuxTxt <BTT %03x ", current);
#endif
	}
#if DEBUG_BTT
	printf(">\n");
#endif

#if DEBUG_BTT
	printf("TuxTxt <btt/plt"); /* page linking table */
#endif
	/* page linking table */
	maxadippg = -1; /* rebuild table of adip pages */
	for (i = 0; i < 10; i++)
	{
		b1 = dehamming[cachetable[0x1f0][0][840 + 8*i +0]];

		if (b1 == 0xE)
		{
#if DEBUG_BTT
			printf(" (u)");
#endif
			continue; /* unused */
		}
		else if (b1 == 0xF)
		{
#if DEBUG_BTT
			printf(" (end)");
#endif
			break; /* end */
		}

		b4 = dehamming[cachetable[0x1f0][0][840 + 8*i +7]];

		if (b4 != 2) /* only adip, ignore multipage (1) */
		{
#if DEBUG_BTT
			printf(" (%x)", b4);
#endif
			continue;
		}

		b2 = dehamming[cachetable[0x1f0][0][840 + 8*i +1]];
		b3 = dehamming[cachetable[0x1f0][0][840 + 8*i +2]];

		if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
		{
#if DEBUG_BTT
			printf(" Biterror!>\n");
#else
			printf("TuxTxt <Biterror in btt/plt index %d>\n", i);
#endif
			cachetable[0x1f0][0][40+799] = 0; /* mark btt as not received */
			return;
		}

		b1 = b1<<8 | b2<<4 | b3; /* page number */
		adippg[++maxadippg] = b1;
#if DEBUG_BTT
		printf(" %03x", b1);
#endif
	}
#if DEBUG_BTT
	printf(">\n");
#elif DEBUG
	printf("TuxTxt <BTT decoded>\n");
#endif
	bttok = 1;
}

void decode_adip() /* additional information table */
{
	int i, p, j, b1, b2, b3, charfound;

	for (i = 0; i <= maxadippg; i++)
	{
		p = adippg[i];
		if (cachetable[p][0]) /* cached (avoid segfault) */
		{
			if (cachetable[p][0][40+20*43+0] != 0x01) /* completely received, 1 is invalid as hamming */
			{
				for (j = 0; j < 44; j++)
				{
					b1 = dehamming[cachetable[p][0][40+20*j+0]];
					if (b1 == 0xE)
					{
#if DEBUG_ADIP
						printf("TuxTxt <adip %03x %2d unused>\n", p, j);
#endif
						continue; /* unused */
					}

					if (b1 == 0xF)
					{
#if DEBUG_ADIP
						printf("TuxTxt <adip %03x %2d end>\n", p, j);
#endif
						break; /* end */
					}

					b2 = dehamming[cachetable[p][0][40+20*j+1]];
					b3 = dehamming[cachetable[p][0][40+20*j+2]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
					{
						printf("TuxTxt <Biterror in ait %03x %d %02x %02x %02x %02x %02x %02x>\n", p, j,
								 cachetable[p][0][40+20*j+0],
								 cachetable[p][0][40+20*j+1],
								 cachetable[p][0][40+20*j+2],
								 b1, b2, b3
								 );
						cachetable[p][40+20*43+0] = 0; /* mark as not received */
						return;
					}

					if (b1>8 || b2>9 || b3>9)
					{
#if DEBUG_ADIP
						printf("TuxTxt <adip %03x %2d %x%x%x ignored>\n", p, j, b1, b2, b3);
#endif
						continue;
					}

					b1 = b1<<8 | b2<<4 | b3; /* page number */
					charfound = 0; /* flag: no printable char found */

					for (b2 = 11; b2 >= 0; b2--)
					{
						b3 = cachetable[p][0][40+20*j + 8 + b2];

						if ((b3&1) ^ ((b3>>1)&1) ^ ((b3>>2)&1) ^ ((b3>>3)&1) ^
						    ((b3>>4)&1) ^ ((b3>>5)&1) ^ ((b3>>6)&1) ^ (b3>>7))
							b3 &= 0x7F;
						else
							b3 = ' ';

						if (b3 < ' ')
							b3 = ' ';

						if (b3 == ' ' && !charfound)
							adip[b1][b2] = '\0';
						else
						{
							adip[b1][b2] = b3;
							charfound = 1;
						}
					}
#if DEBUG_ADIP
					printf("TuxTxt <adip %03x %2d %03x %s>\n", p, j, b1, adip[b1]);
#endif
				}
				adippg[i] = 0; /* completely decoded: clear entry */
			}
		}
#if DEBUG
		printf("TuxTxt <ADIP %03x decoded>\n", p);
#endif
	}

	while (!adippg[maxadippg] && (maxadippg >= 0)) /* and shrink table */
		maxadippg--;
}


int toptext_getnext(int startpage, int up, int findgroup)
{
	int current, nextgrp, nextblk;

	nextgrp = nextblk = 0;
	current = startpage;

	do {
		if (up)
			next_dec(&current);
		else
			prev_dec(&current);

		if (subpagetable[current]!=0xFF) /* only if cached */
		{
			if (findgroup)
			{
				if (basictop[current] >= 6 && basictop[current] <= 7)
					return current;

				if (!nextgrp && (current&0x00F) == 0)
				{
					if (!bttok)
						return current;

					nextgrp = current;
				}
			}
			if (basictop[current] >= 2 && basictop[current] <= 5) /* always find block */
				return current;

			if (!nextblk && (current&0x0FF) == 0)
			{
				if (!bttok)
					return current;

				nextblk = current;
			}
		}
	} while (current != startpage);

	if (nextgrp)
		return nextgrp;
	else if (nextblk)
		return nextblk;
	else
		return startpage;
}

void RenderClearMenuLineBB(char *p, int attrcol, int attr)
{
	int col;

	PosX = screen_mode2 ? TOPMENU169STARTX : TOPMENU43STARTX;
	RenderCharBB(' ', attrcol);			 /* indicator for navigation keys */
#if 0
	RenderCharBB(' ', attr);				 /* separator */
#endif
	for(col = 0; col < TOPMENULINEWIDTH; col++)
	{
		RenderCharBB(*p++, attr);
	}
	PosY += FONTHEIGHT_NORMAL;
	memset(p-TOPMENULINEWIDTH, ' ', TOPMENULINEWIDTH); /* init with spaces */
}


void plugin_exec(PluginParam *par)
{
	char cvs_revision[] = "$Revision: 1.61 $", versioninfo[16];

	/* show versioninfo */
	sscanf(cvs_revision, "%*s %s", versioninfo);
	printf("TuxTxt %s\n", versioninfo);

	/* get params */
	vtxtpid = fb = lcd = rc = sx = ex = sy = ey = -1;

	for (; par; par = par->next)
	{
		if (!strcmp(par->id, P_ID_VTXTPID))
			vtxtpid = atoi(par->val);
		else if (!strcmp(par->id, P_ID_FBUFFER))
			fb = atoi(par->val);
		else if (!strcmp(par->id, P_ID_LCD))
			lcd = atoi(par->val);
		else if (!strcmp(par->id, P_ID_RCINPUT))
			rc = atoi(par->val);
		else if (!strcmp(par->id, P_ID_OFF_X))
			sx = atoi(par->val);
		else if (!strcmp(par->id, P_ID_END_X))
			ex = atoi(par->val);
		else if (!strcmp(par->id, P_ID_OFF_Y))
			sy = atoi(par->val);
		else if (!strcmp(par->id, P_ID_END_Y))
			ey = atoi(par->val);
	}

	if (vtxtpid == -1 || fb == -1 || lcd == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
	{
		printf("TuxTxt <Invalid Param(s)>\n");
		return;
	}

	/* initialisations */
	if (Init() == 0)
		return;

	/* main loop */
	do {
		if (GetRCCode() == 1)
		{
			if (transpmode == 2) /* TV mode */
			{
				switch (RCCode)
				{
				case RC_UP:
				case RC_DOWN:
				case RC_0:
				case RC_1:
				case RC_2:
				case RC_3:
				case RC_4:
				case RC_5:
				case RC_6:
				case RC_7:
				case RC_8:
				case RC_9:
				case RC_RED:
				case RC_GREEN:
				case RC_YELLOW:
				case RC_BLUE:
				case RC_PLUS:
				case RC_MINUS:
				case RC_DBOX:
					transpmode = 1; /* switch to normal mode */
					SwitchTranspMode();
					break;		/* and evaluate key */

				case RC_MUTE:		/* regular toggle to transparent */
					break;

				default:
					continue; /* ignore all other keys */
				}
			}

			switch (RCCode)
			{
			case RC_UP:	GetNextPageOne();	break;
			case RC_DOWN:	GetPrevPageOne();	break;
			case RC_RIGHT:	GetNextSubPage();	break;
			case RC_LEFT:	GetPrevSubPage();	break;
			case RC_OK:
				if (subpagetable[page] == 0xFF)
					continue;
				PageCatching();
				break;

			case RC_0:	PageInput(0);		break;
			case RC_1:	PageInput(1);		break;
			case RC_2:	PageInput(2);		break;
			case RC_3:	PageInput(3);		break;
			case RC_4:	PageInput(4);		break;
			case RC_5:	PageInput(5);		break;
			case RC_6:	PageInput(6);		break;
			case RC_7:	PageInput(7);		break;
			case RC_8:	PageInput(8);		break;
			case RC_9:	PageInput(9);		break;
			case RC_RED:	Prev100();		break;
			case RC_GREEN:	Prev10();		break;
			case RC_YELLOW:	Next10();		break;
			case RC_BLUE:	Next100();		break;
			case RC_PLUS:	SwitchZoomMode();	break;
			case RC_MINUS:	SwitchScreenMode(-1);	break;
			case RC_MUTE:	SwitchTranspMode();	break;
			case RC_HELP:	SwitchHintMode();	break;
			case RC_DBOX:	ConfigMenu(0);		break;
			}
		}

		/* update page or timestring and lcd */
		RenderPage();
	} while ((RCCode != RC_HOME) && (RCCode != RC_STANDBY));

	/* exit */
	CleanUp();
}

/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

#if DEBUG
	if (!result)
		printf("TuxTxt <Font (%s) loaded>\n", (char*)face_id);
	else
		printf("TuxTxt <open font %s failed>\n", (char*)face_id);
#endif

	return result;
}

/******************************************************************************
 * Init                                                                       *
 ******************************************************************************/

int Init()
{
	struct dmx_pes_filter_params dmx_flt;
	int error;
	unsigned char magazine;

	/* init data */
	memset(&cachetable, 0, sizeof(cachetable));
	memset(&subpagetable, 0xFF, sizeof(subpagetable));
	memset(&countrycontrolbitstable, 0xFF, sizeof(countrycontrolbitstable));
	memset(&backbuffer, black, sizeof(backbuffer));

	memset(&basictop, 0, sizeof(basictop));
	memset(&adip, 0, sizeof(adip));
	maxadippg  = -1;
	bttok      = 0;
	maxhotlist = -1;
	pc_old_row = pc_old_col = 0; /* for page catching */

	page_atrb[32] = transp<<4 | transp;
	inputcounter  = 2;
	cached_pages  = 0;

	for (magazine = 1; magazine < 9; magazine++)
	{
		current_page  [magazine] = -1;
		current_subpage [magazine] = -1;
	}
	page_receiving = -1;

	page       = 0x100;
	lastpage   = 0x100;
	prev_100   = 0x100;
	prev_10    = 0x100;
	next_100   = 0x100;
	next_10    = 0x100;
	subpage    = 0;
	pageupdate = 0;

	zap_subpage_manual = 0;

	/* init lcd */
	UpdateLCD();

	/* load config */
	screen_mode1 = 1;
	screen_mode2 = 1;
	color_mode   = 1;
	national_subset = 4;
	auto_national   = 1;

	if ((conf = fopen(CONFIGDIR "/tuxtxt/tuxtxt.conf", "rb+")) == 0)
	{
		perror("TuxTxt <fopen tuxtxt.conf>");
		return 0;
	}

	fread(&screen_mode1, 1, sizeof(screen_mode1), conf);
	fread(&screen_mode2, 1, sizeof(screen_mode2), conf);
	fread(&color_mode, 1, sizeof(color_mode), conf);
	fread(&national_subset, 1, sizeof(national_subset), conf);
	fread(&auto_national, 1, sizeof(auto_national), conf);

	screen_old1 = screen_mode1;
	screen_old2 = screen_mode2;
	color_old   = color_mode;
	national_subset_old = national_subset;
	auto_national_old   = auto_national;

	/* init fontlibrary */
	if ((error = FT_Init_FreeType(&library)))
	{
		printf("TuxTxt <FT_Init_FreeType: 0x%.2X>", error);
		return 0;
	}

	if ((error = FTC_Manager_New(library, 3, 2, 0, &MyFaceRequester, NULL, &manager)))
	{
		printf("TuxTxt <FTC_Manager_New: 0x%.2X>\n", error);
		return 0;
	}

	if ((error = FTC_SBit_Cache_New(manager, &cache)))
	{
		printf("TuxTxt <FTC_SBit_Cache_New: 0x%.2X>\n", error);
		return 0;
	}

	type0.font.face_id = (FTC_FaceID) TUXTXT0;
	type1.font.face_id = (FTC_FaceID) TUXTXT1;
	type2.font.face_id = (FTC_FaceID) TUXTXT2;
#if HAVE_DVB_API_VERSION >= 3
	type0.flags = type1.flags = type2.flags = FT_LOAD_MONOCHROME;
#endif
	type0.font.pix_width  = type1.font.pix_width  = type2.font.pix_width  = (FT_UShort) FONTWIDTH_NORMAL;
	type0.font.pix_height = type1.font.pix_height = type2.font.pix_height = (FT_UShort) FONTHEIGHT_NORMAL+1;

	type0r = type0;
	type1r = type1;
	type2r = type2;
	type0r.font.pix_width  = type1r.font.pix_width  = type2r.font.pix_width  = (FT_UShort) FONTWIDTH_TOPMENUMAIN;
	type0r.font.face_id = (FTC_FaceID) TUXTXT0R;
	type1r.font.face_id = (FTC_FaceID) TUXTXT1R;
	type2r.font.face_id = (FTC_FaceID) TUXTXT2R;

	/* center screen */
	StartX = sx + (((ex-sx) - 40*type0.font.pix_width) / 2);
	StartY = sy + (((ey-sy) - 25*fixfontheight) / 2);

	/* get fixed screeninfo */
	if (ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_FSCREENINFO>");
		return 0;
	}

	/* get variable screeninfo */
	if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_VSCREENINFO>");
		return 0;
	}

	/* set new colormap */
	if (color_mode)
	{
		if (ioctl(fb, FBIOPUTCMAP, &colormap_2) == -1)
		{
			perror("TuxTxt <FBIOPUTCMAP>");
			return 0;
		}
	}
	else
	{
		if (ioctl(fb, FBIOPUTCMAP, &colormap_1) == -1)
		{
			perror("TuxTxt <FBIOPUTCMAP>");
			return 0;
		}
	}

	/* map framebuffer into memory */
	lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	if (!lfb)
	{
		perror("TuxTxt <mmap>");
		return 0;
	}

	/* open demuxer */
	if ((dmx = open(DMX, O_RDWR)) == -1)
	{
		perror("TuxTxt <open DMX>");
		return 0;
	}

	/*  if no vtxtpid for current service, search PIDs */
	if (vtxtpid == 0)
	{
		/* get all vtxt-pids */
		getpidsdone = -1;						 /* don't kill thread */
		if (GetTeletextPIDs() == 0)
		{
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			close(dmx);
			return 0;
		}

		if (pids_found > 1)
			ConfigMenu(1);
		else
		{
			vtxtpid = pid_table[0].vtxt_pid;
			strcpy(country_code, pid_table[0].country_code);

			current_service = 0;
			RenderMessage(ShowServiceName);
		}
	}
	else
	{
		SDT_ready = 0;
		getpidsdone = 0;
		strcpy(country_code, "deu"); /* assume default */
		pageupdate = 1; /* force display of message page not found (but not twice) */
	}

	/* open avs */
	if ((avs = open(AVS, O_RDWR)) == -1)
	{
		perror("TuxTxt <open AVS>");
		return 0;
	}

	ioctl(avs, AVSIOGSCARTPIN8, &fnc_old);
	ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);

	/* open saa */
	if ((saa = open(SAA, O_RDWR)) == -1)
	{
		perror("TuxTxt <open SAA>");
		return 0;
	}

	ioctl(saa, SAAIOGWSS, &saa_old);
	ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

	/* open pig */
	if ((pig = open(PIG, O_RDWR)) == -1)
	{
		perror("TuxTxt <open PIG>");
		return 0;
	}

	/* setup rc */
	fcntl(rc, F_SETFL, O_NONBLOCK);
	ioctl(rc, RC_IOCTL_BCODES, 1);

	if (ioctl(dmx, DMX_SET_BUFFER_SIZE, 64*1024)<0)
	{
		perror("Tuxtxt <DMX_SET_BUFFERSIZE>");
		return 0;
	}

	/* set filter & start demuxer */
	dmx_flt.pid      = vtxtpid;
	dmx_flt.input    = DMX_IN_FRONTEND;
	dmx_flt.output   = DMX_OUT_TAP;
	dmx_flt.pes_type = DMX_PES_OTHER;
	dmx_flt.flags    = DMX_IMMEDIATE_START;

	if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_PES_FILTER>");
		return 0;
	}

	/* create decode-thread */
	if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
	{
		perror("TuxTxt <pthread_create>");
		return 0;
	}

	gethotlist();

	/* init successfull */
	return 1;
}

/******************************************************************************
 * Cleanup                                                                    *
 ******************************************************************************/

void CleanUp()
{
	/* hide pig */
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */

	/* restore videoformat */
	ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);
	ioctl(saa, SAAIOSWSS, &saa_old);

	/* stop decode-thread */
	if (pthread_cancel(thread_id) != 0)
	{
		perror("TuxTxt <pthread_cancel>");
		return;
	}

	if (pthread_join(thread_id, &thread_result) != 0)
	{
		perror("TuxTxt <pthread_join>");
		return;
	}

	/* stop & close demuxer */
	ioctl(dmx, DMX_STOP);
	close(dmx);

	 /* close avs */
	close(avs);

	/* close saa */
	close(saa);

	/* close freetype */
	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	/* unmap framebuffer */
	munmap(lfb, fix_screeninfo.smem_len);

	/* free pagebuffers */
	for (clear_page = 0; clear_page < 0x900; clear_page++)
		for (clear_subpage = 0; clear_subpage < 0x80; clear_subpage++)
			if (cachetable[clear_page][clear_subpage] != 0)
				free(cachetable[clear_page][clear_subpage]);

	/* save config */
	if (screen_mode1 != screen_old1 || screen_mode2 != screen_old2 ||
	    color_mode != color_old || national_subset != national_subset_old || auto_national != auto_national_old)
	{
		rewind(conf);

		fwrite(&screen_mode1, 1, sizeof(screen_mode1), conf);
		fwrite(&screen_mode2, 1, sizeof(screen_mode2), conf);
		fwrite(&color_mode, 1, sizeof(color_mode), conf);
		fwrite(&national_subset, 1, sizeof(national_subset), conf);
		fwrite(&auto_national, 1, sizeof(auto_national), conf);

		printf("TuxTxt <saving config>\n");
	}

	fclose(conf);
}

/******************************************************************************
 * GetTeletextPIDs                                                           *
 ******************************************************************************/

int GetTeletextPIDs()
{
	struct dmx_sct_filter_params dmx_flt;
	struct dmx_pes_filter_params dmx_pes_flt;
	int pat_scan, pmt_scan, sdt_scan, desc_scan, pid_test, byte, diff, first_sdt_sec;

	unsigned char PAT[1024];
	unsigned char SDT[1024];
	unsigned char PMT[1024];

	if (!getpidsdone)							 /* call not from Init */
	{
		/* stop old decode-thread */
		if (pthread_cancel(thread_id) != 0)
		{
			perror("TuxTxt <pthread_cancel>");
		}

		if (pthread_join(thread_id, &thread_result) != 0)
		{
			perror("TuxTxt <pthread_join>");
		}

		/* stop demuxer */
		ioctl(dmx, DMX_STOP);
	}

	/* show infobar */
	RenderMessage(ShowInfoBar);

	/* read PAT to get all PMT's */
#if HAVE_DVB_API_VERSION < 3
	memset(dmx_flt.filter.filter, 0, DMX_FILTER_SIZE);
	memset(dmx_flt.filter.mask, 0, DMX_FILTER_SIZE);
#else
	memset(&dmx_flt.filter, 0x00, sizeof(struct dmx_filter));
#endif

	dmx_flt.pid              = 0x0000;
	dmx_flt.flags            = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
	dmx_flt.filter.filter[0] = 0x00;
	dmx_flt.filter.mask[0]   = 0xFF;
	dmx_flt.timeout          = 5000;

	if (ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_FILTER PAT>");
		return 0;
	}

	if (read(dmx, PAT, sizeof(PAT)) == -1)
	{
		perror("TuxTxt <read PAT>");
		return 0;
	}

	/* scan each PMT for vtxt-pid */
	pids_found = 0;

	for (pat_scan = 0x0A; pat_scan < 0x0A + (((PAT[0x01]<<8 | PAT[0x02]) & 0x0FFF) - 9); pat_scan += 4)
	{
		if (((PAT[pat_scan - 2]<<8) | (PAT[pat_scan - 1])) == 0)
			continue;

		dmx_flt.pid               = (PAT[pat_scan]<<8 | PAT[pat_scan+1]) & 0x1FFF;
		dmx_flt.flags             = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		dmx_flt.filter.filter[0]  = 0x02;
		dmx_flt.filter.mask[0]    = 0xFF;
		dmx_flt.timeout           = 5000;

		if (ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_FILTER PMT>");
			continue;
		}

		if (read(dmx, PMT, sizeof(PMT)) == -1)
		{
			perror("TuxTxt <read PMT>");
			continue;
		}
		for (pmt_scan = 0x0C + ((PMT[0x0A]<<8 | PMT[0x0B]) & 0x0FFF); pmt_scan < (((PMT[0x01]<<8 | PMT[0x02]) & 0x0FFF) - 7); pmt_scan += 5 + PMT[pmt_scan + 4])
		{
			if (PMT[pmt_scan] == 6)
			{
				for (desc_scan = pmt_scan + 5; desc_scan < pmt_scan + ((PMT[pmt_scan + 3]<<8 | PMT[pmt_scan + 4]) & 0x0FFF) + 5; desc_scan += 2 + PMT[desc_scan + 1])
				{
					if (PMT[desc_scan] == 0x56)
					{
						for (pid_test = 0; pid_test < pids_found; pid_test++)
							if (pid_table[pid_test].vtxt_pid == ((PMT[pmt_scan + 1]<<8 | PMT[pmt_scan + 2]) & 0x1FFF))
								goto skip_pid;

						pid_table[pids_found].vtxt_pid     = (PMT[pmt_scan + 1]<<8 | PMT[pmt_scan + 2]) & 0x1FFF;
						pid_table[pids_found].service_id = PMT[0x03]<<8 | PMT[0x04];
						if (PMT[desc_scan + 1] == 5)
						{
							pid_table[pids_found].country_code[0] = PMT[desc_scan + 2] | 0x20;
							pid_table[pids_found].country_code[1] = PMT[desc_scan + 3] | 0x20;
							pid_table[pids_found].country_code[2] = PMT[desc_scan + 4] | 0x20;
							pid_table[pids_found].country_code[3] = 0;
						}
						else
						{
							pid_table[pids_found].country_code[0] = 0;
						}

						if (pid_table[pids_found].vtxt_pid == vtxtpid)
							printf("TuxTxt <Country code \"%s\">\n", pid_table[pids_found].country_code);

						pids_found++;
skip_pid:
					;
					}
				}
			}
		}
	}

	/* check for teletext */
	if (pids_found == 0)
	{
		printf("TuxTxt <no Teletext on TS found>\n");

		RenderMessage(NoServicesFound);
		sleep(3);
		return 0;
	}

	/* read SDT to get servicenames */
	SDT_ready = 0;

	dmx_flt.pid              = 0x0011;
	dmx_flt.flags            = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
	dmx_flt.filter.filter[0] = 0x42;
	dmx_flt.filter.mask[0]   = 0xFF;
	dmx_flt.timeout          = 5000;

	if (ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_FILTER SDT>");

		RenderMessage(ShowServiceName);

		return 1;
	}

	first_sdt_sec = -1;
	while (1)
	{
		if (read(dmx, SDT, 3) == -1)
		{
			perror("TuxTxt <read SDT>");

			ioctl(dmx, DMX_STOP);
			RenderMessage(ShowServiceName);
			return 1;
		}

		if (read(dmx, SDT+3, ((SDT[1] & 0x0f) << 8) | SDT[2]) == -1)
		{
			perror("TuxTxt <read SDT>");

			ioctl(dmx, DMX_STOP);
			RenderMessage(ShowServiceName);
			return 1;
		}

		if (first_sdt_sec == SDT[6])
			break;

		if (first_sdt_sec == -1)
			first_sdt_sec = SDT[6];

		/* scan SDT to get servicenames */
		for (sdt_scan = 0x0B; sdt_scan < ((SDT[1]<<8 | SDT[2]) & 0x0FFF) - 7; sdt_scan += 5 + ((SDT[sdt_scan + 3]<<8 | SDT[sdt_scan + 4]) & 0x0FFF))
		{
			for (pid_test = 0; pid_test < pids_found; pid_test++)
			{
				if ((SDT[sdt_scan]<<8 | SDT[sdt_scan + 1]) == pid_table[pid_test].service_id && SDT[sdt_scan + 5] == 0x48)
				{
					diff = 0;
					pid_table[pid_test].service_name_len = SDT[sdt_scan+9 + SDT[sdt_scan+8]];

					for (byte = 0; byte < pid_table[pid_test].service_name_len; byte++)
					{
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5B;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7B;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5C;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7C;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5D;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7D;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'�')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7E;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] >= 0x80 && SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] <= 0x9F)
							diff--;
						else
							pid_table[pid_test].service_name[byte + diff] = SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte];
					}

					pid_table[pid_test].service_name_len += diff;
				}
			}
		}
	}
	ioctl(dmx, DMX_STOP);
	SDT_ready = 1;

	/* show current servicename */
	current_service = 0;

	if (vtxtpid != 0)
	{
		while (pid_table[current_service].vtxt_pid != vtxtpid && current_service < pids_found)
			current_service++;

		strcpy(country_code, pid_table[current_service].country_code);
		RenderMessage(ShowServiceName);
	}

	if (!getpidsdone)							 /* call not from Init */
	{
		/* restore filter & start demuxer */
		dmx_pes_flt.pid      = vtxtpid;
		dmx_pes_flt.input    = DMX_IN_FRONTEND;
		dmx_pes_flt.output   = DMX_OUT_TAP;
		dmx_pes_flt.pes_type = DMX_PES_OTHER;
		dmx_pes_flt.flags    = DMX_IMMEDIATE_START;

		if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_pes_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_PES_FILTER>");
			return 0;
		}

		/* start new decode-thread */
		if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
			perror("TuxTxt <pthread_create>");
	}
	getpidsdone = 1;

	RenderCharLCD(pids_found/10,  7, 44);
	RenderCharLCD(pids_found%10, 19, 44);

	return 1;
}

/******************************************************************************
 * GetNationalSubset                                                          *
 ******************************************************************************/

int GetNationalSubset(char *cc)
{
	if (memcmp(cc, "cze", 3) == 0 || memcmp(cc, "ces", 3) == 0 ||
	    memcmp(cc, "slo", 3) == 0 || memcmp(cc, "slk", 3) == 0)
		return 0;
	if (memcmp(cc, "eng", 3) == 0)
		return 1;
	if (memcmp(cc, "est", 3) == 0)
		return 2;
	if (memcmp(cc, "fre", 3) == 0 || memcmp(cc, "fra", 3) == 0)
		return 3;
	if (memcmp(cc, "ger", 3) == 0 || memcmp(cc, "deu", 3) == 0)
		return 4;
	if (memcmp(cc, "ita", 3) == 0)
		return 5;
	if (memcmp(cc, "lav", 3) == 0 || memcmp(cc, "lit", 3) == 0)
		return 6;
	if (memcmp(cc, "pol", 3) == 0)
		return 7;
	if (memcmp(cc, "spa", 3) == 0 || memcmp(cc, "por", 3) == 0)
		return 8;
	if (memcmp(cc, "rum", 3) == 0 || memcmp(cc, "ron", 3) == 0)
		return 9;
	if (memcmp(cc, "scc", 3) == 0 || memcmp(cc, "srp", 3) == 0 ||
	    memcmp(cc, "scr", 3) == 0 || memcmp(cc, "hrv", 3) == 0 ||
	    memcmp(cc, "slv", 3) == 0)
		return 10;
	if (memcmp(cc, "swe", 3) == 0 ||
	    memcmp(cc, "dan", 3) == 0 ||
	    memcmp(cc, "nor", 3) == 0 ||
	    memcmp(cc, "fin", 3) == 0 ||
	    memcmp(cc, "hun", 3) == 0)
		return 11;
	if (memcmp(cc, "tur", 3) == 0)
		return 12;

	return countryconverstiontable[countrycontrolbitstable[page][subpage]];
}

/******************************************************************************
 * ConfigMenu                                                                 *
 ******************************************************************************/

#define Menu_StartX (StartX + type0.font.pix_width*9/2)
#define Menu_StartY (StartY + fixfontheight)
#define Menu_Width 31

const char MenuLine[] =
{
	4,9,12,13,16,19,20
};

enum
{
	M_HOT=0,
	M_PID,
	M_SC1,
	M_SC2,
	M_COL,
	M_AUN,
	M_NAT,
	M_Number
};

#define M_Start M_HOT
#define M_MaxDirect M_AUN

void Menu_HighlightLine(char *menu, int line, int high)
{
	char hilitline[] = "ZXXXXXXXXXXXXXXXXXXXXXXXXXXXXZ�";
	int itext = 2*Menu_Width*line; /* index start menuline */
	int byte;

	PosX = Menu_StartX;
	PosY = Menu_StartY + line*fixfontheight;

	for (byte = 0; byte < Menu_Width; byte++)
		RenderCharFB(menu[itext + byte], (high ? hilitline[byte] : menu[itext + byte+Menu_Width]));
}

void Menu_UpdateHotlist(char *menu, int hotindex, int menuitem)
{
	int i, j, k, attr;

	PosX = Menu_StartX + 6*type0.font.pix_width;
	PosY = Menu_StartY + (MenuLine[M_HOT]+1)*fixfontheight;
	j = 2*Menu_Width*(MenuLine[M_HOT]+1) + 6; /* start index in menu */

	for (i=0; i<=maxhotlist+1; i++)
	{
		if (i == maxhotlist+1) /* clear last+1 entry in case it was deleted */
		{
			attr = '�';
			memset(&menu[j], ' ', 3);
		}
		else
		{
			if (i == hotindex)
			{
				attr = '�';
			}
			else
			{
				attr = '�';
			}
			hex2str(&menu[j+2], hotlist[i]);
		}

		for (k = 0; k < 3; k++)
		{
			RenderCharFB(menu[j+k], attr);
		}

		if (i == 4)
		{
			PosX  = Menu_StartX + 6*type0.font.pix_width;
			PosY += fixfontheight;
			j    += 2*Menu_Width - 4*4;
		}
		else
		{
			j    += 4; /* one space distance */
			PosX += type0.font.pix_width;
		}
	}
	hex2str(&menu[2*Menu_Width*MenuLine[M_HOT] + 20 + 2], (hotindex >= 0) ? hotlist[hotindex] : page);
	memcpy(&menu[2*Menu_Width*MenuLine[M_HOT] + 24], (hotindex >= 0) ? "entf." : "dazu ", 5);
	PosX = Menu_StartX + 20*type0.font.pix_width;
	PosY = Menu_StartY + MenuLine[M_HOT]*fixfontheight;

	for (k=20; k < (24+5); k++)
		RenderCharFB(menu[2*Menu_Width*MenuLine[M_HOT] + k], (menuitem == M_HOT) ? 'X' : menu[2*Menu_Width*MenuLine[M_HOT] + k+Menu_Width]);
}


void ConfigMenu(int Init)
{
	struct dmx_pes_filter_params dmx_flt;
	int val, byte, line, menuitem = M_Start;
	int current_pid = 0;
	int hotindex;
	int oldscreenmode;

	char menu[] =
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
		"������������������������������諫�����������������������������"
		"�     Konfigurationsmenue    �髤�����������������������������"
		"������������������������������髫�����������������������������"
		"�                            �������������������������������˛"
		"�1 Favoriten: Seite 111 dazu ��ˤ���������������������������˛"
		"� ����                       ����Ȩ�������������������������˛"
		"� +-?                        �������������������������������˛"
		"�                            �������������������������������˛"
		"�2     Teletext-Auswahl      ��ˤ���������������������������˛"
		"��                          ��������������������������������˛"
		"�                            �������������������������������˛"
		"�      Bildschirmformat      �������������������������������˛"
		"�3  Standard-Modus 16:9 = aus��ˤ���������������������������˛"
		"�4  TextBild-Modus 16:9 = ein��ˤ���������������������������˛"
		"�                            �������������������������������˛"
		"�5        Helligkeit         ��ˤ���������������������������˛"
		"�Anzeige 1/3 reduzieren = aus�������������������������������˛"
		"�                            �������������������������������˛"
		"�6  nationaler Zeichensatz   ��ˤ���������������������������˛"
		"�automatische Erkennung = aus�������������������������������˛"
		"��    DE (#$@[\\]^_`{|}~)    ��������������������������������˛"
		"�                            �������������������������������˛"
		"������������������������������������������������������������˛"
		"������������������������������ꛛ�����������������������������";

	if (!getpidsdone)
		GetTeletextPIDs();

	/* set current vtxt */
	if (vtxtpid == 0)
		vtxtpid = pid_table[0].vtxt_pid;
	else
	{
		while(pid_table[current_pid].vtxt_pid != vtxtpid && current_pid < pids_found)
			current_pid++;
	}

	if (SDT_ready)
	{
		memcpy(&menu[MenuLine[M_PID]*2*Menu_Width+3+(24-pid_table[current_pid].service_name_len)/2],
		       &pid_table[current_pid].service_name,
		       pid_table[current_pid].service_name_len);
	}
	else
		hex2str(&menu[MenuLine[M_PID]*2*Menu_Width + 13 + 3], vtxtpid);


	if (current_pid == 0 || pids_found == 1)
		menu[MenuLine[M_PID]*2*Menu_Width +  1] = ' ';

	if (current_pid == pids_found - 1 || pids_found == 1)
		menu[MenuLine[M_PID]*2*Menu_Width + 28] = ' ';

	/* set 16:9 modi, colors & national subset */
	if (screen_mode1)
		memcpy(&menu[MenuLine[M_SC1]*2*Menu_Width + 26], "ein", 3);
	if (!screen_mode2)
		memcpy(&menu[MenuLine[M_SC2]*2*Menu_Width + 26], "aus", 3);
	if (color_mode)
		memcpy(&menu[MenuLine[M_COL]*2*Menu_Width + 26], "ein", 3);
	if (auto_national)
		memcpy(&menu[MenuLine[M_AUN]*2*Menu_Width + 26], "ein", 3);
	if (national_subset != 4)
		memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
	if (national_subset == 0  || auto_national)
		menu[MenuLine[M_NAT]*2*Menu_Width +  1] = ' ';
	if (national_subset == 12 || auto_national)
		menu[MenuLine[M_NAT]*2*Menu_Width + 28] = ' ';

	/* clear framebuffer */
	memset(lfb, transp, var_screeninfo.xres * var_screeninfo.yres);

	/* reset to normal mode */
	if (zoommode)
		zoommode = 0;

	if (transpmode)
	{
		transpmode = 0;
		memset(&backbuffer, black, sizeof(backbuffer));
	}

	oldscreenmode = screenmode;
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */

	/* render menu */
	PosY = Menu_StartY;
	for (line = 0; line < sizeof(menu)/(2*Menu_Width); line++)
	{
		PosX = Menu_StartX;

		for (byte = 0; byte < Menu_Width; byte++)
			RenderCharFB(menu[line*2*Menu_Width + byte], menu[line*2*Menu_Width + byte+Menu_Width]);

		PosY += fixfontheight;
	}
	Menu_HighlightLine(menu, MenuLine[menuitem], 1);
	hotindex = getIndexOfPageInHotlist();
	Menu_UpdateHotlist(menu, hotindex, menuitem);

	/* set blocking mode */
	val = fcntl(rc, F_GETFL);
	fcntl(rc, F_SETFL, val &~ O_NONBLOCK);

	/* loop */
	do {
		if (GetRCCode() == 1)
		{

			if (
#if (RC_1 > 0)
				RCCode >= RC_1 && /* generates a warning... */
#endif
				RCCode <= RC_1+M_MaxDirect) /* direct access */
			{
				Menu_HighlightLine(menu, MenuLine[menuitem], 0);
				menuitem = RCCode-RC_1;
				Menu_HighlightLine(menu, MenuLine[menuitem], 1);

				if (menuitem != M_PID) /* just select */
					RCCode = RC_OK;
			}

			switch (RCCode)
			{
			case RC_UP:
				if (menuitem > 0 && !(auto_national && (menuitem-1 == M_NAT)))
				{
					Menu_HighlightLine(menu, MenuLine[menuitem], 0);
					menuitem--;
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
				}
				break;

			case RC_DOWN:
				if (menuitem < M_Number-1 && !(auto_national && (menuitem+1 == M_NAT)))
				{
					Menu_HighlightLine(menu, MenuLine[menuitem], 0);
					menuitem++;
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
				}
				break;

			case RC_LEFT:
				switch (menuitem)
				{
				case M_PID:
				{
					if (current_pid > 0)
					{
						current_pid--;

						memset(&menu[MenuLine[M_PID]*2*Menu_Width + 3], ' ', 24);

						if (SDT_ready)
						{
							memcpy(&menu[MenuLine[M_PID]*2*Menu_Width+3+(24-pid_table[current_pid].service_name_len)/2],
							       &pid_table[current_pid].service_name,
							       pid_table[current_pid].service_name_len);
						}
						else
							hex2str(&menu[MenuLine[M_PID]*2*Menu_Width + 13 + 3], vtxtpid);

						if (pids_found > 1)
						{
							if (current_pid == 0)
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = ' ';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = '�';
							}
							else
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = '�';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = '�';
							}
						}

						Menu_HighlightLine(menu, MenuLine[menuitem], 1);

						if (auto_national)
						{
							national_subset = GetNationalSubset(pid_table[current_pid].country_code);

							memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
							Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
						}
					}
					break;
				}

				case M_NAT:
					if (national_subset > 0)
					{
						national_subset--;

						if (national_subset == 0)
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = ' ';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = '�';
						}
						else
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = '�';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = '�';
						}

						memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
						Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					}
					break;

				case M_HOT: /* move towards top of hotlist */
				{
					if (hotindex <= 0) /* if not found, start at end */
						hotindex = maxhotlist;
					else
						hotindex--;
					Menu_UpdateHotlist(menu, hotindex, menuitem);
				}
				break;
				} /* switch menuitem */
				break; /* RC_LEFT */

			case RC_RIGHT:
				switch (menuitem)
				{
				case M_PID:
					if (current_pid < pids_found - 1)
					{
						current_pid++;

						memset(&menu[MenuLine[M_PID]*2*Menu_Width + 3], ' ', 24);

						if (SDT_ready)
							memcpy(&menu[MenuLine[M_PID]*2*Menu_Width + 3 +
											 (24-pid_table[current_pid].service_name_len)/2],
									 &pid_table[current_pid].service_name,
									 pid_table[current_pid].service_name_len);
						else
							hex2str(&menu[MenuLine[M_PID]*2*Menu_Width + 13 + 3], pid_table[current_pid].vtxt_pid);

						if (pids_found > 1)
						{
							if (current_pid == pids_found - 1)
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = '�';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = ' ';
							}
							else
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = '�';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = '�';
							}
						}

						Menu_HighlightLine(menu, MenuLine[menuitem], 1);

						if (auto_national)
						{
							national_subset = GetNationalSubset(pid_table[current_pid].country_code);
							memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
							Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
						}
					}
					break;

				case M_NAT:
					if (national_subset < 12)
					{
						national_subset++;

						if (national_subset == 12)
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = '�';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = ' ';
						}
						else
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = '�';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = '�';
						}

						memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
						Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					}
					break;

				case M_HOT: /* select hotindex */
				{
					if ((unsigned int)hotindex >= maxhotlist) /* if not found, start at 0 */
						hotindex = 0;
					else
						hotindex++;
					Menu_UpdateHotlist(menu, hotindex, menuitem);
				}
				break;
				}
				break; /* RC_RIGHT */

			case RC_PLUS:
				switch (menuitem)
				{
				case M_HOT: /* move towards end of hotlist */
				{
					if (hotindex<0) /* not found: add page at end */
					{
						if (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1)) /* only if still room left */
						{
							hotindex = ++maxhotlist;
							hotlist[hotindex] = page;
							savehotlist();
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
					else /* found */
					{
						if (hotindex < maxhotlist) /* not already at end */
						{
							int temp = hotlist[hotindex];
							hotlist[hotindex] = hotlist[hotindex+1];
							hotlist[hotindex+1] = temp;
							hotindex++;
							savehotlist();
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
				}
				break;
				}
				break; /* RC_PLUS */

			case RC_MINUS:
				switch (menuitem)
				{
				case M_HOT: /* move towards top of hotlist */
				{
					if (hotindex<0) /* not found: add page at top */
					{
						if (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1)) /* only if still room left */
						{
							for (hotindex=maxhotlist; hotindex>=0; hotindex--) /* move rest of list */
							{
								hotlist[hotindex+1] = hotlist[hotindex];
							}
							maxhotlist++;
							hotindex = 0;
							hotlist[hotindex] = page;
							savehotlist();
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
					else /* found */
					{
						if (hotindex > 0) /* not already at front */
						{
							int temp = hotlist[hotindex];
							hotlist[hotindex] = hotlist[hotindex-1];
							hotlist[hotindex-1] = temp;
							hotindex--;
							savehotlist();
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
				}
				break;
				}
				break; /* RC_MINUS */

			case RC_HELP:
				switch (menuitem)
				{
				case M_HOT: /* current page is added to / removed from hotlist */
				{
					if (hotindex<0) /* not found: add page */
					{
						if (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1)) /* only if still room left */
						{
							hotlist[++maxhotlist] = page;
							hotindex = maxhotlist;
							savehotlist();
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
					else /* found: remove */
					{
						if (maxhotlist > 0) /* don't empty completely */
						{
							int i;

							for (i=hotindex; i<maxhotlist; i++) /* move rest of list */
							{
								hotlist[i] = hotlist[i+1];
							}
							maxhotlist--;
							if (hotindex > maxhotlist)
								hotindex = maxhotlist;
							savehotlist();
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
				}
				break;
				}
				break; /* RC_MUTE */

			case RC_OK:
				switch (menuitem)
				{
				case M_PID:
					if (pids_found > 1)
					{
						if (Init)
							vtxtpid = pid_table[current_pid].vtxt_pid;
						else
						{
							unsigned char magazine;

							/* stop old decode-thread */
							if (pthread_cancel(thread_id) != 0)
								perror("TuxTxt <pthread_cancel>");

							if (pthread_join(thread_id, &thread_result) != 0)
								perror("TuxTxt <pthread_join>");

							/* stop demuxer */
							ioctl(dmx, DMX_STOP);

							/* reset data */
							memset(&subpagetable, 0xFF, sizeof(subpagetable));
							memset(&countrycontrolbitstable, 0xFF, sizeof(countrycontrolbitstable));
							memset(&backbuffer, black, sizeof(backbuffer));

							memset(&basictop, 0, sizeof(basictop));
							memset(&adip, 0, sizeof(adip));
							maxadippg = -1;
							bttok = 0;

							page_atrb[32] = transp<<4 | transp;
							inputcounter = 2;
							cached_pages = 0;

							for (magazine = 1; magazine < 9; magazine++)
							{
								current_page   [magazine] = -1;
								current_subpage [magazine] = -1;
							}
							page_receiving = -1;

							page     = 0x100;
							lastpage = 0x100;
							prev_100 = 0x100;
							prev_10  = 0x100;
							next_100 = 0x100;
							next_10  = 0x100;
							subpage  = 0;

							pageupdate = 0;
							zap_subpage_manual = 0;
							hintmode = 0;

							/* free pagebuffers */
							for (clear_page = 0; clear_page < 0x8FF; clear_page++)
							{
								for (clear_subpage = 0; clear_subpage < 0x79; clear_subpage++)
								{
									if (cachetable[clear_page][clear_subpage] != 0);
									{
										free(cachetable[clear_page][clear_subpage]);
										cachetable[clear_page][clear_subpage] = 0;
									}
								}
							}

							/* start demuxer with new vtxtpid */
							vtxtpid = pid_table[current_pid].vtxt_pid;
							strcpy(country_code, pid_table[current_pid].country_code);

							dmx_flt.pid      = vtxtpid;
							dmx_flt.input    = DMX_IN_FRONTEND;
							dmx_flt.output   = DMX_OUT_TAP;
							dmx_flt.pes_type = DMX_PES_OTHER;
							dmx_flt.flags    = DMX_IMMEDIATE_START;

							if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
								perror("TuxTxt <DMX_SET_PES_FILTER>");

							/* start new decode-thread */
							if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
								perror("TuxTxt <pthread_create>");

							pageupdate = 1;
							gethotlist();
						}

						/* show new teletext */
						current_service = current_pid;
						RenderMessage(ShowServiceName);

						fcntl(rc, F_SETFL, O_NONBLOCK);
						RCCode = 0;
						return;
					}
					break;

				case M_SC1:
					screen_mode1++;
					screen_mode1 &= 1;

					if (screen_mode1)
						memcpy(&menu[2*Menu_Width*MenuLine[M_SC1] + 26], "ein", 3);
					else
						memcpy(&menu[2*Menu_Width*MenuLine[M_SC1] + 26], "aus", 3);

					Menu_HighlightLine(menu, MenuLine[menuitem], 1);

					ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
					ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

					break;

				case M_SC2:
					screen_mode2++;
					screen_mode2 &= 1;

					if (screen_mode2)
						memcpy(&menu[2*Menu_Width*MenuLine[M_SC2] + 26], "ein", 3);
					else
						memcpy(&menu[2*Menu_Width*MenuLine[M_SC2] + 26], "aus", 3);

					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					break;

				case M_COL:
					color_mode++;
					color_mode &= 1;

					if (color_mode)
						memcpy(&menu[2*Menu_Width*MenuLine[M_COL] + 26], "ein", 3);
					else
						memcpy(&menu[2*Menu_Width*MenuLine[M_COL] + 26], "aus", 3);

					Menu_HighlightLine(menu, MenuLine[menuitem], 1);

					if (color_mode)
					{
						if (ioctl(fb, FBIOPUTCMAP, &colormap_2) == -1)
							perror("TuxTxt <FBIOPUTCMAP>");
					}
					else
					{
						if (ioctl(fb, FBIOPUTCMAP, &colormap_1) == -1)
							perror("TuxTxt <FBIOPUTCMAP>");
					}
					break;

				case M_AUN:
					auto_national++;
					auto_national &= 1;

					if (auto_national)
					{
						memcpy(&menu[2*Menu_Width*MenuLine[M_AUN] + 26], "ein", 3);
						menu[MenuLine[M_NAT]*2*Menu_Width +  1] = ' ';
						menu[MenuLine[M_NAT]*2*Menu_Width + 28] = ' ';
						national_subset = GetNationalSubset(pid_table[current_pid].country_code);
						memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
					}
					else
					{
						memcpy(&menu[2*Menu_Width*MenuLine[M_AUN] + 26], "aus", 3);
						if (national_subset != 0)
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = '�';
						if (national_subset != 12)
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = '�';
					}

					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
					break;
				case M_HOT: /* show selected page */
				{
					if (hotindex>=0) /* not found: ignore */
					{
						lastpage = page;
						page = hotlist[hotindex];
						subpage = subpagetable[page];
						inputcounter = 2;
						pageupdate = 1;
						RCCode = RC_HOME;		 /* leave menu */
					}
				}
				break;
				} /* RC_OK */
				break;
			}
		}
	} while ((RCCode != RC_HOME) && (RCCode != RC_DBOX) && (RCCode != RC_MUTE));

	/* reset to nonblocking mode */
	fcntl(rc, F_SETFL, O_NONBLOCK);
	pageupdate = 1;
	RCCode = 0;
	if (oldscreenmode)
		SwitchScreenMode(oldscreenmode); /* restore divided screen */
}

/******************************************************************************
 * PageInput                                                                  *
 ******************************************************************************/

void PageInput(int Number)
{
	static int temp_page;
	int zoom = 0;

	/* clear temp_page */
	if (inputcounter == 2)
		temp_page = 0;

	/* check for 0 & 9 on first position */
	if (Number == 0 && inputcounter == 2)
	{
		/* set page */
		temp_page = lastpage; /* 0 toggles to last page as in program switching */
		inputcounter = -1;
	}
	else if (Number == 9 && inputcounter == 2)
	{
		/* set page */
		temp_page = getIndexOfPageInHotlist(); /* 9 toggles through hotlist */

		if (temp_page<0 || temp_page==maxhotlist) /* from any (other) page go to first page in hotlist */
			temp_page = (maxhotlist >= 0) ? hotlist[0] : 0x100;
		else
			temp_page = hotlist[temp_page+1];

		inputcounter = -1;
	}

	/* show pageinput */
	if (zoommode == 2)
	{
		zoommode = 1;
		CopyBB2FB();
	}

	if (zoommode == 1)
		zoom = 1<<10;

	PosY = StartY;

	switch (inputcounter)
	{
	case 2:
		PosX = StartX + 8*type0.font.pix_width;
		RenderCharFB(Number | '0', black<<4 | white);
		RenderCharFB('-', black<<4 | white);
		RenderCharFB('-', black<<4 | white);
		break;

	case 1:
		PosX = StartX + 9*type0.font.pix_width;
		RenderCharFB(Number | '0', black<<4 | white);
		break;

	case 0:
		PosX = StartX + 10*type0.font.pix_width;
		RenderCharFB(Number | '0', black<<4 | white);
		break;
	}

	/* generate pagenumber */
	temp_page |= Number << inputcounter*4;

	inputcounter--;

	if (inputcounter < 0)
	{
		/* disable subpage zapping */
		zap_subpage_manual = 0;

		/* reset input */
		inputcounter = 2;

		/* set new page */
		lastpage = page;

		page = temp_page;

		/* check cache */
		if (subpagetable[page] != 0xFF)
		{
			subpage = subpagetable[page];
			pageupdate = 1;
#if DEBUG
			printf("TuxTxt <DirectInput: %.3X-%.2X>\n", page, subpage);
#endif
		}
		else
		{
			subpage = 0;
			RenderMessage(PageNotFound);
#if DEBUG
			printf("TuxTxt <DirectInput: %.3X not found>\n", page);
#endif
		}
	}
}

/******************************************************************************
 * GetNextPageOne                                                             *
 ******************************************************************************/

void GetNextPageOne()
{
	/* disable subpage zapping */
	zap_subpage_manual = 0;

	/* abort pageinput */
	inputcounter = 2;

	/* find next cached page */
	lastpage = page;

	do {
		next_dec(&page);
	} while (subpagetable[page] == 0xFF && page != lastpage);

	/* update page */
	if (page != lastpage)
	{
		if (zoommode == 2)
			zoommode = 1;

		subpage = subpagetable[page];
		pageupdate = 1;
#if DEBUG
		printf("TuxTxt <NextPageOne: %.3X-%.2X>\n", page, subpage);
#endif
	}
}

/******************************************************************************
 * GetPrevPageOne                                                             *
 ******************************************************************************/

void GetPrevPageOne()
{
	/* disable subpage zapping */
	zap_subpage_manual = 0;

	 /* abort pageinput */
	inputcounter = 2;

	/* find previous cached page */
	lastpage = page;

	do {
		prev_dec(&page);
	} while (subpagetable[page] == 0xFF && page != lastpage);

	/* update page */
	if (page != lastpage)
	{
		if (zoommode == 2)
			zoommode = 1;

		subpage = subpagetable[page];
		pageupdate = 1;
#if DEBUG
		printf("TuxTxt <PrevPageOne: %.3X-%.2X>\n", page, subpage);
#endif
	}
}

/******************************************************************************
 * GetNextSubPage                                                             *
 ******************************************************************************/
void GetNextSubPage()
{
	int loop;

	/* abort pageinput */
	inputcounter = 2;

	/* search subpage */
	if (subpage != 0)
	{
		/* search next subpage */
		for (loop = subpage + 1; loop <= 0x79; loop++)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

		for (loop = 1; loop < subpage; loop++)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

#if DEBUG
		printf("TuxTxt <NextSubPage: no other SubPage>\n");
#endif
	}
	else
	{
#if DEBUG
		printf("TuxTxt <NextSubPage: no SubPages>\n");
#endif
	}
}

/******************************************************************************
 * GetPrevSubPage                                                             *
 ******************************************************************************/

void GetPrevSubPage()
{
	int loop;

	/* abort pageinput */
	inputcounter = 2;

	/* search subpage */
	if (subpage != 0)
	{
		/* search previous subpage */
		for (loop = subpage - 1; loop > 0x00; loop--)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <PrevSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

		for (loop = 0x79; loop > subpage; loop--)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <PrevSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

#if DEBUG
		printf("TuxTxt <PrevSubPage: no other SubPage>\n");
#endif
	}
	else
	{
#if DEBUG
		printf("TuxTxt <PrevSubPage: no SubPages>\n");
#endif
	}
}

/******************************************************************************
 * Prev100                                                                    *
 ******************************************************************************/

void Prev100()
{
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = prev_100;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Prev100: %.3X>\n", page);
#endif
}

/******************************************************************************
 * Prev10                                                                     *
 ******************************************************************************/

void Prev10()
{
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = prev_10;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Prev10: %.3X>\n", page);
#endif
}

/******************************************************************************
 * Next10                                                                    *
 ******************************************************************************/

void Next10()
{
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = next_10;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Next10: %.3X>\n", page);
#endif
}

/******************************************************************************
 * Next100                                                                    *
 ******************************************************************************/

void Next100()
{
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = next_100;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Next100: %.3X>\n", page);
#endif
}

/******************************************************************************
 * PageCatching                                                               *
 ******************************************************************************/

void PageCatching()
{
	int val;

	/* abort pageinput */
	inputcounter = 2;

	/* show info line */
	pagecatching = 1;
	CopyBB2FB();

	/* check for pagenumber(s) */
	CatchNextPage(1);

	if (!catched_page)
	{
		pagecatching = 0;
		CopyBB2FB();
		return;
	}

	/* set blocking mode */
	val = fcntl(rc, F_GETFL);
	fcntl(rc, F_SETFL, val &~ O_NONBLOCK);

	/* loop */
	do {
		GetRCCode();

		switch (RCCode)
		{
		case RC_UP:
			CatchPrevPage();
			break;

		case RC_DOWN:
			CatchNextPage(0);
			break;

		case RC_HOME:
			fcntl(rc, F_SETFL, O_NONBLOCK);
			pageupdate = 1;
			pagecatching = 0;
			RCCode = 0;
			return;
		}
	} while (RCCode != RC_OK);

	/* set new page */
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = catched_page;
	pageupdate   = 1;
	pagecatching = 0;

	if (subpagetable[page] != 0xFF)
		subpage = subpagetable[page];
	else
		subpage = 0;

	/* reset to nonblocking mode */
	fcntl(rc, F_SETFL, O_NONBLOCK);
}

/******************************************************************************
 * CatchNextPage                                                              *
 ******************************************************************************/

void CatchNextPage(int Init)
{
	int tmp_page, pages_found=0;

	/* init */
	if (Init)
	{
		catch_row    = 1;
		catch_col    = 0;
		catched_page = 0;
	}

	/* catch next page */
	for (; catch_row < 24; catch_row++)
	{
		for (; catch_col < 40; catch_col++)
		{
			if (!(page_atrb[catch_row*40 + catch_col] & 1<<8) && (page_char[catch_row*40 + catch_col] >= '0' &&
			    page_char[catch_row*40 + catch_col] <= '9' && page_char[catch_row*40 + catch_col + 1] >= '0' &&
			    page_char[catch_row*40 + catch_col + 1] <= '9' && page_char[catch_row*40 + catch_col + 2] >= '0' &&
			    page_char[catch_row*40 + catch_col + 2] <= '9') && (page_char[catch_row*40 + catch_col - 1] < '0' ||
			    page_char[catch_row*40 + catch_col - 1] > '9') && (page_char[catch_row*40 + catch_col + 3] < '0' ||
			    page_char[catch_row*40 + catch_col + 3] > '9'))
			{
				tmp_page = ((page_char[catch_row*40 + catch_col] - '0')<<8) | ((page_char[catch_row*40 + catch_col + 1] - '0')<<4) | (page_char[catch_row*40 + catch_col + 2] - '0');

				if (tmp_page != catched_page && tmp_page >= 0x100 && tmp_page <= 0x899)
				{
					catched_page = tmp_page;
					pages_found++;

					RenderCatchedPage();
					catch_col += 3;

					printf("TuxTxt <PageCatching: %.3X\n", catched_page);

					return;
				}
			}
		}

		catch_col = 0;
	}

	if (Init)
	{
		printf("TuxTxt <PageCatching: no PageNumber>\n");
		return;
	}

	/* wrap around */
	catch_row = 1;
	catch_col = 0;

	if (!pages_found)
		return;

	pages_found = 0;
	CatchNextPage(0);
}

/******************************************************************************
 * CatchPrevPage                                                              *
 ******************************************************************************/

void CatchPrevPage()
{
	int tmp_page, pages_found=0;

	/* catch prev page */
	for (; catch_row > 0; catch_row--)
	{
		for (; catch_col > 0; catch_col--)
		{
			if (!(page_atrb[catch_row*40 + catch_col] & 1<<8) && (page_char[catch_row*40 + catch_col] >= '0' &&
			    page_char[catch_row*40 + catch_col] <= '9' && page_char[catch_row*40 + catch_col + 1] >= '0' &&
			    page_char[catch_row*40 + catch_col + 1] <= '9' && page_char[catch_row*40 + catch_col + 2] >= '0' &&
			    page_char[catch_row*40 + catch_col + 2] <= '9') && (page_char[catch_row*40 + catch_col - 1] < '0' ||
			    page_char[catch_row*40 + catch_col - 1] > '9') && (page_char[catch_row*40 + catch_col + 3] < '0' ||
			    page_char[catch_row*40 + catch_col + 3] > '9'))
			{
				tmp_page = ((page_char[catch_row*40 + catch_col] - '0')<<8) | ((page_char[catch_row*40 + catch_col + 1] - '0')<<4) | (page_char[catch_row*40 + catch_col + 2] - '0');

				if (tmp_page != catched_page && tmp_page >= 0x100 && tmp_page <= 0x899)
				{
					catched_page = tmp_page;
					pages_found++;

					RenderCatchedPage();
					catch_col -= 3;

					printf("TuxTxt <PageCatching: %.3X\n", catched_page);

					return;
				}
			}
		}
		catch_col = 39;
	}

	/* wrap around */
	catch_row = 23;
	catch_col = 39;

	if (!pages_found)
		return;

	pages_found = 0;
	CatchPrevPage();
}

/******************************************************************************
 * RenderCatchedPage                                                          *
 ******************************************************************************/

void RenderCatchedPage()
{
	int zoom = 0;

	/* handle zoom */
	if (zoommode)
		zoom = 1<<10;

	/* restore pagenumber */
	PosX = StartX + pc_old_col*type0.font.pix_width;

	if (zoommode == 2)
		PosY = StartY + (pc_old_row-12)*fixfontheight*((zoom>>10)+1);
	else
		PosY = StartY + pc_old_row*fixfontheight*((zoom>>10)+1);

	if (pc_old_row && pc_old_col) /* not at first call */
	{
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col    ], page_atrb[pc_old_row*40 + pc_old_col    ]);
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col + 1], page_atrb[pc_old_row*40 + pc_old_col + 1]);
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col + 2], page_atrb[pc_old_row*40 + pc_old_col + 2]);
	}

	pc_old_row = catch_row;
	pc_old_col = catch_col;

	/* mark pagenumber */
	if (zoommode == 1 && catch_row > 11)
	{
		zoommode = 2;
		CopyBB2FB();
	}
	else if (zoommode == 2 && catch_row < 12)
	{
		zoommode = 1;
		CopyBB2FB();
	}

	PosX = StartX + catch_col*type0.font.pix_width;

	if (zoommode == 2)
		PosY = StartY + (catch_row-12)*fixfontheight*((zoom>>10)+1);
	else
		PosY = StartY + catch_row*fixfontheight*((zoom>>10)+1);

	RenderCharFB(page_char[catch_row*40 + catch_col    ], (page_atrb[catch_row*40 + catch_col    ] & 1<<10) | ((page_atrb[catch_row*40 + catch_col    ] & 0x0F)<<4) | ((page_atrb[catch_row*40 + catch_col    ] & 0xF0)>>4));
	RenderCharFB(page_char[catch_row*40 + catch_col + 1], (page_atrb[catch_row*40 + catch_col + 1] & 1<<10) | ((page_atrb[catch_row*40 + catch_col + 1] & 0x0F)<<4) | ((page_atrb[catch_row*40 + catch_col + 1] & 0xF0)>>4));
	RenderCharFB(page_char[catch_row*40 + catch_col + 2], (page_atrb[catch_row*40 + catch_col + 2] & 1<<10) | ((page_atrb[catch_row*40 + catch_col + 2] & 0x0F)<<4) | ((page_atrb[catch_row*40 + catch_col + 2] & 0xF0)>>4));
}

/******************************************************************************
 * SwitchZoomMode                                                             *
 ******************************************************************************/

void SwitchZoomMode()
{
	if (subpagetable[page] != 0xFF)
	{
		/* toggle mode */
		zoommode++;

		if (zoommode == 3)
			zoommode = 0;

		printf("TuxTxt <SwitchZoomMode: %d>\n", zoommode);

		/* update page */
		CopyBB2FB();
	}
}

/******************************************************************************
 * SwitchScreenMode                                                           *
 ******************************************************************************/

void SwitchScreenMode(int newscreenmode)
{
#if HAVE_DVB_API_VERSION >= 3
	struct v4l2_format format;
#endif
	/* reset transparency mode */
	if (transpmode)
		transpmode = 0;

	if (newscreenmode < 0) /* toggle mode */
		screenmode++;
	else /* set directly */
		screenmode = newscreenmode;
	if ((screenmode > (screen_mode2 ? 2 : 1)) || (screenmode < 0))
		screenmode = 0;

	printf("TuxTxt <SwitchScreenMode: %d>\n", screenmode);

	/* update page */
	pageupdate = 1;

	/* clear backbuffer */
#ifndef DREAMBOX
	memset(&backbuffer, black, sizeof(backbuffer));
#else
	memset(&backbuffer, screenmode?transp:black, sizeof(backbuffer));
#endif

	/* set mode */
	if (screenmode)								 /* split */
	{
		int fw, fh, tx, ty, tw, th;
		int sm = 0;

		if (screenmode==1) /* split with topmenu */
		{
			if (screen_mode2)	/* 16:9 */
			{
				fw = FONTWIDTH_SMALL;
				fh = FONTHEIGHT_NORMAL;
				tx = TV169STARTX;
				ty = TV169STARTY;
				tw = TV169WIDTH;
				th = TV169HEIGHT;
			}
			else /* 4:3 */
			{
				fw = FONTWIDTH_TOPMENUMAIN;
				fh = FONTHEIGHT_NORMAL;
				tx = TV43STARTX;
				ty = TV43STARTY;
				tw = TV43WIDTH;
				th = TV43HEIGHT;
			}
		}
		else /* 2: split with full height tv picture */
		{
			fw = FONTWIDTH_SMALL;
			fh = FONTHEIGHT_NORMAL;
			tx = StartX+322;
			ty = StartY;
			tw = 320;
			th = 526;
		}
		
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = fw;
		type0.font.pix_height = type1.font.pix_height = type2.font.pix_height = fh;

#if HAVE_DVB_API_VERSION < 3
		avia_pig_hide(pig);
		avia_pig_set_pos(pig, tx, ty);
		avia_pig_set_size(pig, tw, th);
		avia_pig_set_stack(pig, 2);
		avia_pig_show(pig);
#else
		ioctl(pig, VIDIOC_OVERLAY, &sm);
		sm = 1;
		ioctl(pig, VIDIOC_G_FMT, &format);
		format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		format.fmt.win.w.left   = tx;
		format.fmt.win.w.top    = ty;
		format.fmt.win.w.width  = tw;
		format.fmt.win.w.height = th;
		ioctl(pig, VIDIOC_S_FMT, &format);
		ioctl(pig, VIDIOC_OVERLAY, &sm);
#endif
		ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode2]);
		ioctl(saa, SAAIOSWSS, &saamodes[screen_mode2]);
	}
	else /* not split */
	{
#if HAVE_DVB_API_VERSION < 3
		avia_pig_hide(pig);
#else
		ioctl(pig, VIDIOC_OVERLAY, &screenmode);
#endif
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = FONTWIDTH_NORMAL;
		type0.font.pix_height = type1.font.pix_height = type2.font.pix_height = FONTHEIGHT_NORMAL+1;

		ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
		ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);
	}
}

/******************************************************************************
 * SwitchTranspMode                                                           *
 ******************************************************************************/

void SwitchTranspMode()
{
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */


	/* toggle mode */
	if (!transpmode)
		transpmode = 2;
	else
		transpmode--; /* backward to immediately switch to TV-screen */

	printf("TuxTxt <SwitchTranspMode: %d>\n", transpmode);

	/* set mode */
	if (!transpmode)
	{
		memset(&backbuffer, black, sizeof(backbuffer));
		pageupdate = 1;
	}
	else if (transpmode == 1)
	{
		memset(&backbuffer, transp2, sizeof(backbuffer));
		pageupdate = 1;
	}
	else
	{
		memset(lfb, transp, var_screeninfo.xres * var_screeninfo.yres);
	}
}

/******************************************************************************
 * SwitchHintMode                                                             *
 ******************************************************************************/

void SwitchHintMode()
{
	/* toggle mode */
	hintmode ^= 1;

	printf("TuxTxt <SwitchHintMode: %d>\n", hintmode);

	/* update page */
	pageupdate = 1;
}

/******************************************************************************
 * RenderCharFB                                                               *
 ******************************************************************************/

void RenderCharFB(int Char, int Attribute)
{
	int Row, Pitch, Bit, x = 0, y = 0;
	int error;

#if (FONTWIDTH_TOPMENUMAIN==FONTWIDTH_SMALL)
#define pt0 &type0
#define pt1 &type1
#define pt2 &type2
#else
	FONTTYPE *pt0, *pt1, *pt2;
	if (type0.font.pix_width == FONTWIDTH_TOPMENUMAIN)
	{
		/* pointer to current main font type (8/16pt or 12pt for split
		 * screen with topmenu) */
		pt0 = &type0r;
		pt1 = &type1r;
		pt2 = &type2r;
	}
	else
	{
		pt0 = &type0;
		pt1 = &type1;
		pt2 = &type2;
	}
#endif

	/* skip invalid chars */
	if (Char == 0xFF)
	{
#if DEBUG
		printf("TuxTxt <fb invalid char c%x a%x w%d h%d x%d y%d>\n", Char, Attribute, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
		Char = '?';
		Attribute = black<<4 | white;
#else
		PosX += (*pt0).font.pix_width;
		return;
#endif
	}

	/* load char */
	if ((Attribute>>8) & 3)
	{
		/* G1 */
		if (Char == 0x40 || (Char >= 0x5B && Char <= 0x5F))
		{
			switch (Char)
			{
			case 0x40:	Char = 0x02;	break;
			case 0x5B:	Char = 0x03;	break;
			case 0x5C:	Char = 0x04;	break;
			case 0x5D:	Char = 0x05;	break;
			case 0x5E:	Char = 0x06;	break;
			case 0x5F:	Char = 0x07;	break;
			}

			if ((error = FTC_SBitCache_Lookup(cache, &(*pt2), Char + national_subset*13 + 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup fb(G0): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt2).font.pix_width;
				return;
			}
		}
		else
		{
			if ((error = FTC_SBitCache_Lookup(cache, &(*pt1), Char + ((((Attribute>>8) & 3) - 1) * 96) + 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup fb(G1): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt1).font.pix_width;
				return;
			}
		}
	}
	else
	{
		/* G0 */
		if ((Char >= 0x23 && Char <= 0x24) || Char == 0x40 ||
		    (Char >= 0x5B && Char <= 0x60) || (Char >= 0x7B && Char <= 0x7E))
		{
			switch (Char)
			{
			case 0x23:	Char = 0x00;	break;
			case 0x24:	Char = 0x01;	break;
			case 0x40:	Char = 0x02;	break;
			case 0x5B:	Char = 0x03;	break;
			case 0x5C:	Char = 0x04;	break;
			case 0x5D:	Char = 0x05;	break;
			case 0x5E:	Char = 0x06;	break;
			case 0x5F:	Char = 0x07;	break;
			case 0x60:	Char = 0x08;	break;
			case 0x7B:	Char = 0x09;	break;
			case 0x7C:	Char = 0x0A;	break;
			case 0x7D:	Char = 0x0B;	break;
			case 0x7E:	Char = 0x0C;	break;
			}

			if ((error = FTC_SBitCache_Lookup(cache, &(*pt2), Char + national_subset*13+ 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup fb(NS): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt2).font.pix_width;
				return;
			}
		}
		else
		{
			if ((error = FTC_SBitCache_Lookup(cache, &(*pt0), Char + 1, &sbit, NULL))!=0)
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup fb(G0): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt0).font.pix_width;
				return;
			}
		}
	}

	/* render char */
	for (Row = 0; Row < fixfontheight; Row++)
	{
		int pixtodo = sbit->width;

		for (Pitch = 0; Pitch < sbit->pitch; Pitch++)
		{
			for (Bit = 7; Bit >= 0; Bit--)
			{
				if (--pixtodo < 0)
					break;

				if ((sbit->buffer[Row * sbit->pitch + Pitch]) & 1<<Bit)
				{
					*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = Attribute & 15;

					if (zoommode && (Attribute & 1<<10))
					{
						*(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute & 15;
						*(lfb + (x+PosX) + ((y+PosY+2)*var_screeninfo.xres)) = Attribute & 15;
						*(lfb + (x+PosX) + ((y+PosY+3)*var_screeninfo.xres)) = Attribute & 15;
					}
					else if (zoommode || (Attribute & 1<<10)) *(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute & 15;
				}
				else
				{
					if (transpmode == 1)
					{
						Attribute &= 0xFF0F;
						Attribute |= transp2<<4;
					}

					*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = Attribute>>4 & 15;

					if (zoommode && (Attribute & 1<<10))
					{
						*(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute>>4 & 15;
						*(lfb + (x+PosX) + ((y+PosY+2)*var_screeninfo.xres)) = Attribute>>4 & 15;
						*(lfb + (x+PosX) + ((y+PosY+3)*var_screeninfo.xres)) = Attribute>>4 & 15;
					}
					else if (zoommode || (Attribute & 1<<10)) *(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute>>4 & 15;
				}

				x++;
			}
		}

		x = 0;
		y++;

		if (zoommode && (Attribute & 1<<10))
			y += 3;
		else if (zoommode || (Attribute & 1<<10))
			y++;
	}

	PosX += sbit->width;
}

/******************************************************************************
 * RenderCharBB                                                               *
 ******************************************************************************/

void RenderCharBB(int Char, int Attribute)
{
	int Row, Pitch, Bit, x = 0, y = 0;
	int error;

#if (FONTWIDTH_TOPMENUMAIN==FONTWIDTH_SMALL)
#define pt0 &type0
#define pt1 &type1
#define pt2 &type2
#else
	FONTTYPE *pt0, *pt1, *pt2;
	if (type0.font.pix_width == FONTWIDTH_TOPMENUMAIN)
	{
		/* pointer to current main font type (8/16pt or 12pt for split
		 * screen with topmenu) */
		pt0 = &type0r;
		pt1 = &type1r;
		pt2 = &type2r;
	}
	else
	{
		pt0 = &type0;
		pt1 = &type1;
		pt2 = &type2;
	}
#endif

	/* skip doubleheight chars in lower line */
	if (Char == 0xFF)
	{
		PosX += (*pt0).font.pix_width;
		return;
	}

	/* load char */
	if ((Attribute>>8) & 3)
	{
		/* G1 */
		if (Char == 0x40 || (Char >= 0x5B && Char <= 0x5F))
		{
			switch (Char)
			{
			case 0x40:	Char = 0x02;	break;
			case 0x5B:	Char = 0x03;	break;
			case 0x5C:	Char = 0x04;	break;
			case 0x5D:	Char = 0x05;	break;
			case 0x5E:	Char = 0x06;	break;
			case 0x5F:	Char = 0x07;	break;
			}

			if ((error = FTC_SBitCache_Lookup(cache, &(*pt2), Char + national_subset*13 + 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup bb(NS): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt2).font.pix_width;
				return;
			}
		}
		else
		{
			if ((error = FTC_SBitCache_Lookup(cache, &(*pt1), Char + ((((Attribute>>8) & 3) - 1) * 96) + 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup bb(G1): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt1).font.pix_width;
				return;
			}
		}
	}
	else
	{
		/* G0 */
		if ((Char >= 0x23 && Char <= 0x24) || Char == 0x40 || (Char >= 0x5B && Char <= 0x60) || (Char >= 0x7B && Char <= 0x7E))
		{
			switch (Char)
			{
			case 0x23:	Char = 0x00;	break;
			case 0x24:	Char = 0x01;	break;
			case 0x40:	Char = 0x02;	break;
			case 0x5B:	Char = 0x03;	break;
			case 0x5C:	Char = 0x04;	break;
			case 0x5D:	Char = 0x05;	break;
			case 0x5E:	Char = 0x06;	break;
			case 0x5F:	Char = 0x07;	break;
			case 0x60:	Char = 0x08;	break;
			case 0x7B:	Char = 0x09;	break;
			case 0x7C:	Char = 0x0A;	break;
			case 0x7D:	Char = 0x0B;	break;
			case 0x7E:	Char = 0x0C;	break;
			}

			if ((error = FTC_SBitCache_Lookup(cache, &(*pt2), Char + national_subset*13 + 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup bb(NS): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt2).font.pix_width;
				return;
			}
		}
		else
		{
			if ((error = FTC_SBitCache_Lookup(cache, &(*pt0), Char + 1, &sbit, NULL)))
			{
#if DEBUG
				printf("TuxTxt <FTC_SBitCache_Lookup bb(G0): 0x%x> c%x w%d h%d x%d y%d\n", error, Char, (*pt0).font.pix_width, (*pt0).font.pix_height, PosX, PosY);
#endif
				PosX += (*pt0).font.pix_width;
				return;
			}
		}
	}

	/* render char */
	for (Row = 0; Row < fixfontheight; Row++)
	{
		int pixtodo = sbit->width;

		for (Pitch = 0; Pitch < sbit->pitch; Pitch++)
		{
			for (Bit = 7; Bit >= 0; Bit--)
			{
				if (--pixtodo < 0)
					break;

				if ((sbit->buffer[Row * sbit->pitch + Pitch]) & 1<<Bit)
				{
					backbuffer[(x+PosX) + ((y+PosY)*var_screeninfo.xres)] = Attribute & 15;

					if (Attribute & 1<<10)
						backbuffer[(x+PosX) + ((y+PosY+1)*var_screeninfo.xres)] = Attribute & 15;
				}
				else
				{
					if (transpmode == 1 && PosY < StartY + 24*fixfontheight)
					{
						Attribute &= 0xFF0F;
						Attribute |= transp2<<4;
					}

					backbuffer[(x+PosX) + ((y+PosY)*var_screeninfo.xres)] = Attribute>>4 & 15;

					if (Attribute & 1<<10)
						backbuffer[(x+PosX) + ((y+PosY+1)*var_screeninfo.xres)] = Attribute>>4 & 15;
				}
				x++;
			}
		}
		x = 0;
		y++;

		if (Attribute & 1<<10)
			y++;
	}

	PosX += sbit->width;
}

/******************************************************************************
 * RenderCharLCD                                                             *
 ******************************************************************************/

void RenderCharLCD(int Digit, int XPos, int YPos)
{
	int x, y;

	/* render digit to lcd backbuffer */
	for (y = 0; y < 15; y++)
	{
		for (x = 0; x < 10; x++)
		{
			if (lcd_digits[Digit*15*10 + x + y*10])
				lcd_backbuffer[XPos + x + ((YPos+y)/8)*120] |= 1 << ((YPos+y)%8);
			else
				lcd_backbuffer[XPos + x + ((YPos+y)/8)*120] &= ~(1 << ((YPos+y)%8));
		}
	}
}

/******************************************************************************
 * RenderMessage                                                              *
 ******************************************************************************/

void RenderMessage(int Message)
{
	int byte;
	int fbcolor, timecolor, menucolor;
	int pagecolumn;

	char message_1[] = "��������������������������������������";
	char message_2[] = "�                                   ��";
	char message_3[] = "�   suche nach Teletext-Anbietern   ��";
	char message_4[] = "�                                   ��";
	char message_5[] = "��������������������������������������";
	char message_6[] = "��������������������������������������";

	char message_7[] = "� kein Teletext auf dem Transponder ��";
	char message_8[] = "�  warte auf Empfang von Seite 100  ��";
	char message_9[] = "�     Seite 100 existiert nicht!    ��";

	/* reset zoom */
	zoommode = 0;

	/* set colors */
#ifndef DREAMBOX
	if (screenmode)
	{
		fbcolor   = black;
		timecolor = black<<4 | black;
		menucolor = menu1;
	}
	else
#endif
	{
		fbcolor   = transp;
		timecolor = transp<<4 | transp;
		menucolor = menu3;
	}

	/* clear framebuffer */
	memset(lfb, fbcolor, var_screeninfo.xres * var_screeninfo.yres);

	/* hide timestring */
	page_atrb[32] = timecolor;

	/* set pagenumber */
	if (Message == PageNotFound || Message == ShowServiceName)
	{
		char *msg;

		if (bttok && !basictop[page]) /* page non-existent according to TOP (continue search anyway) */
		{
			pagecolumn = 12;
			msg = message_9;
		}
		else
		{
			pagecolumn = 31;
			msg = message_8;
		}

		memset(&message_3[1], ' ', 35);
		hex2str(msg+pagecolumn+2, page);
		memcpy(&message_4, msg, sizeof(message_8));

		if (SDT_ready)
			memcpy(&message_2[2 + (35 - pid_table[current_service].service_name_len)/2], &pid_table[current_service].service_name, pid_table[current_service].service_name_len);
		else if (Message == ShowServiceName)
			hex2str(&message_2[17+3], vtxtpid);
	}
	else if (Message == NoServicesFound)
		memcpy(&message_3, &message_7, sizeof(message_7));

	/* render infobar */
	PosX = StartX + type0.font.pix_width+5;
	PosY = StartY + fixfontheight*16;
	for (byte = 0; byte < 37; byte++)
		RenderCharFB(message_1[byte], menucolor<<4 | menu2);
	RenderCharFB(message_1[37], fbcolor<<4 | menu2);

	PosX = StartX + type0.font.pix_width+5;
	PosY = StartY + fixfontheight*17;
	RenderCharFB(message_2[0], menucolor<<4 | menu2);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_2[byte], menucolor<<4 | white);
	RenderCharFB(message_2[36], menucolor<<4 | menu2);
	RenderCharFB(message_2[37], fbcolor<<4 | menu2);

	PosX = StartX + type0.font.pix_width+5;
	PosY = StartY + fixfontheight*18;
	RenderCharFB(message_3[0], menucolor<<4 | menu2);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_3[byte], menucolor<<4 | white);
	RenderCharFB(message_3[36], menucolor<<4 | menu2);
	RenderCharFB(message_3[37], fbcolor<<4 | menu2);

	PosX = StartX + type0.font.pix_width+5;
	PosY = StartY + fixfontheight*19;
	RenderCharFB(message_4[0], menucolor<<4 | menu2);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_4[byte], menucolor<<4 | white);
	RenderCharFB(message_4[36], menucolor<<4 | menu2);
	RenderCharFB(message_4[37], fbcolor<<4 | menu2);

	PosX = StartX + type0.font.pix_width+5;
	PosY = StartY + fixfontheight*20;
	for (byte = 0; byte < 37; byte++)
		RenderCharFB(message_5[byte], menucolor<<4 | menu2);
	RenderCharFB(message_5[37], fbcolor<<4 | menu2);

	PosX = StartX + type0.font.pix_width+5;
	PosY = StartY + fixfontheight*21;
	for (byte = 0; byte < 38; byte++)
		RenderCharFB(message_6[byte], fbcolor<<4 | menu2);
}

/******************************************************************************
 * RenderPage                                                                 *
 ******************************************************************************/

void RenderPage()
{
	int row, col, byte;

	/* update lcd */
	UpdateLCD();

	/* update page or timestring */
	if (transpmode != 2 && pageupdate && page_receiving != page && inputcounter == 2)
	{
		/* get national subset */
		if (auto_national)
			national_subset = GetNationalSubset(country_code);

		/* reset update flag */
		pageupdate = 0;

		/* decode page */
		if (subpagetable[page] != 0xFF)
			DecodePage();
		else
		{
			RenderMessage(PageNotFound);
			return;
		}

		/* render page */
		PosY = StartY;

		for (row = 0; row < 24; row++)
		{
			PosX = StartX;

			for (col = 0; col < 40; col++)
				RenderCharBB(page_char[row*40 + col], page_atrb[row*40 + col]);

			PosY += fixfontheight;
		}

		/* update framebuffer */
		CopyBB2FB();
	}
	else if (transpmode != 2 && zoommode != 2)
	{
		/* update timestring */
		PosX = StartX + 32*type0.font.pix_width;
		PosY = StartY;

		for (byte = 0; byte < 8; byte++)
			RenderCharFB(timestring[byte], page_atrb[32]);
	}
}

/******************************************************************************
 * CreateLine25                                                               *
 ******************************************************************************/

void CreateLine25()
{
	int byte;

	char line25_1[] = "   ?00<      ??0<      >??0      >?00   ((((((((((1111111111AAAAAAAAAAXXXXXXXXXX";
#ifndef DREAMBOX
	char line25_2[] = " �� w{hlen   �� anzeigen   �� abbrechen ����������������������������������������";
#else
	char line25_2[] = " �� w{hlen   �� anzeigen   �� abbrechen ����������������������������������������";
#endif

	if (!bttok && cachetable[0x1f0][0] && cachetable[0x1f0][0][40+799]) /* btt received and not yet decoded */
		decode_btt();
	if (maxadippg >= 0)
		decode_adip();

/*  1: blk-1, grp-1, grp+1, blk+1 */
/*  2: blk-1, grp+1, grp+2, blk+1 */
#if (LINE25MODE == 1)
	prev_10  = toptext_getnext(page, 0, 1); /* arguments: startpage, up, findgroup */
	prev_100 = toptext_getnext(prev_10, 0, 0);
	next_10  = toptext_getnext(page, 1, 1);
#else
	prev_100 = toptext_getnext(page, 0, 0);
	prev_10  = toptext_getnext(page, 1, 1);
	next_10  = toptext_getnext(prev_10, 1, 1);
#endif
	next_100 = toptext_getnext(next_10, 1, 0);

	/* FIXME: flexible widths? */
	if (adip[prev_100][0])
		memcpy(&line25_1[0], &adip[prev_100][0], 10);
	else
		hex2str(&line25_1[3+2], prev_100);

	if (adip[prev_10][0])
		memcpy(&line25_1[10], &adip[prev_10][0], 10);
	else
		hex2str(&line25_1[13+2], prev_10);

	if (adip[next_10][0])
		memcpy(&line25_1[20], &adip[next_10][0], 10);
	else
		hex2str(&line25_1[24+2], next_10);

	if (adip[next_100][0])
		memcpy(&line25_1[30], &adip[next_100][0], 10);
	else
		hex2str(&line25_1[34+2], next_100);

	if (bttok && screenmode == 1) /* TOP-Info present, divided screen -> create TOP overview */
	{
#if (TOPMENULINEWIDTH < (TOPMENUINDENTDEF+12+TOPMENUSPC+3+1))
		char line[TOPMENUINDENTDEF+12+TOPMENUSPC+3+1];
#else
		char line[TOPMENULINEWIDTH];
#endif
		int current;
		int prev10done, next10done, next100done, indent;
		int attrcol; /* color attribute for navigation keys */
		int attr;
		int topmenuendy = screen_mode2 ? TOPMENU169ENDY : TOPMENU43ENDY;

#if (FONTWIDTH_TOPMENUMAIN != FONTWIDTH_TOPMENU)
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = FONTWIDTH_TOPMENU;
#endif

		PosY = TOPMENUSTARTY;
		memset(line, ' ', TOPMENULINEWIDTH); /* init with spaces */

		memcpy(line+TOPMENUINDENTBLK, adip[prev_100], 12);
		hex2str(&line[TOPMENUINDENTDEF+12+TOPMENUSPC+2], prev_100);
		RenderClearMenuLineBB(line, '(', black<<4 | yellow);

/*  1: blk-1, grp-1, grp+1, blk+1 */
/*  2: blk-1, grp+1, grp+2, blk+1 */
#if (LINE25MODE == 1)
		current = prev_10 - 1;
#else
		current = page - 1;
#endif

		prev10done = next10done = next100done = 0;
		while (PosY <= (topmenuendy-fixfontheight))
		{
			attr = 0;
			attrcol = black<<4 | white;
			if (!next100done && (PosY > (topmenuendy - 2*fixfontheight))) /* last line */
			{
				attrcol = 'X';
				current = next_100;
			}
			else if (!next10done && (PosY > (topmenuendy - 3*fixfontheight))) /* line before */
			{
				attrcol = 'A';
				current = next_10;
			}
			else if (!prev10done && (PosY > (topmenuendy - 4*fixfontheight))) /* line before */
			{
				attrcol = '1';
				current = prev_10;
			}
			else do
			{
				next_dec(&current);
				if (current == prev_10)
				{
					attrcol = '1';
					prev10done = 1;
					break;
				}
				else if (current == next_10)
				{
					attrcol = 'A';
					next10done = 1;
					break;
				}
				else if (current == next_100)
				{
					attrcol = 'X';
					next100done = 1;
					break;
				}
				else if (current == page)
				{
					attr = black<<4 | magenta;
					break;
				}
			} while (adip[current][0] == 0 && (basictop[current] < 2 || basictop[current] > 7));

			if (basictop[current] >= 2 && basictop[current] <= 5) /* block */
			{
				indent = TOPMENUINDENTBLK;
				if (!attr)
					attr = black<<4 | yellow;
			}
			else if (basictop[current] >= 6 && basictop[current] <= 7) /* group */
			{
				indent = TOPMENUINDENTGRP;
				if (!attr)
					attr = black<<4 | cyan;
			}
			else
			{
				indent = TOPMENUINDENTDEF;
				if (!attr)
					attr = black<<4 | white;
			}
			memcpy(line+indent, adip[current], 12);
			hex2str(&line[TOPMENUINDENTDEF+12+TOPMENUSPC+2], current);
			RenderClearMenuLineBB(line, attrcol, attr);
		}
#if (FONTWIDTH_TOPMENUMAIN != FONTWIDTH_TOPMENU)
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = screen_mode2 ? FONTWIDTH_SMALL : FONTWIDTH_TOPMENUMAIN;
#endif
	}

	/* render line 25 */
	PosX = StartX;
	PosY = StartY + 24*fixfontheight;

	for (byte = 0; byte < 40; byte++)
	{
		if (boxed)
			RenderCharBB(' ', transp<<4 | transp);
		else if (pagecatching)
			RenderCharBB(line25_2[byte], line25_2[byte + 40]);
		else
			RenderCharBB(line25_1[byte], line25_1[byte + 40]);
	}
}

/******************************************************************************
 * CopyBB2FB                                                                  *
 ******************************************************************************/

void CopyBB2FB()
{
	int src, dst = 0;
	int fillcolor = black;

	/* line 25 */
	CreateLine25();

	/* copy backbuffer to framebuffer */
	if (!zoommode)
	{
		memcpy(lfb, &backbuffer, sizeof(backbuffer));
		return;
	}
	else if (zoommode == 1)
		src = StartY*var_screeninfo.xres;
	else
		src = StartY*var_screeninfo.xres + 12*fixfontheight*var_screeninfo.xres;

	if (transpmode)
		fillcolor = transp2;

	memset(lfb, fillcolor, StartY*var_screeninfo.xres);

	do {
		memcpy(lfb + StartY*var_screeninfo.xres + dst, backbuffer + src, var_screeninfo.xres);
		dst += var_screeninfo.xres;
		memcpy(lfb + StartY*var_screeninfo.xres + dst, backbuffer + src, var_screeninfo.xres);
		dst += var_screeninfo.xres;
		src += var_screeninfo.xres;
	} while (dst < var_screeninfo.xres * 24*fixfontheight);

	memcpy(lfb + StartY*var_screeninfo.xres + dst, backbuffer + StartY*var_screeninfo.xres + 24*fixfontheight*var_screeninfo.xres, var_screeninfo.xres*fixfontheight);
	memset(lfb + (StartY + 25*fixfontheight)*var_screeninfo.xres, fillcolor, var_screeninfo.xres*var_screeninfo.yres - (StartY + 25*fixfontheight)*var_screeninfo.xres);
}

/******************************************************************************
 * UpdateLCD                                                                  *
 ******************************************************************************/

void UpdateLCD()
{
	static int init_lcd = 1, old_cached_pages = -1, old_page = -1, old_subpage = -1, old_subpage_max = -1, old_hintmode = -1;
	int  x, y, subpage_max = 0, update_lcd = 0;

	/* init or update lcd */
	if (init_lcd)
	{
		init_lcd = 0;

		for (y = 0; y < 64; y++)
		{
			for (x = 0; x < 120; x++)
			{
				if (lcd_layout[x + y*120])
					lcd_backbuffer[x + (y/8)*120] |= 1 << (y%8);
				else
					lcd_backbuffer[x + (y/8)*120] &= ~(1 << (y%8));
			}
		}

		write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));

		for (y = 15; y <= 58; y++)
		{
			for (x = 1; x < 118; x++)
				lcd_backbuffer[x + (y/8)*120] &= ~(1 << (y%8));
		}

		for (x = 3; x <= 116; x++)
			lcd_backbuffer[x + (39/8)*120] |= 1 << (39%8);

		for (y = 42; y <= 60; y++)
			lcd_backbuffer[35 + (y/8)*120] |= 1 << (y%8);

		for (y = 42; y <= 60; y++)
			lcd_backbuffer[60 + (y/8)*120] |= 1 << (y%8);

		RenderCharLCD(10, 43, 20);
		RenderCharLCD(11, 79, 20);

		return;
	}
	else
	{
		/* page */
		if (old_page != page)
		{
			RenderCharLCD(page>>8,  7, 20);
			RenderCharLCD((page&0x0F0)>>4, 19, 20);
			RenderCharLCD(page&0x00F, 31, 20);

			old_page = page;
			update_lcd = 1;
		}

		/* current subpage */
		if (old_subpage != subpage)
		{
			if (!subpage)
			{
				RenderCharLCD(0, 55, 20);
				RenderCharLCD(1, 67, 20);
			}
			else
			{
				if (subpage >= 0xFF)
					subpage = 1;
				else if (subpage > 99)
					subpage = 0;

				RenderCharLCD(subpage>>4, 55, 20);
				RenderCharLCD(subpage&0x0F, 67, 20);
			}

			old_subpage = subpage;
			update_lcd = 1;
		}

		/* max subpage */
		for (x = 0; x <= 0x79; x++)
		{
			if (cachetable[page][x] != 0)
				subpage_max = x;
		}

		if (old_subpage_max != subpage_max)
		{
			if (!subpage_max)
			{
				RenderCharLCD(0,  91, 20);
				RenderCharLCD(1, 103, 20);
			}
			else
			{
				RenderCharLCD(subpage_max>>4,  91, 20);
				RenderCharLCD(subpage_max&0x0F, 103, 20);
			}

			old_subpage_max = subpage_max;
			update_lcd = 1;
		}

		/* cachestatus */
		if (old_cached_pages != cached_pages)
		{
			RenderCharLCD(cached_pages/1000, 67, 44);
			RenderCharLCD(cached_pages%1000/100, 79, 44);
			RenderCharLCD(cached_pages%100/10, 91, 44);
			RenderCharLCD(cached_pages%10, 103, 44);

			old_cached_pages = cached_pages;
			update_lcd = 1;
		}

		/* mode */
		if (old_hintmode != hintmode)
		{
			if (hintmode)
				RenderCharLCD(12, 43, 44);
			else
				RenderCharLCD(13, 43, 44);

			old_hintmode = hintmode;
			update_lcd = 1;
		}
	}

	if (update_lcd)
		write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));
}

/******************************************************************************
 * DecodePage                                                                 *
 ******************************************************************************/

void DecodePage()
{
	int row, col;
	int hold, clear, loop;
	int foreground, background, doubleheight, charset, mosaictype;
	unsigned char held_mosaic;

	/* copy page to decode buffer */
	if (zap_subpage_manual)
		memcpy(&page_char, cachetable[page][subpage], PAGESIZE);
	else
		memcpy(&page_char, cachetable[page][subpagetable[page]], PAGESIZE);

	/* update timestring */
	memcpy(&page_char[32], &timestring, 8);

	/* check for newsflash & subtitle */
	if (dehamming[page_char[11-6]] & 12 && !screenmode)
		boxed = 1;
	else
		boxed = 0;

	/* modify header */
	if (boxed)
		memset(&page_char, ' ', 40);
	else
		memcpy(&page_char, " TuxTxt ", 8);

	/* decode */
	for (row = 0; row < 24; row++)
	{
		/* start-of-row default conditions */
		foreground   = white;
		background   = black;
		doubleheight = 0;
		charset      = 0;
		mosaictype   = 0;
		hold         = 0;
		held_mosaic  = ' ';

		if (boxed == 1 && memchr(&page_char[row*40], start_box, 40) == 0)
		{
			foreground = transp;
			background = transp;
		}

		for (col = 0; col < 40; col++)
		{
			if (page_char[row*40 + col] < ' ')
			{
				switch (page_char[row*40 + col])
				{
				case alpha_black:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = black;
					charset = 0;
					break;

				case alpha_red:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = red;
					charset = 0;
					break;

				case alpha_green:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = green;
					charset = 0;
					break;

				case alpha_yellow:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = yellow;
					charset = 0;
					break;

				case alpha_blue:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = blue;
					charset = 0;
					break;

				case alpha_magenta:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = magenta;
					charset = 0;
					break;

				case alpha_cyan:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = cyan;
					charset = 0;
					break;

				case alpha_white:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = white;
					charset = 0;
					break;

				case flash:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					/* todo */
					break;

				case steady:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					/* todo */
					break;

				case end_box:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					if (boxed)
					{
						foreground = transp;
						background = transp;
					}
					break;

				case start_box:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					if (boxed)
					{
						for (clear = 0; clear < col; clear++)
							page_atrb[row*40 + clear] = transp<<4 | transp;
					}
					break;

				case normal_size:
					doubleheight = 0;
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case double_height:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					if (row < 23)
						doubleheight = 1;
					break;

				case double_width:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					/* todo */
					break;

				case double_size:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					/* todo */
					break;

				case mosaic_black:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = black;
					charset = 1 + mosaictype;
					break;

				case mosaic_red:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = red;
					charset = 1 + mosaictype;
					break;

				case mosaic_green:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = green;
					charset = 1 + mosaictype;
					break;

				case mosaic_yellow:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = yellow;
					charset = 1 + mosaictype;
					break;

				case mosaic_blue:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = blue;
					charset = 1 + mosaictype;
					break;

				case mosaic_magenta:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = magenta;
					charset = 1 + mosaictype;
					break;

				case mosaic_cyan:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = cyan;
					charset = 1 + mosaictype;
					break;

				case mosaic_white:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					foreground = white;
					charset = 1 + mosaictype;
					break;

				case conceal:
					if (!hintmode)
						foreground = background;
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case contiguous_mosaic:
					mosaictype = 0;
					if (charset)
						charset = 1;
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case separated_mosaic:
					mosaictype = 1;
					if (charset)
						charset = 2;
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case esc:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					/* todo */
					break;

				case black_background:
					background = black;
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case new_background:
					background = foreground;
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case hold_mosaic:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					hold = 1;
					break;

				case release_mosaic:
					page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					hold = 2;
				}

				/* handle spacing attributes */
				if (hold && charset)
					page_char[row*40 + col] = held_mosaic;
				else
					page_char[row*40 + col] = ' ';

				if (hold == 2)
					hold = 0;
			}
			else
			{
				page_atrb[row*40 + col] = doubleheight<<10 | charset<<8 | background<<4 | foreground;

				/* set new held-mosaic char */
				if (charset)
					held_mosaic = page_char[row*40 + col];

				/* skip doubleheight chars in lower line */
				if (doubleheight)
					page_char[(row+1)*40 + col] = 0xFF;
			}
		}

		/* copy attribute to lower line if doubleheight */
		if (memchr(&page_char[(row+1)*40], 0xFF, 40) != 0)
		{
			for (loop = 0; loop < 40; loop++)
				page_atrb[(row+1)*40 + loop] = (page_atrb[row*40 + loop] & 0xF0) | (page_atrb[row*40 + loop] & 0xF0)>>4;

			row++;
		}
	}
}

/******************************************************************************
 * GetRCCode                                                                  *
 ******************************************************************************/

int GetRCCode()
{
#if HAVE_DVB_API_VERSION < 3
	static unsigned short LastKey = -1;
#else
	struct input_event ev;
	static __u16 rc_last_key = KEY_RESERVED;
#endif
	/* get code */
#if HAVE_DVB_API_VERSION < 3
	if (read(rc, &RCCode, 2) == 2)
	{
		if (RCCode != LastKey)
		{
			LastKey = RCCode;

			if ((RCCode & 0xFF00) == 0x5C00)
			{
				switch (RCCode)
#else
	if (read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if (ev.value)
		{
			if (ev.code != rc_last_key)
			{
				rc_last_key = ev.code;
				switch (ev.code)
#endif
				{
				case KEY_UP:		RCCode = RC_UP;		break;
				case KEY_DOWN:		RCCode = RC_DOWN;	break;
				case KEY_LEFT:		RCCode = RC_LEFT;	break;
				case KEY_RIGHT:		RCCode = RC_RIGHT;	break;
				case KEY_OK:		RCCode = RC_OK;		break;
				case KEY_0:		RCCode = RC_0;		break;
				case KEY_1:		RCCode = RC_1;		break;
				case KEY_2:		RCCode = RC_2;		break;
				case KEY_3:		RCCode = RC_3;		break;
				case KEY_4:		RCCode = RC_4;		break;
				case KEY_5:		RCCode = RC_5;		break;
				case KEY_6:		RCCode = RC_6;		break;
				case KEY_7:		RCCode = RC_7;		break;
				case KEY_8:		RCCode = RC_8;		break;
				case KEY_9:		RCCode = RC_9;		break;
				case KEY_RED:		RCCode = RC_RED;	break;
				case KEY_GREEN:		RCCode = RC_GREEN;	break;
				case KEY_YELLOW:	RCCode = RC_YELLOW;	break;
				case KEY_BLUE:		RCCode = RC_BLUE;	break;
				case KEY_VOLUMEUP:	RCCode = RC_PLUS;	break;
				case KEY_VOLUMEDOWN:	RCCode = RC_MINUS;	break;
				case KEY_MUTE:		RCCode = RC_MUTE;	break;
				case KEY_HELP:		RCCode = RC_HELP;	break;
				case KEY_SETUP:		RCCode = RC_DBOX;	break;
				case KEY_HOME:		RCCode = RC_HOME;	break;
				case KEY_POWER:		RCCode = RC_STANDBY;	break;
				}
				return 1;
			}
#if HAVE_DVB_API_VERSION < 3
			else
				RCCode &= 0x003F;
#endif
		}
#if HAVE_DVB_API_VERSION < 3
		else
			RCCode = -1;

		return 1;
#else
		else
		{
			RCCode = 0;
			rc_last_key = KEY_RESERVED;
		}
#endif
	}

	RCCode = 0;
	usleep(1000000/100);

	return 0;
}

/******************************************************************************
 * CacheThread                                                                *
 ******************************************************************************/

void allocate_cache(int magazine)
{
	/* check cachetable and allocate memory if needed */
	if (cachetable[current_page[magazine]][current_subpage[magazine]] == 0)
	{
		cachetable[current_page[magazine]][current_subpage[magazine]] = malloc(PAGESIZE);
		memset(cachetable[current_page[magazine]][current_subpage[magazine]], ' ', PAGESIZE);
		cached_pages++;
	}
}

void *CacheThread(void *arg)
{
	const unsigned char rev_lut[32] = {
		0x00,0x08,0x04,0x0c, /*  upper nibble */
		0x02,0x0a,0x06,0x0e,
		0x01,0x09,0x05,0x0d,
		0x03,0x0b,0x07,0x0f,
		0x00,0x80,0x40,0xc0, /*  lower nibble */
		0x20,0xa0,0x60,0xe0,
		0x10,0x90,0x50,0xd0,
		0x30,0xb0,0x70,0xf0 };
	unsigned char pes_packet[184];
	unsigned char vtxt_row[42];
	int line, byte/*, bit*/;
	int b1, b2, b3, b4;
	int packet_number;
	unsigned char magazine;

	while (1)
	{
		/* check stopsignal */
		pthread_testcancel();

		/* read packet */
		unsigned int readcnt;
		readcnt = read(dmx, &pes_packet, sizeof(pes_packet));
		if (!readcnt||(readcnt!=sizeof(pes_packet))){
#if DEBUG
			printf ("TuxTxt: readerror\n");
#endif
			continue;
		}
		/* analyze it */
		for (line = 0; line < 4; line++)
		{
			unsigned char *vtx_rowbyte = &pes_packet[line*0x2e];
			if ((vtx_rowbyte[0] == 0x02 || vtx_rowbyte[0] == 0x03) && (vtx_rowbyte[1] == 0x2C))
			{
				/* clear rowbuffer */
				/* convert row from lsb to msb (begin with magazin number) */
				for (byte = 4; byte < 46; byte++)
				{
					unsigned char upper,lower;
					upper = (vtx_rowbyte[byte] >> 4) & 0xf;
					lower = vtx_rowbyte[byte] & 0xf;
					vtxt_row[byte-4] = (rev_lut[upper]) | (rev_lut[lower+16]);
				}

				/* get packet number */
				b1 = dehamming[vtxt_row[0]];
				b2 = dehamming[vtxt_row[1]];

				if (b1 == 0xFF || b2 == 0xFF)
				{
#if DEBUG
					printf("TuxTxt <Biterror in Packet>\n");
#endif
					continue;
				}

				b1 &= 8;

				packet_number = b1>>3 | b2<<1;

				/* get magazine number */
				magazine = dehamming[vtxt_row[0]] & 7;
				if (!magazine) magazine = 8;

				/* analyze row */
				if (packet_number == 0)
				{
					/* get pagenumber */
					b1 = dehamming[vtxt_row[0]];
					b2 = dehamming[vtxt_row[3]];
					b3 = dehamming[vtxt_row[2]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
					{
						current_page[magazine] = page_receiving = -1;
#if DEBUG
						printf("TuxTxt <Biterror in Page>\n");
#endif
						continue;
					}

					b1 &= 7;

					current_page[magazine] = page_receiving = b1<<8 | b2<<4 | b3;

					if (current_page[magazine] < 0x100)
					{
						current_page[magazine] += 0x800;
						page_receiving += 0x800;
					}

					if (b2 > 9 || b3 > 9)
					{
						current_subpage[magazine] = 0; /* copy page */
						allocate_cache(magazine); /* FIXME: only until TOP-Info decoded? */
						continue;
					}

					/* check parity */
					for (byte = 10; byte < 42; byte++)
					{
						if ((vtxt_row[byte]&1) ^ ((vtxt_row[byte]>>1)&1) ^
						    ((vtxt_row[byte]>>2)&1) ^ ((vtxt_row[byte]>>3)&1) ^
						    ((vtxt_row[byte]>>4)&1) ^ ((vtxt_row[byte]>>5)&1) ^
						    ((vtxt_row[byte]>>6)&1) ^ (vtxt_row[byte]>>7))
							vtxt_row[byte] &= 127;
						else
							vtxt_row[byte] = ' ';
					}

					/* get subpagenumber */
					b1 = dehamming[vtxt_row[7]];
					b2 = dehamming[vtxt_row[6]];
					b3 = dehamming[vtxt_row[5]];
					b4 = dehamming[vtxt_row[4]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF || b4 == 0xFF)
					{
						current_subpage[magazine] = -1;
#if DEBUG
						printf("TuxTxt <Biterror in SubPage>\n");
#endif
						continue;
					}

					b1 &= 3;
					b3 &= 7;

					if (b1 != 0 || b2 != 0 || b4 > 9)
					{
						current_subpage[magazine] = -1;
						continue;
					}
					else
						current_subpage[magazine] = b3<<4 | b4;

					/* get country control bits */
					b1 = dehamming[vtxt_row[9]];

					if (b1 == 0xFF)
					{
						countrycontrolbitstable[current_page[magazine]][current_subpage[magazine]] = 0xff;
#if DEBUG
						printf("TuxTxt <Biterror in CountryFlags>\n");
#endif
					}
					else
						countrycontrolbitstable[current_page[magazine]][current_subpage[magazine]] =
							((b1 >> 3) & 0x01) | (((b1 >> 2) & 0x01) << 1) | (((b1 >> 1) & 0x01) << 2);

					allocate_cache(magazine);

					/* store current subpage for this page */
					subpagetable[current_page[magazine]] = current_subpage[magazine];

					/* copy timestring */
					memcpy(&timestring, &vtxt_row[34], 8);

					/* set update flag */
					if (current_page[magazine] == page)
					{
						pageupdate = 1;
						if (!zap_subpage_manual)
							subpage = current_subpage[magazine];
					}

					/* check controlbits */
					if (dehamming[vtxt_row[5]] & 8)   /* C4 -> erase page */
						memset(cachetable[current_page[magazine]][current_subpage[magazine]], ' ', PAGESIZE);
				}
				else if (packet_number < 24)
				{
					if ((current_page[magazine] & 0x0F0) <= 0x090 &&
					    (current_page[magazine] & 0x00F) <= 0x009)
					{    /* no parity check for TOP pages, just copy */

						/* check parity */
						for (byte = 2; byte < 42; byte++)
						{
							if ((vtxt_row[byte]&1) ^ ((vtxt_row[byte]>>1)&1) ^
							    ((vtxt_row[byte]>>2)&1) ^ ((vtxt_row[byte]>>3)&1) ^
							    ((vtxt_row[byte]>>4)&1) ^ ((vtxt_row[byte]>>5)&1) ^
							    ((vtxt_row[byte]>>6)&1) ^ (vtxt_row[byte]>>7))
								vtxt_row[byte] &= 127;
							else
								vtxt_row[byte] = ' ';
						}
					}
				}
				/* copy row to pagebuffer */
				if (current_page[magazine] != -1 && current_subpage[magazine] != -1 &&
				    packet_number < 24 && cachetable[current_page[magazine]][current_subpage[magazine]]) /* avoid segfault */
				{
					memcpy(cachetable[current_page[magazine]][current_subpage[magazine]] + packet_number*40, &vtxt_row[2], 40);

					/* set update flag */
					if (current_page[magazine] == page)
					{
						pageupdate = 1;
						if (!zap_subpage_manual)
							subpage = current_subpage[magazine];
					}
				}
			}
		}
	}

	return 0;
}
/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* time-stamp-line-limit:30 */
/* time-stamp-format:"%02d.%02m.%:y %02H:%02M:%02S %u@%s" */
/* End: */

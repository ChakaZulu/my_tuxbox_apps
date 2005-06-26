#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>



/* some data */
 short flofpages[0x900][FLOFSIZE];
unsigned char adip[0x900][13];
unsigned char subpagetable[0x900];
int dmx = -1;
int vtxtpid;
int cached_pages, page, subpage, pageupdate,page_receiving, current_page[9], current_subpage[9];
int receiving, thread_starting, zap_subpage_manual;
char bttok;
int adippg[10];
int maxadippg;
unsigned char basictop[0x900];
int initialized = 0;

unsigned char  timestring[8];
/* cachetable for packets 29 (one for each magazine) */
tstExtData *astP29[9];
/* cachetable */
tstCachedPage *astCachetable[0x900][0x80];

pthread_t thread_id;
void *thread_result;

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

int is_dec(int i)
{
	return ((i & 0x00F) <= 9) && ((i & 0x0F0) <= 0x90);
}

int next_hex(int i) /* return next existing non-decimal page number */
{
	int startpage = i;
	if (startpage < 0x100)
		startpage = 0x100;

	do
	{
		i++;
		if (i > 0x8FF)
			i = 0x100;
		if (i == startpage)
			break;
	}  while ((subpagetable[i] == 0xFF) || is_dec(i));
	return i;
}
/*
 * TOP-Text
 * Info entnommen aus videotext-0.6.19991029,
 * Copyright (c) 1994-96 Martin Buck  <martin-2.buck@student.uni-ulm.de>
 */
void decode_btt()
{
	/* basic top table */
	int i, current, b1, b2, b3, b4;
	unsigned char *btt;

	if (subpagetable[0x1f0] == 0xff || 0 == astCachetable[0x1f0][subpagetable[0x1f0]]) /* not yet received */
		return;
	btt = astCachetable[0x1f0][subpagetable[0x1f0]]->data;
	if (btt[799] == ' ') /* not completely received or error */
		return;

	current = 0x100;
	for (i = 0; i < 800; i++)
	{
		b1 = btt[i];
		if (b1 == ' ')
			b1 = 0;
		else
		{
			b1 = dehamming[b1];
			if (b1 == 0xFF) /* hamming error in btt */
			{
				btt[799] = ' '; /* mark btt as not received */
				return;
			}
		}
		basictop[current] = b1;
		next_dec(&current);
	}
	/* page linking table */
	maxadippg = -1; /* rebuild table of adip pages */
	for (i = 0; i < 10; i++)
	{
		b1 = dehamming[btt[800 + 8*i +0]];

		if (b1 == 0xE)
			continue; /* unused */
		else if (b1 == 0xF)
			break; /* end */

		b4 = dehamming[btt[800 + 8*i +7]];

		if (b4 != 2) /* only adip, ignore multipage (1) */
			continue;

		b2 = dehamming[btt[800 + 8*i +1]];
		b3 = dehamming[btt[800 + 8*i +2]];

		if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
		{
			printf("TuxTxt <Biterror in btt/plt index %d>\n", i);
			btt[799] = ' '; /* mark btt as not received */
			return;
		}

		b1 = b1<<8 | b2<<4 | b3; /* page number */
		adippg[++maxadippg] = b1;
	}
#if DEBUG
	printf("TuxTxt <BTT decoded>\n");
#endif
	bttok = 1;
}

void decode_adip() /* additional information table */
{
	int i, p, j, b1, b2, b3, charfound;
	unsigned char *padip;

	for (i = 0; i <= maxadippg; i++)
	{
		p = adippg[i];
		if (!p || subpagetable[p] == 0xff || 0 == astCachetable[p][subpagetable[p]]) /* not cached (avoid segfault) */
			continue;

		padip = astCachetable[p][subpagetable[p]]->data;
		for (j = 0; j < 44; j++)
		{
			b1 = dehamming[padip[20*j+0]];
			if (b1 == 0xE)
				continue; /* unused */

			if (b1 == 0xF)
				break; /* end */

			b2 = dehamming[padip[20*j+1]];
			b3 = dehamming[padip[20*j+2]];

			if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
			{
				printf("TuxTxt <Biterror in ait %03x %d %02x %02x %02x %02x %02x %02x>\n", p, j,
						 padip[20*j+0],
						 padip[20*j+1],
						 padip[20*j+2],
						 b1, b2, b3
						 );
				return;
			}

			if (b1>8 || b2>9 || b3>9) /* ignore extries with invalid or hex page numbers */
			{
				continue;
			}

			b1 = b1<<8 | b2<<4 | b3; /* page number */
			charfound = 0; /* flag: no printable char found */

			for (b2 = 11; b2 >= 0; b2--)
			{
				b3 = deparity[padip[20*j + 8 + b2]];
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
		} /* next link j */
		adippg[i] = 0; /* completely decoded: clear entry */
#if DEBUG
		printf("TuxTxt <ADIP %03x decoded>\n", p);
#endif
	} /* next adip page i */

	while (!adippg[maxadippg] && (maxadippg >= 0)) /* and shrink table */
		maxadippg--;
}
/******************************************************************************
 * GetSubPage                                                                 *
 ******************************************************************************/
int GetSubPage(int page, int subpage, int offset)
{
	int loop;


	for (loop = subpage + offset; loop != subpage; loop += offset)
	{
		if (loop < 0)
			loop = 0x79;
		else if (loop > 0x79)
			loop = 0;
		if (loop == subpage)
			break;

		if (astCachetable[page][loop])
		{
#if DEBUG
			printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
#endif
			return loop;
		}
	}

#if DEBUG
	printf("TuxTxt <NextSubPage: no other SubPage>\n");
#endif
	return subpage;
}

/******************************************************************************
 * clear_cache                                                                *
 ******************************************************************************/

void clear_cache()
{
	int clear_page, clear_subpage, d26;
	maxadippg  = -1;
	bttok      = 0;
	cached_pages  = 0;
	page_receiving = -1;
	memset(&subpagetable, 0xFF, sizeof(subpagetable));
	memset(&basictop, 0, sizeof(basictop));
	memset(&adip, 0, sizeof(adip));
	memset(&flofpages, 0 , sizeof(flofpages));
	memset(&timestring, 0x20, 8);
 	unsigned char magazine;
	for (magazine = 1; magazine < 9; magazine++)
	{
		current_page  [magazine] = -1;
		current_subpage [magazine] = -1;
	}

	for (clear_page = 0; clear_page < 0x900; clear_page++)
		for (clear_subpage = 0; clear_subpage < 0x80; clear_subpage++)
			if (astCachetable[clear_page][clear_subpage])
			{
				tstPageinfo *p = &(astCachetable[clear_page][clear_subpage]->pageinfo);
				if (p->p24)
					free(p->p24);
				if (p->ext)
				{
					if (p->ext->p27)
						free(p->ext->p27);
					for (d26=0; d26 < 16; d26++)
						if (p->ext->p26[d26])
							free(p->ext->p26[d26]);
					free(p->ext);
				}
				free(astCachetable[clear_page][clear_subpage]);
				astCachetable[clear_page][clear_subpage] = 0;
			}
	for (clear_page = 0; clear_page < 9; clear_page++)
	{
		if (astP29[clear_page])
		{
		    if (astP29[clear_page]->p27)
			free(astP29[clear_page]->p27);
		    for (d26=0; d26 < 16; d26++)
			if (astP29[clear_page]->p26[d26])
			    free(astP29[clear_page]->p26[d26]);
		    free(astP29[clear_page]);
		    astP29[clear_page] = 0;
		}
		current_page  [clear_page] = -1;
		current_subpage [clear_page] = -1;
	}
	memset(&astCachetable, 0, sizeof(astCachetable));
	memset(&astP29, 0, sizeof(astP29));
#if DEBUG
	printf("TuxTxt cache cleared\n");
#endif
}
/******************************************************************************
 * init_demuxer                                                               *
 ******************************************************************************/

int init_demuxer()
{
	/* open demuxer */
	if ((dmx = open(DMX, O_RDWR)) == -1)
	{
		perror("TuxTxt <open DMX>");
		return 0;
	}


	if (ioctl(dmx, DMX_SET_BUFFER_SIZE, 64*1024) < 0)
	{
		perror("TuxTxt <DMX_SET_BUFFERSIZE>");
		return 0;
	}
#if DEBUG
	printf("TuxTxt: initialized\n");
#endif
	/* init successfull */
	return 1;
}
/******************************************************************************
 * CacheThread support functions                                              *
 ******************************************************************************/

void decode_p2829(unsigned char *vtxt_row, tstExtData **ptExtData)
{
	int bitsleft, colorindex;
	unsigned char *p;
	int t1 = deh24(&vtxt_row[7-4]);
	int t2 = deh24(&vtxt_row[10-4]);

	if (t1 < 0 || t2 < 0)
	{
#if DEBUG
		printf("TuxTxt <Biterror in p28>\n");
#endif
		return;
	}

	if (!(*ptExtData))
		(*ptExtData) = calloc(1, sizeof(tstExtData));

	(*ptExtData)->p28Received = 1;
	(*ptExtData)->DefaultCharset = (t1>>7) & 0x7f;
	(*ptExtData)->SecondCharset = ((t1>>14) & 0x0f) | ((t2<<4) & 0x70);
	(*ptExtData)->LSP = !!(t2 & 0x08);
	(*ptExtData)->RSP = !!(t2 & 0x10);
	(*ptExtData)->SPL25 = !!(t2 & 0x20);
	(*ptExtData)->LSPColumns = (t2>>6) & 0x0f;

	bitsleft = 8; /* # of bits not evaluated in val */
	t2 >>= 10; /* current data */
	p = &vtxt_row[13-4];	/* pointer to next data triplet */
	for (colorindex = 0; colorindex < 16; colorindex++)
	{
		if (bitsleft < 12)
		{
			t2 |= deh24(p) << bitsleft;
			if (t2 < 0)	/* hamming error */
				break;
			p += 3;
			bitsleft += 18;
		}
		(*ptExtData)->bgr[colorindex] = t2 & 0x0fff;
		bitsleft -= 12;
		t2 >>= 12;
	}
	if (t2 < 0 || bitsleft != 14)
	{
#if DEBUG
		printf("TuxTxt <Biterror in p28/29 t2=%d b=%d>\n", t2, bitsleft);
#endif
		(*ptExtData)->p28Received = 0;
		return;
	}
	(*ptExtData)->DefScreenColor = t2 & 0x1f;
	t2 >>= 5;
	(*ptExtData)->DefRowColor = t2 & 0x1f;
	(*ptExtData)->BlackBgSubst = !!(t2 & 0x20);
	t2 >>= 6;
	(*ptExtData)->ColorTableRemapping = t2 & 0x07;
}

void erase_page(int magazine)
{
	memset(&(astCachetable[current_page[magazine]][current_subpage[magazine]]->pageinfo), 0, sizeof(tstPageinfo));	/* struct pageinfo */
	memset(astCachetable[current_page[magazine]][current_subpage[magazine]]->p0, ' ', 24);
	memset(astCachetable[current_page[magazine]][current_subpage[magazine]]->data, ' ', 23*40);
}

void allocate_cache(int magazine)
{
	/* check cachetable and allocate memory if needed */
	if (astCachetable[current_page[magazine]][current_subpage[magazine]] == 0)
	{
		astCachetable[current_page[magazine]][current_subpage[magazine]] = malloc(sizeof(tstCachedPage));
		erase_page(magazine);
		cached_pages++;
	}
}

/******************************************************************************
 * CacheThread                                                                *
 ******************************************************************************/

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
	tstPageinfo *pageinfo_thread;

	printf("TuxTxt running thread...(%03x)\n",vtxtpid);
	receiving = 1;
	while (1)
	{
		/* check stopsignal */
		pthread_testcancel();

		if (!receiving) continue;

		/* read packet */
		ssize_t readcnt;
		readcnt = read(dmx, &pes_packet, sizeof(pes_packet));

		if (readcnt != sizeof(pes_packet))
		{
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
					b2 = dehamming[vtxt_row[3]];
					b3 = dehamming[vtxt_row[2]];

					if (b2 == 0xFF || b3 == 0xFF)
					{
						current_page[magazine] = page_receiving = -1;
#if DEBUG
						printf("TuxTxt <Biterror in Page>\n");
#endif
						continue;
					}

					current_page[magazine] = page_receiving = magazine<<8 | b2<<4 | b3;

					if (b2 == 0x0f && b3 == 0x0f)
					{
						current_subpage[magazine] = -1; /* ?ff: ignore data transmissions */
						continue;
					}

					/* get subpagenumber */
					b1 = dehamming[vtxt_row[7]];
					b2 = dehamming[vtxt_row[6]];
					b3 = dehamming[vtxt_row[5]];
					b4 = dehamming[vtxt_row[4]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF || b4 == 0xFF)
					{
#if DEBUG
						printf("TuxTxt <Biterror in SubPage>\n");
#endif
						current_subpage[magazine] = -1;
						continue;
					}

					b1 &= 3;
					b3 &= 7;

					if (is_dec(page_receiving)) /* ignore other subpage bits for hex pages */
					{
#if 0	/* ? */
						if (b1 != 0 || b2 != 0)
						{
#if DEBUG
							printf("TuxTxt <invalid subpage data p%03x %02x %02x %02x %02x>\n", page_receiving, b1, b2, b3, b4);
#endif
							current_subpage[magazine] = -1;
							continue;
						}
						else
#endif
							current_subpage[magazine] = b3<<4 | b4;
					}
					else
						current_subpage[magazine] = b4; /* max 16 subpages for hex pages */

					/* store current subpage for this page */
					subpagetable[current_page[magazine]] = current_subpage[magazine];

					allocate_cache(magazine);
					pageinfo_thread = &(astCachetable[current_page[magazine]][current_subpage[magazine]]->pageinfo);

					if ((page_receiving & 0xff) == 0xfe) /* ?fe: magazine organization table (MOT) */
						pageinfo_thread->function = FUNC_MOT;

					/* check controlbits */
					if (dehamming[vtxt_row[5]] & 8)   /* C4 -> erase page */
						memset(astCachetable[current_page[magazine]][current_subpage[magazine]]->data, ' ', 23*40);

					pageinfo_thread->boxed = !!(dehamming[vtxt_row[7]] & 0x0c);

					/* get country control bits */
					b1 = dehamming[vtxt_row[9]];
					if (b1 == 0xFF)
					{
#if DEBUG
						printf("TuxTxt <Biterror in CountryFlags>\n");
#endif
					}
					else
					{
						pageinfo_thread->nationalvalid = 1;
						pageinfo_thread->national = rev_lut[b1] & 0x07;
					}

					/* check parity, copy line 0 to cache (start and end 8 bytes are not needed and used otherwise) */
					unsigned char *p = astCachetable[current_page[magazine]][current_subpage[magazine]]->p0;
					for (byte = 10; byte < 42-8; byte++)
						*p++ = deparity[vtxt_row[byte]];

					if (!is_dec(page_receiving))
						continue; /* valid hex page number: just copy headline, ignore timestring */

					/* copy timestring */
					p = timestring;
					for (; byte < 42; byte++)
						*p++ = deparity[vtxt_row[byte]];

				} /* (packet_number == 0) */
				else if (packet_number == 29 && dehamming[vtxt_row[2]] == 0) /* packet 29/0 replaces 28/0 for a whole magazine */
				{
					decode_p2829(vtxt_row, &(astP29[magazine]));
				}
				else if (current_page[magazine] != -1 && current_subpage[magazine] != -1)
					/* packet>0, 0 has been correctly received, buffer allocated */
				{
					pageinfo_thread = &(astCachetable[current_page[magazine]][current_subpage[magazine]]->pageinfo);
					/* pointer to current info struct */

					if (packet_number <= 25)
					{
						unsigned char *p;
						if (packet_number < 24)
							p = astCachetable[current_page[magazine]][current_subpage[magazine]]->data + 40*(packet_number-1);
						else
						{
							if (!(pageinfo_thread->p24))
								pageinfo_thread->p24 = calloc(2, 40);
							p = pageinfo_thread->p24 + (packet_number - 24) * 40;
						}

						if (is_dec(current_page[magazine]))
							for (byte = 2; byte < 42; byte++)
								*p++ = deparity[vtxt_row[byte]]; /* check/remove parity bit */
						else if ((current_page[magazine] & 0xff) == 0xfe)
							for (byte = 2; byte < 42; byte++)
								*p++ = dehamming[vtxt_row[byte]]; /* decode hamming 8/4 */
						else /* other hex page: no parity check, just copy */
							memcpy(p, &vtxt_row[2], 40);
					}
					else if (packet_number == 27)
					{
						int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

						if (descode == 0xff)
						{
#if DEBUG
							printf("TuxTxt <Biterror in p27>\n");
#endif
							continue;
						}
						if (descode == 0) // reading FLOF-Pagelinks
						{
							b1 = dehamming[vtxt_row[0]];
							if (b1 != 0xff)
							{
								b1 &= 7;

								for (byte = 0; byte < FLOFSIZE; byte++)
								{
									b2 = dehamming[vtxt_row[4+byte*6]];
									b3 = dehamming[vtxt_row[3+byte*6]];

									if (b2 != 0xff && b3 != 0xff)
									{
										b4 = ((b1 ^ (dehamming[vtxt_row[8+byte*6]]>>1)) & 6) |
											((b1 ^ (dehamming[vtxt_row[6+byte*6]]>>3)) & 1);
										if (b4 == 0)
											b4 = 8;
										if (b2 <= 9 && b3 <= 9)
											flofpages[current_page[magazine] ][byte] = b4<<8 | b2<<4 | b3;
									}
								}

								/* copy last 2 links to adip for TOP-Index */
								if (pageinfo_thread->p24) /* packet 24 received */
								{
									int a, a1, e=39, l=3;
									char *p = pageinfo_thread->p24;
									do
									{
										for (;
											  l >= 2 && 0 == flofpages[current_page[magazine]][l];
											  l--)
											; /* find used linkindex */
										for (;
											  e >= 1 && !isalnum(p[e]);
											  e--)
											; /* find end */
										for (a = a1 = e - 1;
											  a >= 0 && p[a] >= ' ';
											  a--) /* find start */
											if (p[a] > ' ')
											a1 = a; /* first non-space */
										if (a >= 0 && l >= 2)
										{
											strncpy(adip[flofpages[current_page[magazine]][l]],
													  &p[a1],
													  12);
											if (e-a1 < 11)
												adip[flofpages[current_page[magazine]][l]][e-a1+1] = '\0';
#if 0 //DEBUG
											printf(" %03x/%02x %d %d %d %d %03x %s\n",
													 current_page[magazine], current_subpage[magazine],
													 l, a, a1, e,
													 flofpages[current_page[magazine]][l],
													 adip[flofpages[current_page[magazine]][l]]
													 );
#endif
										}
										e = a - 1;
										l--;
									} while (l >= 2);
								}
							}
						}
						else if (descode == 4)	/* level 2.5 links (ignore level 3.5 links of /4 and /5) */
						{
							int i;
							tstp27 *p;

							if (!pageinfo_thread->ext)
								pageinfo_thread->ext = calloc(1, sizeof(tstExtData));
							if (!(pageinfo_thread->ext->p27))
								pageinfo_thread->ext->p27 = calloc(4, sizeof(tstp27));
							p = pageinfo_thread->ext->p27;
							for (i = 0; i < 4; i++)
							{
								int d1 = deh24(&vtxt_row[6*i + 3]);
								int d2 = deh24(&vtxt_row[6*i + 6]);
								if (d1 < 0 || d2 < 0)
								{
#if DEBUG
									printf("TuxTxt <Biterror in p27/4-5>\n");
#endif
									continue;
								}
								p->local = i & 0x01;
								p->drcs = !!(i & 0x02);
								p->l25 = !!(d1 & 0x04);
								p->l35 = !!(d1 & 0x08);
								p->page =
									(((d1 & 0x000003c0) >> 6) |
									 ((d1 & 0x0003c000) >> (14-4)) |
									 ((d1 & 0x00003800) >> (11-8))) ^
									(dehamming[vtxt_row[0]] << 8);
								if (p->page < 0x100)
									p->page += 0x800;
								p->subpage = d2 >> 2;
								if ((p->page & 0xff) == 0xff)
									p->page = 0;
								else if (astCachetable[p->page][0])	/* link valid && linked page cached */
								{
									tstPageinfo *pageinfo_link = &(astCachetable[p->page][0]->pageinfo);
									if (p->local)
										pageinfo_link->function = p->drcs ? FUNC_DRCS : FUNC_POP;
									else
										pageinfo_link->function = p->drcs ? FUNC_GDRCS : FUNC_GPOP;
								}
								p++; /*  */
							}
						}
					}

					else if (packet_number == 26)
					{
						int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

						if (descode == 0xff)
						{
#if DEBUG
							printf("TuxTxt <Biterror in p26>\n");
#endif
							continue;
						}
						if (!pageinfo_thread->ext)
							pageinfo_thread->ext = calloc(1, sizeof(tstExtData));
						if (!(pageinfo_thread->ext->p26[descode]))
							pageinfo_thread->ext->p26[descode] = malloc(13 * 3);
						memcpy(pageinfo_thread->ext->p26[descode], &vtxt_row[3], 13 * 3);
#if 0//DEBUG
						int i, t, m;

						printf("P%03x/%02x %02d/%x",
								 current_page[magazine], current_subpage[magazine],
								 packet_number, dehamming[vtxt_row[2]]);
						for (i=7-4; i <= 45-4; i+=3) /* dump all triplets */
						{
							t = deh24(&vtxt_row[i]); /* mode/adr/data */
							m = (t>>6) & 0x1f;
							printf(" M%02xA%02xD%03x", m, t & 0x3f, (t>>11) & 0x7f);
							if (m == 0x1f)	/* terminator */
								break;
						}
						putchar('\n');
#endif
					}
					else if (packet_number == 28)
					{
						int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

						if (descode == 0xff)
						{
#if DEBUG
							printf("TuxTxt <Biterror in p28>\n");
#endif
							continue;
						}
						if (descode != 2)
						{
							int t1 = deh24(&vtxt_row[7-4]);
							pageinfo_thread->function = t1 & 0x0f;
							if (!pageinfo_thread->nationalvalid)
							{
								pageinfo_thread->nationalvalid = 1;
								pageinfo_thread->national = (t1>>4) & 0x07;
							}
						}

						switch (descode) /* designation code */
						{
						case 0: /* basic level 1 page */
						{
							decode_p2829(vtxt_row, &(pageinfo_thread->ext));
							break;
						}
						case 1: /* G0/G1 designation for older decoders, level 3.5: DCLUT4/16, colors for multicolored bitmaps */
						{
							break; /* ignore */
						}
						case 2: /* page key */
						{
							break; /* ignore */
						}
						case 3: /* types of PTUs in DRCS */
						{
							break; /* TODO */
						}
						case 4: /* CLUTs 0/1, only level 3.5 */
						{
							break; /* ignore */
						}
						default:
						{
							break; /* invalid, ignore */
						}
						} /* switch designation code */
					}
					else if (packet_number == 30)
					{
#if 0//DEBUG
						int i;

						printf("p%03x/%02x %02d/%x ",
								 current_page[magazine], current_subpage[magazine],
								 packet_number, dehamming[vtxt_row[2]]);
						for (i=26-4; i <= 45-4; i++) /* station ID */
							putchar(deparity[vtxt_row[i]]);
						putchar('\n');
#endif
					}
				}
				/* set update flag */
				if (current_page[magazine] == page && current_subpage[magazine] != -1)
				{
					pageupdate = 1;
					if (!zap_subpage_manual)
						subpage = current_subpage[magazine];
				}
			}
		}
	}
	return 0;
}
/******************************************************************************
 * start_thread                                                               *
 ******************************************************************************/
int start_thread()
{
	thread_starting = 1;
	struct dmx_pes_filter_params dmx_flt;

	/* set filter & start demuxer */
	dmx_flt.pid      = vtxtpid;
	dmx_flt.input    = DMX_IN_FRONTEND;
	dmx_flt.output   = DMX_OUT_TAP;
	dmx_flt.pes_type = DMX_PES_OTHER;
	dmx_flt.flags    = DMX_IMMEDIATE_START;

	if (dmx == -1) init_demuxer();

	if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_PES_FILTER>");
		thread_starting = 0;
		return 0;
	}

	/* create decode-thread */
	if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
	{
		perror("TuxTxt <pthread_create>");
		thread_starting = 0;
		return 0;
	}
#if 1//DEBUG
	printf("TuxTxt service started %x\n", vtxtpid);
#endif
	receiving = 1;
	thread_starting = 0;
	return 1;
}
/******************************************************************************
 * stop_thread                                                                *
 ******************************************************************************/

int stop_thread()
{

	/* stop decode-thread */
	if (pthread_cancel(thread_id) != 0)
	{
		perror("TuxTxt <pthread_cancel>");
		return 0;
	}

	if (pthread_join(thread_id, &thread_result) != 0)
	{
		perror("TuxTxt <pthread_join>");
		return 0;
	}
	ioctl(dmx, DMX_STOP);
	if (dmx != -1)
    	    close(dmx);
	dmx = -1;
#if 1//DEBUG
	printf("TuxTxt stopped service %x\n", vtxtpid);
#endif
	return 1;
}

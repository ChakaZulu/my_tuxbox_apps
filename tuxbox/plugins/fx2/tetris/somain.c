/*
** initial coding by fx2
*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <rcinput.h>
#include <draw.h>
#include <colors.h>
#include <board.h>
#include <pig.h>
#include <plugin.h>
#include <fx2math.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <config.h>

extern	int				debug;
extern	int				doexit;
extern	unsigned short	actcode;
extern	unsigned short	realcode;
extern	long			score;

static	char			*proxy_addr=0;
static	char			*proxy_user=0;
static	char			*hscore=0;
static	char			isalloc=0;
static	int				localuser=-1;

typedef struct _HScore
{
	char	name[12];
	long	points;
} HScore;

static	HScore	hsc[8];
static	HScore	ihsc[8];
static	int		use_ihsc=0;

extern	unsigned long BuildCheck( char *user, long score );

static	void	LoadHScore( void )
{
	CURL			*curl;
	CURLcode		res;
	FILE			*fp;
	char			url[ 512 ];
	char			*p;
	int				i;

	FBDrawString( 150,32,32,"try load high score from",GRAY,0);
	FBDrawString( 150,64,32,hscore,GRAY,0);
#ifdef USEX
	FBFlushGrafic();
#endif

	sprintf(url,"%s/games/tetris.php?action=get",hscore);

	curl = curl_easy_init();
	if ( !curl )
		return;
	fp = fopen( "/var/tmp/trash", "w");
	if ( !fp )
	{
		curl_easy_cleanup(curl);
		return;
	}
	curl_easy_setopt( curl, CURLOPT_URL, url );
	curl_easy_setopt( curl, CURLOPT_FILE, fp );
	curl_easy_setopt( curl, CURLOPT_NOPROGRESS, TRUE );
	if ( proxy_addr )
	{
		curl_easy_setopt( curl, CURLOPT_PROXY, proxy_addr );
		if ( proxy_user )
			curl_easy_setopt( curl, CURLOPT_PROXYUSERPWD, proxy_user );
	}
	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	fclose( fp );

	if ( res )
		return;

	fp=fopen( "/var/tmp/trash", "r" );
	if ( !fp )
		return;

	for( i=0; i<8; i++ )
	{
		if ( !fgets(url,512,fp) )
			break;
		p=strchr(url,'\n');
		if ( p )
			*p=0;
		p=strchr(url,'&');
		if ( !p )
			break;
		*p=0;
		p++;

		strncpy(ihsc[i].name,url,10);
		ihsc[i].name[9]=0;
		ihsc[i].points = atoi(p);
	}
	if ( i==8 )
		use_ihsc=1;
	fclose(fp);
	unlink("/var/tmp/trash");
}

static	void	LocalSave( void )
{
	int		x;
	char	*user;
	int		i;

	localuser=-1;

	for( i=0; i < 8; i++ )
		if ( score > hsc[i].points )
			break;
	if ( i==8 )
		return;

	Fx2PigPause();

	FBFillRect( 500,32,3*52,4*52+4,BLACK );

	FBFillRect( 150,420,470,64,BLACK );
	FBDrawRect( 149,419,472,66,WHITE );
	FBDrawRect( 148,418,474,68,WHITE );
	x=FBDrawString( 154,420,64,"name : ",WHITE,0);
	user=FBEnterWord(154+x,420,64,9,WHITE);

	Fx2PigResume();

	if ( i < 7 )
		memmove( hsc+i+1,hsc+i,sizeof(HScore)*(7-i) );
	strcpy(hsc[i].name,user);
	hsc[i].points=score;

	localuser=i;
}

static	void	SaveGame( void )
{
	CURL		*curl;
	CURLcode	res;
	FILE		*fp;
	char		url[ 512 ];
	char		*user=0;
	char		luser[ 32 ];
	int			x;
	int			n;
	char		*p;
	struct timeval	tv;
	unsigned long	chk=0;

	doexit=0;

	if ( score < 31 )
		return;

	LocalSave();

	if ( !use_ihsc )
		return;

	for( x=0; x < 8; x++ )
	{
		if ( score > ihsc[x].points )
			break;
	}
	if ( x == 8 )
		return;

	FBDrawString( 100,230,64,"Inet-Send Highscore ? (OK/BLUE)",GREEN,0);
#ifdef USEX
	FBFlushGrafic();
#endif

	while( realcode != 0xee )
		RcGetActCode();

	actcode=0xee;
	while( !doexit )
	{
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		select( 0,0,0,0, &tv );
		RcGetActCode();
		if ( actcode == RC_BLUE )
			return;
		if ( actcode == RC_OK )
			break;
	}
	if ( doexit )
		return;

	if ( localuser != -1 )
	{
		strcpy(luser,hsc[localuser].name);
		user=luser;
	}
	else
	{
		Fx2PigPause();

		FBFillRect( 500,32,3*52,4*52+4,BLACK );

		FBFillRect( 150,420,470,64,BLACK );
		FBDrawRect( 149,419,472,66,WHITE );
		FBDrawRect( 148,418,474,68,WHITE );
		x=FBDrawString( 154,420,64,"name : ",WHITE,0);
		user=FBEnterWord(154+x,420,64,9,WHITE);

		Fx2PigResume();
	}

	n=FBDrawString( 210,360,48,"sending",BLACK,WHITE);

/* clean name */
	x = strlen(user);
	p=user;
	for( p=user; *p; x--, p++ )
	{
		if (( *p == ' ' ) || ( *p == '&' ) || ( *p == '/' ))
			memcpy(p,p+1,x);
	}

	chk=BuildCheck( user, score );

	sprintf(url,"%s/games/tetris.php?action=put&user=%s&score=%d&chk=%d",
		hscore,user,score,chk);

	curl = curl_easy_init();
	if ( !curl )
		return;
	fp = fopen( "/var/tmp/trash", "w");
	if ( !fp )
	{
		curl_easy_cleanup(curl);
		return;
	}
	curl_easy_setopt( curl, CURLOPT_URL, url );
	curl_easy_setopt( curl, CURLOPT_FILE, fp );
	curl_easy_setopt( curl, CURLOPT_NOPROGRESS, TRUE );
	if ( proxy_addr )
	{
		curl_easy_setopt( curl, CURLOPT_PROXY, proxy_addr );
		if ( proxy_user )
			curl_easy_setopt( curl, CURLOPT_PROXYUSERPWD, proxy_user );
	}
	res = curl_easy_perform(curl);

	FBFillRect( 210,360,n,48,GRAY);
	if ( !res )
		FBDrawString( 210,360,48,"success",GREEN,GRAY);
	else
		FBDrawString( 210,360,48,"failed",RED,GRAY);

	curl_easy_cleanup(curl);
	fclose( fp );
	unlink( "/var/tmp/trash" );

	LoadHScore();

	return;
}

static	void	ShowHScore( HScore *g )
{
	int				i;
	int				x;
	char			pp[64];

	FBFillRect( 0, 0, 720, 576, BLACK );

	if ( g==ihsc )
		FBDrawString( 190, 32, 64, "Internet HighScore", RED, BLACK );
	else
		FBDrawString( 220, 32, 64, "HighScore", RED, BLACK );
	for( i=0; i < 8; i++ )
	{
		FBDrawString( 100, 100+i*48, 48, g[i].name, WHITE, 0 );
		sprintf(pp,"%d",g[i].points);
		x = FBDrawString( 400, 100+i*48, 48, pp, BLACK, BLACK );
		FBDrawString( 500-x, 100+i*48, 48, pp, WHITE, BLACK );
	}
#ifdef USEX
	FBFlushGrafic();
#endif
	while( realcode != 0xee )
		RcGetActCode();
}

static	void	ShowIHScore( void )
{
	int				i = 50;
	struct timeval	tv;

	ShowHScore( ihsc );

	while( !doexit && ( realcode == 0xee ) && ( i>0 ))
	{
		tv.tv_sec=0;
		tv.tv_usec=200000;
		select( 0,0,0,0,&tv);
		RcGetActCode();
		i--;
	}
}

static	void	setup_colors(void)
{
	FBSetColor( YELLOW, 235, 235, 30 );
	FBSetColor( GREEN, 30, 235, 30 );
	FBSetColor( STEELBLUE, 80, 80, 200 );
	FBSetColor( BLUE, 80, 80, 230 );
	FBSetColor( GRAY, 130, 130, 130 );
	FBSetColor( DARK, 60, 60, 60 );
	FBSetColor( RED1, 198, 131, 131 );
	FBSetColor( RED2, 216, 34, 49 );

/* magenta */
	FBSetColor( 30, 216, 175, 216);
	FBSetColor( 31, 205, 160, 207);
	FBSetColor( 32, 183, 131, 188);
	FBSetColor( 33, 230, 196, 231);
	FBSetColor( 34, 159, 56, 171);
	FBSetColor( 35, 178, 107, 182);
	FBSetColor( 36, 172, 85, 180);
	FBSetColor( 37, 180, 117, 184);
	FBSetColor( 38, 120, 1, 127);
	FBSetColor( 39, 89, 1, 98);
/* blue */
	FBSetColor( 40, 165, 172, 226);
	FBSetColor( 41, 148, 156, 219);
	FBSetColor( 42, 119, 130, 200);
	FBSetColor( 43, 189, 196, 238);
	FBSetColor( 44, 81, 90, 146);
	FBSetColor( 45, 104, 114, 185);
	FBSetColor( 46, 91, 103, 174);
	FBSetColor( 47, 109, 119, 192);
	FBSetColor( 48, 46, 50, 81);
	FBSetColor( 49, 34, 38, 63);
/* cyan */
	FBSetColor( 50, 157, 218, 234);
	FBSetColor( 51, 140, 208, 227);
	FBSetColor( 52, 108, 186, 211);
	FBSetColor( 53, 184, 233, 243);
	FBSetColor( 54, 55, 143, 172);
	FBSetColor( 55, 92, 171, 197);
	FBSetColor( 56, 78, 160, 187);
	FBSetColor( 57, 98, 177, 203);
	FBSetColor( 58, 7, 98, 120);
	FBSetColor( 59, 1, 78, 98);
/* green */
	FBSetColor( 60, 173, 218, 177);
	FBSetColor( 61, 158, 209, 165);
	FBSetColor( 62, 130, 189, 140);
	FBSetColor( 63, 195, 232, 199);
	FBSetColor( 64, 89, 138, 98);
	FBSetColor( 65, 115, 174, 122);
	FBSetColor( 66, 102, 163, 112);
	FBSetColor( 67, 121, 180, 129);
	FBSetColor( 68, 50, 77, 55);
	FBSetColor( 69, 38, 59, 41);
/* red */
	FBSetColor( 70, 239, 157, 152);
	FBSetColor( 71, 231, 141, 136);
	FBSetColor( 72, 210, 112, 109);
	FBSetColor( 73, 246, 184, 181);
	FBSetColor( 74, 153, 76, 74);
	FBSetColor( 75, 197, 97, 92);
	FBSetColor( 76, 184, 86, 81);
	FBSetColor( 77, 202, 101, 99);
	FBSetColor( 78, 95, 33, 32);
	FBSetColor( 79, 78, 20, 19);
/* yellow */
	FBSetColor( 80, 238, 239, 152);
	FBSetColor( 81, 230, 231, 136);
	FBSetColor( 82, 207, 214, 105);
	FBSetColor( 83, 246, 246, 181);
	FBSetColor( 84, 148, 157, 70);
	FBSetColor( 85, 194, 200, 89);
	FBSetColor( 86, 180, 189, 76);
	FBSetColor( 87, 199, 206, 95);
	FBSetColor( 88, 88, 93, 34);
	FBSetColor( 89, 69, 75, 22);
/* orange */
	FBSetColor( 90, 243, 199, 148);
	FBSetColor( 91, 237, 185, 130);
	FBSetColor( 92, 220, 159, 99);
	FBSetColor( 93, 249, 220, 178);
	FBSetColor( 94, 184, 113, 43);
	FBSetColor( 95, 208, 144, 81);
	FBSetColor( 96, 198, 132, 67);
	FBSetColor( 97, 213, 150, 88);
	FBSetColor( 98, 127, 63, 1);
	FBSetColor( 99, 98, 46, 1);

	FBSetupColors();
}

int tetris_exec( int fdfb, int fdrc, int fdlcd, char *cfgfile )
{
	struct timeval	tv;
	int				x;
	int				i;
	int				fd;
	FILE			*fp;
	char			*line;
	char			*p;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	setup_colors();

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

/* load setup */
	fp = fopen( CONFIGDIR "/games.cfg", "r" );
	if ( fp )
	{
		line=malloc(128);
		isalloc=1;
		proxy_addr=0;
		proxy_user=0;
		hscore=0;
		while( fgets( line, 128, fp ) )
		{
			if ( *line == '#' )
				continue;
			if ( *line == ';' )
				continue;
			p=strchr(line,'\n');
			if ( p )
				*p=0;
			p=strchr(line,'=');
			if ( !p )
				continue;
			*p=0;
			p++;
			if ( !strcmp(line,"proxy") )
				proxy_addr=strdup(p);
			else if ( !strcmp(line,"proxy_user") )
				proxy_user=strdup(p);
			else if ( !strcmp(line,"hscore") )
				hscore=strdup(p);
		}
		fclose(fp);
	}

	fd = open( GAMESDIR "/tetris.hscore", O_RDONLY );
	if ( fd == -1 )
	{
		mkdir( GAMESDIR, 567 );
		for( i=0; i < 8; i++ )
		{
			strcpy(hsc[i].name,"nobody");
			hsc[i].points=30;
		}
	}
	else
	{
		read( fd, hsc, sizeof(hsc) );
		close(fd);
	}

	if ( hscore )
	{
		LoadHScore();
	}

	Fx2ShowPig( 450, 105, 128, 96 );

	while( doexit != 3 )
	{
		BoardInitialize();
		DrawBoard( );	/* 0 = all */
		NextItem();

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */
			MoveSide();
			if ( !FallDown() )
			{
				RemoveCompl();
				if ( !NextItem() )
					doexit=1;
			}
#ifdef USEX
			FBFlushGrafic();
#endif

			RcGetActCode( );
		}

		if ( doexit != 3 )
		{
			actcode=0xee;
			DrawGameOver();
#ifdef USEX
			FBFlushGrafic();
#endif
			SaveGame();
			doexit=0;
			if ( use_ihsc )
				ShowIHScore();
			ShowHScore(hsc);

#ifdef USEX
			FBFlushGrafic();
#endif
			i=0;
			actcode=0xee;
			while(( actcode != RC_OK ) && !doexit )
			{
				tv.tv_sec = 0;
				tv.tv_usec = 100000;
				x = select( 0, 0, 0, 0, &tv );		/* 100ms pause */
				RcGetActCode( );
				i++;
				if ( i == 50 )
				{
					FBDrawString( 190, 480, 48, "press OK for new game",GRAY,0);
#ifdef USEX
					FBFlushGrafic();
#endif
				}
			}
		}
	}

	Fx2StopPig();

	RcClose();
	FBClose();

/* save hscore */
	fd = open( GAMESDIR "/tetris.hscore", O_CREAT|O_WRONLY, 438 );
	if ( fd != -1 )
	{
		write( fd, hsc, sizeof(hsc) );
		close(fd);
	}

	if ( isalloc )
	{
		if ( proxy_addr )
			free ( proxy_addr );
		if ( proxy_user )
			free ( proxy_user );
		if ( hscore )
			free ( hscore );
	}

	return 0;
}

int plugin_exec( PluginParam *par )
{
	int		fd_fb=-1;
	int		fd_rc=-1;

	for( ; par; par=par->next )
	{
		if ( !strcmp(par->id,P_ID_FBUFFER) )
			fd_fb=_atoi(par->val);
		else if ( !strcmp(par->id,P_ID_RCINPUT) )
			fd_rc=_atoi(par->val);
		else if ( !strcmp(par->id,P_ID_NOPIG) )
			fx2_use_pig=!_atoi(par->val);
		else if ( !strcmp(par->id,P_ID_PROXY) && par->val && *par->val )
			proxy_addr=par->val;
		else if ( !strcmp(par->id,P_ID_HSCORE) && par->val && *par->val )
			hscore=par->val;
		else if ( !strcmp(par->id,P_ID_PROXY_USER) && par->val && *par->val )
			proxy_user=par->val;
	}
	return tetris_exec( fd_fb, fd_rc, -1, 0 );
}

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
#include <board.h>
#include <colors.h>
#include <pig.h>
#include <include/plugin.h>
#include <fx2math.h>

extern	int	doexit;
extern	int	debug;
extern	unsigned short	actcode;

static	void	setup_colors( void )
{
	FBSetColor( YELLOW, 255, 255, 0 );
	FBSetColor( GREEN, 0, 255, 0 );
	FBSetColor( STEELBLUE, 0, 0, 180 );
	FBSetColor( BLUE, 130, 130, 255 );
	FBSetColor( GRAY, 130, 130, 130 );
	FBSetColor( DARK, 1, 1, 1 );

	FBSetupColors( );
}

int mines_exec( int fdfb, int fdrc, int fdlcd, char *cfgfile )
{
	struct timeval	tv;
	int				x;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	setup_colors();

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

	Fx2ShowPig( 440, 300, 160, 144 );

	while( doexit != 3 )
	{
		BoardInitialize();
		DrawBoard( 0 );

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
			x = select( 0, 0, 0, 0, &tv );		/* 100ms pause */
	
			RcGetActCode( );
			MoveMouse();
		}

		if ( doexit != 3 )
		{
			actcode=0xee;
			if ( doexit ==2 )
				DrawScore();
			else
				DrawGameOver();
			doexit=0;
			while(( actcode != RC_OK ) && !doexit )
			{
				tv.tv_sec = 0;
				tv.tv_usec = 100000;
				x = select( 0, 0, 0, 0, &tv );		/* 100ms pause */
				RcGetActCode( );
			}
		}
	}

	Fx2StopPig();

	RcClose();
	FBClose();

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
	}
	return mines_exec( fd_fb, fd_rc, -1, 0 );
}

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
#include <maze.h>

#include <../../mczap/neutrinoNG/plugins/gameplugins.h>

	int		doexit = 0;
	int		debug = 0;
extern	int	gametime;
extern	int	pices;
extern	unsigned short	actcode;

int pacman_exec( int fdfb, int fdrc, int fdlcd )
{
	struct timeval	tv;
	int				x;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

	while( doexit != 3 )
	{
		MazeInitialize();
		DrawMaze( );	/* 0 = all */
		DrawFill();
		DrawGhosts( );
		DrawPac( );

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 1000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */
	
			MovePac( );
			MoveGhosts( );
			DrawGhosts( );
			DrawPac( );
			RcGetActCode( );
			CheckGhosts( );
		}

		if ( doexit != 3 )
		{
			actcode=0xee;
			if ( gametime )
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

	RcClose();
	FBClose();

	return 0;
}

int	pacman_getInfo( struct SPluginInfo *info )
{
	info->pluginversion = 1;
	strcpy(info->name,"Pacman");
	strcpy(info->desc,"The good old pacman in new dboxII generation");
	info->type=1;
	info->needfb=1;
	info->needrc=1;
	info->needlcd=1;

	return 0;
}

/*
** initial coding by fx2
*/


#include <stdio.h>

#include <draw.h>
#include <sys/time.h>
#include <rcinput.h>
#include <malloc.h>
#include <colors.h>
#include <pics.h>
#include <text.h>

#define	STATUS_X		80
#define STATUS_Y		50
#define LOGO_X			600
#define LOGO_Y			30

extern	double	sqrt( double in );

extern	int		doexit;

#define LEFT	0
#define RIGHT	1
#define UP		2
#define DOWN	3

typedef struct _snake_ele
{
	unsigned char	x;
	unsigned char	y;
	unsigned char	dir;
} snake_ele;

extern	unsigned short	actcode;
static	unsigned char	maze[ 44 * 40 ];
static	int				score = 0;
static	int				snake_len = 10;
static	snake_ele		*snake = 0;
static	snake_ele		last;
static	snake_ele		goodie;
static	int				count = 0;

static	int		myrand( int idx )
{
	struct timeval	tv;
	gettimeofday(&tv,0);

	return tv.tv_usec % idx;
}

static	void	SetNewGoodi()
{
	int		goodiok=0;
	int		i;

	while( !goodiok )
	{
		goodie.x = myrand( MAZEW - 8 ) + 4;
		goodie.y = myrand( MAZEH - 10 ) + 6;
		goodiok=1;
		for( i=0; i < snake_len; i++ )
		{
			if (( snake[i].x == goodie.x ) && ( snake[i].y == goodie.y ))
			{
				goodiok=0;
				break;
			}
		}
		if ( goodiok && ( maze [ goodie.y * MAZEW + goodie.x ] != ' ' ) )
			goodiok=0;
	}
	FBCopyImage( goodie.x*16, goodie.y*16, 16, 16, futter );
	count = 100;
}

void	FreeSnake( void )
{
	free( snake );
	snake = 0;
}

static	void	InitSnake( void )
{
	int		i;

	if ( snake )
		free(snake);
	snake_len = 10;
	snake = malloc(sizeof(snake_ele)*snake_len);
	for( i=0;i<snake_len;i++)
	{
		snake[i].x = 20+i;
		snake[i].y = 20;
		snake[i].dir = LEFT;
	}
	last.x = 20+snake_len-1;
	last.y = 20;
	last.dir = LEFT;
}

static	void	DrawScore( void )
{
	int		ww[10] = {	WI0, WI1, WI2, WI3,
						WI4, WI5, WI6, WI7,
						WI8, WI9 };
	unsigned char	*nn[10] = {
						d0, d1, d2, d3,
						d4, d5, d6, d7,
						d8, d9 };

	char	cscore[ 64 ];
	char	*p=cscore;
	int		x = 250;
	int		h;

	sprintf(cscore,"%d",score);

	for( ; *p; p++ )
	{
		h = (*p - 48);
		FBCopyImage( x, 56, ww[h], 32, nn[h] );
		x += ww[h];
	}
	FBFillRect(x,56,16,24,BLACK);
	FBFillRect(x,80,16,8,STEELBLUE);
}

void	DrawSnake( void )
{
	int		i;

	for( i=0; i <snake_len; i++ )
	{
		FBFillRect( snake[i].x*16,snake[i].y*16,16,16,GREEN);
	}
}

void	MoveSnake( void )
{
	int			i;
	snake_ele	*snake2;

	memmove(snake+1,snake,sizeof(snake_ele)*(snake_len-1));
	switch( actcode )
	{
	case RC_LEFT :
		if ( snake[1].dir != RIGHT )
			snake[0].dir = LEFT;
		break;
	case RC_RIGHT :
		if ( snake[1].dir != LEFT )
			snake[0].dir = RIGHT;
		break;
	case RC_UP :
		if ( snake[1].dir != DOWN )
			snake[0].dir = UP;
		break;
	case RC_DOWN :
		if ( snake[1].dir != UP )
			snake[0].dir = DOWN;
		break;
	}
	switch( snake[0].dir )
	{
	case UP :
		snake[0].y--;
		break;
	case DOWN :
		snake[0].y++;
		break;
	case RIGHT :
		snake[0].x++;
		break;
	case LEFT :
		snake[0].x--;
		break;
	}
	if (( snake[0].x == goodie.x ) && ( snake[0].y == goodie.y ))
	{
		score += 100;
		DrawScore();
		snake_len++;
		snake2=realloc(snake,sizeof(snake_ele)*snake_len);
		if ( snake2 )
		{
			snake=snake2;
			memcpy(snake+snake_len-1,snake+snake_len-2,sizeof(snake_ele));
		}
		else
		{
			snake_len--;
		}
		SetNewGoodi();
	}
	else
	{
		count--;
		if ( !count )
		{
			FBFillRect( goodie.x*16, goodie.y*16, 16, 16, BLACK );
			SetNewGoodi();
		}
	}

printf("last.x=%d last.y=%d\n",last.x,last.y);
	FBFillRect( last.x*16, last.y*16, 16, 16, BLACK );
	FBFillRect( snake[0].x*16, snake[0].y*16, 16, 16, GREEN2 );
	FBFillRect( snake[1].x*16, snake[1].y*16, 16, 16, GREEN );
printf("copynow\n");
	memcpy(&last,snake+snake_len-1,sizeof(snake_ele));
printf("copydone\n");
	if ( maze[ snake[0].y*MAZEW+snake[0].x ] == '#' )
	{
		doexit=1;
		return;
	}
	for( i=1; i<snake_len;i++)
	{
		if ( snake[0].x == snake[i].x && snake[0].y == snake[i].y )
		{
			doexit=1;
			return;
		}
	}
}

void	DrawMaze( void )
{
	int				x;
	int				y;
	unsigned char	*p = maze;

	memset(maze,' ',sizeof(maze));

	InitSnake();

	score=0;

	for( y = 5; y < MAZEH-3; y++ )
	{
		for( x = 3; x < MAZEW-3; x++ )
		{
			if ( (y==5) || (x==3) || (y==MAZEH-4) || ( x==MAZEW-4))
				maze[ y*MAZEW+x ]='#';

			if ( ( y == 13 ) && ( x < MAZEW - 6 ) && ( x > 15 ) )
				maze[ y*MAZEW+x ]='#';

			if ( ( x == 8 ) && ( y < MAZEH - 6 ) && ( y > 22 ) )
				maze[ y*MAZEW+x ]='#';
		}
	}
	for( y = MAZEH-9; y < MAZEH-1; y++ )
	{
		for( x = MAZEW-11; x < MAZEW-1; x++ )
		{
			if ( (y==MAZEH-9) || (x==MAZEW-11) || (y==MAZEH-2) || ( x==MAZEW-2))
				maze[ y*MAZEW+x ]='#';
			else
				maze[ y*MAZEW+x ]='z';
		}
	}
	for( y = 0; y < MAZEH; y++ )
	{
		for( x = 0; x < MAZEW; x++ )
		{
			switch ( *p )
			{
			case '#' :
				FBFillRect( x*16, y*16, 16, 16, STEELBLUE );
				break;
			case '.' :
				FBCopyImage( x*16, y*16, 16, 16, futter );
				break;
			case 'z' :
				FBFillRect( x*16, y*16, 16, 16, 0 );
				break;
			default :
				FBFillRect( x*16, y*16, 16, 16, BLACK );
				break;
			}
			p++;
		}
	}
	FBCopyImage( LOGO_X, LOGO_Y, FX_WIDTH, 64, data_fx2 );

	SetNewGoodi();
	DrawSnake();
	DrawScore();
}

void	DrawGameOver( void )
{
	FBCopyImage( 250, 200, GO_WIDTH, 64, data_gameover );
}

void	DrawFinalScore( void )
{
	int		ww[10] = {	NO_0_WIDTH, NO_1_WIDTH, NO_2_WIDTH, NO_3_WIDTH,
						NO_4_WIDTH, NO_5_WIDTH, NO_6_WIDTH, NO_7_WIDTH,
						NO_8_WIDTH, NO_9_WIDTH };
	unsigned char	*nn[10] = {
						data_no0, data_no1, data_no2, data_no3,
						data_no4, data_no5, data_no6, data_no7,
						data_no8, data_no9 };

	char	cscore[ 64 ];
	char	*p=cscore;
	int		x = 250 + SC_WIDTH + 18;
	int		h;

	sprintf(cscore,"%d",score);
	FBFillRect( 250,264, SC_WIDTH + 19, 64, BLACK );
	FBCopyImage( 250, 264, SC_WIDTH, 64, data_score );

	for( ; *p; p++ )
	{
		h = (*p - 48);
		FBCopyImage( x, 264, ww[h], 64, nn[h] );
		x += ww[h];
	}
}

/*
** initial coding by fx2
*/


#include <stdio.h>
#include <stdlib.h>

#include <draw.h>
#include <sys/time.h>
#include <rcinput.h>
#include <colors.h>
#include <sprite.h>

extern	int		doexit;

extern	unsigned short	realcode;
extern	unsigned short	actcode;

extern	unsigned char	*GetPic( int idx,int *maxani,int *width, int *height );
extern	unsigned char *GetMirrorPic( char picid );
extern	void	bgGetImage( int x1, int y1, int width, int height,
					unsigned char *to );

static	Sprite	*root=0;
static	Sprite	*last=0;
extern	int		main_x;

void	DrawSprite( Sprite *s )
{
	int		x;
	int		y;

	x=s->x;
	y=s->y;

	if (( x+s->width+s->width<main_x ) ||
		( x>main_x+656 ))
			return;

	FB2CopyImage( x-main_x+32, y+32, s->width, s->height, s->data+s->ani*s->sz, 1 );
}

int SpriteCollide( Sprite *s, int x, int y )
{
	return ((x <  s->x+s->width+s->width) &&
			(x >= s->x) &&
			(y <  s->y+s->height+s->height) &&
			(y >= s->y ));
}

Sprite *CreateSprite( int picid, int ani, int x, int y )
{
	Sprite *s;
	int		ma;

	s=malloc(sizeof(Sprite));
	memset(s,0,sizeof(Sprite));
	s->countdown = 5;
	s->type = TYP_WALKER;
	s->x = x;
	s->y = y;
	s->oldx = x;
	s->oldy = y;
	s->picid = (char)picid;
	s->ori_data= GetPic( picid, &ma, &s->width, &s->height );
	s->data = s->ori_data;
	s->maxani = (char)ma;
	s->sz=s->width*s->height;
	if ( ani <= s->maxani )
		s->ani = ani;

	s->next=0;
	s->pre=last;

	if ( s->pre )
		s->pre->next = s;

	last=s;

	if ( !root )
		root=s;

	return s;
}

void	DrawSprites( void )
{
	Sprite	*s;

	for( s=root; s; s=s->next )
		DrawSprite(s);
}

void	SpriteNextPic( Sprite *s )
{
	if ( s->anilocked )
		return;
	s->ani++;
	if ( s->ani > s->maxani )
		s->ani=0;
}

void	SpriteSelPic( Sprite *s, int ani )
{
	s->ani = ani;
	if ( s->ani > s->maxani )
		s->ani=0;
}

void	MirrorSprite( Sprite *s )
{
	if ( !s->flip_data )
		s->flip_data=GetMirrorPic( s->picid );
	if ( s->data == s->ori_data )
		s->data = s->flip_data;
	else
		s->data = s->ori_data;
}

void	SpriteGetBackground( Sprite *s )
{
	s->oldx=s->x;
	s->oldy=s->y;
return;
	if ( !s->back )
		s->back=malloc(s->sz*4);
	bgGetImage(s->x,s->y,s->width+s->width,s->height+s->height,s->back);
}

void	SpritesGetBackground( void )
{
	Sprite	*s;

	for( s=root; s; s=s->next )
		if ( !s->backlocked )
			SpriteGetBackground(s);
}

void	FreeSprites( void )
{
	Sprite	*s;

	for( s=root; s; s=s->next )
		if ( s->back )
			free(s->back);
	root=0;
	last=0;
}

void	SpriteChangePic( Sprite *s, int picid )
{
	int		ma;

	if( s->back )
		free(s->back);
	s->back=0;
	s->picid = (char)picid;
	s->ori_data= GetPic( picid, &ma, &s->width, &s->height );
	s->data = s->ori_data;
	s->flip_data=0;
	s->maxani = (char)ma;
	s->sz=s->width*s->height;
	if ( s->ani > s->maxani )
		s->ani = 0;
}

#ifndef DRAW_H
#define DRAW_H

typedef unsigned char	uchar;

extern	void	FBSetColor( int idx, uchar r, uchar g, uchar b );
extern	void	FBSetupColors( void );
extern	int		FBInitialize( int xRes, int yRes, int bpp, int extfd );
extern	void	FBClose( void );
extern	void	FBPaintPixel( int x, int y, unsigned char col );
extern	void	FBDrawLine( int xa, int ya, int xb, int yb, unsigned char col );
extern	void	FBDrawHLine( int x, int y, int dx, unsigned char col );
extern	void	FBDrawVLine( int x, int y, int dy, unsigned char col );
extern	void	FBFillRect( int x, int y, int dx, int dy, unsigned char col );
extern	void	FBDrawRect( int x, int y, int dx, int dy, unsigned char col );
extern	void	FBCopyImage( int x, int y, int dx, int dy, unsigned char *src );
extern	void	FBOverlayImage(int x, int y, int dx, int dy, int relx, int rely,
					unsigned char c1,
					unsigned char *src,
					unsigned char *under,
					unsigned char *right,
					unsigned char *bottom );
extern	void	FBCopyImageCol( int x, int y, int dx, int dy, unsigned char col,
					unsigned char *src );
extern	void	FBBlink( int x, int y, int dx, int dy, int count );
extern	void	FBMove( int x, int y, int x2, int y2, int dx, int dy );
extern	void	FBPrintScreen( void );

#define BNR0			0
#define BLACK			1
#define	WHITE			2

#endif

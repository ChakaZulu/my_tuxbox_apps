#ifndef MAZE_H
#define MAZE_H

extern	void	DrawMaze( void );
extern	void	DrawPac( void );
extern	void	MovePac( void );
extern	void	DrawFill( void );
extern	void	DrawGhosts( void );
extern	void	MoveGhosts( void );
extern	void	MazeInitialize( void );
extern	void	CheckGhosts( void );
extern	void	DrawGameOver( void );
extern	void	DrawScore( void );
extern	void	InitLevel( void );
extern	void	NextLevel( void );

#endif

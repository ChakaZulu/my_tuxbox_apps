// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: dstrings.h,v 1.1 2002/01/16 13:30:31 fx2 Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// $Log: dstrings.h,v $
// Revision 1.1  2002/01/16 13:30:31  fx2
// added doom , dont install bouquet.so, X11-bugfix rcinput.c, yahtzee.c ?
//
//
// DESCRIPTION:
//	DOOM strings, by language.
//
//-----------------------------------------------------------------------------


#ifndef __DSTRINGS__
#define __DSTRINGS__


// All important printed strings.
// Language selection (message strings).
// Use -DFRENCH etc.

#ifdef FRENCH
#include "d_french.h"
#else
#include "d_englsh.h"
#endif

// Misc. other strings.
#define SAVEGAMENAME	"doomsav"


//
// File locations,
//  relative to current position.
// Path names are OS-sensitive.
//
#define DEVMAPS "devmaps"
#define DEVDATA "devdata"


// Not done in french?

// QuitDOOM messages
#define NUM_QUITMESSAGES   22

extern char* endmsg[];


#endif
//-----------------------------------------------------------------------------
//
// $Log: dstrings.h,v $
// Revision 1.1  2002/01/16 13:30:31  fx2
// added doom , dont install bouquet.so, X11-bugfix rcinput.c, yahtzee.c ?
//
//
//-----------------------------------------------------------------------------

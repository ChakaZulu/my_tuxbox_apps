/*
$Id: output.c,v 1.4 2003/11/26 16:27:46 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de

  Output Module


$Log: output.c,v $
Revision 1.4  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.3  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


#include "output.h"


/*
  -- Module Global Vars
*/

static int  verbose_level = 0;

static int  indent_level = 0;
int   table_indent [] = {0,4,8,12,15};

#define MAX_INDENT_LEVEL  ( (sizeof(table_indent)/sizeof(int)) - 1)






/*
   -- set indent-level
   -- and sends a '\r' to reset indent prints
   - +1 = indent plus one level
   - -1 = unindent one level
   -  0 = reset to Level 0 
*/

void indent (int v)

{
  if (v == 0) indent_level  = 0;
  else        indent_level += v;

  if (indent_level < 0)
     indent_level = 0;
  if (indent_level >= MAX_INDENT_LEVEL)
     indent_level = MAX_INDENT_LEVEL;

  fputc ('\r',stdout);
  print_indent();
//    out_nl2 (0);
}



/*
 -- set verbosity level
 -- 0 = highest, 9 = lowest level
 -- print message upto (including) this verbosity level
*/

void setVerboseLevel (int v)
{
  verbose_level = v;
}

int getVerboseLevel ()
{
 return verbose_level;
}




/*
  -- output special printf
  -- out_nl will append \n at the end of the output
*/

void out(int verbose, char *msgfmt,...)
{
  va_list args;

  if (verbose <= verbose_level) {
     va_start (args,msgfmt);
     vfprintf (stdout, msgfmt, args);
     va_end   (args);
  }
}


void out_nl(int verbose, char *msgfmt,...)
{
  va_list args;

  if (verbose <= verbose_level) {
     va_start (args,msgfmt);
     vfprintf (stdout, msgfmt, args);
     va_end   (args);

     out_nl2(verbose);
  }
}


/*
  -- just print a NL
*/

void  out_nl2 (int verbose)
{
  if (verbose <= verbose_level) {
     fputc ('\n',stdout);
     print_indent();
  }

}


void print_indent (void)

{
 int i;

 for (i=0; i<table_indent[indent_level]; i++) {
   fputc (' ',stdout);
 }

}


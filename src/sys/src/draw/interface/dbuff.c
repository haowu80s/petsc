#ifndef lint
static char vcid[] = "$Id: dbuff.c,v 1.2 1996/02/08 18:27:49 bsmith Exp bsmith $";
#endif
/*
       Provides the calling sequences for all the basic Draw routines.
*/
#include "drawimpl.h"  /*I "draw.h" I*/

/*@
   DrawSetDoubleBuffer - Sets a window to be double buffered. 

   Input Parameter:
.  draw - the drawing context

.keywords:  draw, set, double, buffer
@*/
int DrawSetDoubleBuffer(Draw draw)
{
  PETSCVALIDHEADERSPECIFIC(draw,DRAW_COOKIE);
  if (draw->type == NULLWINDOW) return 0;
  if (draw->ops.setdoublebuffer) return (*draw->ops.setdoublebuffer)(draw);
  return 0;
}

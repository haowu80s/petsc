#ifndef lint
static char vcid[] = "$Id: ex15.c,v 1.24 1996/01/13 04:04:05 balay Exp bsmith $";
#endif

static char help[] = "Displays a vector visually.\n\n";

#include "petsc.h"
#include "is.h"
#include "vec.h"
#include "sys.h"
#include "sysio.h"
#include "draw.h"
#include <math.h>

int main(int argc,char **argv)
{
  int        i,n = 50, ierr, flg;
  Scalar     v;
  Vec        x;
  Draw       draw;
  DrawLG     lg;

  PetscInitialize(&argc,&argv,(char*)0,(char*)0,help);
  OptionsGetInt(PETSC_NULL,"-n",&n,&flg);

  /* create vector */
  ierr = VecCreateSeq(MPI_COMM_SELF,n,&x); CHKERRA(ierr);

  for ( i=0; i<n; i++ ) {
    v = (double) i;
    ierr = VecSetValues(x,1,&i,&v,INSERT_VALUES); CHKERRA(ierr);
  }

  ierr = VecAssemblyBegin(x); CHKERRA(ierr);
  ierr = VecAssemblyEnd(x); CHKERRA(ierr);

  ierr = DrawOpenX(MPI_COMM_SELF,0,0,0,0,300,300,&draw); CHKERRA(ierr);
  ierr = DrawLGCreate(draw,1,&lg); CHKERRA(ierr);

  ierr = VecView(x,(Viewer) lg); CHKERRA(ierr);

  ierr = DrawLGDestroy(lg); CHKERRA(ierr);

  ierr = DrawDestroy(draw); CHKERRA(ierr);
  ierr = VecDestroy(x); CHKERRA(ierr);

  PetscFinalize();
  return 0;
}
 

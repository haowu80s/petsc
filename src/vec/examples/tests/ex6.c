#ifndef lint
static char vcid[] = "$Id: ex6.c,v 1.33 1996/11/27 22:51:04 bsmith Exp bsmith $";
#endif

static char help[] = "Demonstrates a scatter with a stride and general index set.\n\n";

#include "petsc.h"
#include "is.h"
#include "vec.h"
#include "sys.h"
#include <math.h>

int main(int argc,char **argv)
{
  int           n = 6, ierr, idx1[3] = {0,1,2}, loc[6] = {0,1,2,3,4,5};
  Scalar        two = 2.0, vals[6] = {10,11,12,13,14,15};
  Vec           x,y;
  IS            is1,is2;
  VecScatter    ctx = 0;

  PetscInitialize(&argc,&argv,(char*)0,help);

  /* create two vector */
  ierr = VecCreateSeq(PETSC_COMM_SELF,n,&x); CHKERRA(ierr);
  ierr = VecDuplicate(x,&y); CHKERRA(ierr);

  /* create two index sets */
  ierr = ISCreateGeneral(PETSC_COMM_SELF,3,idx1,&is1); CHKERRA(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,3,0,2,&is2); CHKERRA(ierr);

  ierr = VecSetValues(x,6,loc,vals,INSERT_VALUES); CHKERRA(ierr);
  ierr = VecView(x,VIEWER_STDOUT_SELF); CHKERRA(ierr);
  PetscPrintf(PETSC_COMM_SELF,"----\n");
  ierr = VecSet(&two,y); CHKERRA(ierr);
  ierr = VecScatterCreate(x,is1,y,is2,&ctx); CHKERRA(ierr);
  ierr = VecScatterBegin(x,y,INSERT_VALUES,SCATTER_FORWARD,ctx); CHKERRA(ierr);
  ierr = VecScatterEnd(x,y,INSERT_VALUES,SCATTER_FORWARD,ctx); CHKERRA(ierr);
  ierr = VecScatterDestroy(ctx); CHKERRA(ierr);
  
  ierr = VecView(y,VIEWER_STDOUT_SELF); CHKERRA(ierr);

  ierr = ISDestroy(is1); CHKERRA(ierr);
  ierr = ISDestroy(is2); CHKERRA(ierr);
  ierr = VecDestroy(x); CHKERRA(ierr);
  ierr = VecDestroy(y); CHKERRA(ierr);

  PetscFinalize();
  return 0;
}
 

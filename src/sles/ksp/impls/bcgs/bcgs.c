#ifndef lint
static char vcid[] = "$Id: bcgs.c,v 1.27 1996/01/22 03:06:33 curfman Exp bsmith $";
#endif

/*                       
    This code implements the BiCGStab (Stabilized version of BiConjugate
    Gradient Squared) method.  Reference: van der Vorst, 1992.

    Note that for the complex numbers version, the VecDot() arguments
    within the code MUST remain in the order given for correct computation
    of inner products.
*/
#include <stdio.h>
#include <math.h>
#include "petsc.h"
#include "kspimpl.h"

static int KSPSetUp_BCGS(KSP ksp)
{
  int ierr;

  if (ksp->pc_side == PC_SYMMETRIC)
    {SETERRQ(2,"KSPSetUp_BCGS:no symmetric preconditioning for KSPBCGS");}
  ierr = KSPCheckDef( ksp ); CHKERRQ(ierr);
  return KSPiDefaultGetWork( ksp, 7 );
}

static int  KSPSolve_BCGS(KSP ksp,int *its)
{
  int       i = 0, maxit, hist_len, cerr = 0,ierr;
  Scalar    rho, rhoold, alpha, beta, omega, omegaold, d1, d2,zero = 0.0, tmp;
  Vec       X,B,V,P,R,RP,T,S, BINVF;
  double    dp, *history;

  maxit   = ksp->max_it;
  history = ksp->residual_history;
  hist_len= ksp->res_hist_size;
  X       = ksp->vec_sol;
  B       = ksp->vec_rhs;
  R       = ksp->work[0];
  RP      = ksp->work[1];
  V       = ksp->work[2];
  T       = ksp->work[3];
  S       = ksp->work[4];
  P       = ksp->work[5];
  BINVF   = ksp->work[6];

  /* Compute initial preconditioned residual */
  ierr = KSPResidual(ksp,X,V,T,R,BINVF,B); CHKERRQ(ierr);

  /* Test for nothing to do */
  ierr = VecNorm(R,NORM_2,&dp); CHKERRQ(ierr);
  if ((*ksp->converged)(ksp,0,dp,ksp->cnvP)) {*its = 0; return 0;}
  MONITOR(ksp,dp,0);
  if (history) history[0] = dp;

  /* Make the initial Rp == R */
  ierr = VecCopy(R,RP); CHKERRQ(ierr);

  rhoold   = 1.0;
  alpha    = 1.0;
  omegaold = 1.0;
  ierr = VecSet(&zero,P); CHKERRQ(ierr);
  ierr = VecSet(&zero,V); CHKERRQ(ierr);

  for (i=0; i<maxit; i++) {
    ierr = VecDot(RP,R,&rho); CHKERRQ(ierr);       /*   rho <- rp' r       */
    if (rho == 0.0) {fprintf(stderr,"Breakdown\n"); *its = -(i+1);return 0;} 
    beta = (rho/rhoold) * (alpha/omegaold);
    tmp = -omegaold; VecAXPY(&tmp,V,P);            /*   p <- p - w v       */
    ierr = VecAYPX(&beta,R,P); CHKERRQ(ierr);      /*   p <- r + p beta    */
    ierr = PCApplyBAorAB(ksp->B,ksp->pc_side,
                         P,V,T); CHKERRQ(ierr);    /*   v <- K p           */
    ierr = VecDot(RP,V,&d1); CHKERRQ(ierr);
    alpha = rho / d1; tmp = -alpha;                /*   a <- rho / (rp' v) */
    ierr = VecWAXPY(&tmp,V,R,S); CHKERRQ(ierr);    /*   s <- r - a v       */
    ierr = PCApplyBAorAB(ksp->B,ksp->pc_side,
                         S,T,R); CHKERRQ(ierr);    /*   t <- K s           */
    ierr = VecDot(T,S,&d1); CHKERRQ(ierr);
    ierr = VecDot(T,T,&d2); CHKERRQ(ierr);
    if (d2 == 0.0) {
      /* t is 0.  if s is 0, then alpha v == r, and hence alpha p
	 may be our solution.  Give it a try? */
      ierr = VecDot(S,S,&d1); CHKERRQ(ierr);
      if (d1 != 0.0) {SETERRQ(1,"KSPSolve_BCGS:Breakdown");}
      ierr = VecAXPY(&alpha,P,X); CHKERRQ(ierr);   /*   x <- x + a p       */
      if (history && hist_len > i+1) history[i+1] = 0.0;
      MONITOR(ksp,0.0,i+1);
      break;
    }
    omega = d1 / d2;                               /*   w <- (t's) / (t't) */
    ierr = VecAXPY(&alpha,P,X); CHKERRQ(ierr);     /*   x <- x + a p       */
    ierr = VecAXPY(&omega,S,X); CHKERRQ(ierr);     /*   x <- x + w s       */
    tmp = -omega; 
    ierr = VecWAXPY(&tmp,T,S,R); CHKERRQ(ierr);    /*   r <- s - w t       */
    ierr = VecNorm(R,NORM_2,&dp); CHKERRQ(ierr);

    rhoold   = rho;
    omegaold = omega;

    if (history && hist_len > i + 1) history[i+1] = dp;
    MONITOR(ksp,dp,i+1);
    cerr = (*ksp->converged)(ksp,i+1,dp,ksp->cnvP);
    if (cerr) break;    
  }
  if (i == maxit) i--;
  if (history) ksp->res_act_size = (hist_len < i + 1) ? hist_len : i + 1;

  ierr = KSPUnwindPre(ksp,X,T); CHKERRQ(ierr);
  if (cerr <= 0) *its = -(i+1);
  else           *its = i + 1;
  return 0;
}

int KSPCreate_BCGS(KSP ksp)
{
  ksp->data                 = (void *) 0;
  ksp->type                 = KSPBCGS;
  ksp->pc_side              = PC_LEFT;
  ksp->calc_res             = 1;
  ksp->setup                = KSPSetUp_BCGS;
  ksp->solver               = KSPSolve_BCGS;
  ksp->adjustwork           = KSPiDefaultAdjustWork;
  ksp->destroy              = KSPiDefaultDestroy;
  ksp->converged            = KSPDefaultConverged;
  ksp->buildsolution        = KSPDefaultBuildSolution;
  ksp->buildresidual        = KSPDefaultBuildResidual;
  ksp->view                 = 0;
  return 0;
}

#ifndef lint
static char vcid[] = "$Id: cg.c,v 1.36 1996/03/05 21:08:53 bsmith Exp bsmith $";
#endif

/*                       
           This implements Preconditioned Conjugate Gradients.       
*/
#include <stdio.h>
#include <math.h>
#include "kspimpl.h"
#include "cgctx.h"

int KSPSetUp_CG(KSP ksp)
{
  KSP_CG *cgP = (KSP_CG *) ksp->data;
  int    maxit = ksp->max_it,ierr;

  /* check user parameters and functions */
  if (ksp->pc_side == PC_RIGHT)
    {SETERRQ(2,"KSPSetUp_CG:no right preconditioning for KSPCG");}
  else if (ksp->pc_side == PC_SYMMETRIC)
    {SETERRQ(2,"KSPSetUp_CG:no symmetric preconditioning for KSPCG");}
  if ((ierr = KSPCheckDef(ksp))) return ierr;

  /* get work vectors from user code */
  if ((ierr = KSPiDefaultGetWork( ksp, 3 ))) return ierr;

  if (ksp->calc_eigs) {
    /* get space to store tridiagonal matrix for Lanczo */
    cgP->e = (Scalar *) PetscMalloc(4*(maxit+1)*sizeof(Scalar)); CHKPTRQ(cgP->e);
    PLogObjectMemory(ksp,4*(maxit+1)*sizeof(Scalar));
    cgP->d  = cgP->e + maxit + 1; 
    cgP->ee = cgP->d + maxit + 1;
    cgP->dd = cgP->ee + maxit + 1;
  }
  return 0;
}

int  KSPSolve_CG(KSP ksp,int *its)
{
  int          ierr, i = 0,maxit,eigs,pres, hist_len, cerr;
  Scalar       dpi, a = 1.0,beta,betaold = 1.0,b,*e = 0,*d = 0, mone = -1.0, ma; 
  double       *history, dp;
  Vec          X,B,Z,R,P;
  KSP_CG       *cgP;
  Mat          Amat, Pmat;
  MatStructure pflag;

  cgP = (KSP_CG *) ksp->data;
  eigs    = ksp->calc_eigs;
  pres    = ksp->use_pres;
  maxit   = ksp->max_it;
  history = ksp->residual_history;
  hist_len= ksp->res_hist_size;
  X       = ksp->vec_sol;
  B       = ksp->vec_rhs;
  R       = ksp->work[0];
  Z       = ksp->work[1];
  P       = ksp->work[2];

  if (eigs) {e = cgP->e; d = cgP->d; e[0] = 0.0; b = 0.0; }
  ierr = PCGetOperators(ksp->B,&Amat,&Pmat,&pflag); CHKERRQ(ierr);

  if (!ksp->guess_zero) {
    ierr = MatMult(Amat,X,R); CHKERRQ(ierr);       /*   r <- b - Ax       */
    ierr = VecAYPX(&mone,B,R); CHKERRQ(ierr);
  }
  else { 
    ierr = VecCopy(B,R); CHKERRQ(ierr);            /*     r <- b (x is 0) */
  }
  ierr = PCApply(ksp->B,R,Z); CHKERRQ(ierr);       /*     z <- Br         */
  if (pres) {
      ierr = VecNorm(Z,NORM_2,&dp); CHKERRQ(ierr); /*    dp <- z'*z       */
  }
  else {
      ierr = VecNorm(R,NORM_2,&dp); CHKERRQ(ierr); /*    dp <- r'*r       */
  }
  cerr = (*ksp->converged)(ksp,0,dp,ksp->cnvP);
  if (cerr) {*its =  0; return 0;}
  MONITOR(ksp,dp,0);
  if (history) history[0] = dp;

  for ( i=0; i<maxit; i++) {
     ierr = VecDot(R,Z,&beta); CHKERRQ(ierr);      /*     beta <- r'z     */
     if (i == 0) {
       if (beta == 0.0) break;
       ierr = VecCopy(Z,P); CHKERRQ(ierr);         /*     p <- z          */
     }
     else {
         b = beta/betaold;
#if !defined(PETSC_COMPLEX)
         if (b<0.0) SETERRQ(1,"KSPSolve_CG:Nonsymmetric/bad preconditioner");
#endif
         if (eigs) {
           e[i] = sqrt(b)/a;  
         }
         ierr = VecAYPX(&b,Z,P); CHKERRQ(ierr);    /*     p <- z + b* p   */
     }
     betaold = beta;
     ierr = MatMult(Amat,P,Z); CHKERRQ(ierr);      /*     z <- Kp         */
     ierr = VecDot(P,Z,&dpi); CHKERRQ(ierr);
     a = beta/dpi;                                 /*     a = beta/p'z    */
     if (eigs) {
       d[i] = sqrt(b)*e[i] + 1.0/a;
     }
     ierr = VecAXPY(&a,P,X); CHKERRQ(ierr);        /*     x <- x + ap     */
     ma = -a; VecAXPY(&ma,Z,R);                    /*     r <- r - az     */
     if (pres) {
       ierr = PCApply(ksp->B,R,Z); CHKERRQ(ierr);    /*     z <- Br         */
       ierr = VecNorm(Z,NORM_2,&dp); CHKERRQ(ierr);  /*    dp <- z'*z       */
     }
     else {
       ierr = VecNorm(R,NORM_2,&dp); CHKERRQ(ierr);  /*    dp <- r'*r       */
     }
     if (history && hist_len > i + 1) history[i+1] = dp;
     MONITOR(ksp,dp,i+1);
     cerr = (*ksp->converged)(ksp,i+1,dp,ksp->cnvP);
     if (cerr) break;
     if (!pres) 
      {ierr = PCApply(ksp->B,R,Z); CHKERRQ(ierr);} /*     z <- Br         */
  }
  if (i == maxit) i--;
  if (history) ksp->res_act_size = (hist_len < i + 1) ? hist_len : i + 1;
  if (cerr <= 0) *its = -(i+1);
  else           *its = i+1;
  return 0;
}

int KSPDestroy_CG(PetscObject obj)
{
  KSP    ksp = (KSP) obj;
  KSP_CG *cgP = (KSP_CG *) ksp->data;

  /* free space used for eigenvalue calculations */
  if ( ksp->calc_eigs ) {
    PetscFree(cgP->e);
  }

  KSPiDefaultFreeWork( ksp );
  
  /* free the context variables */
  PetscFree(cgP); 
  return 0;
}

int KSPCreate_CG(KSP ksp)
{
  KSP_CG *cgP = (KSP_CG*) PetscMalloc(sizeof(KSP_CG));  CHKPTRQ(cgP);

  PLogObjectMemory(ksp,sizeof(KSP_CG));
  ksp->data                 = (void *) cgP;
  ksp->type                 = KSPCG;
  ksp->pc_side              = PC_LEFT;
  ksp->calc_res             = 1;
  ksp->setup                = KSPSetUp_CG;
  ksp->solver               = KSPSolve_CG;
  ksp->adjustwork           = KSPiDefaultAdjustWork;
  ksp->destroy              = KSPDestroy_CG;
  ksp->converged            = KSPDefaultConverged;
  ksp->buildsolution        = KSPDefaultBuildSolution;
  ksp->buildresidual        = KSPDefaultBuildResidual;
  ksp->view                 = 0;
  return 0;
}





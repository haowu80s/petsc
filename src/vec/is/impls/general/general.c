#define PETSCVEC_DLL
/*
     Provides the functions for index sets (IS) defined by a list of integers.
*/
#include "../src/vec/is/impls/general/general.h" /*I  "petscis.h"  I*/
#include "petscvec.h"

#undef __FUNCT__  
#define __FUNCT__ "ISDuplicate_General" 
PetscErrorCode ISDuplicate_General(IS is,IS *newIS)
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General *)is->data;

  PetscFunctionBegin;
  ierr = ISCreateGeneral(((PetscObject)is)->comm,sub->n,sub->idx,newIS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISDestroy_General" 
PetscErrorCode ISDestroy_General(IS is)
{
  IS_General     *is_general = (IS_General*)is->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (is_general->allocated) {
    ierr = PetscFree(is_general->idx);CHKERRQ(ierr);
  }
  ierr = PetscFree(is_general);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISIdentity_General" 
PetscErrorCode ISIdentity_General(IS is,PetscBool  *ident)
{
  IS_General *is_general = (IS_General*)is->data;
  PetscInt   i,n = is_general->n,*idx = is_general->idx;

  PetscFunctionBegin;
  is->isidentity = PETSC_TRUE;
  *ident         = PETSC_TRUE;
  for (i=0; i<n; i++) {
    if (idx[i] != i) {
      is->isidentity = PETSC_FALSE;
      *ident         = PETSC_FALSE;
      break;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISCopy_General"
static PetscErrorCode ISCopy_General(IS is,IS isy)
{
  IS_General *is_general = (IS_General*)is->data,*isy_general = (IS_General*)isy->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (is_general->n != isy_general->n || is_general->N != isy_general->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Index sets incompatible");
  isy_general->sorted = is_general->sorted;
  ierr = PetscMemcpy(isy_general->idx,is_general->idx,is_general->n*sizeof(PetscInt));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISGetIndices_General" 
PetscErrorCode ISGetIndices_General(IS in,const PetscInt *idx[])
{
  IS_General *sub = (IS_General*)in->data;

  PetscFunctionBegin;
  *idx = sub->idx; 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISRestoreIndices_General" 
PetscErrorCode ISRestoreIndices_General(IS in,const PetscInt *idx[])
{
  IS_General *sub = (IS_General*)in->data;

  PetscFunctionBegin;
  if (*idx != sub->idx) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Must restore with value from ISGetIndices()");
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISGetSize_General" 
PetscErrorCode ISGetSize_General(IS is,PetscInt *size)
{
  IS_General *sub = (IS_General *)is->data;

  PetscFunctionBegin;
  *size = sub->N; 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISGetLocalSize_General" 
PetscErrorCode ISGetLocalSize_General(IS is,PetscInt *size)
{
  IS_General *sub = (IS_General *)is->data;

  PetscFunctionBegin;
  *size = sub->n; 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISInvertPermutation_General" 
PetscErrorCode ISInvertPermutation_General(IS is,PetscInt nlocal,IS *isout)
{
  IS_General     *sub = (IS_General *)is->data;
  PetscInt       i,*ii,n = sub->n,nstart;
  const PetscInt *idx = sub->idx;
  PetscMPIInt    size;
  IS             istmp,nistmp;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(((PetscObject)is)->comm,&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = PetscMalloc(n*sizeof(PetscInt),&ii);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      ii[idx[i]] = i;
    }
    ierr = ISCreateGeneral(PETSC_COMM_SELF,n,ii,isout);CHKERRQ(ierr);
    ierr = ISSetPermutation(*isout);CHKERRQ(ierr);
    ierr = PetscFree(ii);CHKERRQ(ierr);
  } else {
    /* crude, nonscalable get entire IS on each processor */
    if (nlocal == PETSC_DECIDE) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Do not yet support nlocal of PETSC_DECIDE");
    ierr = ISAllGather(is,&istmp);CHKERRQ(ierr);
    ierr = ISSetPermutation(istmp);CHKERRQ(ierr);
    ierr = ISInvertPermutation(istmp,PETSC_DECIDE,&nistmp);CHKERRQ(ierr);
    ierr = ISDestroy(istmp);CHKERRQ(ierr);
    /* get the part we need */
    ierr    = MPI_Scan(&nlocal,&nstart,1,MPIU_INT,MPI_SUM,((PetscObject)is)->comm);CHKERRQ(ierr);
#if defined(PETSC_USE_DEBUG)
    {
      PetscMPIInt rank;
      ierr = MPI_Comm_rank(((PetscObject)is)->comm,&rank);CHKERRQ(ierr);
      if (rank == size-1) {
        if (nstart != sub->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Sum of nlocal lengths %d != total IS length %d",nstart,sub->N);
      }
    }
#endif
    nstart -= nlocal;
    ierr    = ISGetIndices(nistmp,&idx);CHKERRQ(ierr);
    ierr    = ISCreateGeneral(((PetscObject)is)->comm,nlocal,idx+nstart,isout);CHKERRQ(ierr);    
    ierr    = ISRestoreIndices(nistmp,&idx);CHKERRQ(ierr);
    ierr    = ISDestroy(nistmp);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISView_General" 
PetscErrorCode ISView_General(IS is,PetscViewer viewer)
{
  IS_General     *sub = (IS_General *)is->data;
  PetscErrorCode ierr;
  PetscInt       i,n = sub->n,*idx = sub->idx;
  PetscBool      iascii;

  PetscFunctionBegin;
  ierr = PetscTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    MPI_Comm    comm;
    PetscMPIInt rank,size;

    ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
    ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

    if (size > 1) {
      if (is->isperm) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] Index set is permutation\n",rank);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] Number of indices in set %D\n",rank,n);CHKERRQ(ierr);
      for (i=0; i<n; i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] %D %D\n",rank,i,idx[i]);CHKERRQ(ierr);
      }
    } else {
      if (is->isperm) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Index set is permutation\n");CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Number of indices in set %D\n",n);CHKERRQ(ierr);
      for (i=0; i<n; i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%D %D\n",i,idx[i]);CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"Viewer type %s not supported for this object",((PetscObject)viewer)->type_name);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISSort_General" 
PetscErrorCode ISSort_General(IS is)
{
  IS_General     *sub = (IS_General *)is->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (sub->sorted) PetscFunctionReturn(0);
  ierr = PetscSortInt(sub->n,sub->idx);CHKERRQ(ierr);
  sub->sorted = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISSorted_General" 
PetscErrorCode ISSorted_General(IS is,PetscBool  *flg)
{
  IS_General *sub = (IS_General *)is->data;

  PetscFunctionBegin;
  *flg = sub->sorted;
  PetscFunctionReturn(0);
}

static struct _ISOps myops = { ISGetSize_General,
                               ISGetLocalSize_General,
                               ISGetIndices_General,
                               ISRestoreIndices_General,
                               ISInvertPermutation_General,
                               ISSort_General,
                               ISSorted_General,
                               ISDuplicate_General,
                               ISDestroy_General,
                               ISView_General,
                               ISIdentity_General,
                               ISCopy_General };

#undef __FUNCT__  
#define __FUNCT__ "ISCreateGeneral_Private" 
PetscErrorCode ISCreateGeneral_Private(IS is)
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;
  PetscInt       n = sub->n,i,min,max;
  const PetscInt *idx = sub->idx;
  PetscBool      sorted = PETSC_TRUE;
  PetscBool      flg = PETSC_FALSE;

  PetscFunctionBegin;
  ierr = MPI_Allreduce(&n,&sub->N,1,MPIU_INT,MPI_SUM,((PetscObject)is)->comm);CHKERRQ(ierr);
  for (i=1; i<n; i++) {
    if (idx[i] < idx[i-1]) {sorted = PETSC_FALSE; break;}
  }
  if (n) {min = max = idx[0];} else {min = max = 0;}
  for (i=1; i<n; i++) {
    if (idx[i] < min) min = idx[i];
    if (idx[i] > max) max = idx[i];
  }
  sub->sorted = sorted;
  is->min     = min;
  is->max     = max;
  ierr = PetscMemcpy(is->ops,&myops,sizeof(myops));CHKERRQ(ierr);
  is->isperm     = PETSC_FALSE;
  is->isidentity = PETSC_FALSE;
  ierr = PetscOptionsGetTruth(PETSC_NULL,"-is_view",&flg,PETSC_NULL);CHKERRQ(ierr);
  if (flg) {
    PetscViewer viewer;
    ierr = PetscViewerASCIIGetStdout(((PetscObject)is)->comm,&viewer);CHKERRQ(ierr);
    ierr = ISView(is,viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISCreateGeneral" 
/*@
   ISCreateGeneral - Creates a data structure for an index set 
   containing a list of integers.

   Collective on MPI_Comm

   Input Parameters:
+  comm - the MPI communicator
.  n - the length of the index set
-  idx - the list of integers

   Output Parameter:
.  is - the new index set

   Notes:
   The index array is copied to internally allocated storage. After the call,
   the user can free the index array. Use ISCreateGeneralNC() to use the pointers
   passed in and NOT make a copy of the index array.

   When the communicator is not MPI_COMM_SELF, the operations on IS are NOT
   conceptually the same as MPI_Group operations. The IS are then
   distributed sets of indices and thus certain operations on them are
   collective.

   ISCreateGeneral() allocates space for the list and indices and copies that list over. ISCreateGeneralNC() does not copy the list over,
   instead keeps the pointer to the list and frees the list when the ISDestroy() is called. ISCreateGeneralWithArray() is the 
   same as ISCreateGeneralNC() except it does NOT free the list when ISDestroy() is called.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneralWithArray(), ISCreateStride(), ISCreateBlock(), ISAllGather(), ISCreateGeneralNC()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISCreateGeneral(MPI_Comm comm,PetscInt n,const PetscInt idx[],IS *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = ISCreate(comm,is);CHKERRQ(ierr);
  ierr = ISSetType(*is,ISGENERAL);CHKERRQ(ierr);
  ierr = ISGeneralSetIndices(*is,n,idx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISGeneralSetIndices" 
/*@
   ISGeneralSetIndices - Sets the indices for an ISGENERAL index set

   Collective on IS

   Input Parameters:
+  is - the index set
.  n - the length of the index set
-  idx - the list of integers


   Notes:
   The index array is copied to internally allocated storage. After the call,
   the user can free the index array. Use ISCreateGeneralNC() or ISGeneralSetIndicesNC() to use the pointers
   passed in and NOT make a copy of the index array.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneralWithArray(), ISCreateStride(), ISCreateBlock(), ISAllGather(), ISCreateGeneralNC()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISGeneralSetIndices(IS is,PetscInt n,const PetscInt idx[])
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;

  PetscFunctionBegin;
  if (n < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"length < 0");
  if (n) {PetscValidIntPointer(idx,3);}

  ierr           = PetscMalloc(n*sizeof(PetscInt),&sub->idx);CHKERRQ(ierr);
  ierr           = PetscLogObjectMemory(is,n*sizeof(PetscInt));CHKERRQ(ierr);
  ierr           = PetscMemcpy(sub->idx,idx,n*sizeof(PetscInt));CHKERRQ(ierr);
  sub->n         = n;
  sub->allocated = PETSC_TRUE;
  ierr = ISCreateGeneral_Private(is); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISGeneralSetIndicesNC" 
/*@C
   ISGeneralSetIndicesNC - Sets the indices for an ISGENERAL index set reuses the index array passed in

   Collective on IS

   Input Parameters:
+  is - the index set
.  n - the length of the index set
-  idx - the list of integers


   Notes:
   The index array is not to internally allocated storage. After the call,
   the user can free the index array. Use ISCreateGeneralNC() or ISGeneralSetIndicesNC() to use the pointers
   passed in and NOT make a copy of the index array.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneralWithArray(), ISCreateStride(), ISCreateBlock(), ISAllGather(), ISCreateGeneralNC()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISGeneralSetIndicesNC(IS is,PetscInt n,const PetscInt idx[])
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;

  PetscFunctionBegin;
  if (n < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"length < 0");
  if (n) {PetscValidIntPointer(idx,3);}

  sub->idx      = (PetscInt*)idx;
  sub->n         = n;
  sub->allocated = PETSC_TRUE;
  ierr = ISCreateGeneral_Private(is); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISGeneralSetIndicesWithArray" 
/*@C
   ISGeneralSetIndicesWithArray - Sets the indices for an ISGENERAL index set reuses the index array passed in

   Collective on IS

   Input Parameters:
+  is - the index set
.  n - the length of the index set
-  idx - the list of integers


   Notes:
   The index array is not to internally allocated storage. After the call,
   the user can free the index array. Use ISCreateGeneral() or ISGeneralSetIndices() to copy the data in.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneralWithArray(), ISCreateStride(), ISCreateBlock(), ISAllGather(), ISCreateGeneralNC()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISGeneralSetIndicesWithArray(IS is,PetscInt n,const PetscInt idx[])
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;

  PetscFunctionBegin;
  if (n < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"length < 0");
  if (n) {PetscValidIntPointer(idx,3);}

  sub->idx      = (PetscInt*)idx;
  sub->n         = n;
  sub->allocated = PETSC_FALSE;
  ierr = ISCreateGeneral_Private(is); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "ISCreate_General" 
PetscErrorCode PETSCVEC_DLLEXPORT ISCreate_General(IS is)
{
  PetscErrorCode ierr;
  IS_General     *sub;

  PetscFunctionBegin;
  ierr       = PetscNewLog(is,IS_General,&sub);CHKERRQ(ierr);
  is->data   = (void*)sub;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "ISCreateGeneralNC"
/*@C
   ISCreateGeneralNC - Creates a data structure for an index set 
   containing a list of integers.

   Collective on IS

   Input Parameters:
+  comm - the MPI communicator
.  n - the length of the index set
-  idx - the list of integers, must be obtained with PetscMalloc()

   Output Parameter:
.  is - the new index set

   Notes: This routine does not copy the indices, just keeps the pointer to the
   indices. The ISDestroy() will free the space so it must be obtained
   with PetscMalloc() and it must not be freed nor modified elsewhere.
   Use ISCreateGeneral() if you wish to copy the indices passed into the routine.
   Use ISCreateGeneralWithArray() to NOT copy the indices and NOT free the space when
   ISDestroy() is called.

   When the communicator is not MPI_COMM_SELF, the operations on IS are NOT
   conceptually the same as MPI_Group operations. The IS are then
   distributed sets of indices and thus certain operations on them are
   collective.

   ISCreateGeneral() allocates space for the list and indices and copies that list over. ISCreateGeneralNC() does not copy the list over,
   instead keeps the pointer to the list and frees the list when the ISDestroy() is called. ISCreateGeneralWithArray() is the 
   same as ISCreateGeneralNC() except it does NOT free the list when ISDestroy() is called.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneral(), ISCreateGeneralWithArray(), ISCreateStride(), ISCreateBlock(), ISAllGather()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISCreateGeneralNC(MPI_Comm comm,PetscInt n,const PetscInt idx[],IS *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = ISCreate(comm,is);CHKERRQ(ierr);
  ierr = ISSetType(*is,ISGENERAL);CHKERRQ(ierr);
  ierr = ISGeneralSetIndicesNC(*is,n,idx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISCreateGeneralWithArray"
/*@C
   ISCreateGeneralWithArray - Creates a data structure for an index set 
   containing a list of integers.

   Collective on MPI_Comm

   Input Parameters:
+  comm - the MPI communicator
.  n - the length of the index set
-  idx - the list of integers

   Output Parameter:
.  is - the new index set

   Notes:
   Unlike with ISCreateGeneral(), the indices are not copied to internally
   allocated storage. The user array is not freed by ISDestroy().

   When the communicator is not MPI_COMM_SELF, the operations on IS are NOT
   conceptually the same as MPI_Group operations. The IS are then
   distributed sets of indices and thus certain operations on them are collective.

   ISCreateGeneral() allocates space for the list and indices and copies that list over. ISCreateGeneralNC() does not copy the list over,
   instead keeps the pointer to the list and frees the list when the ISDestroy() is called. ISCreateGeneralWithArray() is the 
   same as ISCreateGeneralNC() except it does NOT free the list when ISDestroy() is called.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneral(), ISCreateStride(), ISCreateBlock(), ISAllGather(), ISCreateGeneralNC()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISCreateGeneralWithArray(MPI_Comm comm,PetscInt n,PetscInt idx[],IS *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = ISCreate(comm,is);CHKERRQ(ierr);
  ierr = ISSetType(*is,ISGENERAL);CHKERRQ(ierr);
  ierr = ISGeneralSetIndicesWithArray(*is,n,idx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}





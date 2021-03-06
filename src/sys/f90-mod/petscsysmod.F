#define PETSC_USE_FORTRAN_MODULES

        module petscsysdef
#include <petscconf.h>
#if defined(PETSC_HAVE_MPI_F90MODULE) || defined(PETSC_HAVE_MPIUNI)
        use mpi
#define PETSC_AVOID_MPIF_H
#endif
#include <petsc/finclude/petscsys.h>
#include <petsc/finclude/petscdraw.h>
#include <petsc/finclude/petscviewer.h>
#include <petsc/finclude/ftn-custom/petscsysdef.h90>
#include <petsc/finclude/petscdrawdef.h>
#include <petsc/finclude/ftn-custom/petscviewerdef.h90>
        end module

        module petscsys
        use petscsysdef
#include <petsc/finclude/petscsys.h90>
#include <petsc/finclude/petscviewer.h90>
        end module


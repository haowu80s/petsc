/*$Id: appview.c,v 1.3 2000/01/06 20:43:21 bsmith Exp bsmith $*/


/*
       Demonstrates how to draw two dimensional grids and solutions. See appctx.h
       for information on how this is to be used.
*/

#include "appctx.h"

#undef __FUNC__
#define __FUNC__ "AppCxtView"
int AppCtxView(Draw idraw,void *iappctx)
{
  AppCtx                 *appctx = (AppCtx *)iappctx;
  AppGrid                *grid = &appctx->grid;

  int                    cell_n,vertex_local_n,ncell = 4,*verts,nverts;

  /*
        These contain the  vertex lists in local numbering
  */ 
  int                    *cell_vertex;

  /* 
        These contain the global numbering for local objects
  */
  int                    *cell_global,*vertex_global;
  
  double                 *vertex_coords;

  PetscBT                vertex_boundary_flag;

  int                    ierr,i,rank,c,j,ijp;
  int                    ij;

  Draw                   drawglobal = appctx->view.drawglobal;
  Draw                   drawlocal = appctx->view.drawlocal;
  double                 xl,yl,xr,yr,xm,ym,xp,yp;
  char                   num[5];

  ierr = DrawCheckResizedWindow(drawglobal);CHKERRQ(ierr);
  ierr = DrawCheckResizedWindow(drawlocal);CHKERRQ(ierr);

  MPI_Comm_rank(appctx->comm,&rank); c = rank + 2;

  ierr = ISGetIndices(grid->cell_global,&cell_global);CHKERRQ(ierr);
  ierr = ISGetIndices(grid->vertex_global,&vertex_global);CHKERRQ(ierr);

  cell_n               = grid->cell_n;
  cell_vertex          = grid->cell_vertex;
  vertex_local_n             = grid->vertex_local_n;
  vertex_coords         = grid->vertex_coords;
  vertex_boundary_flag = grid->vertex_boundary_flag;

  ierr = ISGetSize(grid->vertex_boundary,&nverts);CHKERRQ(ierr);
  ierr = ISGetIndices(grid->vertex_boundary,&verts);CHKERRQ(ierr);

  /*
        Draw edges of local cells and number them
  */

    for (i=0; i<cell_n; i++) {
      xp = 0.0; yp = 0.0;
      xl = vertex_coords[2*cell_vertex[ncell*i]]; yl = vertex_coords[2*cell_vertex[ncell*i] + 1];
      for (j=0; j<ncell; j++) {
        ij = ncell*i + ((j+1) % ncell);
        xr = vertex_coords[2*cell_vertex[ij]]; yr = vertex_coords[2*cell_vertex[ij] + 1];
        ierr = DrawLine(drawglobal,xl,yl,xr,yr,c);CHKERRQ(ierr);
        ierr = DrawLine(drawlocal,xl,yl,xr,yr,DRAW_BLUE);CHKERRQ(ierr);
        xp += xl;         yp += yl;
        xl  = xr;         yl =  yr;
      }
        xp /= ncell; yp /= ncell;
        sprintf(num,"%d",i);
        ierr = DrawString(drawlocal,xp,yp,DRAW_GREEN,num);CHKERRQ(ierr);
        sprintf(num,"%d",cell_global[i]);
        ierr = DrawString(drawglobal,xp,yp,c,num);CHKERRQ(ierr);
    }


  /*
       Draws only boundary edges 
  */

    for (i=0; i<cell_n; i++) {
      xp = 0.0; yp = 0.0;
      xl  = vertex_coords[2*cell_vertex[ncell*i]]; yl = vertex_coords[2*cell_vertex[ncell*i] + 1];
      ijp = ncell*i;
      for (j=0; j<ncell; j++) {
        ij = ncell*i + ((j+1) % ncell);
        xr = vertex_coords[2*cell_vertex[ij]]; yr = vertex_coords[2*cell_vertex[ij] + 1];
        if (PetscBTLookup(vertex_boundary_flag,cell_vertex[ijp]) && PetscBTLookup(vertex_boundary_flag,cell_vertex[ij])) {
          ierr = DrawLine(drawglobal,xl,yl,xr,yr,c);CHKERRQ(ierr);
          ierr = DrawLine(drawlocal,xl,yl,xr,yr,DRAW_BLUE);CHKERRQ(ierr);
        }
        xp += xl;         yp += yl;
        xl  = xr;         yl =  yr;
        ijp = ij;
      }

        xp /= ncell; yp /= ncell;
        sprintf(num,"%d",i);
        ierr = DrawString(drawlocal,xp,yp,DRAW_GREEN,num);CHKERRQ(ierr);
        sprintf(num,"%d",cell_global[i]);
        ierr = DrawString(drawglobal,xp,yp,c,num);CHKERRQ(ierr);

  }

  /*
      Number vertices
  */

    for (i=0; i<vertex_local_n; i++) {
      xm = vertex_coords[2*i]; ym = vertex_coords[2*i + 1];
      ierr = DrawString(drawglobal,xm,ym,DRAW_BLUE,num);CHKERRQ(ierr);
      ierr = DrawPoint(drawglobal,xm,ym,DRAW_ORANGE);CHKERRQ(ierr);
      ierr = DrawPoint(drawlocal,xm,ym,DRAW_ORANGE);CHKERRQ(ierr);

        sprintf(num,"%d",i);
        ierr = DrawString(drawlocal,xm,ym,DRAW_BLUE,num);CHKERRQ(ierr);
        sprintf(num,"%d",vertex_global[i]);
        ierr = DrawString(drawglobal,xm,ym,DRAW_BLUE,num);CHKERRQ(ierr);
    }

    for (i=0; i<nverts; i++) {
      xm = vertex_coords[2*verts[i]]; ym = vertex_coords[2*verts[i] + 1];
      ierr = DrawPoint(drawglobal,xm,ym,DRAW_RED);CHKERRQ(ierr);
      ierr = DrawPoint(drawlocal,xm,ym,DRAW_RED);CHKERRQ(ierr);
    }

  ierr = DrawSynchronizedFlush(drawglobal);CHKERRQ(ierr);
  ierr = DrawSynchronizedFlush(drawlocal);CHKERRQ(ierr);

  ierr = ISRestoreIndices(grid->vertex_boundary,&verts);CHKERRQ(ierr);
  ierr = ISRestoreIndices(grid->cell_global,&cell_global);CHKERRQ(ierr);
  ierr = ISRestoreIndices(grid->vertex_global,&vertex_global);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}


/* ----------------------------------------------------------------------- */
/*
   AppCtxViewMatlab - Views solution using Matlab via socket connections.

   Input Parameter:
   appctx - user-defined application context

   Note:
   See the companion Matlab file mscript.m for usage instructions.
*/
#undef __FUNC__
#define __FUNC__ "AppCxtViewMatlab"
int AppCtxViewMatlab(AppCtx* appctx)
{
  int    ierr,*cell_vertex,rstart,rend;
  Viewer viewer = VIEWER_SOCKET_WORLD;
  double *vertex_coords;
  IS     isvertex;

  PetscFunctionBegin;

  /* First, send solution vector to Matlab */
  ierr = VecView(appctx->algebra.g,viewer);CHKERRQ(ierr);

  /* Next, send vertices to Matlab */
  ierr = AODataKeyGetOwnershipRange(appctx->aodata,"vertex",&rstart,&rend);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_WORLD,rend-rstart,rstart,1,&isvertex);CHKERRQ(ierr);
  ierr = AODataSegmentGetIS(appctx->aodata,"vertex","values",isvertex,(void **)&vertex_coords);CHKERRQ(ierr);
  ierr = PetscDoubleView(2*(rend-rstart),vertex_coords,viewer);CHKERRQ(ierr);
  ierr = AODataSegmentRestoreIS(appctx->aodata,"vertex","values",PETSC_NULL,(void **)&vertex_coords);CHKERRQ(ierr);
  ierr = ISDestroy(isvertex);CHKERRQ(ierr);

  /* 
     Send list of vertices for each cell; these MUST be in the global (not local!) numbering); 
     this cannot use appctx->grid->cell_vertex 
  */
  ierr = AODataSegmentGetIS(appctx->aodata,"cell","vertex",appctx->grid.cell_global,(void **)&cell_vertex);CHKERRQ(ierr);
  ierr = PetscIntView(4*appctx->grid.cell_n,cell_vertex,viewer);CHKERRQ(ierr);
  ierr = AODataSegmentRestoreIS(appctx->aodata,"cell","vertex",PETSC_NULL,(void **)&cell_vertex);CHKERRQ(ierr);
  
  PetscFunctionReturn(0);
}

#undef __FUNC__
#define __FUNC__ "AppCxtViewSolution"
int AppCtxViewSolution(Draw idraw,void *iappctx)
{
  AppCtx                 *appctx = (AppCtx *)iappctx;
  AppGrid                *grid = &appctx->grid;
  AppAlgebra             *algebra = &appctx->algebra;
  int                    cell_n,ncell = 4;

  /*
        These contain the vertex lists in local numbering
  */ 
  int                    *cell_vertex;

  /* 
        These contain the global numbering for local objects
  */
  int                    *cell_global,*vertex_global;
  
  double                 *vertex_coords;


  int                    ierr,i;

  Draw                   drawglobal = appctx->view.drawglobal,popup;
  double                 x0,x1,x2,y_0,y_1,y2,vmin,vmax;
  int                    c0,c1,c2;
  Scalar                 *values;

  ierr = DrawCheckResizedWindow(drawglobal);CHKERRQ(ierr);

  cell_n        = grid->cell_n;
  ierr = ISGetIndices(grid->cell_global,&cell_global);CHKERRQ(ierr);
  ierr = ISGetIndices(grid->vertex_global,&vertex_global);CHKERRQ(ierr);

  cell_vertex   = grid->cell_vertex;
  vertex_coords  = grid->vertex_coords;

  ierr = VecMin(algebra->x,PETSC_NULL,&vmin);CHKERRQ(ierr);
  ierr = VecMax(algebra->x,PETSC_NULL,&vmax);CHKERRQ(ierr);
  
  ierr = VecCopy(algebra->w_local,appctx->algebra.x_local);CHKERRQ(ierr);
  ierr = VecContourScale(algebra->x_local,vmin,vmax);CHKERRQ(ierr);
  ierr = DrawGetPopup(drawglobal,&popup);CHKERRQ(ierr);
  if (popup) {ierr = DrawScalePopup(popup,vmin,vmax);CHKERRQ(ierr);}

  ierr = VecGetArray(algebra->x_local,&values);CHKERRQ(ierr);

  for (i=0; i<cell_n; i++) {
    x0 = vertex_coords[2*cell_vertex[ncell*i]];   y_0 = vertex_coords[2*cell_vertex[ncell*i] + 1];
    x1 = vertex_coords[2*cell_vertex[ncell*i+1]]; y_1 = vertex_coords[2*cell_vertex[ncell*i+1] + 1];
    x2 = vertex_coords[2*cell_vertex[ncell*i+2]]; y2 = vertex_coords[2*cell_vertex[ncell*i+2] + 1];
    c0 = (int)values[cell_vertex[ncell*i]];
    c1 = (int)values[cell_vertex[ncell*i+1]];
    c2 = (int)values[cell_vertex[ncell*i+2]];
    ierr = DrawTriangle(drawglobal,x0,y_0,x1,y_1,x2,y2,c0,c1,c2);CHKERRQ(ierr);
    x0 = vertex_coords[2*cell_vertex[ncell*i]];   y_0 = vertex_coords[2*cell_vertex[ncell*i] + 1];
    x1 = vertex_coords[2*cell_vertex[ncell*i+3]]; y_1 = vertex_coords[2*cell_vertex[ncell*i+3] + 1];
    x2 = vertex_coords[2*cell_vertex[ncell*i+2]]; y2 = vertex_coords[2*cell_vertex[ncell*i+2] + 1];
    c0 = (int)values[cell_vertex[ncell*i]];
    c1 = (int)values[cell_vertex[ncell*i+3]];
    c2 = (int)values[cell_vertex[ncell*i+2]];
    ierr = DrawTriangle(drawglobal,x0,y_0,x1,y_1,x2,y2,c0,c1,c2);CHKERRQ(ierr);
  }

  ierr = DrawSynchronizedFlush(drawglobal);CHKERRQ(ierr);

  ierr = VecRestoreArray(algebra->x_local,&values);CHKERRQ(ierr);
  ierr = ISRestoreIndices(grid->cell_global,&cell_global);CHKERRQ(ierr);
  ierr = ISRestoreIndices(grid->vertex_global,&vertex_global);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}



















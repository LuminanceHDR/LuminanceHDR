/**
 * @file pde.cpp
 * @brief Solving Partial Differential Equations
 *
 * Full Multigrid Algorithm and Successive Overrelaxation.
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * Some code from Numerical Recipes in C
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * $Id: pde.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include "pde.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/manip/copy.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/numeric.h"
#include "Libpfs/utils/sse.h"

#include "TonemappingOperators/pfstmo.h"

using namespace std;

//////////////////////////////////////////////////////////////////////

// tune the multi-level solver
#define MODYF 0 /* 1 or 0 (1 is better) */
#define MINS 16 /* minimum size 4 6 or 100 */
//#define MODYF_SQRT -1.0f /* -1 or 0 */
#define SMOOTH_IT 1   // orig: 1
#define BCG_STEPS 20  // orig: 20
#define BCG_TOL 1e-3  // orig: 1e-3
#define V_CYCLE 2     // orig: 2
// post improvement of the solution using additional cg-iterations
#define BCG_POST_IMPROVE false
#define BCG_POST_STEPS 1000  // very slow if > 100, only use on small image
#define BCG_POST_TOL 1e-7

// precision
#define EPS 1.0e-12

#define OMP_THRESHOLD 1000000

static void linbcg(unsigned long n, const float b[], float x[], float tol,
                   int itmax, int *iter, float *err, int rows, int cols);

inline float max(float a, float b) { return a > b ? a : b; }

inline float min(float a, float b) { return a < b ? a : b; }

//!! TODO: for debugging purposes
// #define PFSEOL "\x0a"
// static void dumpPFS( const char *fileName, const pfs::Array2D *data, const
// char *channelName )
// {
//   FILE *fh = fopen( fileName, "wb" );
//   assert( fh != NULL );

//   int width = data->getCols();
//   int height = data->getRows();

//   fprintf( fh, "PFS1" PFSEOL "%d %d" PFSEOL "1" PFSEOL "0" PFSEOL
//     "%s" PFSEOL "0" PFSEOL "ENDH", width, height, channelName );

//   for( int y = 0; y < height; y++ )
//     for( int x = 0; x < width; x++ ) {
//       fwrite( &((*data)(x,y)), sizeof( float ), 1, fh );
//     }

//   fclose( fh );
// }

//////////////////////////////////////////////////////////////////////
// Full Multigrid Algorithm for solving partial differential equations
//////////////////////////////////////////////////////////////////////

static void restrict(const pfs::Array2Df *in, pfs::Array2Df *out) {
    const float inRows = in->getRows();
    const float inCols = in->getCols();

    const int outRows = out->getRows();
    const int outCols = out->getCols();

    const float dx = (float)in->getCols() / (float)out->getCols();
    const float dy = (float)in->getRows() / (float)out->getRows();

    const float filterSize = 0.5;

    float sx, sy;
    int x, y;

    for (y = 0, sy = dy / 2 - 0.5; y < outRows; y++, sy += dy) {
        for (x = 0, sx = dx / 2 - 0.5; x < outCols; x++, sx += dx) {
            float pixVal = 0;
            float w = 0;
            for (float ix = max(0, ceilf(sx - dx * filterSize));
                 ix <= min(floorf(sx + dx * filterSize), inCols - 1); ix++)
                for (float iy = max(0, ceilf(sy - dx * filterSize));
                     iy <= min(floorf(sy + dx * filterSize), inRows - 1);
                     iy++) {
                    pixVal += (*in)((int)ix, (int)iy);
                    w += 1;
                }
            (*out)(x, y) = pixVal / w;
        }
    }
}

// from_level>to_level, from_size>to_size
// void restrict( pfs::Array2D *F, pfs::Array2D *T )
// {
// //   DEBUG_STR << "restrict" << endl;

//   int sxt = T->getCols();
//   int syt = T->getRows();
//   int sxf = F->getCols();
//   int syf = F->getRows();
//   int x,y;

//   float w[] = {.25, .5, .25};
//   for( int x=0 ; x<sxt ; x++ )
//     for( int y=0 ; y<syt ; y++ )
//     {
//       float sum=0.0; float norm=0.0f;
//       for( int m=-1 ; m<=1 ; m++ )
//     for( int n=-1 ; n<=1 ; n++ )
//         {
//           int xf = 2*x+m;
//           int yf = 2*y+n;
//           if( xf>=0 && yf>=0 && xf<sxf && yf<syf )
//           {
//             sum += w[1+m]*w[1+n] * (*F)(xf,yf);
//             norm += w[1+m]*w[1+n];
//           }
//         }
//       assert(norm!=0.0f);
//       (*T)(x,y)=sum/norm;
//     }
// }

static void prolongate(const pfs::Array2Df *in, pfs::Array2Df *out) {
    float dx = (float)in->getCols() / (float)out->getCols();
    float dy = (float)in->getRows() / (float)out->getRows();

    const int outRows = out->getRows();
    const int outCols = out->getCols();

    const float inRows = in->getRows();
    const float inCols = in->getCols();

    const float filterSize = 1;

    float sx, sy;
    int x, y;
    for (y = 0, sy = -dy / 2; y < outRows; y++, sy += dy)
        for (x = 0, sx = -dx / 2; x < outCols; x++, sx += dx) {
            float pixVal = 0;
            float weight = 0;

            for (float ix = max(0, ceilf(sx - filterSize));
                 ix <= min(floorf(sx + filterSize), inCols - 1); ix++)
                for (float iy = max(0, ceilf(sy - filterSize));
                     iy <= min(floorf(sy + filterSize), inRows - 1); iy++) {
                    float fx = fabs(sx - ix);
                    float fy = fabs(sy - iy);

                    const float fval = (1 - fx) * (1 - fy);

                    pixVal += (*in)((int)ix, (int)iy) * fval;
                    weight += fval;
                }

            assert(weight != 0);
            (*out)(x, y) = pixVal / weight;
        }
}

static void exact_sollution(pfs::Array2Df * /*F*/, pfs::Array2Df *U) {
    //   DEBUG_STR << "exact sollution" << endl;

    /*  int sx = F->getCols();
      int sy = F->getRows();
      int x,y;

      float h = 1.0/sqrt(sx*sy*1.0f);
      float h2 = h*h;
    */
    U->reset();

    /* also works well?? */
    return;

    //   if( sx==3 && sy==3 )
    //   {
    //     (*U)(1,1) = -h2* (*F)(1,1) / 4.0f;

    //     // boundary points
    //     for( x=0 ; x<sx ; x++ )
    //       (*U)(x,0) = (*U)(x,sy-1) = 0.0f;
    //     for( y=0 ; y<sy ; y++ )
    //       (*U)(0,y) = (*U)(sx-1,y) = 0.0f;
    //   }
    //   else
    //   {
    //     U.fill(0.0f); return;   /* also works well?? */

    //     // TODO: this produces incorrect results
    // //     solve_pde_sor(F,U);
    // //     for( y=0 ; y<sy ; y++ )
    // //       for( x=0 ; x<sx ; x++ )
    // //         (*U)(x,y) *= h2;
    //   }
}

// static int rows, cols;

// smooth u using f at level
static void smooth(pfs::Array2Df *U, const pfs::Array2Df *F) {
    //   DEBUG_STR << "smooth" << endl;

    int rows = U->getRows();
    int cols = U->getCols();

    const int n = rows * cols;

    int iter;
    float err;

    linbcg(n, F->data(), U->data(), BCG_TOL, BCG_STEPS, &iter, &err, rows,
           cols);

    //   fprintf( stderr, "." );

    // Gauss relaxation is too slow

    //   int sx = F->getCols();
    //   int sy = F->getRows();
    //   int x,y,i;
    //   int shx;    shift x

    //   float h = 1.0f/sqrt(sx*sy*1.0f);
    //   float h2 = h*h;

    //   h2 = 1;

    //   for( int pass=0 ; pass<2 ; pass++ )
    //   {
    //     shx=pass;
    //     for( y=0; y<sy ; y++ )
    //     {
    //       shx= (y+pass)%2;
    //       for( x=shx ; x<sx ; x+=2 )
    //       {
    //         int w, n, e, s;
    //         w = (x == 0 ? 0 : x-1);
    //         n = (y == 0 ? 0 : y-1);
    //         s = (y+1 == sy ? y : y+1);
    //         e = (x+1 == sx ? x : x+1);

    //     (*U)(x,y) = .25 * ( (*U)(e,y) + (*U)(w,y) + (*U)(x,s) + (*U)(x,n)
    //           - h2 * (*F)(x,y) );
    //       }
    //     }
    //   }
}

static void calculate_defect(pfs::Array2Df *D, const pfs::Array2Df *U,
                             const pfs::Array2Df *F) {
    //   DEBUG_STR << "calculate defect" << endl;

    int sx = F->getCols();
    int sy = F->getRows();

    // float h = 1.0f/sqrt(sx*sy*1.0f);
    // float h2i = 1.0/(h*h);

    // h2i = 1;

    for (int y = 0; y < sy; y++)
        for (int x = 0; x < sx; x++) {
            int w, n, e, s;
            w = (x == 0 ? 0 : x - 1);
            n = (y == 0 ? 0 : y - 1);
            s = (y + 1 == sy ? y : y + 1);
            e = (x + 1 == sx ? x : x + 1);

            (*D)(x, y) = (*F)(x, y) - ((*U)(e, y) + (*U)(w, y) + (*U)(x, n) +
                                       (*U)(x, s) - 4.0 * (*U)(x, y));
        }
}

static void add_correction(pfs::Array2Df *U, const pfs::Array2Df *C) {
    //   DEBUG_STR << "add_correction" << endl;

    int sx = C->getCols();
    int sy = C->getRows();

    for (int i = 0; i < sx * sy; i++) (*U)(i) += (*C)(i);
}

void solve_pde_multigrid(pfs::Array2Df *F, pfs::Array2Df *U,
                         pfs::Progress &ph) {
    int xmax = F->getCols();
    int ymax = F->getRows();

    int i;   // index for simple loops
    int k;   // index for iterating through levels
    int k2;  // index for iterating through levels in V-cycles

    // 1. restrict f to coarse-grid (by the way count the number of levels)
    //      k=0: fine-grid = f
    //      k=levels: coarsest-grid
    int levels = 0;
    int mins = (xmax < ymax) ? xmax : ymax;
    while (mins >= MINS) {
        levels++;
        mins = mins / 2 + MODYF;
    }

    // given function f restricted on levels
    pfs::Array2Df **RHS = new pfs::Array2Df *[levels + 1];

    // approximate initial sollutions on levels
    pfs::Array2Df **IU = new pfs::Array2Df *[levels + 1];
    // target functions in cycles (approximate sollution error (uh - ~uh) )
    pfs::Array2Df **VF = new pfs::Array2Df *[levels + 1];

    VF[0] = new pfs::Array2Df(xmax, ymax);
    RHS[0] = F;
    IU[0] = new pfs::Array2Df(xmax, ymax);
    pfs::copy(U, IU[0]);

    int sx = xmax;
    int sy = ymax;
    for (k = 0; k < levels; k++) {
        // calculate size of next level
        sx = sx / 2 + MODYF;
        sy = sy / 2 + MODYF;

        RHS[k + 1] = new pfs::Array2Df(sx, sy);
        IU[k + 1] = new pfs::Array2Df(sx, sy);
        VF[k + 1] = new pfs::Array2Df(sx, sy);

        // restrict from level k to level k+1 (coarser-grid)
        restrict(RHS[k], RHS[k + 1]);
    }

    // 2. find exact sollution at the coarsest-grid (k=levels)
    exact_sollution(RHS[levels], IU[levels]);

    // 3. nested iterations
    for (k = levels - 1; k >= 0; k--) {
        ph.setValue(20 + 70 * (levels - k) / (levels + 1));
        // 4. interpolate sollution from last coarse-grid to finer-grid
        // interpolate from level k+1 to level k (finer-grid)
        prolongate(IU[k + 1], IU[k]);

        // 4.1. first target function is the equation target function
        //      (following target functions are the defect)
        pfs::copy(RHS[k], VF[k]);

        // 5. V-cycle (twice repeated)
        for (int cycle = 0; cycle < V_CYCLE; cycle++) {
            // 6. downward stroke of V
            for (k2 = k; k2 < levels; k2++) {
                // 7. pre-smoothing of initial sollution using target function
                //    zero for initial guess at smoothing
                //    (except for level k when iu contains prolongated result)
                if (k2 != k) IU[k2]->reset();

                //        fprintf( stderr, "Level: %d --------\n", k2 );

                for (i = 0; i < SMOOTH_IT; i++) smooth(IU[k2], VF[k2]);

                // 8. calculate defect at level
                //    d[k2] = Lh * ~u[k2] - f[k2]
                pfs::Array2Df *D =
                    new pfs::Array2Df(IU[k2]->getCols(), IU[k2]->getRows());
                calculate_defect(D, IU[k2], VF[k2]);

                // 9. restrict deffect as target function for next coarser-grid
                //    def -> f[k2+1]
                restrict(D, VF[k2 + 1]);
                delete D;
            }

            // 10. solve on coarsest-grid (target function is the deffect)
            //     iu[levels] should contain sollution for
            //     the f[levels] - last deffect, iu will now be the correction
            exact_sollution(VF[levels], IU[levels]);

            // 11. upward stroke of V
            for (k2 = levels - 1; k2 >= k; k2--) {
                // 12. interpolate correction from last coarser-grid to
                // finer-grid
                //     iu[k2+1] -> cor
                pfs::Array2Df *C =
                    new pfs::Array2Df(IU[k2]->getCols(), IU[k2]->getRows());
                prolongate(IU[k2 + 1], C);

                // 13. add interpolated correction to initial sollution at level
                // k2
                add_correction(IU[k2], C);
                delete C;

                //        fprintf( stderr, "Level: %d --------\n", k2 );

                // 14. post-smoothing of current sollution using target function
                for (i = 0; i < SMOOTH_IT; i++) smooth(IU[k2], VF[k2]);
            }

        }  //--- end of V-cycle

    }  //--- end of nested iteration

    // 15. final sollution
    //     IU[0] contains the final sollution

    //   for( k=0 ; k<levels ; k++ ) {
    //     char name[200];
    //     sprintf( name, "u_%d.pfs", k );
    //     dumpPFS( name, IU[k], "Y" );
    //     sprintf( name, "rh_%d.pfs", k );
    //     dumpPFS( name, RHS[k], "Y" );
    //     sprintf( name, "v_%d.pfs", k );
    //     dumpPFS( name, VF[k], "Y" );
    //   }

    pfs::copy(IU[0], U);

    // further improvement of the solution
    if (BCG_POST_IMPROVE) {
        int iter;
        float err;
        // DEBUG_STR << "FMG: cg post improving ..., maxiter=" <<
        // BCG_POST_STEPS;
        // DEBUG_STR << ", tol=" << BCG_POST_TOL << std::endl;
        linbcg(xmax * ymax, F->data(), U->data(), BCG_POST_TOL, BCG_POST_STEPS,
               &iter, &err, ymax, xmax);
        // DEBUG_STR << "FMG: cg post improvement: iter=" << iter << ", err=" <<
        // err;
        // DEBUG_STR << std::endl;
    }

    ph.setValue(90);

    delete VF[0];
    delete IU[0];

    for (k = 1; k <= levels; k++) {
        delete RHS[k];
        delete IU[k];
        delete VF[k];
    }

    delete[] RHS;
    delete[] IU;
    delete[] VF;
}

//#define EPS 1.0e-14

static void asolve(const float b[], float x[], int rows, int cols) {
    for (int j = 0; j < rows * cols; j++) x[j] = -4 * b[j];
}

#define idx(R, C) ((R)*cols + (C))

static void atimes(const float x[], float res[], int rows, int cols) {
#pragma omp parallel for shared(x, res) if (rows *cols > OMP_THRESHOLD) \
                                                schedule(static)
    for (int r = 1; r < rows - 1; r++)
        for (int c = 1; c < cols - 1; c++) {
            res[idx(r, c)] = x[idx(r - 1, c)] + x[idx(r + 1, c)] +
                             x[idx(r, c - 1)] + x[idx(r, c + 1)] -
                             4 * x[idx(r, c)];
        }

    for (int r = 1; r < rows - 1; r++) {
        res[idx(r, 0)] = x[idx(r - 1, 0)] + x[idx(r + 1, 0)] + x[idx(r, 1)] -
                         3 * x[idx(r, 0)];
        res[idx(r, cols - 1)] = x[idx(r - 1, cols - 1)] +
                                x[idx(r + 1, cols - 1)] + x[idx(r, cols - 2)] -
                                3 * x[idx(r, cols - 1)];
    }

    for (int c = 1; c < cols - 1; c++) {
        res[idx(0, c)] = x[idx(1, c)] + x[idx(0, c - 1)] + x[idx(0, c + 1)] -
                         3 * x[idx(0, c)];
        res[idx(rows - 1, c)] = x[idx(rows - 2, c)] + x[idx(rows - 1, c - 1)] +
                                x[idx(rows - 1, c + 1)] -
                                3 * x[idx(rows - 1, c)];
    }
    res[idx(0, 0)] = x[idx(1, 0)] + x[idx(0, 1)] - 2 * x[idx(0, 0)];
    res[idx(rows - 1, 0)] =
        x[idx(rows - 2, 0)] + x[idx(rows - 1, 1)] - 2 * x[idx(rows - 1, 0)];
    res[idx(0, cols - 1)] =
        x[idx(1, cols - 1)] + x[idx(0, cols - 2)] - 2 * x[idx(0, cols - 1)];
    res[idx(rows - 1, cols - 1)] = x[idx(rows - 2, cols - 1)] +
                                   x[idx(rows - 1, cols - 2)] -
                                   2 * x[idx(rows - 1, cols - 1)];
}

#undef idx

static float snrm(unsigned long n, const float sx[]) {
    float ans = 0.0f;

#pragma omp parallel for shared(sx) \
    reduction(+ : ans) if (n > OMP_THRESHOLD) schedule(static)
    for (long i = 0; i < static_cast<long>(n); i++) {
        ans += sx[i] * sx[i];
    }
    return sqrt(ans);
}

/**
 * Biconjugate Gradient Method
 * from Numerical Recipes in C
 */
static void linbcg(unsigned long n, const float b[], float x[], float tol,
                   int itmax, int *iter, float *err, int rows, int cols) {
    float ak, akden, bk, bkden = 1.0, bknum, bnrm = 1.0;  // zm1nrm,znrm;
    float *p, *pp, *r, *rr, *z, *zz;

    p = new float[n + 1];
    pp = new float[n + 1];
    r = new float[n + 1];
    rr = new float[n + 1];
    z = new float[n + 1];
    zz = new float[n + 1];

    *iter = 0;
    atimes(x, r, rows, cols);
    for (unsigned long j = 0; j < n; j++) {
        r[j] = b[j] - r[j];
    }
    for (unsigned long j = 0; j < n; j++) {
        rr[j] = r[j];
    }
    atimes(r, rr, rows, cols);  // minimum residual
    // znrm=1.0;
    bnrm = snrm(n, b);
    asolve(r, z, rows, cols);

    while (*iter <= itmax) {
        ++(*iter);
        // zm1nrm=znrm;
        asolve(rr, zz, rows, cols);
        bknum = 0.0;
#pragma omp parallel for shared(z, rr) reduction( \
    + : bknum) if (n > OMP_THRESHOLD) schedule(static)
        for (long j = 0; j < static_cast<long>(n); j++) {
            bknum += z[j] * rr[j];
        }
        if (*iter == 1) {
            for (long j = 0; j < static_cast<long>(n); j++) {
                p[j] = z[j];
            }
            for (long j = 0; j < static_cast<long>(n); j++) {
                pp[j] = zz[j];
            }
        } else {
            bk = bknum / bkden;
            pfs::utils::vadds(z, bk, p, p, n);
            pfs::utils::vadds(zz, bk, pp, pp, n);
        }
        bkden = bknum;
        atimes(p, z, rows, cols);
        akden = 0.0;
#pragma omp parallel for shared(z, pp) reduction( \
    + : akden) if (n > OMP_THRESHOLD) schedule(static)
        for (long j = 0; j < static_cast<long>(n); j++) {
            akden += z[j] * pp[j];
        }
        ak = bknum / akden;
        atimes(pp, zz, rows, cols);
        pfs::utils::vadds(x, ak, p, x, n);
        pfs::utils::vsubs(r, ak, z, r, n);
        pfs::utils::vsubs(rr, ak, zz, rr, n);
        asolve(r, z, rows, cols);
        // znrm = 1.0f;
        *err = snrm(n, r) / bnrm;
        //        fprintf( stderr, "iter=%4d err=%12.6f\n",*iter,*err);
        if (*err <= tol) break;
    }

    delete[] p;
    delete[] pp;
    delete[] r;
    delete[] rr;
    delete[] z;
    delete[] zz;
}
//#undef EPS

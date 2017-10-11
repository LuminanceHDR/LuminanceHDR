/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Tino Kluge
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
 */

/**
 *
 * @file pde_fft.cpp
 * @brief Direct Poisson solver using the discrete cosine transform
 *
 * @author Tino Kluge (tino.kluge@hrz.tu-chemnitz.de)
 *
 */

//////////////////////////////////////////////////////////////////////
// Direct Poisson solver using the discrete cosine transform
//////////////////////////////////////////////////////////////////////
// by Tino Kluge (tino.kluge@hrz.tu-chemnitz.de)
//
// let U and F be matrices of order (n1,n2), ie n1=height, n2=width
// and L_x of order (n2,n2) and L_y of order (n1,n1) and both
// representing the 1d Laplace operator with Neumann boundary conditions,
// ie L_x and L_y are tridiagonal matrices of the form
//
//  ( -2  2          )
//  (  1 -2  1       )
//  (     .  .  .    )
//  (        1 -2  1 )
//  (           2 -2 )
//
// then this solver computes U given F based on the equation
//
//  -------------------------
//  L_y U + (L_x U^tr)^tr = F
//  -------------------------
//
// Note, if the first and last row of L_x and L_y contained one's instead of
// two's then this equation would be exactly the 2d Poisson equation with
// Neumann boundary conditions. As a simple rule:
// - Neumann: assume U(-1)=U(0) --> U(i-1) - 2 U(i) + U(i+1) becomes
//        i=0: U(0) - 2 U(0) + U(1) = -U(0) + U(1)
// - our system: assume U(-1)=U(1) --> this becomes
//        i=0: U(1) - 2(0) + U(1) = -2 U(0) + 2 U(1)
//
// The multi grid solver solve_pde_multigrid() solves the 2d Poisson pde
// with the right Neumann boundary conditions, U(-1)=U(0), see function
// atimes(). This means the assembly of the right hand side F is different
// for both solvers.

#include <iostream>

#include <boost/math/constants/constants.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include "arch/math.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <fftw3.h>
#include <vector>

#include <Common/init_fftw.h>
#include <Libpfs/array2d.h>
#include <Libpfs/progress.h>
#include "pde.h"

using namespace std;

#ifndef SQR
#define SQR(x) (x) * (x)
#endif

// returns T = EVy A EVx^tr
// note, modifies input data
void transform_ev2normal(pfs::Array2Df &A, pfs::Array2Df &T) {
    int width = A.getCols();
    int height = A.getRows();
    assert((int)T.getCols() == width && (int)T.getRows() == height);

    // the discrete cosine transform is not exactly the transform needed
    // need to scale input values to get the right transformation
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++) A(x, y) *= 0.25f;

    for (int x = 1; x < width - 1; x++) {
        A(x, 0) *= 0.5f;
        A(x, height - 1) *= 0.5f;
    }
    for (int y = 1; y < height - 1; y++) {
        A(0, y) *= 0.5;
        A(width - 1, y) *= 0.5f;
    }

    // note, fftw provides its own memory allocation routines which
    // ensure that memory is properly 16/32 byte aligned so it can
    // use SSE/AVX operations (2/4 double ops in parallel), if our
    // data is not properly aligned fftw won't use SSE/AVX
    // (I believe new() aligns memory to 16 byte so avoid overhead here)
    //
    // double* in = (double*) fftwf_malloc(sizeof(double) * width*height);
    // fftwf_free(in);

    // executes 2d discrete cosine transform
    fftwf_plan p;
    FFTW_MUTEX::fftw_mutex_plan.lock();
    p = fftwf_plan_r2r_2d(height, width, A.data(), T.data(), FFTW_REDFT00,
                          FFTW_REDFT00, FFTW_ESTIMATE);
    FFTW_MUTEX::fftw_mutex_plan.unlock();

    fftwf_execute(p);

    FFTW_MUTEX::fftw_mutex_destroy_plan.lock();
    fftwf_destroy_plan(p);
    FFTW_MUTEX::fftw_mutex_destroy_plan.unlock();
}

// returns T = EVy^-1 * A * (EVx^-1)^tr
void transform_normal2ev(pfs::Array2Df &A, pfs::Array2Df &T) {
    int width = A.getCols();
    int height = A.getRows();
    assert((int)T.getCols() == width && (int)T.getRows() == height);

    // executes 2d discrete cosine transform
    fftwf_plan p;
    FFTW_MUTEX::fftw_mutex_plan.lock();
    p = fftwf_plan_r2r_2d(height, width, A.data(), T.data(), FFTW_REDFT00,
                          FFTW_REDFT00, FFTW_ESTIMATE);
    FFTW_MUTEX::fftw_mutex_plan.unlock();

    fftwf_execute(p);

    FFTW_MUTEX::fftw_mutex_destroy_plan.lock();
    fftwf_destroy_plan(p);
    FFTW_MUTEX::fftw_mutex_destroy_plan.unlock();

    // need to scale the output matrix to get the right transform
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            T(x, y) *= (1.0f / ((height - 1) * (width - 1)));

    for (int x = 0; x < width; x++) {
        T(x, 0) *= 0.5f;
        T(x, height - 1) *= 0.5f;
    }
    for (int y = 0; y < height; y++) {
        T(0, y) *= 0.5f;
        T(width - 1, y) *= 0.5f;
    }
}

// returns the eigenvalues of the 1d laplace operator
std::vector<double> get_lambda(int n) {
    assert(n > 1);
    std::vector<double> v(n);
    for (int i = 0; i < n; i++) {
        v[i] = -4.0 * SQR(sin((double)i / (2 * (n - 1)) *
                              boost::math::double_constants::pi));
    }

    return v;
}

void make_compatible_boundary(pfs::Array2Df &F)
// makes boundary conditions compatible so that a solution exists
{
    int width = F.getCols();
    int height = F.getRows();

    double sum = 0.0;
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++) sum += F(x, y);

    for (int x = 1; x < width - 1; x++)
        sum += 0.5 * (F(x, 0) + F(x, height - 1));

    for (int y = 1; y < height - 1; y++)
        sum += 0.5 * (F(0, y) + F(width - 1, y));

    sum += 0.25 * (F(0, 0) + F(0, height - 1) + F(width - 1, 0) +
                   F(width - 1, height - 1));

    // DEBUG_STR << "compatible_boundary: int F = " << sum ;
    // DEBUG_STR << " (should be 0 to be solvable)" << std::endl;

    double add = -sum / (height + width - 3);
    // DEBUG_STR << "compatible_boundary: adjusting boundary by " << add <<
    // std::endl;
    for (int x = 0; x < width; x++) {
        F(x, 0) += add;
        F(x, height - 1) += add;
    }
    for (int y = 1; y < height - 1; y++) {
        F(0, y) += add;
        F(width - 1, y) += add;
    }
}

// solves Laplace U = F with Neumann boundary conditions
// if adjust_bound is true then boundary values in F are modified so that
// the equation has a solution, if adjust_bound is set to false then F is
// not modified and the equation might not have a solution but an
// approximate solution with a minimum error is then calculated
// double precision version
void solve_pde_fft(pfs::Array2Df &F, pfs::Array2Df &U, pfs::Progress &ph,
                   bool adjust_bound) {
    ph.setValue(20);
    // DEBUG_STR << "solve_pde_fft: solving Laplace U = F ..." << std::endl;
    int width = F.getCols();
    int height = F.getRows();
    assert((int)U.getCols() == width && (int)U.getRows() == height);

    // activate parallel execution of fft routines
    init_fftw();

    // in general there might not be a solution to the Poisson pde
    // with Neumann boundary conditions unless the boundary satisfies
    // an integral condition, this function modifies the boundary so that
    // the condition is exactly satisfied
    if (adjust_bound) {
        // DEBUG_STR << "solve_pde_fft: checking boundary conditions" <<
        // std::endl;
        make_compatible_boundary(F);
    }

    // transforms F into eigenvector space: Ftr =
    // DEBUG_STR << "solve_pde_fft: transform F to ev space (fft)" << std::endl;
    pfs::Array2Df F_tr(width, height);
    transform_normal2ev(F, F_tr);
    // TODO: F no longer needed so could release memory, but as it is an
    // input parameter we won't do that
    ph.setValue(50);
    if (ph.canceled()) {
        return;
    }

    // DEBUG_STR << "solve_pde_fft: F_tr(0,0) = " << F_tr(0,0);
    // DEBUG_STR << " (must be 0 for solution to exist)" << std::endl;

    // in the eigenvector space the solution is very simple
    // DEBUG_STR << "solve_pde_fft: solve in eigenvector space" << std::endl;
    pfs::Array2Df U_tr(width, height);
    std::vector<double> l1 = get_lambda(height);
    std::vector<double> l2 = get_lambda(width);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x == 0 && y == 0)
                U_tr(x, y) =
                    0.0;  // any value ok, only adds a const to the solution
            else
                U_tr(x, y) = F_tr(x, y) / (l1[y] + l2[x]);
        }
    }
    ph.setValue(55);

    // transforms U_tr back to the normal space
    // DEBUG_STR << "solve_pde_fft: transform U_tr to normal space (fft)" <<
    // std::endl;
    transform_ev2normal(U_tr, U);
    ph.setValue(85);

    // the solution U as calculated will satisfy something like int U = 0
    // since for any constant c, U-c is also a solution and we are mainly
    // working in the logspace of (0,1) data we prefer to have
    // a solution which has no positive values: U_new(x,y)=U(x,y)-max
    // (not really needed but good for numerics as we later take exp(U))
    // DEBUG_STR << "solve_pde_fft: removing constant from solution" <<
    // std::endl;
    double max = 0.0;
    for (int i = 0; i < width * height; i++)
        if (max < U(i)) max = U(i);

    for (int i = 0; i < width * height; i++) U(i) -= max;

    // fft parallel threads cleanup, better handled outside this function?
    // fftwf_cleanup_threads();

    ph.setValue(90);
    // DEBUG_STR << "solve_pde_fft: done" << std::endl;
}

// ---------------------------------------------------------------------
// the functions below are only for test purposes to check the accuracy
// of the pde solvers

// returns the norm of (Laplace U - F) of all interior points
// useful to compare solvers
float residual_pde(pfs::Array2Df &U, pfs::Array2Df &F) {
    int width = U.getCols();
    int height = U.getRows();
    assert((int)F.getCols() == width && (int)F.getRows() == height);

    double res = 0.0;
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++) {
            double laplace = -4.0 * U(x, y) + U(x - 1, y) + U(x + 1, y) +
                             U(x, y - 1) + U(x, y + 1);
            res += SQR(laplace - F(x, y));
        }
    return static_cast<float>(sqrt(res));
}

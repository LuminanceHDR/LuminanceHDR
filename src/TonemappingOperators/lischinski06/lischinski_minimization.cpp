/*
 * @brief Lischinski Tone Mapping Operator:
 *    "Interactive Local Adjustment of Tonal Values"
 *     by Dani Lischinski, Zeev Farbman, Matt Uyttendaele, Richard Szeliski
 *     in Proceedings of SIGGRAPH 2006
 *
 * This file is a part of LuminanceHDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2018 Franco Comida
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
 * @author Franco Comida, <fcomida@users.sourceforge.net>
 *
 * Code adapted for Luminance HDR from:
 *
 *  PICCANTE
 *  The hottest HDR imaging library!
 *  http://vcg.isti.cnr.it/piccante
 *  Copyright (C) 2014
 *  Visual Computing Laboratory - ISTI CNR
 *  http://vcg.isti.cnr.it
 *  First author: Francesco Banterle
 *
 */

#include <Eigen/Sparse>
#include <Eigen/src/SparseCore/SparseMatrix.h>

#include "Libpfs/manip/resize.h"
#include "lischinski_minimization.h"

using namespace std;
using namespace pfs;

inline float LischinskiFunction(float Lcur, float Lref, float param[2],
                                float LISCHINSKI_EPSILON = 0.0001f) {
    return -param[1] / (powf(fabsf(Lcur - Lref), param[0]) + LISCHINSKI_EPSILON);
}

void LischinskiMinimization(Array2Df &L_orig, Array2Df &g_orig, Array2Df &omega_orig, Array2Df &F,
                            float alpha, float lambda, float LISCHINSKI_EPSILON) {

    const int orig_width = L_orig.getCols();
    const int orig_height = L_orig.getRows();

    int xSize = 512;

    if (orig_width < 2*xSize)
        xSize = 256;

    const int width = xSize;
    const int height = (int)(orig_height * (float)xSize /
                      (float)orig_width);

    const int size = height * width;
    Array2Df L(width, height);
    Array2Df g(width, height);
    Array2Df omega(width, height);

    resize(&L_orig, &L, BilinearInterp);
    resize(&g_orig, &g, BilinearInterp);
    resize(&omega_orig, &omega, BilinearInterp);

    Eigen::VectorXd b, x;
    b = Eigen::VectorXd::Zero(size);

    std::vector< Eigen::Triplet< double > > tL;

    float param[2];
    param[0] = alpha;
    param[1] = lambda;

    for(int i = 0; i < height; i++) {
        int tmpInd = i * width;

        for(int j = 0; j < width; j++) {

            double sum = 0.0;
            double tmp;
            int indJ;
            int indI = tmpInd + j;
            float Lref = L(indI);

            b[indI] = omega(indI) * g(indI);

            if((i - 1) >= 0) {
                indJ = indI - width;
                tmp = LischinskiFunction(L(indJ), Lref, param, LISCHINSKI_EPSILON);
                tL.push_back(Eigen::Triplet< double > (indI, indJ, tmp));
                sum += tmp;
            }

            if((i + 1) < height) {
                indJ = indI + width;
                tmp = LischinskiFunction(L(indJ), Lref, param, LISCHINSKI_EPSILON);
                tL.push_back(Eigen::Triplet< double > (indI, indJ, tmp));

                sum += tmp;
            }

            if((j - 1) >= 0) {
                indJ = indI - 1;
                tmp = LischinskiFunction(L(indJ), Lref, param, LISCHINSKI_EPSILON);
                tL.push_back(Eigen::Triplet< double > (indI, indJ, tmp));
                sum += tmp;
            }

            if((j + 1) < width) {
                indJ = indI + 1;
                tmp = LischinskiFunction(L(indJ), Lref, param, LISCHINSKI_EPSILON);
                tL.push_back(Eigen::Triplet< double > (indI, indJ, tmp));
                sum += tmp;
            }

            tL.push_back(Eigen::Triplet< double >{indI, indI, omega(indI) - sum});
        }
    }

    Eigen::SparseMatrix<double> A = Eigen::SparseMatrix<double>(size, size);
    A.setFromTriplets(tL.begin(), tL.end());

    Eigen::SimplicialCholesky<Eigen::SparseMatrix<double> > solver(A);
    x = solver.solve(b);

    if(solver.info() != Eigen::Success) {
        #ifndef NDDEBUG
            printf("SOLVER FAILED!\n");
        #endif
        return;
    }

    Array2Df F_res(width, height);

#pragma omp parallel for
    for(int i = 0; i < height; i++) {
        int counter = i * width;

        for(int j = 0; j < width; j++) {
            F_res(j, i) =  float(x(counter + j));
        }
    }
    resize(&F_res, &F, BilinearInterp);
}

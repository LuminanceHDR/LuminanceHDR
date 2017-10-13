/**
 * @brief Display Adaptive TMO
 *
 * From:
 * Rafal Mantiuk, Scott Daly, Louis Kerofsky.
 * Display Adaptive Tone Mapping.
 * To appear in: ACM Transactions on Graphics (Proc. of SIGGRAPH'08) 27 (3)
 * http://www.mpi-inf.mpg.de/resources/hdr/datmo/
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: display_adaptive_tmo.cpp,v 1.19 2009/02/23 18:46:36 rafm Exp $
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "display_adaptive_tmo.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include "cqp/gsl_cqp.h"

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/sse.h"

#ifdef BRANCH_PREDICTION
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

// Computing Conditional Density takes about 90% of the time
#define PROGRESS_CDF 90

#define MIN_PHVAL 1e-8f  // Minimum value allowed in HDR images
#define MAX_PHVAL 1e8f   // Maximum value allowed in HDR images

void my_gsl_error_handler (const char * reason,
              const char * file,
              int line,
              int gsl_errno) {
    throw pfs::Exception(reason);
}

/**
 *  Simple RAII wrapper for gsl_matrix
 */
class auto_matrix {
    gsl_matrix *m;

   public:
    auto_matrix(gsl_matrix *m) : m(m) {}
    ~auto_matrix() { gsl_matrix_free(m); }
    operator gsl_matrix *() { return m; }
};

/**
 *  Simple RAII wrapper for gsl_vector
 */
class auto_vector {
    gsl_vector *v;

   public:
    auto_vector(gsl_vector *v) : v(v) {}
    ~auto_vector() { gsl_vector_free(v); }
    operator gsl_vector *() { return v; }
};

/**
 *  Simple RAII wrapper for gsl_cqpminimizer
 */
class auto_cqpminimizer {
    gsl_cqpminimizer *v;

   public:
    auto_cqpminimizer(gsl_cqpminimizer *v) : v(v) {}
    ~auto_cqpminimizer() { gsl_cqpminimizer_free(v); }
    operator gsl_cqpminimizer *() { return v; }
};

// =============== Tone-curve filtering ==============

datmoToneCurve::datmoToneCurve() : own_y_i(false), size(0), x_i(NULL), y_i(NULL) {}

datmoToneCurve::~datmoToneCurve() { free(); }

void datmoToneCurve::init(size_t n_size, const double *n_x_i, double *n_y_i) {
    free();

    size = n_size;
    x_i = n_x_i;
    if (n_y_i == NULL) {
        y_i = new double[size];
        own_y_i = true;
    } else {
        y_i = n_y_i;
        own_y_i = false;
    }
}

void datmoToneCurve::free() {
    if (y_i != NULL && own_y_i) delete[] y_i;
}

// =============== Utils ==============

#define round_int(x) (int)((x) + 0.5)
#define sign(x) ((x) < 0 ? -1 : 1)

static inline float safe_log10(float x, const float min_x = MIN_PHVAL,
                               const float max_x = MAX_PHVAL) {
    if (x < min_x)
        return log10(min_x);
    else if (x > max_x)
        return log10(max_x);
    else
        return log10(x);
}

#ifdef LUMINANCE_USE_SSE

#define LOG2_10 3.3219280948874f
#define LOG2_10__1 (1.0f / LOG2_10)
static inline v4sf safe_log10(v4sf x, const float min_x = MIN_PHVAL,
                              const float max_x = MAX_PHVAL) {
    x = _mm_max_ps(x, _mm_set1_ps(min_x));
    x = _mm_min_ps(x, _mm_set1_ps(max_x));
    return _mm_log2_ps(x) * _mm_set1_ps(LOG2_10__1);
}

#endif

/**
 * Find the lowest non-zero value. Used to avoid log10(0).
 */
static float min_positive(const float *x, size_t len) {
    float min_val = MAX_PHVAL;
    for (size_t k = 0; k < len; k++)
        if (unlikely(x[k] < min_val && x[k] > 0)) min_val = x[k];

    return min_val;
}

static void mult_rows(const gsl_matrix *A, const gsl_vector *b, gsl_matrix *C) {
    assert(A->size1 == b->size);
    for (size_t j = 0; j < A->size2; j++)
        for (size_t i = 0; i < A->size1; i++)
            gsl_matrix_set(C, i, j,
                           gsl_matrix_get(A, i, j) * gsl_vector_get(b, i));
}

/**
 * Lookup table on a uniform array & interpolation
 *
 * x_i must be at least two elements
 * y_i must be initialized after creating an object
 */
class UniformArrayLUT {
    const double *x_i;
    size_t lut_size;
    double delta;

    bool own_y_i;

   public:
    double *y_i;

    UniformArrayLUT(size_t lut_size, const double *x_i, double *y_i = NULL)
        : x_i(x_i), lut_size(lut_size), delta(x_i[1] - x_i[0]) {
        if (y_i == NULL) {
            this->y_i = new double[lut_size];
            own_y_i = true;
        } else {
            this->y_i = y_i;
            own_y_i = false;
        }
    }

    UniformArrayLUT() : x_i(0), lut_size(0), delta(0.), own_y_i(0), y_i(0) {}

    UniformArrayLUT(const UniformArrayLUT &other)
        : x_i(other.x_i), lut_size(other.lut_size), delta(other.delta) {
        this->y_i = new double[lut_size];
        own_y_i = true;
        memcpy(this->y_i, other.y_i, lut_size * sizeof(double));
    }

    UniformArrayLUT &operator=(const UniformArrayLUT &other) {
        if (&other != this) {
            this->lut_size = other.lut_size;
            this->delta = other.delta;
            this->x_i = other.x_i;
            this->y_i = new double[lut_size];
            own_y_i = true;
            memcpy(this->y_i, other.y_i, lut_size * sizeof(double));
        }

        return *this;
    }

    ~UniformArrayLUT() {
        if (own_y_i) delete[] y_i;
    }

    double interp(double x) {
        const double ind_f = (x - x_i[0]) / delta;
        const size_t ind_low = (size_t)(ind_f);
        const size_t ind_hi = (size_t)ceil(ind_f);

        if (unlikely(ind_f < 0))  // Out of range checks
            return y_i[0];
        if (unlikely(ind_hi >= lut_size)) return y_i[lut_size - 1];

        if (unlikely(ind_low == ind_hi))
            return y_i[ind_low];  // No interpolation necessary

        return y_i[ind_low] +
               (y_i[ind_hi] - y_i[ind_low]) *
                   (ind_f - (double)ind_low);  // Interpolation
    }
};

#ifdef DEBUG
#define PFSEOL "\x0a"
static void dumpPFS(const char *fileName, const int width, const int height,
                    float *data, const char *channelName) {
    FILE *fh = fopen(fileName, "wb");
    assert(fh != NULL);

    fprintf(fh, "PFS1" PFSEOL "%d %d" PFSEOL "1" PFSEOL "0" PFSEOL "%s" PFSEOL
                "0" PFSEOL "ENDH",
            width, height, channelName);

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            fwrite(&data[x + y * width], sizeof(float), 1, fh);
        }

    fclose(fh);
}
#endif

static void compute_gaussian_level(const int width, const int height,
                                   const pfs::Array2Df &in, pfs::Array2Df &out,
                                   int level, pfs::Array2Df &temp) {
    const float kernel_a = 0.4f;

    const int kernel_len = 5;
    const int kernel_len_2 = kernel_len / 2;
    const float kernel[kernel_len] = {0.25f - kernel_a / 2.f, 0.25f, kernel_a,
                                      0.25f, 0.25f - kernel_a / 2.f};

    const int step = 1 << level;

    // FIXME: without the following (with using operator () ) there
    // seems to be a performance drop - need to investigate if gcc can
    // be made to optimize this better.  If this can't be made faster
    // with using the overloaded operator then this optimization should
    // be applied to the other TMOs as well.
    const float *in_raw = in.data();
    float *temp_raw = temp.data();
    float *out_raw = out.data();

// Filter rows
#pragma omp parallel for default(none) shared(in_raw, temp_raw, kernel)
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            float sum = 0;
            for (int j = 0; j < kernel_len; j++) {
                int l = (j - kernel_len_2) * step + c;
                if (unlikely(l < 0)) l = -l;
                if (unlikely(l >= width)) l = 2 * width - 2 - l;
                sum += in_raw[r * width + l] * kernel[j];
            }
            temp_raw[r * width + c] = sum;
        }
    }

// Filter columns
#pragma omp parallel for default(none) shared(temp_raw, out_raw, kernel)
    for (int c = 0; c < width; c++) {
        for (int r = 0; r < height; r++) {
            float sum = 0;
            for (int j = 0; j < kernel_len; j++) {
                int l = (j - kernel_len_2) * step + r;
                if (unlikely(l < 0)) l = -l;
                if (unlikely(l >= height)) l = 2 * height - 2 - l;
                sum += temp_raw[l * width + c] * kernel[j];
            }
            out_raw[r * width + c] = sum;
        }
    }
}

static inline float clamp_channel(const float v) {
    return (v > MIN_PHVAL ? v : MIN_PHVAL);
}

/** Compute conditional probability density function
 */

// round_int( (l_max-l_min)/delta ) + 1;
#define X_COUNT (round_int((8.f + 8.f) / 0.1) + 1)

class conditional_density : public datmoConditionalDensity {
   public:
    static const double l_min, l_max, delta;
    static double x_scale[X_COUNT];  // input log luminance scale
    double *g_scale;                 // contrast scale
    double *f_scale;                 // frequency scale

    const double g_max;

    double total;

    int x_count, g_count, f_count;  // Number of elements

    double *C;  // Conditional probability function

    conditional_density(const float pix_per_deg = 30.f) : g_max(0.7f), total(0) {
        x_count = X_COUNT;
        g_count = round_int(g_max / delta) * 2 + 1;

        // Find freq. band < 3 cyc per deg
        int f;
        for (f = 0; f < 8; f++) {
            float b_freq = 0.5f * pix_per_deg / (float)(1 << f);
            if (b_freq <= 3) break;
        }
        f_count = f + 1;

        g_scale = new double[g_count];
        f_scale = new double[f_count];

        C = new double[x_count * g_count * f_count];
        memset(C, 0, x_count * g_count * f_count * sizeof(double));

        if (x_scale[0] == 0) {
            for (int i = 0; i < x_count; i++) x_scale[i] = l_min + delta * i;
        }

        for (int i = 0; i < g_count; i++) g_scale[i] = -g_max + delta * i;

        for (int i = 0; i < f_count; i++)
            f_scale[i] = 0.5f * pix_per_deg / (float)(1 << i);
    }

    ~conditional_density() {
        delete[] C;
        delete[] g_scale;
        delete[] f_scale;
    }

    double &operator()(int x, int g, int f) {
        assert((x + g * x_count + f * x_count * g_count >= 0) &&
               (x + g * x_count + f * x_count * g_count <
                x_count * g_count * f_count));
        return C[x + g * x_count + f * x_count * g_count];
    }
};

datmoConditionalDensity::~datmoConditionalDensity() {}

const double conditional_density::l_min = -8.f,
             conditional_density::l_max = 8.f,
             conditional_density::delta = 0.1f;
double conditional_density::x_scale[X_COUNT] = {
    0};  // input log luminance scale

std::unique_ptr<datmoConditionalDensity> datmo_compute_conditional_density(
    int width, int height, const float *L, pfs::Progress &ph) {
    gsl_set_error_handler (my_gsl_error_handler);
    ph.setValue(0);

    pfs::Array2Df buf_1(width, height);
    pfs::Array2Df buf_2(width, height);
    pfs::Array2Df temp(width, height);

    std::unique_ptr<conditional_density> C(new conditional_density());

    const float thr = 0.0043f;  // Approx. discrimination threshold in log10
    const int pix_count = width * height;

    pfs::Array2Df *LP_low = &buf_1;
    pfs::Array2Df *LP_high = &buf_2;
    float *LP_high_raw = LP_high->data();

    const float min_val = std::max(min_positive(L, pix_count), MIN_PHVAL);

// Compute log10 of an image
#pragma omp parallel for default(none) shared(LP_high_raw, L)
#ifndef LUMINANCE_USE_SSE
    for (int i = 0; i < pix_count; i++)
        LP_high_raw[i] = safe_log10(L[i], min_val);
#else
    for (int i = 0; i < pix_count - 3; i += 4)
        _mm_store_ps(&LP_high_raw[i], safe_log10(_mm_load_ps(&L[i]), min_val));
    // In case pix_count%4 > 0
    for (int i = pix_count - 3; i < pix_count; i++)
        LP_high_raw[i] = safe_log10(L[i], min_val);
#endif

    bool warn_out_of_range = false;
    C->total = 0;

    for (int f = 0; f < C->f_count; f++) {
        compute_gaussian_level(width, height, *LP_high, *LP_low, f, temp);

// For debug purposes only
#ifdef DEBUG
        char fname[20];
        sprintf(fname, "l_%d.pfs", f + 1);
        dumpPFS(fname, width, height, LP_low, "Y");
#endif

        const int gi_tp = C->g_count / 2 + 1;
        const int gi_tn = C->g_count / 2 - 1;
        const int gi_t = C->g_count / 2;
        // Can't parallelize this loop due to the increments on shared data
        // at the end, and making those increments critical sections makes
        // things worse.
        for (int i = 0; i < pix_count; i++) {
            float g = (*LP_high)(i) - (*LP_low)(i);  // Compute band-pass
            int x_i = round_int(((*LP_low)(i)-C->l_min) / C->delta);
            if (unlikely(x_i < 0 || x_i >= C->x_count)) {
                warn_out_of_range = true;
                continue;
            }
            int g_i = round_int((g + C->g_max) / C->delta);
            if (unlikely(g_i < 0 || g_i >= C->g_count)) continue;

            if (g > thr && g < C->delta / 2) {
                // above the threshold +
                (*C)(x_i, gi_tp, f)++;
            } else if (g < -thr && g > -C->delta / 2) {
                // above the threshold -
                (*C)(x_i, gi_tn, f)++;
            } else
                (*C)(x_i, g_i, f)++;
        }

        for (int i = 0; i < C->x_count; i++) {
            // Special case: flat field and no gradients
            if ((*C)(i, gi_t, f) == 0) continue;
            bool gradient_exist = false;
            for (int m = 0; m < C->g_count; m++)
                if (m != gi_t && (*C)(i, m, f) != 0) {
                    gradient_exist = true;
                    break;
                }
            if (!gradient_exist) {
                // generate some gradient data to avoid bad conditioned problem
                (*C)(i, gi_tp, f)++;
                (*C)(i, gi_tn, f)++;
            }

            // Compute C->total
            for (int m = 0; m < C->g_count; m++)
                if (likely(m != gi_t)) C->total += (*C)(i, m, f);
        }
        std::swap(LP_low, LP_high);

        ph.setValue((f + 1) * PROGRESS_CDF / C->f_count);
        if (ph.canceled()) break;
    }

    if (warn_out_of_range)
        std::cerr << "Warning: Luminance value out of permissible range\n";

    // For debugging purposes only
    //    FILE *fh = fopen( "c_dens.dat", "wt" );
    //    std::cerr << "x: " << C->x_count << " g: " << C->g_count << " f: " <<
    //    C->f_count << "\n";
    //    for( int i=0; i<(C->x_count*C->g_count*C->f_count); i++ ) {
    //      fprintf( fh, "%g\n", C->C[i] );
    //    }
    //    fclose( fh );

    return std::move(C);
}

// =============== Quadratic programming solver ==============

const static gsl_matrix null_matrix = {0, 0, 0, 0, 0, 0};
const static gsl_vector null_vector = {0, 0, 0, 0, 0};

/* objective function: 0.5*(x^t)Qx+(q^t)x */
/* constraints: Cx>=d */
/* Ax=b; is not used in our problem */
static int solve(gsl_matrix *Q, gsl_vector *q, gsl_matrix *C, gsl_vector *d,
                 gsl_vector *x) {
    gsl_cqp_data cqpd;

    cqpd.Q = Q;
    cqpd.q = q;

    // Do not use any equality constraints (Ax=b)

    // Unfortunatelly GSL does not allow for 0-size vectors and matrices
    // As a work-around we create a phony gsl_matrix that has 0-size.
    // This matrix must not be passed to any gsl function!

    cqpd.A = &null_matrix;
    cqpd.b = &null_vector;

    cqpd.C = C;
    cqpd.d = d;

    size_t n = cqpd.Q->size1;
    size_t me = cqpd.b->size;
    size_t mi = cqpd.d->size;

    const size_t max_iter = 100;

    size_t iter = 1;

    int status;

    const gsl_cqpminimizer_type *T;

    T = gsl_cqpminimizer_mg_pdip;
    auto_cqpminimizer s(gsl_cqpminimizer_alloc(T, n, me, mi));

    status = gsl_cqpminimizer_set(s, &cqpd);

    const bool verbose = false;

    if (verbose)
        fprintf(stderr,
                "== Itn ======= f ======== ||gap|| ==== ||residual||\n\n");

    do {
        status = gsl_cqpminimizer_iterate(s);
        status = gsl_cqpminimizer_test_convergence(s, 1e-10, 1e-10);

        if (verbose)
            fprintf(stderr, "%4lu   %14.8f  %13.6e  %13.6e\n", iter,
                    gsl_cqpminimizer_f(s), gsl_cqpminimizer_gap(s),
                    gsl_cqpminimizer_residuals_norm(s));

        if (status == GSL_SUCCESS) {
            size_t j;
            if (verbose) {
                fprintf(stderr, "\nMinimum is found at\n");
                for (j = 0; j < gsl_cqpminimizer_x(s)->size; j++)
                    fprintf(stderr, "%9.6f ",
                            gsl_vector_get(gsl_cqpminimizer_x(s), j));
                fprintf(stderr, "\n\n");
            }

            //       printf("\nLagrange-multipliers for Ax=b\n");
            //       for(j=0; j<gsl_cqpminimizer_lm_eq(s)->size; j++)
            //         printf("%9.6f ",gsl_vector_get(gsl_cqpminimizer_lm_eq(s),
            //         j));
            //       printf("\n\n");

            //       printf("\nLagrange-multipliers for Cx>=d\n");
            //       for(j=0; j<gsl_cqpminimizer_lm_ineq(s)->size; j++)
            //         printf("%9.6f
            //         ",gsl_vector_get(gsl_cqpminimizer_lm_ineq(s),
            //         j));
            //       printf("\n\n");
        } else {
            iter++;
        }

    } while (status == GSL_CONTINUE && iter <= max_iter);

    bool valid_solution = true;

    // If the solver bahaves instable, stop at this point
    if (status != GSL_SUCCESS) {
        if (gsl_cqp_minimizer_test_infeasibility(s, 1e-10) != GSL_SUCCESS)
            valid_solution = false;
    }

    if (valid_solution) gsl_vector_memcpy(x, gsl_cqpminimizer_x(s));

    return GSL_SUCCESS;
}

// =============== HVS functions ==============

static double contrast_transducer(double C, double sensitivity,
                                  datmoVisualModel visual_model) {
    if (visual_model & vm_contrast_masking) {
        const double W = pow(10, fabs(C)) - 1.;

        const double Q = 3., A = 3.291, B = 3.433, E = 0.8, k = 0.2599;
        const double SC = sensitivity * W;

        return sign(C) * A * (pow(1. + pow(SC, Q), 1. / 3.) - 1.) /
               (k * pow(B + SC, E));
    } else {
        return C * sensitivity;
    }
}

/**
 * Contrast Sensitivity Function from Daly's VDP paper
 *
 * @param rho  spatial frequency in cycles per degree
 * @param theta  spatial angle in radians
 * @param l_adapt  luminance of adaptation
 * @param img_size  image size given in visual degrees^2
 * @param viewing_dist  viewing distance in meters (default = 0.5m)
 * @return sensitivity
 */
static double csf_daly(double rho, double theta, double l_adapt, double im_size,
                       double viewing_dist = 0.5) {
    if (rho == 0) return 0;  // To avoid singularity

    // Sensitivity calibration constant (from Daly's paper: 250)
    const double P = 250.f;

    const double eps = 0.9;
    const double i_pow2 = im_size;
    const double l = l_adapt;
    const double A = 0.801 * pow(1 + 0.7 / l, -0.20);
    const double B = 0.3 * pow(1 + 100 / l, 0.15);

    const double r_a = 0.856 * powf(viewing_dist, 0.14);
    const double e = 0.0;  // eccentricity in visual degrees
    const double r_e = 1.0 / (1.0 + 0.24 * e);
    const double r_a_r_e = r_a * r_e;

    double S1, S2;

    double B1 = B * eps * rho;
    S1 = pow(pow(3.23 * pow(rho * rho * i_pow2, -0.3), 5.0) + 1.0, -0.2) * A *
         eps * rho * exp(-B1) * sqrt(1 + 0.06 * exp(B1));

    const double ob = 0.78;
    const double r_theta = (1 - ob) / 2 * cosf(4.0 * theta) + (1 + ob) / 2;
    rho = rho / (r_a_r_e * r_theta);

    B1 = B * eps * rho;
    S2 = powf(pow(3.23 * pow(rho * rho * i_pow2, -0.3), 5.0) + 1.0, -0.2) * A *
         eps * rho * exp(-B1) * sqrt(1 + 0.06 * exp(B1));

    return (S1 > S2 ? S2 : S1) * P;
}

static double csf_datmo(double rho, double l_adapt,
                        datmoVisualModel visual_model) {
    if (!(visual_model & vm_luminance_masking)) l_adapt = 1000.;
    if (!(visual_model & vm_csf)) rho = 4.;

    return csf_daly(rho, 0, l_adapt, 1);
}

static void compute_y(double *y, const gsl_vector *x, int *skip_lut,
                      int x_count, int L, double Ld_min, double Ld_max) {
    double sum_d = 0;
    double alpha = 1;
    for (int k = 0; k < L; k++) {
        sum_d += gsl_vector_get(x, k);
    }
    double cy = log10(Ld_min) + alpha * (log10(Ld_max) - log10(Ld_min) - sum_d);
    double dy;
    y[0] = cy;
    dy = 0;
    for (int i = 0; i < x_count - 1; i++) {
        if (skip_lut[i] != -1) {
            // Check how many nodes spans this d_i
            int j;
            for (j = i + 1; j < (x_count - 1) && skip_lut[j] == -1; j++) {
            };

            if (j == (x_count - 1)) {  // The last node
                dy = 0;
                y[i] = cy;
                cy += gsl_vector_get(x, skip_lut[i]);
                continue;
            } else
                dy = gsl_vector_get(x, skip_lut[i]) / (double)(j - i);
        }
        y[i] = cy;
        cy += dy;
    }
    y[x_count - 1] = cy;
}

// =============== Tone mapping ==============

/**
 * Solve the quadratic programming problem to find the optimal tone
 * curve for the given conditional denstity structure.
 *
 * @param y output luminance value for the nodes C->x_scale. y must be
 * a pre-allocated array and has the same size as C->x_scale.
 */
static int optimize_tonecurve(datmoConditionalDensity *C_pub,
                              DisplayFunction *dm, DisplaySize * /*ds*/,
                              float enh_factor, double *y, const float white_y,
                              datmoVisualModel visual_model,
                              double scene_l_adapt, pfs::Progress &ph) {
    conditional_density *C = (conditional_density *)C_pub;

    double d_dr =
        log10(dm->display(1.f) / dm->display(0.f));  // display dynamic range

    // Create LUTs for CSF to speed up computations
    std::vector<UniformArrayLUT> csf_lut(C->f_count);
    for (int f = 0; f < C->f_count; f++) {
        csf_lut[f] = UniformArrayLUT(C->x_count, C->x_scale);
        for (int i = 0; i < C->x_count; i++)
            // csf_lut[f].y_i[i] = csf_daly( C->f_scale[f], 0, pow( 10.,
            // C->x_scale[i]
            // ), 1 );
            csf_lut[f].y_i[i] =
                csf_datmo(C->f_scale[f], pow(10., C->x_scale[i]),
                          visual_model);  // In pfstmo 2.0.5
    }

    const int max_neigh = (C->g_count - 1) / 2;

    // count number of needed equations and remove not used variables
    // Create a structure of disconnected frameworks, which can be connected
    // later
    int k = 0;
    std::vector<int> skip_lut(C->x_count - 1);  // LUT used to skip unused nodes
    std::vector<int> used_var(C->x_count - 1);  // Used / unused variables
    memset(&used_var[0], 0, sizeof(used_var[0]) * (C->x_count - 1));

    int minmax_i[2] = {C->x_count - 1, 0};

    for (int f = 0; f < C->f_count; f++)
        for (int i = 0; i < C->x_count; i++)
            for (int j = std::max(0, i - max_neigh);
                 j < std::min(C->x_count - 1, i + max_neigh); j++) {
                if (i == j || (*C)(i, j - i + max_neigh, f) == 0) continue;
                k++;

                const int from = std::min(i, j);
                const int to = std::max(i, j);
                for (int l = from; l <= to - 1; l++) {
                    used_var[l] = 1;
                }
                minmax_i[0] = std::min(minmax_i[0], from);
                minmax_i[1] = std::max(minmax_i[1], to - 1);
            }

    int white_i = 0;
    if (white_y > 0) {
        k++;
        const float white_l = log10(white_y);
        // find i that corresponds to the reference white
        int i;
        for (i = C->x_count - 1; i >= 0; i--)
            if (C->x_scale[i] <= white_l) break;
        white_i = i;
        used_var[white_i] = 1;
        minmax_i[0] = std::min(minmax_i[0], white_i);
        minmax_i[1] = std::max(minmax_i[1], white_i);
    }

    // Create skip lookup table (to remove selected columns that contain all
    // zeros)
    int i = 0;
    int fwrk = 0;  // Number if missing contrast ranges
    for (int l = 0; l < C->x_count - 1; l++) {
        if (l < minmax_i[0] || l > minmax_i[1]) {
            skip_lut[l] = -1;
            continue;
        }
        if (!used_var[l]) {
            if (l > 0 && !used_var[l - 1]) {
                skip_lut[l] = -1;
                continue;
            }
            fwrk++;
        }
        skip_lut[l] = i++;
    }

    const int M = k + fwrk;  // number of equations
    const int L = i;         // Number of non-zero d_i variables

    // Constraints
    // all intervals must be >=0
    // sum of intervals must be equal displayable dynamic range

    // Ale = [eye(interval_count); -ones(1,interval_count)];

    auto_matrix Ale(gsl_matrix_calloc(L + 1, L));
    gsl_matrix_set_identity(Ale);
    gsl_matrix_view lower_row = gsl_matrix_submatrix(Ale, L, 0, 1, L);
    gsl_matrix_set_all(&lower_row.matrix, -1);

    // ble = [zeros(interval_count,1); -d_dr];
    auto_vector ble(gsl_vector_calloc(L + 1));
    gsl_vector_set(ble, L, -d_dr);

    auto_matrix A(gsl_matrix_calloc(M, L));
    auto_vector B(gsl_vector_alloc(M));
    auto_vector N(gsl_vector_alloc(M));

    std::vector<size_t> band(M);    // Frequency band (index)
    std::vector<size_t> back_x(M);  // Background luminance (index)

    k = 0;
    for (int f = 0; f < C->f_count; f++) {
        // const double sensitivity = csf_daly( C->f_scale[f], 0., 1000., 1. );
        double sensitivity =
            csf_daly(C->f_scale[f], 0., 1000.,
                     1.);  // In pfstmo 2.0.5 sensitivity may be uninizialized
        if (scene_l_adapt != -1)
            sensitivity = csf_datmo(C->f_scale[f], scene_l_adapt, visual_model);

        for (int i = 0; i < C->x_count; i++)
            for (int j = std::max(0, i - max_neigh);
                 j < std::min(C->x_count - 1, i + max_neigh); j++) {
                if (i == j || (*C)(i, j - i + max_neigh, f) == 0) continue;

                const int from = std::min(i, j);
                const int to = std::max(i, j);

                //      A(k,min(i,j):(max(i,j)-1)) = 1;
                for (int l = from; l <= to - 1; l++) {
                    if (skip_lut[l] == -1) continue;
                    gsl_matrix_set(A, k, skip_lut[l], 1);
                }

                if (scene_l_adapt == -1) {
                    sensitivity = csf_lut[f].interp(C->x_scale[from]);
                }

                //      B(k,1) = l_scale(max(i,j)) - l_scale(min(i,j));
                gsl_vector_set(
                    B, k, contrast_transducer(
                              (C->x_scale[to] - C->x_scale[from]) * enh_factor,
                              sensitivity, visual_model));

                //      N(k,k) = jpf(j-i+max_neigh+1,i,band);
                gsl_vector_set(N, k, (*C)(i, j - i + max_neigh, f));

                band[k] = f;
                back_x[k] = i;

                k++;
            }
    }

    if (white_y > 0) {
        for (int l = white_i; l < C->x_count - 1; l++) {
            if (skip_lut[l] == -1) continue;
            gsl_matrix_set(A, k, skip_lut[l], 1);
        }
        gsl_vector_set(B, k, 0);
        gsl_vector_set(
            N, k,
            C->total * 0.1);  // Strength of reference white anchoring
        band[k] = 0;
        back_x[k] = white_i;
        k++;
    }

    // Connect disconnected frameworks
    // This is the case when there is no contrast between some patches in an
    // image
    {
        for (int i = minmax_i[0]; i <= minmax_i[1]; i++) {
            if (!used_var[i]) {
                const int from = i;
                int to = i + 1;
                while (!used_var[to]) to++;
                assert(k < M);
                for (int l = from; l <= to - 1; l++) {
                    if (skip_lut[l] == -1) continue;
                    gsl_matrix_set(A, k, skip_lut[l], 1);
                }
                // const double sensitivity = csf_daly(
                // C->f_scale[C->f_count-1], 0.,
                // 1000., 1. );
                // gsl_vector_set( B, k, contrast_transducer( (C->x_scale[to] -
                // C->x_scale[from])*enh_factor, sensitivity ) );
                double sensitivity;
                if (scene_l_adapt == -1) {
                    sensitivity =
                        csf_lut[C->f_count - 1].interp(C->x_scale[from]);
                } else
                    sensitivity = csf_datmo(C->f_scale[C->f_count - 1],
                                            scene_l_adapt, visual_model);

                // const double sensitivity = csf_datmo(
                // C->f_scale[C->f_count-1],
                // scene_l_adapt, visual_model );
                gsl_vector_set(
                    B, k, contrast_transducer(
                              (C->x_scale[to] - C->x_scale[from]) * enh_factor,
                              sensitivity, visual_model));

                gsl_vector_set(
                    N, k,
                    C->total * 0.1);  // Strength of framework anchoring
                band[k] = C->f_count - 1;
                back_x[k] = to;
                k++;
                i = to;
            }
        }
    }

    auto_matrix H(gsl_matrix_alloc(L, L));
    auto_vector f(gsl_vector_alloc(L));
    auto_matrix NA(gsl_matrix_alloc(M, L));
    auto_matrix AK(gsl_matrix_alloc(M, L));
    auto_vector Ax(gsl_vector_alloc(M));
    auto_vector K(gsl_vector_alloc(M));
    auto_vector x(gsl_vector_alloc(L));
    auto_vector x_old(gsl_vector_alloc(L));

    gsl_vector_set_all(x, d_dr / L);

    int max_iter = 200;
    if (!(visual_model & vm_contrast_masking)) max_iter = 1;

    for (int it = 0; it < max_iter; it++) {
        //    fprintf( stderr, "Iteration #%d\n", it );

        // Compute y values for the current solution
        compute_y(y, x, &skip_lut[0], C->x_count, L, dm->display(0),
                  dm->display(1));

        // Ax = A*x
        gsl_blas_dgemv(CblasNoTrans, 1, A, x, 0, Ax);

        // T(rng{band}) = cont_transd( Ax(rng{band}), band, DD(rng{band},:)*y' )
        // ./
        // Axd(rng{band});
        for (int k = 0; k < M; k++) {
            double sensitivity = csf_lut[band[k]].interp(y[back_x[k]]);
            const double Ax_k = gsl_vector_get(Ax, k);
            const double denom = (fabs(Ax_k) < 0.0001 ? 1. : Ax_k);
            // gsl_vector_set( K, k, contrast_transducer( Ax_k, sensitivity ) /
            // denom
            // );
            gsl_vector_set(
                K, k,
                contrast_transducer(Ax_k, sensitivity, visual_model) / denom);
        }

        // AK = A*K;
        mult_rows(A, K, AK);

        // NA = N*A;
        mult_rows(AK, N, NA);

        // H = AK'*NA;
        gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, AK, NA, 0, H);

        // f = -B'*NA = - NA' * B;
        gsl_blas_dgemv(CblasTrans, -1, NA, B, 0, f);

        gsl_vector_memcpy(x_old, x);

        solve(H, f, Ale, ble, x);

        /*
        if (status == GSL_FAILURE)
        {
            std::cout << "GSL_FAILURE" << std::endl;
            return PFSTMO_ERROR;
        }
        */

        // Check for convergence
        double min_delta =
            (C->x_scale[1] - C->x_scale[0]) / 10.;  // minimum acceptable change
        bool converged = true;
        for (int i = 0; i < L; i++) {
            double delta =
                fabs(gsl_vector_get(x, i) - gsl_vector_get(x_old, i));
            if (delta > min_delta) {
                converged = false;
                break;
            }
        }
        if (converged) break;
    }
    ph.setValue(95);
    if (ph.canceled()) return PFSTMO_ABORTED;  // PFSTMO_OK is right

    //   for( int i=0; i < L; i++ )
    //     fprintf( stderr, "%9.6f ", gsl_vector_get( x, i ) );
    //   fprintf( stderr, "\n" );

    compute_y(y, x, &skip_lut[0], C->x_count, L, dm->display(0),
              dm->display(1));

    return PFSTMO_OK;
}

int datmo_compute_tone_curve(datmoToneCurve *tc,
                             datmoConditionalDensity *cond_dens,
                             DisplayFunction *df, DisplaySize *ds,
                             const float enh_factor, const float white_y,
                             datmoVisualModel visual_model,
                             double scene_l_adapt, pfs::Progress &ph) {
    conditional_density *c = (conditional_density *)cond_dens;
    tc->init(c->x_count, c->x_scale);
    return optimize_tonecurve(cond_dens, df, ds, enh_factor, tc->y_i, white_y,
                              visual_model, scene_l_adapt, ph);
}

/**
 * Apply tone curve with color correction
 * (http://zgk.wi.ps.pl/color_correction/)
 */
int datmo_apply_tone_curve_cc(float *R_out, float *G_out, float *B_out,
                              int width, int height, const float *R_in,
                              const float *G_in, const float *B_in,
                              const float *L_in, datmoToneCurve *tc,
                              DisplayFunction *df,
                              const float saturation_factor) {
    // Create LUT: log10( lum factor ) -> pixel value
    UniformArrayLUT tc_lut(tc->size, tc->x_i);
    for (size_t i = 0; i < tc->size; i++) {
        tc_lut.y_i[i] = (float)pow(10, tc->y_i[i]);
        //    tc_lut.y_i[i] = df->inv_display( (float)pow( 10, tc->y_i[i] ) );
    }

    // Create LUT: log10( lum factor ) -> saturation correction (for the
    // tone-level)
    UniformArrayLUT cc_lut(tc->size, tc->x_i);
    for (size_t i = 0; i < tc->size - 1; i++) {
        // const float contrast = std::max(
        // (tc->y_i[i+1]-tc->y_i[i])/(tc->x_i[i+1]-tc->x_i[i]), 0.d ); // In
        // pfstmo
        // 2.0.5
        const float contrast =
            std::max((float)(tc->y_i[i + 1] - tc->y_i[i]) /
                         (float)(tc->x_i[i + 1] - tc->x_i[i]),
                     0.0f);  // In pfstmo 2.0.5
        const float k1 = 1.48f;
        const float k2 = 0.82f;
        cc_lut.y_i[i] = ((1 + k1) * pow(contrast, k2)) /
                        (1 + k1 * pow(contrast, k2)) * saturation_factor;
    }
    cc_lut.y_i[tc->size - 1] = 1;

    const long pix_count = width * height;

#pragma omp parallel for default(none) \
    shared(R_in, G_in, B_in, L_in, R_out, G_out, B_out, tc_lut, cc_lut, df)
    for (long i = 0; i < pix_count; i++) {
        float L_fix = clamp_channel(L_in[i]);
        const float L_out = tc_lut.interp(log10(L_fix));
        const float s = cc_lut.interp(log10(L_fix));  // color correction
#ifdef LUMINANCE_USE_SSE
        v4sf vec =
            _mm_set_ps(R_in[i], G_in[i], B_in[i], 0) / _mm_set1_ps(L_fix);
        vec = _mm_max_ps(vec, _mm_set1_ps(MIN_PHVAL));
        vec = _mm_pow_ps(vec, _mm_set1_ps(s));
        vec = vec * _mm_set1_ps(L_out);
        vec = df->inv_display(vec);
        float tmp[4];
        _mm_store_ps(tmp, vec);
        R_out[i] = tmp[3];
        G_out[i] = tmp[2];
        B_out[i] = tmp[1];
#else
        R_out[i] =
            df->inv_display(powf(clamp_channel(R_in[i] / L_fix), s) * L_out);
        G_out[i] =
            df->inv_display(powf(clamp_channel(G_in[i] / L_fix), s) * L_out);
        B_out[i] =
            df->inv_display(powf(clamp_channel(B_in[i] / L_fix), s) * L_out);
#endif
    }

    return PFSTMO_OK;
}

// Pre-computed IIR filters - for different frame rates
double t_filter_a_25fps[] = {1.000000000000000, -2.748835809214676,
                             2.528231219142559, -0.777638560238080};
double t_filter_b_25fps[] = {0.000219606211225409, 0.000658818633676228,
                             0.000658818633676228, 0.000219606211225409};

double t_filter_a_30fps[] = {1.000000000000000, -2.790655305284069,
                             2.602653173508124, -0.810960871907291};
double t_filter_b_30fps[] = {0.000129624539595474, 0.000388873618786423,
                             0.000388873618786423, 0.000129624539595474};

double t_filter_a_60fps[] = {1.000000000000000, -2.895292177877897,
                             2.795994584283360, -0.900566088981622};
double t_filter_b_60fps[] = {0.0000170396779801130, 0.0000511190339403389,
                             0.0000511190339403389, 0.0000170396779801130};

datmoTCFilter::datmoTCFilter(float fps, float y_min, float y_max)
    : y_min(y_min), y_max(y_max) {
    assert(fps == 25 || fps == 30 || fps == 60);
    if (fps == 60) {
        t_filter_a = t_filter_a_60fps;
        t_filter_b = t_filter_b_60fps;
    } else if (fps == 30) {
        t_filter_a = t_filter_a_30fps;
        t_filter_b = t_filter_b_30fps;
    } else {
        t_filter_a = t_filter_a_25fps;
        t_filter_b = t_filter_b_25fps;
    }

    pos = -1;
    sz = 0;
}

datmoToneCurve *datmoTCFilter::getToneCurvePtr() {
    pos++;
    if (pos == DATMO_TF_TAPSIZE) pos = 0;

    sz++;
    if (sz > DATMO_TF_TAPSIZE) sz = DATMO_TF_TAPSIZE;

    return ring_buffer_org + pos;
}

datmoToneCurve *datmoTCFilter::filterToneCurve() {
    datmoToneCurve *tc_o = ring_buffer_org + pos;
    datmoToneCurve *tc_f = ring_buffer_filt + pos;

    tc_f->init(tc_o->size, tc_o->x_i);
    if (tc_filt_clamp.x_i == NULL) tc_filt_clamp.init(tc_o->size, tc_o->x_i);
    for (size_t j = 0; j < tc_f->size; j++) tc_f->y_i[j] = 0;

    for (int tt = 0; tt < DATMO_TF_TAPSIZE; tt++) {
        datmoToneCurve *x = get_tc(ring_buffer_org, tt);
        datmoToneCurve *y;
        if (tt >= sz)
            y = x;
        else
            y = get_tc(ring_buffer_filt, tt);

        for (size_t j = 0; j < tc_f->size; j++) {
            tc_f->y_i[j] += t_filter_b[tt] * x->y_i[j];
            if (tt > 0) tc_f->y_i[j] -= t_filter_a[tt] * y->y_i[j];
        }
    }

    // Copy to dest array and clamp
    // Note that the clamped values cannot be used for filtering as they
    // would cause too much rippling for the IIR filter
    for (size_t j = 0; j < tc_f->size; j++) {
        if (tc_f->y_i[j] < y_min) {
            tc_filt_clamp.y_i[j] = y_min;
        } else if (tc_f->y_i[j] > y_max) {
            tc_filt_clamp.y_i[j] = y_max;
        } else
            tc_filt_clamp.y_i[j] = tc_f->y_i[j];
    }

    return &tc_filt_clamp;
}

datmoToneCurve *datmoTCFilter::get_tc(datmoToneCurve *ring_buf, int time) {
    if (time >= sz) time = sz - 1;

    int p = pos - time;
    if (p < 0) p = p + DATMO_TF_TAPSIZE;

    return ring_buf + p;
}

#ifndef WHITEBALANCE_H
#define WHITEBALANCE_H

#include <Libpfs/array2d.h>

/**
 * @brief simplest color balance on RGB channels
 *
 * The input image is normalized by affine transformation on each RGB
 * channel, saturating a percentage of the pixels at the beginning and
 * end of the color space on each channel.
 */
void colorbalance_rgb_f32(pfs::Array2Df& R, pfs::Array2Df& G, pfs::Array2Df& B,
                          size_t size, size_t nb_min, size_t nb_max);
void robustAWB(pfs::Array2Df* R, pfs::Array2Df* G, pfs::Array2Df* B);
void shadesOfGrayAWB(pfs::Array2Df* R_orig, pfs::Array2Df* G_orig, pfs::Array2Df* B_orig);

#endif // WHITEBALANCE_H

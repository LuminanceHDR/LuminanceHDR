#ifndef WHITEBALANCE_H
#define WHITEBALANCE_H

#include <Libpfs/array2d.h>
#include <Libpfs/frame.h>

/**
 * @brief simplest color balance on RGB channels
 *
 * The input image is normalized by affine transformation on each RGB
 * channel, saturating a percentage of the pixels at the beginning and
 * end of the color space on each channel.
 */
void colorBalanceRGB(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                     float nb_min, float nb_max);
void robustAWB(pfs::Array2Df *R, pfs::Array2Df *G, pfs::Array2Df *B);
void shadesOfGrayAWB(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B);

enum WhiteBalanceType {
    WB_COLORBALANCE = 0,
    WB_ROBUST = 1,
    WB_SHADESOFGRAY = 2
};

void whiteBalance(pfs::Frame &frame, WhiteBalanceType type);
void whiteBalance(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                  WhiteBalanceType type);

#endif  // WHITEBALANCE_H

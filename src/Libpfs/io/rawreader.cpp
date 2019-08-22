/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

#include "librtprocess.h"

#include <Libpfs/colorspace/copy.h>
#include <Libpfs/colorspace/gamma.h>
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/fixedstrideiterator.h>
#include <Libpfs/frame.h>
#include <Libpfs/io/rawreader.h>
#include <Libpfs/utils/transform.h>
#include "sleef.c"
#include "opthelper.h"

bool callback(double a) { return false; }

using namespace pfs;

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "RAWReader: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

namespace pfs {
namespace io {

/**************************** From UFRAW sourcecode ********************************
 *
 * Convert between Temperature and RGB.
 * Base on information from http://www.brucelindbloom.com/
 * The fit for D-illuminant between 4000K and 15000K are from CIE
 * The generalization to 2000K < T < 4000K and the blackbody fits
 * are my own and should be taken with a grain of salt.
 */
static const double XYZ_to_RGB[3][3] = {{3.24071, -0.969258, 0.0556352},
                                        {-1.53726, 1.87599, -0.203996},
                                        {-0.498571, 0.0415557, 1.05707}};

static void temperatureToRGB(double T, double RGB[3]) {
    int c;
    double xD, yD, X, Y, Z, max;
    // Fit for CIE Daylight illuminant
    if (T <= 4000) {
        xD = 0.27475e9 / (T * T * T) - 0.98598e6 / (T * T) + 1.17444e3 / T +
             0.145986;
    } else if (T <= 7000) {
        xD = -4.6070e9 / (T * T * T) + 2.9678e6 / (T * T) + 0.09911e3 / T +
             0.244063;
    } else {
        xD = -2.0064e9 / (T * T * T) + 1.9018e6 / (T * T) + 0.24748e3 / T +
             0.237040;
    }
    yD = -3 * xD * xD + 2.87 * xD - 0.275;

    // Fit for Blackbody using CIE standard observer function at 2 degrees
    // xD = -1.8596e9/(T*T*T) + 1.37686e6/(T*T) + 0.360496e3/T + 0.232632;
    // yD = -2.6046*xD*xD + 2.6106*xD - 0.239156;

    // Fit for Blackbody using CIE standard observer function at 10 degrees
    // xD = -1.98883e9/(T*T*T) + 1.45155e6/(T*T) + 0.364774e3/T + 0.231136;
    // yD = -2.35563*xD*xD + 2.39688*xD - 0.196035;

    X = xD / yD;
    Y = 1;
    Z = (1 - xD - yD) / yD;
    max = 0;
    for (c = 0; c < 3; c++) {
        RGB[c] =
            X * XYZ_to_RGB[0][c] + Y * XYZ_to_RGB[1][c] + Z * XYZ_to_RGB[2][c];
        if (RGB[c] > max) max = RGB[c];
    }
    for (c = 0; c < 3; c++) {
        RGB[c] = RGB[c] / max;
    }
}
/*********** END UFRAW CODE *****************************************************/

#define USER_QUALITY 10  // default to AMaZE interpolation

struct RAWReaderParams {
    RAWReaderParams()
        : gamma0_(1),
          gamma1_(1),
          fourColorRGB_(0),
          useFujiRotate_(-1),
          userQuality_(10),
          medPasses_(0),
          wbMethod_(1),
          wbTemperature_(6500),
          wbGreen_(1.0f),
          highlightsMethod_(2),  // blend
          highlightsRebuildMethod_(0),
          blackLevel_(std::numeric_limits<int>::min()),
          saturation_(std::numeric_limits<int>::min()),
          autoBrightness_(false),
          autoBrightnessThreshold_(0.001f),
          brightness_(1.f),
          noiseReductionThreshold_(-std::numeric_limits<float>::max()),
          chromaAberation_(false),
          chroma0_(1.0),
          chroma1_(1.0),
          chroma2_(1.0),
          chroma3_(1.0),
          cameraProfile_() {}

    void parse(const Params &params) {
        int tempInt;
        float tempFloat;
        bool tempBool;
        double tempDouble;
        // general settings
        if (params.get("raw.four_color", tempInt)) {
            fourColorRGB_ = tempInt;
        }
        if (params.get("raw.fuji_rotate", tempInt)) {
            useFujiRotate_ = tempInt;
        }
        if (params.get("raw.user_quality", tempInt)) {
            userQuality_ = tempInt;
        }
        if (params.get("raw.med_passes", tempInt)) {
            medPasses_ = tempInt;
        }
        // white balance
        if (params.get("raw.wb_method", tempInt)) {
            wbMethod_ = tempInt;
        }
        if (params.get("raw.wb_temperature", tempInt)) {
            wbTemperature_ = tempInt;
        }
        if (params.get("raw.wb_green", tempFloat)) {
            wbGreen_ = tempFloat;
        }
        // highlight handling
        if (params.get("raw.highlights", tempInt)) {
            highlightsMethod_ = tempInt;
        }
        if (params.get("raw.highlights_rebuild", tempInt)) {
            highlightsRebuildMethod_ = tempInt;
        }

        // color
        if (params.get("raw.black_level", tempInt)) {
            blackLevel_ = tempInt;
        }
        if (params.get("raw.saturation", tempInt)) {
            saturation_ = tempInt;
        }
        if (params.get("raw.auto_brightness", tempBool)) {
            autoBrightness_ = tempBool;
        }
        if (params.get("raw.auto_brightness_threshold", tempFloat)) {
            autoBrightnessThreshold_ = tempFloat;
        }
        if (params.get("raw.brightness", tempFloat)) {
            brightness_ = tempFloat;
        }
        if (params.get("raw.noise_reduction_threshold", tempFloat)) {
            noiseReductionThreshold_ = tempFloat;
        }
        if (params.get("raw.chroma_aber", tempBool)) {
            chromaAberation_ = tempBool;
            if (tempBool) {
                if (params.get("raw.chroma_aber_0", tempDouble)) {
                    chroma0_ = tempDouble;
                }
                if (params.get("raw.chroma_aber_1", tempDouble)) {
                    chroma1_ = tempDouble;
                }
                if (params.get("raw.chroma_aber_2", tempDouble)) {
                    chroma2_ = tempDouble;
                }
                if (params.get("raw.chroma_aber_3", tempDouble)) {
                    chroma3_ = tempDouble;
                }
            }
        }

        std::string tempString;
        if (params.get("raw.camera_profile", tempString)) {
            if (!tempString.empty()) {
                cameraProfile_.swap(tempString);
            }
        }
    }

    bool isBlackLevel() const {
        return (blackLevel_ != std::numeric_limits<int>::min());
    }

    bool isSaturation() const {
        return (saturation_ != std::numeric_limits<int>::min());
    }

    bool isNoiseReduction() const {
        return (noiseReductionThreshold_ !=
                (-std::numeric_limits<float>::max()));
    }

   public:
    float gamma0_;
    float gamma1_;

    int fourColorRGB_;
    int useFujiRotate_;

    int userQuality_;
    int medPasses_;

    // 1: camera, 2: auto, 3: custom
    int wbMethod_;
    int wbTemperature_;
    float wbGreen_;

    int highlightsMethod_;
    int highlightsRebuildMethod_;

    int blackLevel_;
    int saturation_;  // white point
    bool autoBrightness_;
    float autoBrightnessThreshold_;
    float brightness_;

    float noiseReductionThreshold_;

    bool chromaAberation_;
    double chroma0_;
    double chroma1_;
    double chroma2_;
    double chroma3_;

    std::string cameraProfile_;
};

ostream &operator<<(ostream &out, const RAWReaderParams &p) {
    stringstream ss;
    ss << "[gamma0: " << p.gamma0_ << ", gamma1: " << p.gamma1_;
    ss << ", 4-Color RGB: " << p.fourColorRGB_;
    ss << ", Fuji Rotate: " << p.useFujiRotate_;
    ss << ", User Quality (Demosaicing method): " << p.userQuality_;
    ss << ", Median Filter Passes: " << p.medPasses_;
    ss << ", WB Method: " << p.wbMethod_;
    ss << ", WB Temp: " << p.wbTemperature_;
    ss << ", WB Green: " << p.wbGreen_;
    ss << ", Highlight Method: " << p.highlightsMethod_;
    ss << ", Highlight Rebuild: " << p.highlightsRebuildMethod_;
    if (p.isBlackLevel()) {
        ss << ", Black Level: " << p.blackLevel_;
    } else {
        ss << ", Black Level: N/A";
    }
    if (p.isSaturation()) {
        ss << ", Saturation: " << p.saturation_;
    } else {
        ss << ", Saturation: N/A";
    }
    ss << ", Auto Brightness: " << p.autoBrightness_;
    ss << ", Auto Brightness Threshold: " << p.autoBrightnessThreshold_;
    ss << ", Brightness: " << p.brightness_;
    if (p.isNoiseReduction()) {
        ss << ", Noise Reduction: ON";
        ss << ", Noise Reduction Threshold: " << p.noiseReductionThreshold_;
    } else {
        ss << ", Noise Reduction: OFF";
    }

    ss << ", Chromatic Aberration: " << p.chromaAberation_;
    if (p.chromaAberation_) {
        ss << ", Chroma {" << p.chroma0_ << ", " << p.chroma1_;
        ss << ", " << p.chroma2_ << ", " << p.chroma3_ << "}";
    }
    ss << "]";

    return (out << ss.str());
}

const char *embbededProfile = "embed";

static void setParams(LibRaw &processor, const RAWReaderParams &params) {
    libraw_output_params_t &outParams = processor.imgdata.params;

    outParams.output_bps = 16;
    outParams.output_color = 1;          // sRGB
    outParams.gamm[0] = params.gamma0_;
    outParams.gamm[1] = params.gamma1_;
    // use 4-color demosaicing algorithm
    outParams.four_color_rgb = params.fourColorRGB_;
    // do not rotate or stretch pixels on fuji cameras - default = 1 (rotate)
    outParams.use_fuji_rotate = params.useFujiRotate_;
    // demosaicing parameters
    outParams.user_qual = params.userQuality_;
    outParams.med_passes = params.medPasses_;
    outParams.user_flip = 0;  // exif orientation is done afterwards

    switch (params.wbMethod_) {
        case 1:  // camera
        {
            outParams.use_camera_wb = 1;
        } break;
        case 3:  // custom
        {
            double temperature = params.wbTemperature_;
            double RGB[3];

            temperatureToRGB(temperature, RGB);

            RGB[1] = RGB[1] / params.wbGreen_;

            bool identify = true;
            if (processor.adjust_sizes_info_only() != LIBRAW_SUCCESS) {
                identify = false;
            }

            if (identify && processor.imgdata.idata.colors >= 3) {
                RGB[0] = processor.imgdata.color.pre_mul[0] / RGB[0];
                RGB[1] = processor.imgdata.color.pre_mul[1] / RGB[1];
                RGB[2] = processor.imgdata.color.pre_mul[2] / RGB[2];
            } else {
                RGB[0] = 1.0 / RGB[0];
                RGB[1] = 1.0 / RGB[1];
                RGB[2] = 1.0 / RGB[2];
            }

            outParams.user_mul[0] = RGB[0];
            outParams.user_mul[1] = RGB[1];
            outParams.user_mul[2] = RGB[2];
            outParams.user_mul[3] = RGB[1];
        } break;
        case 2:  // auto
        default: { outParams.use_auto_wb = 1; } break;
    }

    outParams.highlight = params.highlightsMethod_;
    if (params.highlightsMethod_ >= 3) {
        outParams.highlight += params.highlightsRebuildMethod_;
    }

    outParams.no_auto_bright = !params.autoBrightness_;
    outParams.auto_bright_thr = params.autoBrightnessThreshold_;
    outParams.bright = params.brightness_;

    if (params.isBlackLevel()) {
        outParams.user_black = params.blackLevel_;
    }
    if (params.isSaturation()) {
        outParams.user_sat = params.saturation_;
    }
    if (params.isNoiseReduction()) {
        outParams.threshold = params.noiseReductionThreshold_;
    }

    // chromatic aberration
    if (params.chromaAberation_) {
        outParams.aber[0] = params.chroma0_;
        // outParams.aber[1] = params.chroma1_;
        outParams.aber[2] = params.chroma2_;
        // outParams.aber[3] = params.chroma3_;
    }

    outParams.no_interpolation = 1;

    // camera profile
    if (params.cameraProfile_.empty()) {
        outParams.camera_profile = (char *)embbededProfile;
    } else {
        outParams.camera_profile = (char *)params.cameraProfile_.c_str();
    }
}

#define P1 m_processor.imgdata.idata
#define S m_processor.imgdata.sizes
#define C m_processor.imgdata.color
#define T m_processor.imgdata.thumbnail
#define P2 m_processor.imgdata.other
#define OUT m_processor.imgdata.params

RAWReader::RAWReader(const std::string &filename) : FrameReader(filename) {
    RAWReader::open();
}

RAWReader::~RAWReader() { RAWReader::close(); }

void RAWReader::open() {
    RAWReader::close();
    if (m_processor.open_file(filename().c_str()) != LIBRAW_SUCCESS) {
        throw pfs::io::InvalidFile("RAWReader: cannot open file " + filename());
    }
    setWidth(S.width);
    setHeight(S.height);
}

bool RAWReader::isOpen() const { return true; }

void RAWReader::close() { m_processor.recycle(); }

void RAWReader::read(Frame &frame, const Params &params) {
    RAWReaderParams p;
    p.parse(params);

    PRINT_DEBUG(p);

    setParams(m_processor, p);

    open();

    if (m_processor.unpack() != LIBRAW_SUCCESS) {
        m_processor.recycle();
        throw pfs::io::ReadException("Error Unpacking RAW File");
    }

    PRINT_DEBUG("Width: " << S.width << " Height: " << S.height);
    PRINT_DEBUG("iWidth: " << S.iwidth << " iHeight: " << S.iheight);
    PRINT_DEBUG("Make: " << P1.make);
    PRINT_DEBUG("Model: " << P1.model);
    PRINT_DEBUG("Software: " << P1.software);
    PRINT_DEBUG("Raw Count: " << P1.raw_count);
    PRINT_DEBUG("Is Foveon: " << P1.is_foveon);
    PRINT_DEBUG("DNG Version: " << P1.dng_version);
    PRINT_DEBUG("Colors: " << P1.colors);
    PRINT_DEBUG("Filters: " << P1.filters);
    PRINT_DEBUG("Color Desc: " << P1.cdesc);
    PRINT_DEBUG("Xtrans: " << P1.xtrans);
    PRINT_DEBUG("Xtrans_abs: " << P1.xtrans_abs);
    PRINT_DEBUG("ISO: " << P2.iso_speed);
    PRINT_DEBUG("Shutter: " << P2.shutter);
    PRINT_DEBUG("Aperture: " << P2.aperture);
    PRINT_DEBUG("Focal Length: " << P2.focal_len);

    libraw_decoder_info_t *dec_info = (libraw_decoder_info_t *) malloc(sizeof(libraw_decoder_info_t));
    m_processor.get_decoder_info(dec_info);
    PRINT_DEBUG("Decoder Name: " << dec_info->decoder_name);
    free (dec_info);

    m_filters = P1.filters;

    bool isFoveon = P1.is_foveon;

    // TODO check if super ccd can be identified by filters == 1263225675
    if ((p.userQuality_ < 3) || isFoveon || !(isBayer() || isXtrans()) || (m_filters == 1263225675)) {
        OUT.no_interpolation = 0;
    }

    p.wbMethod_ = 3;

    if (m_processor.dcraw_process() != LIBRAW_SUCCESS) {
        m_processor.recycle();
        throw pfs::io::ReadException("Error Processing RAW File");
    }

    libraw_processed_image_t *image = m_processor.dcraw_make_mem_image();

    if (!image)  // ret != LIBRAW_SUCCESS ||
    {
        m_processor.recycle();
        throw pfs::io::ReadException("Memory Error in processing RAW File");
    }

    unsigned W = image->width;
    unsigned H = image->height;

    assert(image->data_size == W * H * 3 * sizeof(uint16_t));

    pfs::Frame tempFrame(W, H);

    pfs::Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    const uint16_t *raw_data = reinterpret_cast<const uint16_t *>(image->data);

    if ((p.userQuality_ < 3) || isFoveon || !(isBayer() || isXtrans()) || (m_filters == 1263225675)) {

        PRINT_DEBUG("LibRaw internal demosaicing or Foveon or SUPER CCD");
        utils::transform(
            FixedStrideIterator<const uint16_t *, 3>(raw_data),
            FixedStrideIterator<const uint16_t *, 3>(raw_data + H * W * 3),
            FixedStrideIterator<const uint16_t *, 3>(raw_data + 1),
            FixedStrideIterator<const uint16_t *, 3>(raw_data + 2), Xc->begin(),
            Yc->begin(), Zc->begin(),
            colorspace::Gamma<pfs::colorspace::Gamma1_0>());

        LibRaw::dcraw_clear_mem(image);
        m_processor.recycle();

        FrameReader::read(tempFrame, params);
        frame.swap(tempFrame);
        return;
    }

    PRINT_DEBUG("Data size: " << image->data_size << " " << W * H * 3 * sizeof(uint16_t));
    PRINT_DEBUG("W: " << W << " H: " << H);

    float rCamMul = m_processor.imgdata.color.cam_mul[0];
    float gCamMul = m_processor.imgdata.color.cam_mul[1];
    float bCamMul = m_processor.imgdata.color.cam_mul[2];
    float minMult = min(min(rCamMul, gCamMul), bCamMul);
    rCamMul /= minMult;
    gCamMul /= minMult;
    bCamMul /= minMult;
    float rPreMul = m_processor.imgdata.color.pre_mul[0];
    float gPreMul = m_processor.imgdata.color.pre_mul[1];
    float bPreMul = m_processor.imgdata.color.pre_mul[2];
    minMult = min(min(rPreMul, gPreMul), bPreMul);
    rPreMul /= minMult;
    gPreMul /= minMult;
    bPreMul /= minMult;

    unsigned cf_array[2][2];
    if ( isBayer() ) {
        PRINT_DEBUG("Bayer");
        for(unsigned i = 0; i < 2; i++) {
            for(unsigned j = 0; j < 2; j++) {
                cf_array[i][j] = m_processor.COLOR(i, j);
            }
        }
    }

    unsigned xtrans[6][6];
    //get xtrans color filter array
    if (isXtrans()) {
        PRINT_DEBUG("Xtrans");
        for (int i=0; i<6; i++) {
            for (int j=0; j<6; j++) {
                xtrans[i][j] = unsigned(m_processor.imgdata.idata.xtrans[i][j]);
            }
        }
    }

    float **rawdata;
    rawdata = (float **) malloc(H * sizeof(float *));
    rawdata[0] = (float *) malloc(W*H * sizeof(float));
    for(unsigned i = 1; i < H; i++) {
        rawdata[i] = rawdata[i-1] + W;
    }

    if ( isBayer() ) {
        const float mult = 1.0f / 65535.f;
    #ifdef _OPENMP
        #pragma omp parallel for
    #endif
        for(unsigned i = 0; i < H; i++) {
            unsigned k = 0;
            for(unsigned j = 0; j < W; j++) {
                unsigned c = cf_array[i%2][j%2];
                unsigned pos = k + i*W*3;
                rawdata[i][j] = raw_data[pos + c] * mult;
                k += 3;
            }
        }
    }
    else if ( isXtrans() ) {
        const float mult = 1.0f / 65535.f;
    #ifdef _OPENMP
        #pragma omp parallel for
    #endif
        for(unsigned i = 0; i < H; i++) {
            unsigned k = 0;
            for(unsigned j = 0; j < W; j++) {
                unsigned c = xtrans[(i) % 6][(j) % 6];
                unsigned pos = k + i*W*3;
                rawdata[i][j] = raw_data[pos + c] * mult;
                k += 3;
            }
        }
    }

    LibRaw::dcraw_clear_mem(image);
    m_processor.recycle();

    float **r;
    float **g;
    float **b;

    r = (float **) malloc(H * sizeof(float *));
    g = (float **) malloc(H * sizeof(float *));
    b = (float **) malloc(H * sizeof(float *));
    r[0] = Xc->data();
    g[0] = Yc->data();
    b[0] = Zc->data();
    for(unsigned i = 1; i < H; i++) {
        r[i] = r[i-1] + W;
        g[i] = g[i-1] + W;
        b[i] = b[i-1] + W;
    }

    if (p.chromaAberation_) {
        PRINT_DEBUG("CA_correct");
        double fitparams[2][2][16];
        CA_correct(0, 0, W, H, true, 1, 0.0, 0.0, true, rawdata, rawdata, cf_array, callback, fitparams, false);
    }

    try {
        if ( isBayer() ) {
            switch (p.userQuality_) {
                case 3:
                    PRINT_DEBUG("AHD DEMOSAICING");
                    ahd_demosaic(W, H, rawdata, r, g, b, cf_array, C.rgb_cam, callback);
                break;
                case 4:
                    PRINT_DEBUG("VNG4 DEMOSAICING");
                    vng4_demosaic(W, H, rawdata, r, g, b, cf_array, callback);
                break;
                case 5:
                    PRINT_DEBUG("HPHD DEMOSAICING");
                    hphd_demosaic(W, H, rawdata, r, g, b, cf_array, callback);
                break;
                case 6:
                    PRINT_DEBUG("IGV DEMOSAICING");
                    igv_demosaic(W, H, rawdata, r, g, b, cf_array, callback);
                break;
                case 7:
                    PRINT_DEBUG("LMMSE DEMOSAICING");
                    lmmse_demosaic(W, H, rawdata, r, g, b, cf_array, callback, 1);
                break;
                case 8:
                    PRINT_DEBUG("DCB DEMOSAICING");
                    dcb_demosaic(W, H, rawdata, r, g, b, cf_array, callback, 3, true);
                break;
                case 9:
                    PRINT_DEBUG("RCD DEMOSAICING");
                    rcd_demosaic(W, H, rawdata, r, g, b, cf_array, callback);
                break;
                case 10:
                    PRINT_DEBUG("AMAZE DEMOSAICING");
                    amaze_demosaic(W, H, 0, 0, W, H, rawdata, r, g, b, cf_array, callback, 1.0, 0, 1.0f, 1.0f);
                break;
            }
        }
        else if ( isXtrans() ) {
            PRINT_DEBUG("MARKESTEIJN DEMOSAICING");
            markesteijn_demosaic(W, H, rawdata, r, g, b, xtrans, C.rgb_cam, callback, 3, true);
        }
    }
    catch(...) {
        PRINT_DEBUG("DEMOSAICING FAILED");
        free ( b );
        free ( g );
        free ( r );
        free ( rawdata[0] );
        free ( rawdata );
        throw pfs::io::ReadException("DEMOSICING FAILED");
    }

    if (p.highlightsMethod_ >= 3) {
        PRINT_DEBUG("HL_recovery");
        auto  minmaxR = std::minmax_element(Xc->begin(), Xc->end());
        auto  minmaxG = std::minmax_element(Yc->begin(), Yc->end());
        auto  minmaxB = std::minmax_element(Zc->begin(), Zc->end());

        PRINT_DEBUG("maxR: " << *minmaxR.second);
        PRINT_DEBUG("minR: " << *minmaxR.first);
        PRINT_DEBUG("maxG: " << *minmaxG.second);
        PRINT_DEBUG("minG: " << *minmaxG.first);
        PRINT_DEBUG("maxB: " << *minmaxB.second);
        PRINT_DEBUG("minB: " << *minmaxB.first);

        const float chmax[3] = {*minmaxR.second, *minmaxG.second, *minmaxB.second};
        //Max clip point:
        const float clmax[3] = {rCamMul, gCamMul, bCamMul};
        HLRecovery_inpaint(W, H, r, g, b, chmax, clmax, callback);
    }

    free ( b );
    free ( g );
    free ( r );
    free ( rawdata[0] );
    free ( rawdata );

    Array2Df *Ch[3] = {Xc, Yc, Zc};
#ifdef _OPENMP
    #pragma omp parallel for
#endif
        for (size_t k = 0; k < W*H; k++) {
            float r = (*Ch[0])(k);
            float g = (*Ch[1])(k);
            float b = (*Ch[2])(k);
            if(!std::isnormal(r) || !std::isnormal(g) || !std::isnormal(b)) {
                (*Ch[0])(k) = 0.f;
                (*Ch[1])(k) = 0.f;
                (*Ch[2])(k) = 0.f;
            }
            if(std::isnan(r) || std::isnan(g) || std::isnan(b)) {
                (*Ch[0])(k) = 0.f;
                (*Ch[1])(k) = 0.f;
                (*Ch[2])(k) = 0.f;
            }
            if ( (r < 0.f) || (g < 0.f) || (b < 0.f)) {
                (*Ch[0])(k) = 0.f;
                (*Ch[1])(k) = 0.f;
                (*Ch[2])(k) = 0.f;
            }
            if ( (r > 1.f) || (g > 1.f) || (b > 1.f)) {
                (*Ch[0])(k) = 1.f;
                (*Ch[1])(k) = 1.f;
                (*Ch[2])(k) = 1.f;
            }
        }

    FrameReader::read(tempFrame, params);
    frame.swap(tempFrame);
}

#undef P1
#undef S
#undef C
#undef T
#undef P2
#undef OUT

}  // io
}  // pfs

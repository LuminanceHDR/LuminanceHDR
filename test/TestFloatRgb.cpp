#include <gtest/gtest.h>
#include <inttypes.h>
#include <QRgb>
#include "Common/FloatRgbToQRgb.h"

TEST(FloatRgbConverter, Uint16_Test1)
{
    FloatRgbToQRgb d;

    float inRed = 1.0f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    uint16_t outRed;
    uint16_t outGreen;
    uint16_t outBlue;

    d.toQUint16(inRed, inGreen, inBlue,
                outRed, outGreen, outBlue);

    EXPECT_EQ(outRed, 65535);
    EXPECT_EQ(outGreen, 0);
    EXPECT_EQ(outBlue, 0);
}

TEST(FloatRgbConverter, Uint16_Test2)
{
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    uint16_t outRed;
    uint16_t outGreen;
    uint16_t outBlue;

    d.toQUint16(inRed, inGreen, inBlue,
                outRed, outGreen, outBlue);

    EXPECT_EQ(outRed, 65535);
    EXPECT_EQ(outGreen, 0);
    EXPECT_EQ(outBlue, 0);
}

TEST(FloatRgbConverter, Uint16_Test3)
{
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = -0.0f;
    float inBlue = 0.0f;

    uint16_t outRed;
    uint16_t outGreen;
    uint16_t outBlue;

    d.toQUint16(inRed, inGreen, inBlue,
                outRed, outGreen, outBlue);

    EXPECT_EQ(outRed, 65535);
    EXPECT_EQ(outGreen, 0);
    EXPECT_EQ(outBlue, 0);
}

TEST(FloatRgbConverter, Qrgb_Test1)
{
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = -0.0f;
    float inBlue = 0.5f;

    QRgb rgb;

    d.toQRgb(inRed, inGreen, inBlue, rgb);

    EXPECT_EQ(qRed(rgb), 255);
    EXPECT_EQ(qGreen(rgb), 0);
    EXPECT_EQ(qBlue(rgb), 128);
}

TEST(FloatRgbConverter, Qrgb_Test2)
{
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = 0.5f;
    float inBlue = -0.5f;

    QRgb rgb;

    d.toQRgb(inRed, inGreen, inBlue, rgb);

    EXPECT_EQ(qRed(rgb), 255);
    EXPECT_EQ(qGreen(rgb), 128);
    EXPECT_EQ(qBlue(rgb), 0);
}

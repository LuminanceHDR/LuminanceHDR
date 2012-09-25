#include <gtest/gtest.h>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/assert.hpp>

#include <Libpfs/colorspace.h>

typedef std::vector<float> ColorSpaceSamples;

using namespace std;
using namespace boost::assign;  // bring 'operator+=()' into scope

inline
float computeLuminance(float red, float green, float blue)
{
    return (red*0.2126729f + green*0.7151522f + blue*0.0721750f);
}


TEST(TestSRGB2Y, TestSRGB2Y)
{
    ColorSpaceSamples redInput;
    ColorSpaceSamples greenInput;
    ColorSpaceSamples blueInput;

    redInput    += 0.f, 1.f, 0.f, 0.f, 0.2f, 1.f;
    greenInput  += 0.f, 0.f, 1.f, 0.f, 0.3f, 1.f;
    blueInput   += 0.f, 0.f, 0.f, 1.f, 0.4f, 1.f;

    ASSERT_EQ( redInput.size(), blueInput.size() );
    ASSERT_EQ( greenInput.size(), blueInput.size() );

    ColorSpaceSamples yTemp(redInput.size());

    pfs::Array2D A2DRed(redInput.size(), 1, redInput.data());
    pfs::Array2D A2DGreen(redInput.size(), 1, greenInput.data());
    pfs::Array2D A2DBlue(redInput.size(), 1, blueInput.data());
    pfs::Array2D A2DY(redInput.size(), 1, yTemp.data());

    // function under unit test!
    pfs::transformRGB2Y( &A2DRed,
                         &A2DGreen,
                         &A2DBlue,
                         &A2DY );

    for (size_t idx = 0; idx < yTemp.size(); ++idx)
    {
        EXPECT_NEAR(yTemp[idx],
                    computeLuminance(redInput[idx],
                                     greenInput[idx],
                                     blueInput[idx]),
                    10e-6);
    }
}

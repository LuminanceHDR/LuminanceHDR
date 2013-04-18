#include <gtest/gtest.h>

#include <Libpfs/colorspace/convert.h>

using namespace pfs::colorspace;

TEST(TestConvertSample, TestConvertSample_Test1)
{
    uint8_t s = ConvertSample<uint8_t, float>()(1.f);
    ASSERT_EQ(s, 255u);

    s = ConvertSample<uint8_t, float>()(0.f);
    ASSERT_EQ(s, 0u);

    s = ConvertSample<uint8_t, float>()(0.5f);
    ASSERT_EQ(s, 128u);
}

TEST(TestConvertSample, TestConvertSample_Test2)
{
    float s = ConvertSample<float, uint8_t>()(128u);
    ASSERT_NEAR(s, 0.502f, 10e-4);

    s = ConvertSample<float, uint8_t>()(255u);
    ASSERT_NEAR(s, 1.f, 10e-6);

    s = ConvertSample<uint8_t, float>()(0u);
    ASSERT_NEAR(s, 0.f, 10e-6);
}

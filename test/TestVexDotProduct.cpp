#include <gtest/gtest.h>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <boost/bind.hpp>

#include <Libpfs/vex/dotproduct.h>

float myRand()
{
    return ( static_cast<float>(rand())/(RAND_MAX) )*2000;
}

TEST(TestVexDotProduct, SmallSize)
{
    std::vector<float> inputVector(100);    // 100 pixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), 0.0f);

    float avgVex = vex::dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e-5);
}

TEST(TestVexDotProduct, MediumSize)
{
    std::vector<float> inputVector(1000000);    // 1 mpixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), 0.0f);

    float avgVex = vex::dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e-5);
}

TEST(TestVexDotProduct, BigSize)
{
    std::vector<float> inputVector(10000000);   // 10 mpixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), 0.0f);

    float avgVex = vex::dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e-5);
}

TEST(TestVexDotProduct, HugeSize)
{
    std::vector<float> inputVector(36000000); // 36 mpixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), 0.0f);

    float avgVex = vex::dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e-5);
}

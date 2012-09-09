#include <gtest/gtest.h>
#include <stdlib.h>
#include <vector>

#include "mantiuk06/contrast_domain.h"
#include "TonemappingOperators/mantiuk06/pyramid.h"

struct RandZeroOne
{
    float operator()()
    {
        return (static_cast<float>(rand())/RAND_MAX);
    }
};

TEST(TestMantiuk06, UpsampleFull)
{
    const size_t outputCols = 4373;
    const size_t outputRows = 2173;
    const size_t inputCols = (outputCols >> 1);
    const size_t inputRows = (outputRows >> 1);

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    test_mantiuk06::matrix_upsample_full(outputCols, outputRows,
                                    origin.data(), referenceOutput.data());

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    matrixUpsample(outputCols, outputRows,
                         origin.data(), testOutput.data());


    ASSERT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        ASSERT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
}

TEST(TestMantiuk06, UpsampleSimple)
{
    const size_t outputCols = 5000;
    const size_t outputRows = 2600;
    const size_t inputCols = (outputCols >> 1);
    const size_t inputRows = (outputRows >> 1);

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    test_mantiuk06::matrix_upsample_simple(outputCols, outputRows,
                                      origin.data(), referenceOutput.data());

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    matrixUpsample(outputCols, outputRows,
                   origin.data(), testOutput.data());

    ASSERT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        ASSERT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
}

TEST(TestMantiuk06, DownsampleFull)
{
    const size_t inputCols = 4373;
    const size_t inputRows = 2173;

    const size_t outputCols = (inputCols >> 1);
    const size_t outputRows = (inputRows >> 1);

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    test_mantiuk06::matrix_downsample_full(inputCols, inputRows,
                                      origin.data(),
                                      referenceOutput.data());

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    matrixDownsample(inputCols, inputRows,
                     origin.data(),
                     testOutput.data());

    ASSERT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        ASSERT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
}

TEST(TestMantiuk06, DownsampleSimple)
{
    const size_t inputCols = 4000;
    const size_t inputRows = 2000;

    const size_t outputCols = (inputCols >> 1);
    const size_t outputRows = (inputRows >> 1);

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    test_mantiuk06::matrix_downsample_simple(inputCols, inputRows,
                                             origin.data(), referenceOutput.data());

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    matrixDownsample(inputCols, inputRows,
                     origin.data(),
                     testOutput.data());

    ASSERT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        ASSERT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
}

TEST(TestMantiuk06, TestMantiuk06AddDivergence)
{
    const size_t cols = 4373;
    const size_t rows = 2173;

    std::vector<float> Gx(cols*rows);
    std::vector<float> Gy(cols*rows);

    // fill data with samples between zero and one!
    generate(Gx.begin(), Gx.end(), RandZeroOne());
    generate(Gy.begin(), Gy.end(), RandZeroOne());

    std::vector<float> referenceOutput(cols*rows);
    std::fill(referenceOutput.begin(), referenceOutput.end(), 0.f);

    test_mantiuk06::calculate_and_add_divergence(cols, rows,
                                            Gx.data(), Gy.data(),
                                            referenceOutput.data());

    std::vector<float> testOutput(cols*rows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    calculateAndAddDivergence(cols, rows,
                                 Gx.data(), Gy.data(),
                                 testOutput.data());

    ASSERT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        ASSERT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
}

TEST(TestMantiuk06, TestMantiuk06CalculateGradient)
{
    const size_t cols = 4373;
    const size_t rows = 2173;

    std::vector<float> input(cols*rows);
    // fill data with samples between zero and one!
    generate(input.begin(), input.end(), RandZeroOne());

    // REFERENCE
    std::vector<float> referenceGx(cols*rows);
    std::vector<float> referenceGy(cols*rows);

    test_mantiuk06::calculate_gradient(cols, rows, input.data(),
                                  referenceGx.data(), referenceGy.data());

    // COMPUTED
    std::vector<float> computedGx(cols*rows);
    std::vector<float> computedGy(cols*rows);

    calculateGradients(cols, rows, input.data(),
                       computedGx.data(), computedGy.data());

    // CHECK
    for (size_t idx = 0; idx < referenceGx.size(); ++idx)
    {
        ASSERT_NEAR(referenceGx[idx], computedGx[idx], 10e-6f);
    }
    for (size_t idx = 0; idx < referenceGy.size(); ++idx)
    {
        ASSERT_NEAR(referenceGy[idx], computedGy[idx], 10e-6f);
    }
}

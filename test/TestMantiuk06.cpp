#include <gtest/gtest.h>
#include <stdlib.h>
#include <vector>

#include "TestMantiuk06Reference.h"

struct RandZeroOne
{
    float operator()()
    {
        return (static_cast<float>(rand())/RAND_MAX);
    }
};

void matrix_upsample_full(const int outCols, const int outRows,
                          const float* const in, float* const out);
void matrix_upsample_simple(const int outCols, const int outRows,
                            const float* const in, float* const out);

void calculate_gradient(const int COLS, const int ROWS,
                        const float* const lum, float* const Gx, float* const Gy);

void calculate_and_add_divergence(const int COLS, const int ROWS,
                                  const float* const Gx, const float* const Gy,
                                  float* const divG);

TEST(TestMantiuk06, UpsampleFull)
{
    const size_t outputCols = 4373;
    const size_t outputRows = 2173;
    const size_t inputCols = outputCols/2;
    const size_t inputRows = outputRows/2;

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    reference::matrix_upsample_full(outputCols, outputRows,
                                    origin.data(), referenceOutput.data());

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    matrix_upsample_full(outputCols, outputRows,
                         origin.data(), testOutput.data());

    EXPECT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        EXPECT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
}

TEST(TestMantiuk06, UpsampleSimple)
{
    const size_t outputCols = 5000;
    const size_t outputRows = 2600;
    const size_t inputCols = outputCols >> 1;
    const size_t inputRows = outputRows >> 1;

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    reference::matrix_upsample_simple(outputCols, outputRows,
                                      origin.data(), referenceOutput.data());

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    matrix_upsample_simple(outputCols, outputRows,
                           origin.data(), testOutput.data());

    EXPECT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        EXPECT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
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

    reference::calculate_and_add_divergence(cols, rows,
                                            Gx.data(), Gy.data(),
                                            referenceOutput.data());

    std::vector<float> testOutput(cols*rows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    calculate_and_add_divergence(cols, rows,
                                 Gx.data(), Gy.data(),
                                 testOutput.data());

    EXPECT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        EXPECT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
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

    reference::calculate_gradient(cols, rows, input.data(),
                                  referenceGx.data(), referenceGy.data());

    // COMPUTED
    std::vector<float> computedGx(cols*rows);
    std::vector<float> computedGy(cols*rows);

    calculate_gradient(cols, rows, input.data(),
                       computedGx.data(), computedGy.data());

    // CHECK
    for (size_t idx = 0; idx < referenceGx.size(); ++idx)
    {
        EXPECT_NEAR(referenceGx[idx], computedGx[idx], 10e-6f);
    }
    for (size_t idx = 0; idx < referenceGy.size(); ++idx)
    {
        EXPECT_NEAR(referenceGy[idx], computedGy[idx], 10e-6f);
    }
}

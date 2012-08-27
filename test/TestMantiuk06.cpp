#include <gtest/gtest.h>
#include <stdlib.h>
#include <vector>
#include <tbb/tbb.h>

namespace reference
{
int imin(int a, int b)
{
    return (a > b) ? b : a;
}

//! \brief Reference implementation of matrix_upsample_full
void matrix_upsample_full(const size_t outCols, const size_t outRows,
                          const float* in, float* out)
{
    const size_t inRows = outRows/2;
    const size_t inCols = outCols/2;

    // Transpose of experimental downsampling matrix (theoretically the correct
    // thing to do)

    const float dx = (float)inCols / ((float)outCols);
    const float dy = (float)inRows / ((float)outRows);
    // This gives a genuine upsampling matrix, not the transpose of the
    // downsampling matrix
    // Theoretically, this should be the best.
    // const float factor = 1.0f;
    const float factor = 1.0f / (dx*dy);

    for (size_t y = 0; y < outRows; y++)
    {
        const float sy = y * dy;
        const int iy1 =      (  y   * inRows) / outRows;
        const int iy2 = imin(((y+1) * inRows) / outRows, inRows-1);

        for (size_t x = 0; x < outCols; x++)
        {
            const float sx = x * dx;
            const int ix1 =      (  x   * inCols) / outCols;
            const int ix2 = imin(((x+1) * inCols) / outCols, inCols-1);

            out[x + y*outCols] = (((ix1+1) - sx)*((iy1+1 - sy))
                                  * in[ix1 + iy1*inCols]
                                  +
                                  ((ix1+1) - sx)*(sy+dy - (iy1+1))
                                  * in[ix1 + iy2*inCols]
                                  +
                                  (sx+dx - (ix1+1))*((iy1+1 - sy))
                                  * in[ix2 + iy1*inCols]
                                  +
                                  (sx+dx - (ix1+1))*(sy+dx - (iy1+1))
                                  * in[ix2 + iy2*inCols])*factor;
        }
    }
}

} // reference

struct RandZeroOne
{
    float operator()()
    {
        return (static_cast<float>(rand())/RAND_MAX);
    }
};

// header for Mantiuk06
void matrix_upsample_full(const int outCols, const int outRows,
                          const float* const in, float* const out);

TEST(TestMantiuk06, TestMantiuk06UpsampleFull)
{
    const size_t outputCols = 4373;
    const size_t outputRows = 2173;
    const size_t inputCols = outputCols/2;
    const size_t inputRows = outputRows/2;

    // checking preconditions!
    //EXPECT_EQ(inputCols, 35u);
    //EXPECT_EQ(inputRows, 50u);

    std::vector<float> origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    std::vector<float> referenceOutput(outputCols*outputRows);

    tbb::tick_count ref_t0 = tbb::tick_count::now();
    reference::matrix_upsample_full(outputCols, outputRows,
                                    origin.data(), referenceOutput.data());
    tbb::tick_count ref_t1 = tbb::tick_count::now();
    double ref_t = (ref_t1 - ref_t0).seconds();

    std::vector<float> testOutput(outputCols*outputRows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    tbb::tick_count test_t0 = tbb::tick_count::now();
    matrix_upsample_full(outputCols, outputRows,
                         origin.data(), testOutput.data());
    tbb::tick_count test_t1 = tbb::tick_count::now();
    double test_t = (test_t1 - test_t0).seconds();

    EXPECT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        EXPECT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
    EXPECT_GT(ref_t, test_t);

    std::cout << "Speed up: " << ref_t/test_t << std::endl;
}

namespace reference
{
void calculate_and_add_divergence(size_t COLS, size_t ROWS,
                                  const float* Gx,
                                  const float* Gy,
                                  float* divG)
{
    float divGx, divGy;

    // kx = 0 AND ky = 0;
    divG[0] += Gx[0] + Gy[0];                       // OUT

    // ky = 0
    for (size_t kx=1; kx < COLS; kx++)
    {
        divGx = Gx[kx] - Gx[kx - 1];
        divGy = Gy[kx];
        divG[kx] += divGx + divGy;                    // OUT
    }

    for (size_t ky=1; ky < ROWS; ky++)
    {
        // kx = 0
        divGx = Gx[ky*COLS];
        divGy = Gy[ky*COLS] - Gy[ky*COLS - COLS];
        divG[ky*COLS] += divGx + divGy;               // OUT

        // kx > 0
        for (size_t kx=1; kx<COLS; kx++)
        {
            divGx = Gx[kx + ky*COLS] - Gx[kx + ky*COLS-1];
            divGy = Gy[kx + ky*COLS] - Gy[kx + ky*COLS - COLS];
            divG[kx + ky*COLS] += divGx + divGy;        // OUT
        }
    }

}
}

void calculate_and_add_divergence(const int COLS, const int ROWS,
                                  const float* const Gx, const float* const Gy,
                                  float* const divG);

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

    tbb::tick_count ref_t0 = tbb::tick_count::now();
    reference::calculate_and_add_divergence(cols, rows,
                                            Gx.data(), Gy.data(),
                                            referenceOutput.data());
    tbb::tick_count ref_t1 = tbb::tick_count::now();
    double ref_t = (ref_t1 - ref_t0).seconds();

    std::vector<float> testOutput(cols*rows);
    std::fill(testOutput.begin(), testOutput.end(), 0.f);

    tbb::tick_count test_t0 = tbb::tick_count::now();
    calculate_and_add_divergence(cols, rows,
                                 Gx.data(), Gy.data(),
                                 testOutput.data());
    tbb::tick_count test_t1 = tbb::tick_count::now();
    double test_t = (test_t1 - test_t0).seconds();

    EXPECT_EQ(referenceOutput.size(), testOutput.size());
    for (size_t idx = 0; idx < testOutput.size(); ++idx)
    {
        EXPECT_NEAR(referenceOutput[idx], testOutput[idx], 10e-6f);
    }
    EXPECT_GT(ref_t, test_t);

    std::cout << "Speed up: " << ref_t/test_t << std::endl;
}

#include <gtest/gtest.h>
#include <algorithm>

#include "Libpfs/array2d.h"
#include "Libpfs/manip/cut.h"

#include "SeqInt.h"
#include "PrintArray2D.h"

using namespace pfs;

TEST(TestPfsCut, NoCrop)
{
    const float ref[] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f,
                         6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
                         12.f, 13.f, 14.f, 15.f, 16.f, 17.f,
                         18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
                        24.f, 25.f, 26.f, 27.f, 28.f, 29.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2Df input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    Array2Df output(cols, rows);
    cut(&input, &output, 0, 0, input.getCols(), input.getRows());

    const float* outData = output.data();

    // print(input);
    // print(output);

    for (size_t idx = 0, idxEnd = output.getRows()*output.getCols(); idx < idxEnd; ++idx)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsCut, Crop_OneOne_ZeroZero)
{
    const float ref[] = {7.f, 8.f, 9.f, 10.f, 11.f,
                         13.f, 14.f, 15.f, 16.f, 17.f,
                         19.f, 20.f, 21.f, 22.f, 23.f,
                        25.f, 26.f, 27.f, 28.f, 29.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2Df input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    Array2Df output(cols-1, rows-1);
    cut(&input, &output, 1, 1, input.getCols(), input.getRows());

    const float* outData = output.data();

    // print(input);
    // print(output);

    for (size_t idx = 0, idxEnd = output.getRows()*output.getCols(); idx < idxEnd; ++idx)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsCut, Crop_OneOne_OneOne)
{
    const float ref[] = {7.f, 8.f, 9.f, 10.f,
                         13.f, 14.f, 15.f, 16.f,
                         19.f, 20.f, 21.f, 22.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2Df input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    Array2Df output(cols-2, rows-2);
    cut(&input, &output, 1, 1, input.getCols()-1, input.getRows()-1);

    const float* outData = output.data();

    // print(input);
    // print(output);

    for (size_t idx = 0, idxEnd = output.getRows()*output.getCols(); idx < idxEnd; ++idx)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsCut, Crop_ZeroZero_OneOne)
{
    const float ref[] = {0.f, 1.f, 2.f, 3.f, 4.f,
                         6.f, 7.f, 8.f, 9.f, 10.f,
                         12.f, 13.f, 14.f, 15.f, 16.f,
                         18.f, 19.f, 20.f, 21.f, 22.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2Df input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    Array2Df output(cols-1, rows-1);
    cut(&input, &output, 0, 0, input.getCols()-1, input.getRows()-1);

    const float* outData = output.data();

    // print(input);
    // print(output);

    // for (int idx = 0; idx < output.getRows()*output.getCols(); idx++)
    for (size_t idx = 0, idxEnd = output.getRows()*output.getCols(); idx < idxEnd; ++idx)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsCut, Crop_OneTwo_TwoOne)
{
    const float ref[] = { 13.f, 14.f, 15.f,
                          19.f, 20.f, 21.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2Df input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    Array2Df output(cols-3, rows-3);
    cut(&input, &output, 1, 2, input.getCols()-2, input.getRows()-1);

    const float* outData = output.data();

    // print(input);
    // print(output);

    //for (int idx = 0; idx < output.getRows()*output.getCols(); idx++)
    for (size_t idx = 0, idxEnd = output.getRows()*output.getCols(); idx < idxEnd; ++idx)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

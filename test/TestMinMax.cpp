#include <gtest/gtest.h>
#include <Libpfs/utils/minmax.h>

using namespace pfs::utils;

TEST(MinMax, Test1) {

    int min;
    int max;

    minmax(3, 2, 1, min, max);

    ASSERT_EQ(max, 3);
    ASSERT_EQ(min, 1);

    minmax(2,2,1, min, max);

    ASSERT_EQ(max, 2);
    ASSERT_EQ(min, 1);

    minmax(2,1,3, min, max);

    ASSERT_EQ(max, 3);
    ASSERT_EQ(min, 1);

    minmax(1,2,1, min, max);

    ASSERT_EQ(max, 2);
    ASSERT_EQ(min, 1);

    minmax(0,1,1, min, max);

    ASSERT_EQ(max, 1);
    ASSERT_EQ(min, 0);

    minmax(3,2,3, min, max);

    ASSERT_EQ(max, 3);
    ASSERT_EQ(min, 2);

}

#include <gtest/gtest.h>
#include <Libpfs/vex/vex.h>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace std;
using namespace boost::assign;  // bring 'operator+=()' into scope

static const size_t V_SIZE = 100;

namespace
{

template <typename T>
struct vmul
{
    T operator()(const T& t1, const T& t2) const
    {
        return (t1 * t2);
    }
};

}

class TestVex : public testing::Test
{
public:
    typedef std::vector<float> TestVexContainer;

protected:
    TestVexContainer input1;
    TestVexContainer input2;
    TestVexContainer outputReference;
    TestVexContainer outputComputed;

    // helper function
    static float randZeroOne()
    {
        return (static_cast<float>(rand())/RAND_MAX);
    }

    TestVex()
        : input1(V_SIZE)
        , input2(V_SIZE)
        , outputReference(V_SIZE)
        , outputComputed(V_SIZE)
    {
        std::generate(input1.begin(), input1.end(), randZeroOne);
        std::generate(input2.begin(), input2.end(), randZeroOne);
        std::fill(outputReference.begin(), outputReference.end(), 0.f);
        std::fill(outputComputed.begin(), outputComputed.end(), 0.f);
    }
};

TEST_F(TestVex, VMUL)
{
    vex::vmul(input1.data(), input2.data(),
              outputComputed.data(), outputComputed.size());

    std::transform(input1.begin(), input1.end(),
                   input2.begin(),
                   outputReference.begin(),
                   vmul<TestVex::TestVexContainer::value_type>());

    for (size_t idx = 0; idx < outputComputed.size(); ++idx)
    {
        EXPECT_NEAR(outputComputed[idx], outputReference[idx], 10e-6);
    }
}

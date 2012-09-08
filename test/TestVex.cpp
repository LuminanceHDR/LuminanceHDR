#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <Libpfs/vex/vex.h>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace std;

static const size_t V_SIZE = 1000000;

template <typename T>
class FillValue;

template<>
struct FillValue<float>
{
    float operator()() { return (static_cast<float>(rand())/RAND_MAX)*2.f - 1.f; }
};

template<>
struct FillValue<uint8_t>
{
    uint8_t operator()() { return (static_cast<uint8_t>(rand())%255) + 1; }
};

template<>
struct FillValue<uint16_t>
{
    uint16_t operator()() { return (static_cast<uint16_t>(rand())%255) + 1; }
};

template<>
struct FillValue<int32_t>
{
    int32_t operator()() {
        int32_t v = static_cast<int32_t>(rand())%256 - 128;
        return (!v) ? 1 : v;
    }
};

template<typename T>
T getConst();

template<>
float getConst<float>() { return static_cast<float>(M_PI); }

template<>
uint8_t getConst<uint8_t>() { return 3u; }

template<>
uint16_t getConst<uint16_t>() { return 3u; }

template<>
int32_t getConst<int32_t>() { return 5; }

template <typename T>
struct vmul
{
    T operator()(const T& t1, const T& t2) const { return (t1 * t2); }
};

template <typename T>
struct vdiv
{
    T operator()(const T& t1, const T& t2) const { return (t1 / t2); }
};

template <typename T>
struct vadd
{
    T operator()(const T& t1, const T& t2) const { return (t1 + t2); }
};

template <typename T>
struct vadds
{
    vadds(const T& s) : s_(s) {}
    T operator()(const T& t1, const T& t2) const { return (t1 + (s_*t2)); }
private:
    T s_;
};

template <typename T>
struct vsub
{
    T operator()(const T& t1, const T& t2) const { return (t1 - t2); }
};

template <typename T>
struct vsubs
{
    vsubs(const T& s) : s_(s) {}
    T operator()(const T& t1, const T& t2) const { return (t1 - (s_*t2)); }
private:
    T s_;
};

template <class T>
class TestVex : public testing::Test
{
protected:
    typedef T ValueType;
    typedef std::vector<T> TestVexContainer;


    TestVexContainer input1;
    TestVexContainer input2;
    TestVexContainer outputReference;
    TestVexContainer outputComputed;
    T s_;

    TestVex()
        : input1(V_SIZE)
        , input2(V_SIZE)
        , outputReference(V_SIZE)
        , outputComputed(V_SIZE)
        , s_(getConst<T>())
    {
        std::generate(input1.begin(), input1.end(), FillValue<T>());
        std::generate(input2.begin(), input2.end(), FillValue<T>());
        std::fill(outputReference.begin(), outputReference.end(), T());
        std::fill(outputComputed.begin(), outputComputed.end(), T());
    }

    void compareResult()
    {
        for (size_t idx = 0; idx < this->outputComputed.size(); ++idx)
        {
            EXPECT_NEAR(this->outputComputed[idx],
                        this->outputReference[idx],
                        10e-9);
        }
    }

    void computeVmul()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vmul<ValueType>());
    }

    void computeVdiv()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vdiv<ValueType>());
    }

    void computeVadd()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vadd<ValueType>());
    }
    void computeVadds()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vadds<ValueType>(s_));
    }

    void computeVsub()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vsub<ValueType>());
    }
    void computeVsubs()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vsubs<ValueType>(s_));
    }
};

using testing::Types;
// The list of types we want to test.
typedef testing::Types<float, uint8_t, uint16_t, int32_t> Implementations;

TYPED_TEST_CASE(TestVex, Implementations);

TYPED_TEST(TestVex, vmul)
{
    vex::vmul(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVmul();
    this->compareResult();
}

TYPED_TEST(TestVex, vdiv)
{
    vex::vdiv(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVdiv();
    this->compareResult();
}

TYPED_TEST(TestVex, vadd)
{
    vex::vadd(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVadd();
    this->compareResult();
}
TYPED_TEST(TestVex, vadds)
{
    vex::vadds(this->input1.data(),
               this->s_,
               this->input2.data(),
               this->outputComputed.data(),
               this->outputComputed.size());

    this->computeVadds();
    this->compareResult();
}

TYPED_TEST(TestVex, vsub)
{
    vex::vsub(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVsub();
    this->compareResult();
}
TYPED_TEST(TestVex, vsubs)
{
    vex::vsubs(this->input1.data(),
               this->s_,
               this->input2.data(),
               this->outputComputed.data(),
               this->outputComputed.size());

    this->computeVsubs();
    this->compareResult();
}

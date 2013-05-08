#include <vector>
#include <iostream>
#include <cmath>

#include <HdrCreation/weights.h>

using namespace std;
using namespace libhdr::fusion;

static const
size_t SAMPLES = (2 << 8) - 1;

int main() // int argc, char** argv)
{
    WeightTriangular wtriangular;
    vector<float> data(SAMPLES);

    weights_triangle(data.data(), SAMPLES);

    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = wtriangular.getWeight(((float)idx)/SAMPLES);

        cout << idx << " " << data[idx] << " "
             << w << " " << abs(data[idx] - w)
             << "\n";
    }

    return 0;
}

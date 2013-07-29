#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#include <HdrCreation/responses.h>

using namespace std;
using namespace libhdr::fusion;

static const size_t SAMPLES = 256;

void printLinear()
{
    ResponseLinear response;
    vector<float> data(SAMPLES);

    ofstream outputFile("response_linear.dat");
    responseLinear(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = response.getResponse((float)idx/(SAMPLES - 1));

        outputFile << idx << " " << data[idx] << " "
                   << w << " " << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

void printLog10()
{
    ResponseLog10 response;
    vector<float> data(SAMPLES);

    ofstream outputFile("response_log10.dat");
    responseLog10(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = response.getResponse((float)idx/(SAMPLES-1));

        outputFile << idx << " " << data[idx] << " "
                   << w << " " << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

void printGamma()
{
    ResponseGamma response;
    vector<float> data(SAMPLES);

    ofstream outputFile("response_gamma.dat");
    responseGamma(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = response.getResponse((float)idx/(SAMPLES-1));

        outputFile << idx << " " << data[idx] << " "
                   << w << " " << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

int main()
{
    printLinear();
    printLog10();
    printGamma();

    return 0;
}

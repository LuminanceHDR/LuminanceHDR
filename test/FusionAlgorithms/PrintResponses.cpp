#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#include <HdrCreation/responses.h>

using namespace std;
using namespace libhdr::fusion;

static const size_t SAMPLES = 1024;

static float getSample(int sample)
{
    return static_cast<float>(sample)/(SAMPLES - 1);
}

void printResponseCurve(const ResponseCurve& responseFunction1,
                        void (*responseFunction2)(float*, int) ,
                        const std::string& fileNameGnuplot,
                        const std::string& fileNameMatlab)
{
    vector<float> data(SAMPLES);

    ofstream outputFile(fileNameGnuplot.c_str());
    responseFunction2(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float sample = getSample(idx);
        float w = responseFunction1.getResponse(sample);

        outputFile << idx << " "
                   << sample << " "
                   << data[idx] << " "
                   << w << " "
                   << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();

    responseFunction1.writeToFile(fileNameMatlab);
}

void responseLinear( float* I, int M )
{
    for (int m=0 ; m<M ; m++)
    {
        I[m] = m / float(M-1); // range is not important, values are normalized later
    }
}

void responseLog10( float* I, int M )
{
    const float mid = 0.5f * M;
    const float norm = 0.0625f * M;
    const float maxValue = powf(10.0f, float(M-1 - mid) / norm);

    for (int m=0 ; m < M; ++m)
    {
        I[m] = powf(10.0f, float(m - mid) / norm) / maxValue;
    }
}

void responseGamma( float* I, int M )
{
    float norm = M / 4.0f;

    // response curve decided empirically
    for( int m=0 ; m<M ; m++ )
        I[m] = powf( m/norm, 1.7f ) + 1e-4;
}

int main()
{
    printResponseCurve(ResponseCurve(RESPONSE_LINEAR), &responseLinear,
                       "response_linear.dat", "response_linear.m");
    printResponseCurve(ResponseCurve(RESPONSE_LOG10), &responseLog10,
                       "response_log10.dat", "response_log10.m");
    printResponseCurve(ResponseCurve(RESPONSE_GAMMA), &responseGamma,
                       "response_gamma.dat", "response_gamma.m");
    printResponseCurve(ResponseCurve(RESPONSE_SRGB), &responseLinear,
                       "response_srgb.dat", "response_srgb.m");

    return 0;
}

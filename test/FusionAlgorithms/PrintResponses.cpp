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

void responseLinear( float* I, int M )
{
    for( int m=0 ; m<M ; m++ )
        I[m] = m / float(M-1); // range is not important, values are normalized later
}

void printLinear()
{
    ResponseLinear response;
    vector<float> data(SAMPLES);

    ofstream outputFile("response_linear.dat");
    responseLinear(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float sample = getSample(idx);
        float w = response.getResponse(sample);

        outputFile << idx << " "
                   << sample << " "
                   << data[idx] << " "
                   << w << " "
                   << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

void responseLog10( float* I, int M )
{
    const float mid = 0.5f * M;
    const float norm = 0.0625f * M;
    const float maxValue = powf(10.0f, float(M-1 - mid) / norm);

    for( int m=0 ; m < M; ++m)
    {
        I[m] = powf(10.0f, float(m - mid) / norm) / maxValue;
    }
}

void printLog10()
{
    ResponseLog10 response;
    vector<float> data(SAMPLES);

    ofstream outputFile("response_log10.dat");
    responseLog10(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float sample = getSample(idx);
        float w = response.getResponse(sample);

        outputFile << idx << " "
                   << sample << " "
                   << data[idx] << " "
                   << w << " "
                   << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

void responseGamma( float* I, int M )
{
    float norm = M / 4.0f;

    // response curve decided empirically
    for( int m=0 ; m<M ; m++ )
        I[m] = powf( m/norm, 1.7f ) + 1e-4;
}


void printGamma()
{
    ResponseGamma response;
    vector<float> data(SAMPLES);

    ofstream outputFile("response_gamma.dat");
    responseGamma(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float sample = getSample(idx);
        float w = response.getResponse(sample);

        outputFile << idx << " "
                   << sample << " "
                   << data[idx] << " "
                   << w << " "
                   << abs(data[idx] - w)
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

#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#include <HdrCreation/weights.h>

using namespace std;
using namespace libhdr::fusion;

static const size_t SAMPLES = (1 << 8) - 1;  // 255

void printTriangular()
{
    WeightTriangular weights;
    vector<float> data(SAMPLES);

    ofstream outputFile("data_triangular.dat");
    weights_triangle(data.data(), SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = weights.getWeight(((float)idx)/SAMPLES);

        outputFile << idx << " " << data[idx] << " "
                   << w << " " << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

void printGaussian()
{
    WeightGaussian weights;
    vector<float> data(SAMPLES);

    ofstream outputFile("data_gauss.dat");
    weightsGauss(data.data(), SAMPLES, 0, SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = weights.getWeight(((float)idx)/SAMPLES);

        outputFile << idx << " " << data[idx] << " "
                   << w << " " << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

void printPlateau()
{
    WeightPlateau weights;
    vector<float> data(SAMPLES);

    ofstream outputFile("data_plateau.dat");
    exposure_weights_icip06(data.data(), SAMPLES, 0, SAMPLES);
    for (size_t idx = 0; idx < SAMPLES; ++idx)
    {
        float w = weights.getWeight(((float)idx)/SAMPLES);

        outputFile << idx << " " << data[idx] << " "
                   << w << " " << abs(data[idx] - w)
                   << "\n";
    }
    outputFile.close();
}

int main() // int argc, char** argv)
{
    printTriangular();
    printGaussian();
    printPlateau();

    return 0;
}

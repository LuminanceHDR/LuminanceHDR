#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#include <HdrCreation/weights.h>

using namespace std;
using namespace libhdr::fusion;

static const size_t SAMPLES = WeightFunction::NUM_BINS;

// OLD STUFF
#define MIN_WEIGHT 1e-3

void exposure_weights_icip06( float* w, int M, int Mmin, int Mmax )
{
    for( int m=0 ; m<M ; m++ )
        if( m<Mmin || m>Mmax )
            w[m] = 0.0f;
        else
            w[m]=1.0f-pow( ( (2.0f*float(m-Mmin)/float(Mmax-Mmin)) - 1.0f), 12.0f);
}

void weightsGauss( float* w, int M, int Mmin, int Mmax, float sigma  = 8.0f)
{
    float mid = Mmin + (Mmax-Mmin)/2.0f - 0.5f;
    float mid2 = (mid-Mmin) * (mid-Mmin);
    for( int m=0 ; m<M ; m++ ) {
        if ( m<Mmin || m>Mmax ) {
            w[m] = 0.0f;
        } else {
            // gkrawczyk: that's not really a gaussian, but equation is
            // taken from Robertson02 paper.
            float weight = exp( -sigma * (m-mid) * (m-mid) / mid2 );

            if( weight < MIN_WEIGHT )           // ignore very low weights
                w[m] = 0.0f;
            else
                w[m] = weight;
        }
    }
}

void weights_triangle( float* w, int M /*, int Mmin, int Mmax*/ )
{
    for(int i=0;i<int(float(M)/2.0f);i++) {
        w[i]=i/ (float(M)/2.0f);
        if (w[i]<0.06f)w[i]=0;
    }
    for(int i=int(float(M)/2.0f);i<M;i++) {
        w[i]=(M-1-i)/(float(M)/2.0f);
        if (w[i]<0.06f)w[i]=0;
    }
    //   for( int m=0 ; m<M ; m++ )
    //     if( m<Mmin || m>Mmax )
    //       w[m] = 0.0f;
    //     else
    //     {
    // 	if ( m<int(Mmin+ (Mmax-Mmin)/2.0f +1) )
    // 		w[m]=(m-Mmin)/float(Mmin+(Mmax-Mmin)/2.0f);
    // 	else
    // 		w[m]=( -m+Mmin+((Mmax-Mmin)) )/float(Mmin+(Mmax-Mmin)/2.0f);
    //     }

    // 	  if (w[i]<0.06f)w[i]=0;
}

void printTriangular()
{
    WeightFunction weights(WEIGHT_TRIANGULAR);
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
    WeightFunction weights(WEIGHT_GAUSSIAN);
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
    WeightFunction weights(WEIGHT_PLATEAU);
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

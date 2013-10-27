#include "normalizer.h"

Normalizer::Normalizer(float m, float M)
    : m(m)
    , M(M)
{}

float Normalizer::operator()(float sample)
{
    return (sample - m)/(M-m);
}

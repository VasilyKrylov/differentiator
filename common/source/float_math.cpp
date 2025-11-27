#include <math.h>
#include <assert.h>

#include "float_math.h"

static const double EPS = 1.11e-16; // https://en.wikipedia.org/wiki/Machine_epsilon

bool IsEqual(double a, double b) 
{
    assert(isfinite(a));
    assert(isfinite(b));

    return abs(a - b) <= EPS;
}

double logWithBase (double base, double x) 
{
    return log (x) / log (base);
}
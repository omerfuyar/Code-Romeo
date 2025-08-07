#include "utilities/Algebra.h"
#include <math.h>

float Log(float value, float base)
{
    return logf(value) / logf(base);
}

float Log10(float value)
{
    return log10f(value);
}

float LogE(float value)
{
    return logf(value);
}

float Root(float value, float root)
{
    return powf(value, 1.0f / root);
}

float SquareRoot(float value)
{
    return sqrtf(value);
}

float CubeRoot(float value)
{
    return cbrtf(value);
}

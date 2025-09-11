#include "utilities/Maths.h"
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

float Sin(float deg)
{
    return sinf(DegToRad(deg));
}

float Cos(float deg)
{
    return cosf(DegToRad(deg));
}

float Tan(float deg)
{
    return tanf(DegToRad(deg));
}

float Cot(float deg)
{
    return 1.0f / Tan(deg);
}

float ATan2(float x, float y)
{
    return RadToDeg(atan2f(y, x));
}

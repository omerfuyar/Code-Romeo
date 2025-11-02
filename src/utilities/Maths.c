#include "utilities/Maths.h"

#include <math.h>

float Maths_Log(float value, float base)
{
    return logf(value) / logf(base);
}

float Maths_Root(float value, float rootDegree)
{
    return powf(value, 1.0f / rootDegree);
}

float Maths_Power(float base, float exponent)
{
    return powf(base, exponent);
}

float Maths_SquareRoot(float value)
{
    return sqrtf(value);
}

float CubeRoot(float value)
{
    return cbrtf(value);
}

float Maths_Sin(float degree)
{
    return sinf(Maths_DegToRad(degree));
}

float Maths_Cos(float degree)
{
    return cosf(Maths_DegToRad(degree));
}

float Maths_Tan(float degree)
{
    return tanf(Maths_DegToRad(degree));
}

float Maths_Cot(float degree)
{
    return 1.0f / Maths_Tan(degree);
}

float Maths_ATan2(float x, float y)
{
    return Maths_RadToDeg(atan2f(y, x));
}

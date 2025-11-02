#include "utilities/Maths.h"
#include <math.h>

float Maths_Log(float value, float base)
{
    return logf(value) / logf(base);
}

float Maths_Log10(float value)
{
    return log10f(value);
}

float Maths_LogE(float value)
{
    return logf(value);
}

float Maths_Root(float value, float root)
{
    return powf(value, 1.0f / root);
}

float Maths_SquareRoot(float value)
{
    return sqrtf(value);
}

float CubeRoot(float value)
{
    return cbrtf(value);
}

float Maths_Sin(float deg)
{
    return sinf(Maths_DegToRad(deg));
}

float Maths_Cos(float deg)
{
    return cosf(Maths_DegToRad(deg));
}

float Maths_Tan(float deg)
{
    return tanf(Maths_DegToRad(deg));
}

float Maths_Cot(float deg)
{
    return 1.0f / Maths_Tan(deg);
}

float Maths_ATan2(float x, float y)
{
    return Maths_RadToDeg(atan2f(y, x));
}

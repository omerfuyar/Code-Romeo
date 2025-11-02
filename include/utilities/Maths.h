#pragma once

#include "Global.h"

#pragma region Macros

// These constants have a suffix of '_M' to not conflict with any system variables or macros.

// The value of Pi
#define MATHS_PI 3.14159265f

// The value of Euler's number
#define MATHS_E 2.71828183f

// The Earth's gravity in m/s^2
#define MATHS_GRAVITY 9.80665f

// The Universal gravitational constant in m^3 kg^-1 s^-2
#define MATHS_G 6.67430e-11f

// The speed of light in m/s
#define MATHS_C 299792458.0f

#define Maths_Min(a, b) ((a) < (b) ? (a) : (b))
#define Maths_Max(a, b) ((a) > (b) ? (a) : (b))
#define Maths_Abs(a) ((a) < 0 ? -(a) : (a))
#define Maths_Clamp(value, minValue, maxValue) (Maths_Min(Maths_Max(value, minValue), maxValue))
#define Maths_DegToRad(deg) (deg * MATHS_PI / 180.0f)
#define Maths_RadToDeg(rad) (rad * 180.0f / MATHS_PI)
#define Maths_RandomRange(min, max) (rand() % ((max) - (min) + 1) + (min))

#pragma endregion Macros

// Returns the logarithm of the value to the base
float Maths_Log(float value, float base);

// Returns the logarithm of the value to the base 10
float Maths_Log10(float value);

// Returns the natural logarithm of the value
float Maths_LogE(float value);

// Returns the root of the float value
float Maths_Root(float value, float root);

// Returns the sine of the given degree
float Maths_Sin(float deg);

// Returns the cosine of the given degree
float Maths_Cos(float deg);

// Returns the tangent of the given degree
float Maths_Tan(float deg);

// Returns the cotangent of the given degree
float Maths_Cot(float deg);

// Returns the degree value of the given x and y values
float Maths_ATan2(float x, float y);

#pragma once

#include "Global.h"

#pragma region Macros

// These constants have a suffix of '_M' to not conflict with any system variables or macros.

// The value of Pi
#define PI_M 3.14159265f

// The value of Euler's number
#define E_M 2.71828183f

// The square root of 2
#define SQRT2_M 1.41421356f

// The square root of 3
#define SQRT3_M 1.73205081f

// The square root of 5
#define SQRT5_M 2.23606798f

// The Earth's gravity in m/s^2
#define GRAVITY_M 9.80665f

// The Universal gravitational constant in m^3 kg^-1 s^-2
#define G_M 6.67430e-11f

// The speed of light in m/s
#define C_M 299792458.0f

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Abs(a) ((a) < 0 ? -(a) : (a))
#define Clamp(value, minValue, maxValue) (Min(Max(value, minValue), maxValue))
#define DegToRad(deg) (deg * PI_M / 180.0f)
#define RadToDeg(rad) (rad * 180.0f / PI_M)
#define RandomRange(min, max) (rand() % ((max) - (min) + 1) + (min))

#pragma endregion Macros

// Returns the logarithm of the value to the base
float Log(float value, float base);

// Returns the logarithm of the value to the base 10
float Log10(float value);

// Returns the natural logarithm of the value
float LogE(float value);

// Returns the root of the float value
float Root(float value, float root);

// Return square root of the float value
float SquareRoot(float value);

// Returns the cube root of the float value
float CubeRoot(float value);

// Returns the sine of the given degree
float Sin(float deg);

// Returns the cosine of the given degree
float Cos(float deg);

// Returns the tangent of the given degree
float Tan(float deg);

// Returns the cotangent of the given degree
float Cot(float deg);

// Returns the degree value of the given x and y values
float ATan2(float x, float y);

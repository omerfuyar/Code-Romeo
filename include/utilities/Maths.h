#pragma once

#include "RJGlobal.h"

#pragma region Macros

/// @brief The value of Pi
#define MATHS_PI 3.14159265f
/// @brief The value of Euler's number
#define MATHS_E 2.71828183f
/// @brief The Earth's gravity in m/s^2
#define MATHS_GRAVITY 9.80665f
/// @brief The Universal gravitational constant in m^3 kg^-1 s^-2
#define MATHS_G 6.67430e-11f
/// @brief The speed of light in m/s
#define MATHS_C 299792458.0f

/// @brief Selects the minimum of two values.
#define Maths_Min(a, b) ((a) < (b) ? (a) : (b))
/// @brief Selects the maximum of two values.
#define Maths_Max(a, b) ((a) > (b) ? (a) : (b))
/// @brief Returns the absolute value of a.
#define Maths_Abs(a) ((a) < 0 ? -(a) : (a))
/// @brief Clamps a value between a minimum and maximum value.
#define Maths_Clamp(value, minValue, maxValue) (Maths_Min(Maths_Max(value, minValue), maxValue))
/// @brief Converts degrees to radians.
#define Maths_DegToRad(deg) (deg * MATHS_PI / 180.0f)
/// @brief Converts radians to degrees.
#define Maths_RadToDeg(rad) (rad * 180.0f / MATHS_PI)
/// @brief Generates a random integer between min and max (inclusive).
#define Maths_RandomRangeI(min, max) (rand() % ((max) - (min) + 1) + (min))
/// @brief Generates a random float between min and max.
#define Maths_RandomRangeF(min, max) (((float)rand() / (float)RAND_MAX) * ((max) - (min)) + (min))

#pragma endregion Macros

/// @brief Returns the logarithm of the value to the base.
/// @param value The value to calculate the logarithm for.
/// @param base The base of the logarithm.
/// @return The logarithm of the value to the base.
float Maths_Log(float value, float base);

/// @brief Calculates the root of a value.
/// @param value Value to calculate the root of.
/// @param rootDegree The degree of the root.
/// @return The root of the value.
float Maths_Root(float value, float rootDegree);

/// @brief Calculates the power of a base raised to an exponent.
/// @param base The base value.
/// @param exponent The exponent value.
/// @return The result of base raised to the power of exponent.
float Maths_Power(float base, float exponent);

/// @brief Calculates the sine of a given angle in degrees.
/// @param degree The angle in degrees.
/// @return The sine of the angle.
float Maths_Sin(float degree);

/// @brief Calculates the cosine of a given angle in degrees.
/// @param degree The angle in degrees.
/// @return The cosine of the angle.
float Maths_Cos(float degree);

/// @brief Calculates the tangent of a given angle in degrees.
/// @param degree The angle in degrees.
/// @return The tangent of the angle.
float Maths_Tan(float degree);

/// @brief Calculates the cotangent of a given angle in degrees.
/// @param degree The angle in degrees.
/// @return The cotangent of the angle.
float Maths_Cot(float degree);

/// @brief Calculates the angle in degrees from the x and y coordinates.
/// @param x The x value.
/// @param y The y value.
/// @return The angle in degrees.
float Maths_ATan2(float x, float y);

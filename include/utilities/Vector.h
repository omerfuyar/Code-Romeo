#pragma once

#include "RJGlobal.h"

#pragma region typedefs

/// @brief A vector that contains 2 float values. Can be used with helper functions.
typedef struct Vector2
{
    float x;
    float y;
} Vector2;

/// @brief A vector that contains 3 float values. Can be used with helper functions.
typedef struct Vector3
{
    float x;
    float y;
    float z;
} Vector3;

/// @brief A vector that contains 4 float values. Can be used with helper functions.
typedef struct Vector4
{
    float x;
    float y;
    float z;
    float w;
} Vector4;

typedef Vector4 Color;

/// @brief A vector that contains 2 integer values. Can be used with helper functions.
typedef struct Vector2Int
{
    int x;
    int y;
} Vector2Int;

/// @brief A vector that contains 3 integer values. Can be used with helper functions.
typedef struct Vector3Int
{
    int x;
    int y;
    int z;
} Vector3Int;

/// @brief A vector that contains 4 integer values. Can be used with helper functions.
typedef struct Vector4Int
{
    int x;
    int y;
    int z;
    int w;
} Vector4Int;

#pragma endregion typedefs

/// @brief Creates a new Vector2 struct.
#define Vector2_New(x, y) ((Vector2){(float)(x), (float)(y)})
/// @brief Creates a new Vector3 struct.
#define Vector3_New(x, y, z) ((Vector3){(float)(x), (float)(y), (float)(z)})
/// @brief Creates a new Vector4 struct.
#define Vector4_New(x, y, z, w) ((Vector4){(float)(x), (float)(y), (float)(z), (float)(w)})
/// @brief Creates a new Color struct.
#define Color_New(x, y, z, w) Vector4_New(x, y, z, w)
/// @brief Creates a new Vector2Int struct.
#define Vector2Int_New(x, y) ((Vector2Int){(int)(x), (int)(y)})
/// @brief Creates a new Vector3Int struct.
#define Vector3Int_New(x, y, z) ((Vector3Int){(int)(x), (int)(y), (int)(z)})
/// @brief Creates a new Vector4Int struct.
#define Vector4Int_New(x, y, z, w) ((Vector4Int){(int)(x), (int)(y), (int)(z), (int)(w)})

/// @brief Creates new Vector2 with all components set to the same value.
#define Vector2_NewN(n) (Vector2_New(n, n))
/// @brief Creates new Vector3 with all components set to the same value.
#define Vector3_NewN(n) (Vector3_New(n, n, n))
/// @brief Creates new Vector4 with all components set to the same value.
#define Vector4_NewN(n) (Vector4_New(n, n, n, n))
/// @brief Creates new Vector2Int with all components set to the same value.
#define Vector2Int_NewN(n) (Vector2Int_New(n, n))
/// @brief Creates new Vector3Int with all components set to the same value.
#define Vector3Int_NewN(n) (Vector3Int_New(n, n, n))
/// @brief Creates new Vector4Int with all components set to the same value.
#define Vector4Int_NewN(n) (Vector4Int_New(n, n, n, n))

#define Vector2_Zero Vector2_NewN(0.0f)
#define Vector3_Zero Vector3_NewN(0.0f)
#define Vector4_Zero Vector4_NewN(0.0f)

#define Vector2_One Vector2_NewN(1.0f)
#define Vector3_One Vector3_NewN(1.0f)
#define Vector4_One Vector4_NewN(1.0f)

#define Vector2_Up Vector2_New(0.0f, 1.0f)
#define Vector2_Down Vector2_New(0.0f, -1.0f)
#define Vector2_Right Vector2_New(1.0f, 0.0f)
#define Vector2_Left Vector2_New(-1.0f, 0.0f)

#define Vector3_Up Vector3_New(0.0f, 1.0f, 0.0f)
#define Vector3_Down Vector3_New(0.0f, -1.0f, 0.0f)
#define Vector3_Right Vector3_New(1.0f, 0.0f, 0.0f)
#define Vector3_Left Vector3_New(-1.0f, 0.0f, 0.0f)
#define Vector3_Forward Vector3_New(0.0f, 0.0f, 1.0f)
#define Vector3_Backward Vector3_New(0.0f, 0.0f, -1.0f)

#define Color_White Color_New(1.0f, 1.0f, 1.0f, 1.0f)
#define Color_Black Color_New(0.0f, 0.0f, 0.0f, 1.0f)
#define Color_Red Color_New(1.0f, 0.0f, 0.0f, 1.0f)
#define Color_Green Color_New(0.0f, 1.0f, 0.0f, 1.0f)
#define Color_Blue Color_New(0.0f, 0.0f, 1.0f, 1.0f)
#define Color_Yellow Color_New(1.0f, 1.0f, 0.0f, 1.0f)
#define Color_Cyan Color_New(0.0f, 1.0f, 1.0f, 1.0f)
#define Color_Magenta Color_New(1.0f, 0.0f, 1.0f, 1.0f)
#define Color_Gray Color_New(0.5f, 0.5f, 0.5f, 1.0f)
#define Color_Clear Color_New(0.0f, 0.0f, 0.0f, 0.0f)

#define Vector2_Compare(v1, v2) (((v1).x == (v2).x) && ((v1).y == (v2).y))
#define Vector3_Compare(v1, v2) (((v1).x == (v2).x) && ((v1).y == (v2).y) && ((v1).z == (v2).z))
#define Vector4_Compare(v1, v2) (((v1).x == (v2).x) && ((v1).y == (v2).y) && ((v1).z == (v2).z) && ((v1).w == (v2).w))
#define Color_Compare(c1, c2) Vector4_Compare(c1, c2)

// Generic functions for both float and integer vectors

#define Vector2_Sum(v1, v2) ((typeof(v1)){(v1).x + (v2).x, (v1).y + (v2).y})
#define Vector3_Sum(v1, v2) ((typeof(v1)){(v1).x + (v2).x, (v1).y + (v2).y, (v1).z + (v2).z})
#define Vector4_Sum(v1, v2) ((typeof(v1)){(v1).x + (v2).x, (v1).y + (v2).y, (v1).z + (v2).z, (v1).w + (v2).w})

#define Vector2_Scale(v1, s) ((typeof(v1)){(v1).x * s, (v1).y * s})
#define Vector3_Scale(v1, s) ((typeof(v1)){(v1).x * s, (v1).y * s, v1.z * s})
#define Vector4_Scale(v1, s) ((typeof(v1)){(v1).x * s, (v1).y * s, (v1).z * s, (v1).w * s})

#define Vector2_ToInt(v) (Vector2Int_New((v).x, (v).y))
#define Vector3_ToInt(v) (Vector3Int_New((v).x, (v).y, (v).z))
#define Vector4_ToInt(v) (Vector4Int_New((v).x, (v).y, (v).z, (v).w))

#define Vector2Int_ToFloat(v) (Vector2_New((v).x, (v).y))
#define Vector3Int_ToFloat(v) (Vector3_New((v).x, (v).y, (v).z))
#define Vector4Int_ToFloat(v) (Vector4_New((v).x, (v).y, (v).z, (v).w))

#pragma region Vector2

/// @brief Normalizes a 2D vector to have a magnitude of 1.
/// @param vector The vector to normalize.
/// @return The normalized vector.
Vector2 Vector2_Normalized(Vector2 vector);

/// @brief Calculates the magnitude (length) of a 2D vector.
/// @param vector The vector to calculate the magnitude for.
/// @return The magnitude of the vector.
float Vector2_Magnitude(Vector2 vector);

/// @brief Calculates the dot product of two 2D vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The dot product of the two vectors.
float Vector2_Dot(Vector2 vector1, Vector2 vector2);

/// @brief Linearly interpolates between two 2D vectors.
/// @param startVector The starting vector.
/// @param endVector The ending vector.
/// @param time The interpolation factor (0.0 to 1.0).
/// @return The interpolated vector.
Vector2 Vector2_Lerp(Vector2 startVector, Vector2 endVector, float time);

#pragma endregion Vector2

#pragma region Vector3

/// @brief Normalizes a 3D vector to have a magnitude of 1.
/// @param vector The vector to normalize.
/// @return The normalized vector.
Vector3 Vector3_Normalized(Vector3 vector);

/// @brief Calculates the magnitude (length) of a 3D vector.
/// @param vector The vector to calculate the magnitude for.
/// @return The magnitude of the vector.
float Vector3_Magnitude(Vector3 vector);

/// @brief Calculates the dot product of two 3D vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The dot product of the two vectors.
float Vector3_Dot(Vector3 vector1, Vector3 vector2);

/// @brief Calculates the cross product of two Vector3 vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The cross product of the two vectors.
Vector3 Vector3_Cross(Vector3 vector1, Vector3 vector2);

/// @brief Linearly interpolates between two vectors.
/// @param startVector The starting vector.
/// @param endVector The ending vector.
/// @param time The interpolation factor (0.0 to 1.0).
/// @return The interpolated vector.
Vector3 Vector3_Lerp(Vector3 startVector, Vector3 endVector, float time);

#pragma endregion Vector3

#pragma region Vector4

/// @brief Normalizes a 4D vector to have a magnitude of 1.
/// @param vector The vector to normalize.
/// @return The normalized vector.
Vector4 Vector4_Normalized(Vector4 vector);

/// @brief Calculates the magnitude (length) of a 4D vector.
/// @param vector The vector to calculate the magnitude for.
/// @return The magnitude of the vector.
float Vector4_Magnitude(Vector4 vector);

/// @brief Calculates the dot product of two 4D vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The dot product of the two vectors.
float Vector4_Dot(Vector4 vector1, Vector4 vector2);

/// @brief Linearly interpolates between two 4D vectors.
/// @param startVector The starting vector.
/// @param endVector The ending vector.
/// @param time The interpolation factor (0.0 to 1.0).
/// @return The interpolated vector.
Vector4 Vector4_Lerp(Vector4 startVector, Vector4 endVector, float time);

#pragma endregion Vector4

#pragma region Vector2Int

/// @brief Calculates the dot product of two 2D integer vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The dot product of the two vectors.
float Vector2Int_Dot(Vector2Int vector1, Vector2Int vector2);

/// @brief Calculates the magnitude (length) of a 2D integer vector.
/// @param vector The vector to calculate the magnitude for.
/// @return The magnitude of the vector.
float Vector2Int_Magnitude(Vector2Int vector);

#pragma endregion Vector2Int

#pragma region Vector3Int

/// @brief Calculates the dot product of two 3D integer vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The dot product of the two vectors.
float Vector3Int_Dot(Vector3Int vector1, Vector3Int vector2);

/// @brief Calculates the magnitude (length) of a 3D integer vector.
/// @param vector The vector to calculate the magnitude for.
/// @return The magnitude of the vector.
float Vector3Int_Magnitude(Vector3Int vector);

#pragma endregion Vector3Int

#pragma region Vector4Int

/// @brief Calculates the dot product of two 4D integer vectors.
/// @param vector1 The first vector.
/// @param vector2 The second vector.
/// @return The dot product of the two vectors.
float Vector4Int_Dot(Vector4Int vector1, Vector4Int vector2);

/// @brief Calculates the magnitude (length) of a 4D integer vector.
/// @param vector The vector to calculate the magnitude for.
/// @return The magnitude of the vector.
float Vector4Int_Magnitude(Vector4Int vector);

#pragma endregion Vector4Int

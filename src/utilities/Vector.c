#include "utilities/Vector.h"

#include <math.h>

#pragma region Vector2

Vector2 Vector2_Normalized(Vector2 vector)
{
    float magnitude = Vector2_Magnitude(vector);
    if (magnitude == 0.0f)
        return Vector2_Zero;
    return Vector2_Scale(vector, 1.0f / magnitude);
}

float Vector2_Magnitude(Vector2 vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y);
}

float Vector2_Dot(Vector2 vector1, Vector2 vector2)
{
    return (vector1.x * vector2.x) + (vector1.y * vector2.y);
}

Vector2 Vector2_Lerp(Vector2 startVector, Vector2 endVector, float time)
{
    return Vector2_New(
        startVector.x + (endVector.x - startVector.x) * time,
        startVector.y + (endVector.y - startVector.y) * time);
}

#pragma endregion Vector2

#pragma region Vector3

Vector3 Vector3_Normalized(Vector3 vector)
{
    float magnitude = Vector3_Magnitude(vector);
    return magnitude == 0.0f ? Vector3_Zero : Vector3_Scale(vector, 1.0f / magnitude);
}

float Vector3_Magnitude(Vector3 vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

float Vector3_Dot(Vector3 vector1, Vector3 vector2)
{
    return (vector1.x * vector2.x) + (vector1.y * vector2.y) + (vector1.z * vector2.z);
}

Vector3 Vector3_Cross(Vector3 vector1, Vector3 vector2)
{
    return Vector3_New(
        vector1.y * vector2.z - vector1.z * vector2.y,
        vector1.z * vector2.x - vector1.x * vector2.z,
        vector1.x * vector2.y - vector1.y * vector2.x);
}

Vector3 Vector3_Lerp(Vector3 startVector, Vector3 endVector, float time)
{
    return Vector3_New(
        startVector.x + (endVector.x - startVector.x) * time,
        startVector.y + (endVector.y - startVector.y) * time,
        startVector.z + (endVector.z - startVector.z) * time);
}

#pragma endregion Vector3

#pragma region Vector4

Vector4 Vector4_Normalized(Vector4 vector)
{
    float magnitude = Vector4_Magnitude(vector);
    if (magnitude == 0.0f)
        return Vector4_Zero;
    return Vector4_Scale(vector, 1.0f / magnitude);
}

float Vector4_Magnitude(Vector4 vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w);
}

float Vector4_Dot(Vector4 vector1, Vector4 vector2)
{
    return (vector1.x * vector2.x) + (vector1.y * vector2.y) + (vector1.z * vector2.z) + (vector1.w * vector2.w);
}

Vector4 Vector4_Lerp(Vector4 startVector, Vector4 endVector, float time)
{
    return Vector4_New(
        startVector.x + (endVector.x - startVector.x) * time,
        startVector.y + (endVector.y - startVector.y) * time,
        startVector.z + (endVector.z - startVector.z) * time,
        startVector.w + (endVector.w - startVector.w) * time);
}

#pragma endregion Vector4

#pragma region Vector2Int

float Vector2Int_Magnitude(Vector2Int vector)
{
    return sqrtf((float)(vector.x * vector.x + vector.y * vector.y));
}

float Vector2Int_Dot(Vector2Int vector1, Vector2Int vector2)
{
    return (float)(vector1.x * vector2.x) + (float)(vector1.y * vector2.y);
}

#pragma endregion Vector2Int

#pragma region Vector3Int

float Vector3Int_Magnitude(Vector3Int vector)
{
    return sqrtf((float)(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z));
}

float Vector3Int_Dot(Vector3Int vector1, Vector3Int vector2)
{
    return (float)(vector1.x * vector2.x) + (float)(vector1.y * vector2.y) + (float)(vector1.z * vector2.z);
}

#pragma endregion Vector3Int

#pragma region Vector4Int

float Vector4Int_Magnitude(Vector4Int vector)
{
    return sqrtf((float)(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w));
}

float Vector4Int_Dot(Vector4Int vector1, Vector4Int vector2)
{
    return (float)(vector1.x * vector2.x) + (float)(vector1.y * vector2.y) + (float)(vector1.z * vector2.z) + (float)(vector1.w * vector2.w);
}

#pragma endregion Vector4Int

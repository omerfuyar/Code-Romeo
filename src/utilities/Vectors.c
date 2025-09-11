#include "utilities/Vectors.h"
#include "utilities/Maths.h"

#pragma region Vector2

Vector2 Vector2_Add(Vector2 vector1, Vector2 vector2)
{
    return NewVector2(vector1.x + vector2.x, vector1.y + vector2.y);
}

Vector2 Vector2_Scale(Vector2 vector, float scalar)
{
    return NewVector2(vector.x * scalar, vector.y * scalar);
}

Vector2Int Vector2_ToInt(Vector2 vector)
{
    return NewVector2Int((int)vector.x, (int)vector.y);
}

Vector2 Vector2_Normalized(Vector2 vector)
{
    float magnitude = Vector2_Magnitude(vector);
    if (magnitude == 0.0f)
        return Vector2_Zero;
    return Vector2_Scale(vector, 1.0f / magnitude);
}

float Vector2_Magnitude(Vector2 vector)
{
    return SquareRoot(vector.x * vector.x + vector.y * vector.y);
}

float Vector2_Dot(Vector2 vector1, Vector2 vector2)
{
    return (vector1.x * vector2.x) + (vector1.y * vector2.y);
}

Vector2 Vector2_Lerp(Vector2 startVector, Vector2 endVector, float time)
{
    return NewVector2(
        startVector.x + (endVector.x - startVector.x) * time,
        startVector.y + (endVector.y - startVector.y) * time);
}

#pragma endregion Vector2

#pragma region Vector3

Vector3 Vector3_Add(Vector3 vector1, Vector3 vector2)
{
    return NewVector3(vector1.x + vector2.x, vector1.y + vector2.y, vector1.z + vector2.z);
}

Vector3 Vector3_Scale(Vector3 vector, float scalar)
{
    return NewVector3(vector.x * scalar, vector.y * scalar, vector.z * scalar);
}

Vector3Int Vector3_ToInt(Vector3 vector)
{
    return NewVector3Int((int)vector.x, (int)vector.y, (int)vector.z);
}

Vector3 Vector3_Normalized(Vector3 vector)
{
    float magnitude = Vector3_Magnitude(vector);
    return magnitude == 0.0f ? Vector3_Zero : Vector3_Scale(vector, 1.0f / magnitude);
}

float Vector3_Magnitude(Vector3 vector)
{
    return SquareRoot(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

float Vector3_Dot(Vector3 vector1, Vector3 vector2)
{
    return (vector1.x * vector2.x) + (vector1.y * vector2.y) + (vector1.z * vector2.z);
}

Vector3 Vector3_Cross(Vector3 vector1, Vector3 vector2)
{
    return NewVector3(
        vector1.y * vector2.z - vector1.z * vector2.y,
        vector1.z * vector2.x - vector1.x * vector2.z,
        vector1.x * vector2.y - vector1.y * vector2.x);
}

Vector3 Vector3_Lerp(Vector3 startVector, Vector3 endVector, float time)
{
    return NewVector3(
        startVector.x + (endVector.x - startVector.x) * time,
        startVector.y + (endVector.y - startVector.y) * time,
        startVector.z + (endVector.z - startVector.z) * time);
}

#pragma endregion Vector3

#pragma region Vector4

Vector4 Vector4_Add(Vector4 vector1, Vector4 vector2)
{
    return NewVector4(vector1.x + vector2.x, vector1.y + vector2.y, vector1.z + vector2.z, vector1.w + vector2.w);
}

Vector4 Vector4_Scale(Vector4 vector, float scalar)
{
    return NewVector4(vector.x * scalar, vector.y * scalar, vector.z * scalar, vector.w * scalar);
}

Vector4Int Vector4_ToInt(Vector4 vector)
{
    return NewVector4Int((int)vector.x, (int)vector.y, (int)vector.z, (int)vector.w);
}

Vector4 Vector4_Normalized(Vector4 vector)
{
    float magnitude = Vector4_Magnitude(vector);
    if (magnitude == 0.0f)
        return Vector4_Zero;
    return Vector4_Scale(vector, 1.0f / magnitude);
}

float Vector4_Magnitude(Vector4 vector)
{
    return SquareRoot(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w);
}

float Vector4_Dot(Vector4 vector1, Vector4 vector2)
{
    return (vector1.x * vector2.x) + (vector1.y * vector2.y) + (vector1.z * vector2.z) + (vector1.w * vector2.w);
}

Vector4 Vector4_Lerp(Vector4 startVector, Vector4 endVector, float time)
{
    return NewVector4(
        startVector.x + (endVector.x - startVector.x) * time,
        startVector.y + (endVector.y - startVector.y) * time,
        startVector.z + (endVector.z - startVector.z) * time,
        startVector.w + (endVector.w - startVector.w) * time);
}

#pragma endregion Vector4

#pragma region Vector2Int

Vector2Int Vector2Int_Add(Vector2Int vector1, Vector2Int vector2)
{
    return NewVector2Int(vector1.x + vector2.x, vector1.y + vector2.y);
}

Vector2Int Vector2Int_Scale(Vector2Int vector, float scalar)
{
    return NewVector2Int((int)((float)vector.x * scalar), (int)((float)vector.y * scalar));
}

Vector2 Vector2_ToFloat(Vector2Int vector)
{
    return NewVector2((float)vector.x, (float)vector.y);
}

float Vector2Int_Magnitude(Vector2Int vector)
{
    return SquareRoot((float)(vector.x * vector.x + vector.y * vector.y));
}

float Vector2Int_Dot(Vector2Int vector1, Vector2Int vector2)
{
    return (float)(vector1.x * vector2.x) + (float)(vector1.y * vector2.y);
}

#pragma endregion Vector2Int

#pragma region Vector3Int

Vector3Int Vector3Int_Add(Vector3Int vector1, Vector3Int vector2)
{
    return NewVector3Int(vector1.x + vector2.x, vector1.y + vector2.y, vector1.z + vector2.z);
}

Vector3Int Vector3Int_Scale(Vector3Int vector, float scalar)
{
    return NewVector3Int((int)((float)vector.x * scalar), (int)((float)vector.y * scalar), (int)((float)vector.z * scalar));
}

Vector3 Vector3Int_ToFloat(Vector3Int vector)
{
    return NewVector3((float)vector.x, (float)vector.y, (float)vector.z);
}

float Vector3Int_Magnitude(Vector3Int vector)
{
    return SquareRoot((float)(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z));
}

float Vector3Int_Dot(Vector3Int vector1, Vector3Int vector2)
{
    return (float)(vector1.x * vector2.x) + (float)(vector1.y * vector2.y) + (float)(vector1.z * vector2.z);
}

#pragma endregion Vector3Int

#pragma region Vector4Int

Vector4Int Vector4Int_Add(Vector4Int vector1, Vector4Int vector2)
{
    return NewVector4Int(vector1.x + vector2.x, vector1.y + vector2.y, vector1.z + vector2.z, vector1.w + vector2.w);
}

Vector4Int Vector4Int_Scale(Vector4Int vector, float scalar)
{
    return NewVector4Int((int)((float)vector.x * scalar), (int)((float)vector.y * scalar), (int)((float)vector.z * scalar), (int)((float)vector.w * scalar));
}

Vector4 Vector4Int_ToFloat(Vector4Int vector)
{
    return NewVector4((float)vector.x, (float)vector.y, (float)vector.z, (float)vector.w);
}

float Vector4Int_Magnitude(Vector4Int vector)
{
    return SquareRoot((float)(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w));
}

float Vector4Int_Dot(Vector4Int vector1, Vector4Int vector2)
{
    return (float)(vector1.x * vector2.x) + (float)(vector1.y * vector2.y) + (float)(vector1.z * vector2.z) + (float)(vector1.w * vector2.w);
}

#pragma endregion Vector4Int

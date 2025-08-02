#pragma once
#include <Global.h>

/// @brief
typedef struct ArrayFloat
{
    float *values;
    size_t length;
    size_t capacity;
} ArrayFloat;

/// @brief
/// @param capacity
/// @return
ArrayFloat *ArrayFloat_Create(size_t capacity);

/// @brief
/// @param array
/// @param index
/// @return
float ArrayFloat_Get(ArrayFloat *array, size_t index);

/// @brief
/// @param array
void ArrayFloat_Destroy(ArrayFloat *array);

/// @brief
/// @param baseArray
/// @param targetArray
void ArrayFloat_OverrideCopy(ArrayFloat *baseArray, ArrayFloat *targetArray);

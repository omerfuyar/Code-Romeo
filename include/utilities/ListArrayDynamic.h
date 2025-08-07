#pragma once

#include "Global.h"
#include "utilities/ListArray.h"

/// @brief A dynamic element to indicate where an element is located in the array and its size.
typedef struct ListArrayDynamicElement
{
    size_t offsetInBytes;
    size_t size;
} ListArrayDynamicElement;

/// @brief A list array that can hold different type of elements in a single array.
typedef struct ListArrayDynamic
{
    ListArray elements;
    void *data;
    size_t count;
    size_t usedSizeInBytes;
    size_t capacityInBytes;
} ListArrayDynamic;

/// @brief Creates a new ListArrayDynamic with an initial capacity in bytes.
/// @param initialCapacityInBytes The initial capacity in bytes for the dynamic list array.
/// @return The created ListArrayDynamic.
ListArrayDynamic ListArrayDynamic_Create(size_t initialCapacityInBytes);

/// @brief Destroys the dynamic list array and frees all associated memory.
/// @param list Pointer to the ListArrayDynamic to destroy.
void ListArrayDynamic_Destroy(ListArrayDynamic *list);

/// @brief Adds a new element to the dynamic list array.
/// @param list Pointer to the ListArrayDynamic to add the element to.
/// @param element Pointer to the element to add.
/// @param elementSize Size of the element to add.
void ListArrayDynamic_Add(ListArrayDynamic *list, void *element, size_t elementSize);

/// @brief Removes an element from the dynamic list array at the specified index.
/// @param list Pointer to the ListArrayDynamic to remove the element from.
/// @param index The index of the element to remove.
void ListArrayDynamic_Remove(ListArrayDynamic *list, size_t index);

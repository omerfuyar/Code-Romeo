#pragma once

#include "Global.h"

// The resize multiplier used when the ListArray size reached to the capacity when adding new item
#define ARRAY_LIST_RESIZE_MULTIPLIER 2

// The limit for resizing the ListArray when it's size is less than 1/x of the capacity
#define ARRAY_LIST_MIN_DECIMAL_LIMIT 4

/// @brief A dynamic array list implementation. Can be used in any type. Copies passed items to its own property. Shouldn't be used without helper functions.
typedef struct ListArray
{
    void *data;
    size_t capacity;
    size_t count;
    size_t sizeOfItem;
} ListArray;

/// @brief Creator function for ListArray.
/// @param sizeOfItem Size of the item type to store in.
/// @param initialCapacity How many items can hold in this ListArray. Can be resized later on.
/// @return The created ListArray struct
ListArray ListArray_Create(size_t sizeOfItem, size_t initialCapacity);

/// @brief Destroyer function for ListArray.
/// @param list ListArray to destroy.
void ListArray_Destroy(ListArray *list);

/// @brief Resize function for ListArray. Can enlarge or trunc the ListArray.
/// @param list ListArray to resize.
/// @param newCapacity Capacity to increase or decrease.
void ListArray_Resize(ListArray *list, size_t newCapacity);

/// @brief Getter function for ListArray. Should be casted before dereference / usage like : *(ListType*)function...
/// @param list ListArray to get item in index.
/// @param index Index to get item.
/// @return The item in given index.
void *ListArray_Get(ListArray list, size_t index);

/// @brief Item setter function for ListArray. Works only in range of size. Uses memcpy to copy the item to the given index.
/// @param list ListArray to change item in.
/// @param index Index to replace item at.
/// @param item New item at index.
void ListArray_Set(ListArray list, size_t index, const void *item);

/// @brief Adder function for ListArray. Sets the last last index to given item. Uses memcpy to copy the item to the last.
/// @param list ListArray to add item.
/// @param item Item to add to ListArray.
void ListArray_Add(ListArray *list, const void *item);

/// @brief Adds a range of items to the ListArray. Uses memcpy to copy the items to the last index.
/// @param list ListArray to add items to.
/// @param item Pointer to the first item to add.
/// @param itemCount Number of items to add.
void ListArray_AddRange(ListArray *list, const void *item, size_t itemCount);

/// @brief Remover function using index for ListArray. Removes the item at the given index. Uses memmove to shift all indices of items by -1 after the removed index.
/// @param list ListArray to remove item from.
/// @param index Index to remove item at.
void ListArray_RemoveAtIndex(ListArray *list, size_t index);

/// @brief Remover function using range for ListArray. Removes items from starting index. Uses memmove to shift all indices of items by item count after the removed index.
/// @param list ListArray to remove item from.
/// @param index Index to start remove item at.
/// @param itemCount Item count to remove.
void ListArray_RemoveRange(ListArray *list, size_t index, size_t itemCount);

/// @brief Remover function using item pointer for ListArray. Removes the first appearance of the given item. Uses ListArray_RemoveAtIndex and ListArray_IndexOf.
/// @param list ListArray to remove item from.
/// @param item Item to find and remove.
void ListArray_RemoveItem(ListArray *list, const void *item);

/// @brief Pop function for ListArray. Removes the last item in the list and returns it.
/// @param list ListArray to pop item from.
/// @return The popped item. NULL if the list is empty.
void *ListArray_Pop(ListArray *list);

/// @brief Clear function for ListArray. Sets all the data unusable and size to 0. Capacity remains the same.
/// @param list ListArray to clear.
void ListArray_Clear(ListArray *list);

/// @brief Index finder for ListArray. Searches the list linearly. Uses memcmp to compare items.
/// @param list List to search for item.
/// @param item Item to find index of.
/// @return The index of found item. -1 if the item is absent in the list.
long long ListArray_IndexOf(ListArray list, const void *item);

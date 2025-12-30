#pragma once

#include "RJGlobal.h"

/// @brief The resize multiplier used when the ListArray size reached to the capacity when adding new item
#define LIST_ARRAY_RESIZE_MULTIPLIER 2.0f
/// @brief The minimum decimal limit to trigger a cut resize when removing items from the ListArray
#define LIST_ARRAY_MIN_DECIMAL_LIMIT 4.0f
/// @brief Whether remove functions should resize the list to be smaller or not.
#define LIST_ARRAY_CUT_RESIZE false

#pragma region Typedefs

/// @brief A dynamic array list implementation. Can be used in any type. Copies passed items to its own property. Shouldn't be used without helper functions.
typedef struct ListArray
{
    void *data;
    RJ_Size capacity;
    RJ_Size count;
    RJ_Size sizeOfItem;
    char *nameOfType;
} ListArray;

#pragma endregion Typedefs

/// @brief Creator function for ListArray.
/// @param nameOfType Name of the type stored in the list.
/// @param sizeOfItem Size of the item type to store in.
/// @param initialCapacity How many items can hold in this ListArray. Can be resized later on.
/// @return The created ListArray struct
ListArray ListArray_Create(const char *nameOfType, RJ_Size sizeOfItem, RJ_Size initialCapacity);

/// @brief Destroyer function for ListArray.
/// @param list ListArray to destroy.
void ListArray_Destroy(ListArray *list);

/// @brief Creates a copy of the given ListArray with its own data.
/// @param list Pointer to the ListArray to copy.
/// @return The copied ListArray.
ListArray ListArray_Copy(const ListArray *list);

/// @brief Resize function for ListArray. Can enlarge or trunc the ListArray.
/// @param list ListArray to resize.
/// @param newCapacity Capacity to increase or decrease.
void ListArray_Resize(ListArray *list, RJ_Size newCapacity);

/// @brief Getter function for ListArray. Should be cast before using. ['(<TYPE>*)Getter(...)' will give you the pointer of the stored data. You can dereference it as you want]
/// @param list Pointer to the ListArray to get item from.
/// @param index Index to get item.
/// @return Pointer to the item at the given index.
void *ListArray_Get(const ListArray *list, RJ_Size index);

/// @brief Item setter function for ListArray. Works only in range of size. Uses memcpy to copy the item to the given index.
/// @param list Pointer to the ListArray to change item in.
/// @param index Index to replace item at.
/// @param item Pointer to the new item at index.
void ListArray_Set(ListArray *list, RJ_Size index, const void *item);

/// @brief Adder function for ListArray. Copies {sizeOfItem} amount of data from parameter {item} to the end of the array. Uses memcpy.
/// @param list ListArray to add item.
/// @param item Item to add to ListArray.
/// @return The address of the added item
void *ListArray_Add(ListArray *list, const void *item);

/// @brief Adds a range of items to the ListArray. Uses memcpy to copy the items to the last index.
/// @param list ListArray to add items to.
/// @param item Pointer to the first item to add.
/// @param itemCount Number of items to add.
/// @return The address of the first added item.
void *ListArray_AddRange(ListArray *list, const void *item, RJ_Size itemCount);

/// @brief Adds an item at a specific index in the ListArray.
/// @param list Pointer to the ListArray to add item to.
/// @param index Index to add item at.
/// @param item Pointer to the item to add.
/// @return The address of the added item.
void *ListArray_AddToIndex(ListArray *list, RJ_Size index, const void *item);

/// @brief Remover function using index for ListArray. Removes the item at the given index. Uses memmove to shift all indices of items by -1 after the removed index.
/// @param list ListArray to remove item from.
/// @param index Index to remove item at.
void ListArray_RemoveAtIndex(ListArray *list, RJ_Size index);

/// @brief Remover function using range for ListArray. Removes items from starting index. Uses memmove to shift all indices of items by item count after the removed index.
/// @param list ListArray to remove item from.
/// @param index Index to start remove item at.
/// @param itemCount Item count to remove.
void ListArray_RemoveRange(ListArray *list, RJ_Size index, RJ_Size itemCount);

/// @brief Remover function using item pointer for ListArray. Removes the first appearance of the given item. Uses ListArray_RemoveAtIndex and ListArray_IndexOf.
/// @param list ListArray to remove item from.
/// @param item Item to find and remove.
void ListArray_RemoveItem(ListArray *list, const void *item);

/// @brief Pop function for ListArray. Removes the last item in the list and returns it.
/// @param list ListArray to pop item from.
/// @return The popped item. NULL if the list is empty. Use the value only for copying, as the data is invalidated after popping.
void *ListArray_Pop(ListArray *list);

/// @brief Clear function for ListArray. Sets all the data unusable and size to 0. Capacity remains the same.
/// @param list ListArray to clear.
void ListArray_Clear(ListArray *list);

/// @brief Index finder for ListArray. Searches the list linearly. Uses memcmp to compare items.
/// @param list Pointer to the ListArray to search.
/// @param item Pointer to the item to find.
/// @return The index of found item. -1 if the item is absent in the list.
long long ListArray_IndexOf(const ListArray *list, const void *item);

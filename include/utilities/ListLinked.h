#pragma once

#include "Global.h"

/// @brief A node in the linked list.
typedef struct ListLinkedNode
{
    void *data;
    struct ListLinkedNode *next;
} ListLinkedNode;

/// @brief A dynamic linked list implementation. Can store any type of data. Copies passed items to its own property. Shouldn't be used without helper functions.
typedef struct ListLinked
{
    ListLinkedNode *head;
    size_t count;
    size_t sizeOfItem;
} ListLinked;

/// @brief Creator function for ListLinked.
/// @param sizeOfItem Size of the item type to store in the list.
/// @return The created ListLinked struct.
ListLinked ListLinked_Create(size_t sizeOfItem);

/// @brief Destroyer function for ListLinked. Frees all nodes and the list itself recursively.
/// @param list ListLinked to destroy.
void ListLinked_Destroy(ListLinked *list);

/// @brief Head getter function for ListLinked.
/// @param list ListLinked to get the head node from.
/// @return Pointer to the head node of the ListLinked.
void *ListLinked_Head(ListLinked list);

/// @brief Getter function for ListLinked. Retrieves the item at the specified index. Works only in range of count.
/// @param list ListLinked to get the item from.
/// @param index Index of the item to retrieve.
/// @return Pointer to the item at the given index. Should be cast before dereferencing.
void *ListLinked_Get(ListLinked list, size_t index);

/// @brief Item setter function for ListLinked. Replaces the item at the specified index.
/// @param list ListLinked to modify.
/// @param index Index of the item to replace.
/// @param item Pointer to the new item to set at the index.
void ListLinked_Set(ListLinked list, size_t index, const void *item);

/// @brief Adder function for ListLinked. Appends the item to the end of the list.
/// @param list ListLinked to add the item to.
/// @param item Pointer to the item to add.
void ListLinked_Add(ListLinked *list, const void *item);

/// @brief Remover function using index for ListLinked. Removes the item at the specified index.
/// @param list ListLinked to remove the item from.
/// @param index Index of the item to remove.
/// @return Pointer to the removed item.
void ListLinked_RemoveAtIndex(ListLinked *list, size_t index);

/// @brief Remover function using item pointer for ListLinked. Removes the first occurrence of the specified item.
/// @param list ListLinked to remove the item from.
/// @param item Pointer to the item to find and remove.
/// @return Pointer to the removed item. NULL if the item is not found in the list.
void ListLinked_RemoveItem(ListLinked *list, const void *item);

/// @brief Clear function for ListLinked. Removes all nodes and resets the count to 0.
/// @param list ListLinked to clear.
void ListLinked_Clear(ListLinked *list);

/// @brief Index finder for ListLinked. Searches the list linearly for the specified item.
/// @param list ListLinked to search in.
/// @param item Pointer to the item to find.
/// @return The index of the found item. -1 if the item is not found in the list.
long long ListLinked_IndexOf(ListLinked list, const void *item);

/// @brief Size getter for ListLinked.
/// @param list ListLinked to get the count of.
/// @return The number of items in the ListLinked.
size_t ListLinked_GetSize(ListLinked list);

/// @brief Size of item getter for ListLinked.
/// @param list ListLinked to get the count of the item from.
/// @return The count of the item stored in the ListLinked.
size_t ListLinked_GetSizeOfItem(ListLinked list);

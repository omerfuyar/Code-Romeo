#include "utilities/ListArray.h"
#include <string.h>

#pragma region Source Only

typedef struct ListArray
{
    void *data;
    size_t capacity;
    size_t size;
    size_t sizeOfItem;
} ListArray;

#pragma endregion Source Only

ListArray *ListArray_Create(size_t sizeOfItem, size_t initialCapacity)
{
    ListArray *list = (ListArray *)malloc(sizeof(ListArray));
    DebugAssertPointerNullCheck(list);

    list->capacity = initialCapacity;
    list->size = 0;
    list->sizeOfItem = sizeOfItem;
    list->data = (void *)malloc(initialCapacity * sizeOfItem);
    DebugAssert(list->data != NULL, "Memory allocation failed for ListArray data.");

    DebugInfo("ListArray created with initial capacity: %zu, size of item: %zu", initialCapacity, sizeOfItem);
    return list;
}

void ListArray_Destroy(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    free(list->data);
    list->data = NULL;

    free(list);
    list = NULL;

    DebugInfo("ListArray destroyed.");
}

void ListArray_Resize(ListArray *list, size_t newCapacity)
{
    DebugAssertPointerNullCheck(list);

    void *newData = realloc(list->data, newCapacity * list->sizeOfItem);
    DebugAssertPointerNullCheck(newData);

    list->data = newData;
    list->capacity = newCapacity;

    if (list->size > newCapacity)
    {
        list->size = newCapacity;
    }

    DebugWarning("ListArray resized from %zu to %zu", list->capacity, newCapacity);
}

void *ListArray_Get(const ListArray *list, size_t index)
{
    DebugAssertPointerNullCheck(list);
    DebugAssert(index < list->size, "Index out of range. List size : %du, index : %du", list->size, index);

    void *itemLocation = (void *)((char *)(list->data) + index * list->sizeOfItem);
    DebugAssertPointerNullCheck(itemLocation);

    return itemLocation;
}

void ListArray_Set(const ListArray *list, size_t index, const void *item)
{
    DebugAssertPointerNullCheck(list);
    DebugAssert(index < list->size, "Index out of range. List size : %du, index : %du", list->size, index);

    void *targetLocation = ListArray_Get(list, index);

    memcpy(targetLocation, item, list->sizeOfItem);
}

void ListArray_Add(ListArray *list, const void *item)
{
    DebugAssertPointerNullCheck(list);

    void *targetLocation = (void *)((char *)(list->data) + list->size * list->sizeOfItem);

    memcpy(targetLocation, item, list->sizeOfItem);

    list->size++;

    if (list->size >= list->capacity)
    {
        DebugWarning("ListArray is full. Resizing it from %du to %du.", list->capacity, list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER);
        ListArray_Resize(list, list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER);
    }
}

void ListArray_RemoveAtIndex(ListArray *list, size_t index)
{
    DebugAssertPointerNullCheck(list);
    DebugAssert(index < list->size, "Index out of range. List size : %du, index : %du", list->size, index);

    void *targetLocation = ListArray_Get(list, index);

    memset(targetLocation, 0, list->sizeOfItem);

    size_t bytesToMove = (list->size - index - 1) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove(targetLocation, (char *)targetLocation + list->sizeOfItem, bytesToMove);
    }

    list->size--;

    if (list->size < list->capacity / ARRAY_LIST_MIN_DECIMAL_LIMIT)
    {
        DebugWarning("ListArray is less than 1/%d full. Resizing it from %du to %du.", ARRAY_LIST_MIN_DECIMAL_LIMIT, list->capacity, list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER);
        ListArray_Resize(list, list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER);
    }
}

void ListArray_RemoveItem(ListArray *list, const void *item)
{
    DebugAssertPointerNullCheck(list);

    long long itemIndex = ListArray_IndexOf(list, item);
    if (itemIndex == -1)
    {
        DebugWarning("Item not found in ListArray. Cannot remove.");
        return;
    }

    ListArray_RemoveAtIndex(list, (size_t)itemIndex);
}

void *ListArray_Pop(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    void *item = ListArray_Get(list, list->size - 1);
    ListArray_RemoveAtIndex(list, list->size - 1);

    return item;
}

void ListArray_Clear(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    list->size = 0;

    if (list->data != NULL)
    {
        memset(list->data, 0, list->capacity * list->sizeOfItem);
    }
}

long long ListArray_IndexOf(const ListArray *list, const void *item)
{
    DebugAssertPointerNullCheck(list);
    void *currentItem = list->data;

    for (size_t i = 0; i < list->size; i++)
    {
        currentItem = (void *)((char *)currentItem + i * list->sizeOfItem);

        if (memcmp(currentItem, item, list->sizeOfItem) == 0)
        {
            return (long long)i;
        }
    }

    DebugWarning("Item not found in ListArray. Returning -1.");
    return -1;
}

void *ListArray_GetData(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    return list->data;
}

size_t ListArray_GetSize(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    return list->size;
}

size_t ListArray_GetCapacity(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    return list->capacity;
}

size_t ListArray_GetSizeOfItem(ListArray *list)
{
    DebugAssertPointerNullCheck(list);

    return list->sizeOfItem;
}

#include "utilities/ListArray.h"

ListArray ListArray_Create(size_t sizeOfItem, size_t initialCapacity)
{
    ListArray list;
    list.capacity = initialCapacity;
    list.sizeOfItem = sizeOfItem;
    list.count = 0;
    list.data = (void *)malloc(initialCapacity * sizeOfItem);
    DebugAssertNullPointerCheck(list.data);

    DebugInfo("ListArray created with initial capacity: %zu, size of item: %zu", initialCapacity, sizeOfItem);
    return list;
}

void ListArray_Destroy(ListArray *list)
{
    DebugAssertNullPointerCheck(list);

    free(list->data);
    list->data = NULL;

    list->capacity = 0;
    list->count = 0;
    list->sizeOfItem = 0;

    DebugInfo("ListArray destroyed.");
}

void ListArray_Resize(ListArray *list, size_t newCapacity)
{
    DebugAssertNullPointerCheck(list);

    void *newData = realloc(list->data, newCapacity * list->sizeOfItem);
    DebugAssertNullPointerCheck(newData);

    list->data = newData;
    list->capacity = newCapacity;

    if (list->count > newCapacity)
    {
        list->count = newCapacity;
    }

    DebugWarning("ListArray resized from %zu to %zu", list->capacity, newCapacity);
}

void *ListArray_Get(ListArray list, size_t index)
{
    DebugAssert(index < list.count, "Index out of range. List size : %zu, index : %zu", list.count, index);

    void *itemLocation = (void *)((char *)list.data + index * list.sizeOfItem);
    DebugAssertNullPointerCheck(itemLocation);

    return itemLocation;
}

void ListArray_Set(ListArray list, size_t index, const void *item)
{
    DebugAssertNullPointerCheck(item);
    DebugAssert(index < list.count, "Index out of range. List size : %zu, index : %zu", list.count, index);

    void *targetLocation = ListArray_Get(list, index);

    memcpy(targetLocation, item, list.sizeOfItem);
}

void ListArray_Add(ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(item);

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);
    DebugAssertNullPointerCheck(targetLocation);

    memcpy(targetLocation, item, list->sizeOfItem);

    list->count++;

    if (list->count >= list->capacity)
    {
        DebugWarning("ListArray is full. Resizing it from %zu to %zu.", list->capacity, list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER);
        ListArray_Resize(list, list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER);
    }
}

void ListArray_RemoveAtIndex(ListArray *list, size_t index)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index < list->count, "Index out of range. List size : %zu, index : %zu", list->count, index);

    void *targetLocation = ListArray_Get(*list, index);

    memset(targetLocation, 0, list->sizeOfItem);

    size_t bytesToMove = (list->count - index - 1) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove(targetLocation, (char *)targetLocation + list->sizeOfItem, bytesToMove);
    }

    list->count--;

    if (list->count > 0 && list->capacity > ARRAY_LIST_RESIZE_MULTIPLIER && list->count < list->capacity / ARRAY_LIST_MIN_DECIMAL_LIMIT)
    {
        DebugWarning("ListArray is less than 1/%d full. Resizing it from %zu to %zu.", ARRAY_LIST_MIN_DECIMAL_LIMIT, list->capacity, list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER);
        ListArray_Resize(list, list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER);
    }
}

void ListArray_RemoveItem(ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(item);

    long long itemIndex = ListArray_IndexOf(*list, item);
    if (itemIndex == -1)
    {
        DebugWarning("Item not found in ListArray. Cannot remove.");
        return;
    }

    ListArray_RemoveAtIndex(list, (size_t)itemIndex);
}

void *ListArray_Pop(ListArray *list)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(list->count > 0, "Cannot pop from an empty list.");

    void *item = ListArray_Get(*list, list->count - 1);
    ListArray_RemoveAtIndex(list, list->count - 1);

    return item;
}

void ListArray_Clear(ListArray *list)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(list->data);

    list->count = 0;

    memset(list->data, 0, list->capacity * list->sizeOfItem);
}

long long ListArray_IndexOf(ListArray list, const void *item)
{
    for (size_t i = 0; i < list.count; i++)
    {
        void *currentItem = (void *)((char *)list.data + i * list.sizeOfItem);

        if (memcmp(currentItem, item, list.sizeOfItem) == 0)
        {
            return (long long)i;
        }
    }

    DebugWarning("Item not found in ListArray. Returning -1.");
    return -1;
}

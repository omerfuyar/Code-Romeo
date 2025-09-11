#include "utilities/ListArray.h"
#include "utilities/Maths.h"

ListArray ListArray_Create(char *nameOfType, size_t sizeOfItem, size_t initialCapacity)
{
    ListArray list;
    list.capacity = initialCapacity;
    list.sizeOfItem = sizeOfItem;
    list.nameOfType = nameOfType;
    list.count = 0;
    list.data = (void *)malloc(initialCapacity * sizeOfItem);
    DebugAssertNullPointerCheck(list.data);

    DebugInfo("ListArray '%s' created with initial capacity: %zu, size of item: %zu", nameOfType, initialCapacity, sizeOfItem);
    return list;
}

void ListArray_Destroy(ListArray *list)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(list->data);

    char *name = list->nameOfType;

    free(list->data);
    list->data = NULL;

    list->nameOfType = NULL;
    list->capacity = 0;
    list->count = 0;
    list->sizeOfItem = 0;

    DebugInfo("ListArray '%s' destroyed.", name);
}

ListArray ListArray_Copy(ListArray list)
{
    ListArray copiedList = ListArray_Create(list.nameOfType, list.sizeOfItem, list.capacity);

    ListArray_AddRange(&copiedList, list.data, list.count);

    DebugInfo("ListArray '%s' copied. count: %zu, capacity: %zu", list.nameOfType, list.count, list.capacity);

    return copiedList;
}

void ListArray_Resize(ListArray *list, size_t newCapacity)
{
    DebugAssertNullPointerCheck(list);

    if (newCapacity != 0)
    {
        list->data = realloc(list->data, newCapacity * list->sizeOfItem);
        DebugAssertNullPointerCheck(list->data);
    }

    DebugInfo("ListArray '%s' resized from %zu to %zu.", list->nameOfType, list->capacity, newCapacity);

    list->capacity = newCapacity;
    list->count = Min(list->count, list->capacity);
}

void *ListArray_Get(ListArray list, size_t index)
{
    DebugAssert(index < list.count, "Index out of range for ListArray '%s'. List size: %zu, index trying to access: %zu", list.nameOfType, list.count, index);

    void *itemLocation = (void *)((char *)list.data + index * list.sizeOfItem);
    DebugAssertNullPointerCheck(itemLocation);

    return itemLocation;
}

void ListArray_Set(ListArray list, size_t index, const void *item)
{
    DebugAssert(index < list.count, "Index out of range for ListArray '%s'. List size: %zu, index: %zu", list.nameOfType, list.count, index);

    void *targetLocation = ListArray_Get(list, index);

    MemoryCopy(targetLocation, list.sizeOfItem, item);
}

void ListArray_Add(ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);

    while (list->count + 1 > list->capacity)
    {
        DebugWarning("ListArray '%s' is full. Resizing it from %zu to %zu.", list->nameOfType, list->capacity, (size_t)((double)list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (size_t)((double)list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);
    DebugAssertNullPointerCheck(targetLocation);

    MemoryCopy(targetLocation, list->sizeOfItem, item);

    list->count++;
}

void ListArray_AddRange(ListArray *list, const void *item, size_t itemCount)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(itemCount > 0, "Item count to add to ListArray '%s' must be greater than 0.", list->nameOfType);

    while (list->count + itemCount > list->capacity)
    {
        DebugWarning("ListArray '%s' is full. Resizing it...", list->nameOfType);
        ListArray_Resize(list, (size_t)((double)list->capacity * ARRAY_LIST_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);
    DebugAssertNullPointerCheck(targetLocation);

    MemoryCopy(targetLocation, list->sizeOfItem, item);

    list->count += itemCount;
}

void ListArray_RemoveAtIndex(ListArray *list, size_t index)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %zu, index: %zu", list->nameOfType, list->count, index);

    if (list->count - 1 > 0 && (double)list->capacity > ARRAY_LIST_RESIZE_MULTIPLIER && list->count - 1 < (size_t)((double)list->capacity / ARRAY_LIST_MIN_DECIMAL_LIMIT))
    {
        DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %zu to %zu.", list->nameOfType, ARRAY_LIST_MIN_DECIMAL_LIMIT, list->capacity, (size_t)((double)list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (size_t)((double)list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER));
    }

    void *targetLocation = ListArray_Get(*list, index);

    memset(targetLocation, 0, list->sizeOfItem);

    size_t bytesToMove = (list->count - index - 1) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove(targetLocation, (char *)targetLocation + list->sizeOfItem, bytesToMove);
    }

    list->count--;
}

void ListArray_RemoveRange(ListArray *list, size_t index, size_t itemCount)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index + itemCount <= list->count, "Index out of range for ListArray '%s'. List size: %zu, index: %zu, count: %zu", list->nameOfType, list->count, index, itemCount);
    DebugAssert(itemCount > 0, "Item count to remove from ListArray '%s' must be greater than 0.", list->nameOfType);

    if (list->count - itemCount > 0 && (double)list->capacity > ARRAY_LIST_RESIZE_MULTIPLIER && list->count - itemCount < (size_t)((double)list->capacity / ARRAY_LIST_MIN_DECIMAL_LIMIT))
    {
        DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %zu to %zu.", list->nameOfType, ARRAY_LIST_MIN_DECIMAL_LIMIT, list->capacity, (size_t)((double)list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (size_t)((double)list->capacity / ARRAY_LIST_RESIZE_MULTIPLIER));
    }

    void *targetLocation = ListArray_Get(*list, index);
    size_t bytesToMove = (list->count - index - itemCount) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove(targetLocation, (char *)targetLocation + list->sizeOfItem * itemCount, bytesToMove);
    }

    list->count -= itemCount;
}

void ListArray_RemoveItem(ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(item);

    long long itemIndex = ListArray_IndexOf(*list, item);
    if (itemIndex == -1)
    {
        DebugWarning("Item not found in ListArray '%s'. Cannot remove.", list->nameOfType);
        return;
    }

    ListArray_RemoveAtIndex(list, (size_t)itemIndex);
}

void *ListArray_Pop(ListArray *list)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(list->count > 0, "Cannot pop from an empty ListArray '%s'.", list->nameOfType);

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

    DebugWarning("Item not found in ListArray '%s'. Returning -1.", list.nameOfType);
    return -1;
}

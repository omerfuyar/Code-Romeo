#include "utilities/ListArray.h"
#include "utilities/Maths.h"

ListArray ListArray_Create(const char *nameOfType, size_t sizeOfItem, size_t initialCapacity)
{
    DebugAssert(initialCapacity > 0, "Capacity of %s list can not be 0.", nameOfType);

    ListArray list;
    list.capacity = initialCapacity;
    list.sizeOfItem = sizeOfItem;
    list.nameOfType = (char *)malloc(strlen(nameOfType) + 1);
    DebugAssertNullPointerCheck(list.nameOfType);
    MemoryCopy(list.nameOfType, strlen(nameOfType) + 1, nameOfType);
    list.nameOfType[strlen(nameOfType)] = '\0';
    list.count = 0;
    list.data = (void *)malloc(initialCapacity * sizeOfItem);
    DebugAssertNullPointerCheck(list.data);

    MemorySet(list.data, list.sizeOfItem * list.capacity, 0);

    DebugInfo("ListArray '%s' created with initial capacity: %zu, size of item: %zu", nameOfType, initialCapacity, sizeOfItem);
    return list;
}

void ListArray_Destroy(ListArray *list)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(list->data);

    char tempTitle[RJ_TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(RJ_TEMP_BUFFER_SIZE, strlen(list->nameOfType) + 1), list->nameOfType);

    free(list->data);
    list->data = NULL;

    free(list->nameOfType);
    list->nameOfType = NULL;
    list->capacity = 0;
    list->count = 0;
    list->sizeOfItem = 0;

    DebugInfo("ListArray '%s' destroyed.", tempTitle);
}

ListArray ListArray_Copy(const ListArray *list)
{
    DebugAssertNullPointerCheck(list);

    ListArray copiedList = ListArray_Create(list->nameOfType, list->sizeOfItem, list->capacity);

    ListArray_AddRange(&copiedList, list->data, list->count);

    DebugInfo("ListArray '%s' copied. count: %zu, capacity: %zu", list->nameOfType, list->count, list->capacity);

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

void *ListArray_Get(const ListArray *list, size_t index)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %zu, index trying to access: %zu", list->nameOfType, list->count, index);

    void *itemLocation = (void *)((char *)list->data + index * list->sizeOfItem);

    return itemLocation;
}

void ListArray_Set(ListArray *list, size_t index, const void *item)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %zu, index: %zu", list->nameOfType, list->count, index);

    void *targetLocation = ListArray_Get(list, index);

    MemoryCopy(targetLocation, list->sizeOfItem, item);
}

void *ListArray_Add(ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);

    while (list->count + 1 > list->capacity)
    {
        DebugWarning("ListArray '%s' is full. Resizing it from %zu to %zu.", list->nameOfType, list->capacity, (size_t)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (size_t)((double)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);

    if (item == NULL)
    {
        MemorySet(targetLocation, list->sizeOfItem, 0);
    }
    else
    {
        MemoryCopy(targetLocation, list->sizeOfItem, item);
    }

    list->count++;

    return targetLocation;
}

void *ListArray_AddRange(ListArray *list, const void *item, size_t itemCount)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(itemCount > 0, "Item count to add to ListArray '%s' must be greater than 0.", list->nameOfType);

    while (list->count + itemCount > list->capacity)
    {
        DebugWarning("ListArray '%s' is full. Resizing it...", list->nameOfType);
        ListArray_Resize(list, (size_t)((double)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);

    if (item == NULL)
    {
        MemorySet(targetLocation, list->sizeOfItem * itemCount, 0);
    }
    else
    {
        MemoryCopy(targetLocation, list->sizeOfItem * itemCount, item);
    }

    list->count += itemCount;

    return targetLocation;
}

void ListArray_RemoveAtIndex(ListArray *list, size_t index)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %zu, index: %zu", list->nameOfType, list->count, index);

    if (list->count - 1 > 0 && (double)list->capacity > LIST_ARRAY_RESIZE_MULTIPLIER && list->count - 1 < (size_t)((double)list->capacity / LIST_ARRAY_MIN_DECIMAL_LIMIT))
    {
        DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %zu to %zu.", list->nameOfType, LIST_ARRAY_MIN_DECIMAL_LIMIT, list->capacity, (size_t)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (size_t)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = ListArray_Get(list, index);

    size_t bytesToMove = (list->count - index - 1) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        MemoryMove(targetLocation, bytesToMove, (char *)targetLocation + list->sizeOfItem);
    }

    list->count--;
}

void ListArray_RemoveRange(ListArray *list, size_t index, size_t itemCount)
{
    DebugAssertNullPointerCheck(list);
    DebugAssert(index + itemCount <= list->count, "Index out of range for ListArray '%s'. List size: %zu, index: %zu, count: %zu", list->nameOfType, list->count, index, itemCount);
    DebugAssert(itemCount > 0, "Item count to remove from ListArray '%s' must be greater than 0.", list->nameOfType);

    if (list->count - itemCount > 0 && (double)list->capacity > LIST_ARRAY_RESIZE_MULTIPLIER && list->count - itemCount < (size_t)((double)list->capacity / LIST_ARRAY_MIN_DECIMAL_LIMIT))
    {
        DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %zu to %zu.", list->nameOfType, LIST_ARRAY_MIN_DECIMAL_LIMIT, list->capacity, (size_t)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (size_t)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = ListArray_Get(list, index);
    size_t bytesToMove = (list->count - index - itemCount) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        MemoryMove(targetLocation, bytesToMove, (char *)targetLocation + list->sizeOfItem * itemCount);
    }

    list->count -= itemCount;
}

void ListArray_RemoveItem(ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);
    DebugAssertNullPointerCheck(item);

    long long itemIndex = ListArray_IndexOf(list, item);
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

    void *item = ListArray_Get(list, list->count - 1);
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

long long ListArray_IndexOf(const ListArray *list, const void *item)
{
    DebugAssertNullPointerCheck(list);

    for (size_t i = 0; i < list->count; i++)
    {
        void *currentItem = (void *)((char *)list->data + i * list->sizeOfItem);

        if (memcmp(currentItem, item, list->sizeOfItem) == 0)
        {
            return (long long)i;
        }
    }

    DebugWarning("Item not found in ListArray '%s'. Returning -1.", list->nameOfType);
    return -1;
}

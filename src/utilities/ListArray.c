//! disable debug info for this file
// #undef RJ_DEBUG_INFO
// #define RJ_DEBUG_INFO false

#include "utilities/ListArray.h"

#define ListArray_Min(a, b) ((a) < (b) ? (a) : (b))
#define ListArray_Max(a, b) ((a) > (b) ? (a) : (b))

ListArray ListArray_Create(const char *nameOfType, RJ_Size sizeOfItem, RJ_Size initialCapacity)
{
    RJ_DebugAssert(initialCapacity > 0, "Capacity of %s list can not be 0.", nameOfType);

    ListArray list;
    list.capacity = initialCapacity;
    list.sizeOfItem = sizeOfItem;

    RJ_Size nameOfTypeLength = (RJ_Size)strlen(nameOfType);

    RJ_DebugAssertAllocationCheck(char, list.nameOfType, nameOfTypeLength + 1);
    memcpy(list.nameOfType, nameOfType, nameOfTypeLength + 1);
    list.nameOfType[nameOfTypeLength] = '\0';

    list.count = 0;
    RJ_DebugAssertAllocationCheck(char, list.data, initialCapacity *sizeOfItem);

    RJ_DebugInfo("ListArray '%s' created with initial capacity: %u, size of item: %u", nameOfType, initialCapacity, sizeOfItem);
    return list;
}

void ListArray_Destroy(ListArray *list)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(list->data);

    RJ_Size nameOfTypeLength = (RJ_Size)strlen(list->nameOfType);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    memcpy(tempTitle, list->nameOfType, ListArray_Min(RJ_TEMP_BUFFER_SIZE - 1, nameOfTypeLength));
    tempTitle[ListArray_Min(RJ_TEMP_BUFFER_SIZE - 1, nameOfTypeLength)] = '\0';

    free(list->data);
    list->data = NULL;

    free(list->nameOfType);
    list->nameOfType = NULL;
    list->capacity = 0;
    list->count = 0;
    list->sizeOfItem = 0;

    RJ_DebugInfo("ListArray '%s' destroyed.", tempTitle);
}

ListArray ListArray_Copy(const ListArray *list)
{
    RJ_DebugAssertNullPointerCheck(list);

    ListArray copiedList = ListArray_Create(list->nameOfType, list->sizeOfItem, list->capacity);

    ListArray_AddRange(&copiedList, list->data, list->count);

    RJ_DebugInfo("ListArray '%s' copied. count: %u, capacity: %u", list->nameOfType, list->count, list->capacity);

    return copiedList;
}

void ListArray_Resize(ListArray *list, RJ_Size newCapacity)
{
    RJ_DebugAssertNullPointerCheck(list);

    if (newCapacity != 0)
    {
        list->data = realloc(list->data, newCapacity * list->sizeOfItem);
        RJ_DebugAssertNullPointerCheck(list->data);
    }

    RJ_DebugInfo("ListArray '%s' resized from %u to %u.", list->nameOfType, list->capacity, newCapacity);

    list->capacity = newCapacity;
    list->count = ListArray_Min(list->count, list->capacity);
}

void *ListArray_Get(const ListArray *list, RJ_Size index)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index trying to access: %u", list->nameOfType, list->count, index);

    void *itemLocation = (void *)((char *)list->data + index * list->sizeOfItem);

    return itemLocation;
}

void ListArray_Set(ListArray *list, RJ_Size index, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index: %u", list->nameOfType, list->count, index);

    void *targetLocation = ListArray_Get(list, index);

    memcpy(targetLocation, item, list->sizeOfItem);
}

void *ListArray_Add(ListArray *list, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);

    while (list->count + 1 > list->capacity)
    {
        RJ_DebugWarning("ListArray '%s' is full. Resizing it from %u to %u.", list->nameOfType, list->capacity, (RJ_Size)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (RJ_Size)((double)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);

    if (item == NULL)
    {
        memset(targetLocation, 0, list->sizeOfItem);
    }
    else
    {
        memcpy(targetLocation, item, list->sizeOfItem);
    }

    list->count++;

    return targetLocation;
}

void *ListArray_AddRange(ListArray *list, const void *item, RJ_Size itemCount)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(itemCount > 0, "Item count to add to ListArray '%s' must be greater than 0.", list->nameOfType);

    while (list->count + itemCount > list->capacity)
    {
        RJ_DebugWarning("ListArray '%s' is full. Resizing it from %u to %u.", list->nameOfType, list->capacity, (RJ_Size)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (RJ_Size)((double)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);

    if (item == NULL)
    {
        memset(targetLocation, 0, list->sizeOfItem * itemCount);
    }
    else
    {
        memcpy(targetLocation, item, list->sizeOfItem * itemCount);
    }

    list->count += itemCount;

    return targetLocation;
}

void *ListArray_AddToIndex(ListArray *list, RJ_Size index, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index trying to access: %u", list->nameOfType, list->count, index);

    while (list->count + 1 > list->capacity)
    {
        RJ_DebugWarning("ListArray '%s' is full. Resizing it from %u to %u.", list->nameOfType, list->capacity, (RJ_Size)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (RJ_Size)((double)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + index * list->sizeOfItem);

    RJ_Size bytesToMove = (list->count - index) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove((char *)targetLocation + list->sizeOfItem, targetLocation, bytesToMove);
    }

    if (item == NULL)
    {
        memset(targetLocation, 0, list->sizeOfItem);
    }
    else
    {
        memcpy(targetLocation, item, list->sizeOfItem);
    }

    list->count++;

    return targetLocation;
}

void ListArray_RemoveAtIndex(ListArray *list, RJ_Size index)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index: %u", list->nameOfType, list->count, index);

#if LIST_ARRAY_CUT_RESIZE
    if (list->count - 1 > 0 && (double)list->capacity > LIST_ARRAY_RESIZE_MULTIPLIER && list->count - 1 < (RJ_Size)((double)list->capacity / LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT))
    {
        RJ_DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %u to %u.", list->nameOfType, LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT, list->capacity, (RJ_Size)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (RJ_Size)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
    }
#endif

    void *targetLocation = ListArray_Get(list, index);

    RJ_Size bytesToMove = (list->count - index - 1) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove(targetLocation, (char *)targetLocation + list->sizeOfItem, bytesToMove);
    }

    list->count--;
}

void ListArray_RemoveRange(ListArray *list, RJ_Size index, RJ_Size itemCount)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index + itemCount <= list->count, "Index out of range for ListArray '%s'. List size: %u, index: %u, count: %u", list->nameOfType, list->count, index, itemCount);
    RJ_DebugAssert(itemCount > 0, "Item count to remove from ListArray '%s' must be greater than 0.", list->nameOfType);

#if LIST_ARRAY_CUT_RESIZE
    if (list->count - itemCount > 0 && (double)list->capacity > LIST_ARRAY_RESIZE_MULTIPLIER && list->count - itemCount < (RJ_Size)((double)list->capacity / LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT))
    {
        RJ_DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %u to %u.", list->nameOfType, LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT, list->capacity, (RJ_Size)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (RJ_Size)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
    }
#endif

    void *targetLocation = ListArray_Get(list, index);
    RJ_Size bytesToMove = (list->count - index - itemCount) * list->sizeOfItem;

    if (bytesToMove > 0)
    {
        memmove(targetLocation, (char *)targetLocation + list->sizeOfItem * itemCount, bytesToMove);
    }

    list->count -= itemCount;
}

void ListArray_RemoveItem(ListArray *list, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(item);

    long long iteListArray_Mindex = ListArray_IndexOf(list, item);
    if (iteListArray_Mindex == -1)
    {
        RJ_DebugWarning("Item not found in ListArray '%s'. Cannot remove.", list->nameOfType);
        return;
    }

    ListArray_RemoveAtIndex(list, (RJ_Size)iteListArray_Mindex);
}

void *ListArray_Pop(ListArray *list)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(list->count > 0, "Cannot pop from an empty ListArray '%s'.", list->nameOfType);

    void *item = ListArray_Get(list, list->count - 1);
    ListArray_RemoveAtIndex(list, list->count - 1);

    return item;
}

void ListArray_Clear(ListArray *list)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(list->data);

    list->count = 0;

    memset(list->data, 0, list->capacity * list->sizeOfItem);
}

long long ListArray_IndexOf(const ListArray *list, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);

    for (RJ_Size i = 0; i < list->count; i++)
    {
        void *currentItem = (void *)((char *)list->data + i * list->sizeOfItem);

        if (memcmp(currentItem, item, list->sizeOfItem) == 0)
        {
            return (long long)i;
        }
    }

    RJ_DebugWarning("Item not found in ListArray '%s'. Returning -1.", list->nameOfType);
    return -1;
}

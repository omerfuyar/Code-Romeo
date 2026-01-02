//! disable debug info for this file
// #undef RJ_DEBUG_INFO
// #define RJ_DEBUG_INFO false

#include "utilities/ListArray.h"

#define ListArray_Min(a, b) ((a) < (b) ? (a) : (b))
#define ListArray_Max(a, b) ((a) > (b) ? (a) : (b))

RJ_Result ListArray_Create(ListArray *retList, const char *title, RJ_Size sizeOfItem, RJ_Size initialCapacity)
{
    RJ_DebugAssert(initialCapacity > 0, "Capacity of %s list can not be 0.", title);

    retList->capacity = initialCapacity;
    retList->sizeOfItem = sizeOfItem;

    if (title == NULL)
    {
        title = "ListArray";
    }

    size_t titleLength = strlen(title);
    if (titleLength >= LIST_ARRAY_MAX_TITLE_LENGTH)
    {
        RJ_DebugWarning("ListArray title '%s' is longer than the maximum length of %d characters. It will be truncated.", title, LIST_ARRAY_MAX_TITLE_LENGTH - 1);
    }

    titleLength = ListArray_Min(LIST_ARRAY_MAX_TITLE_LENGTH - 1, titleLength);

    memcpy(retList->title, title, titleLength);
    retList->title[titleLength] = '\0';

    retList->count = 0;

    RJ_ReturnAllocate(char, retList->data, initialCapacity *sizeOfItem);

    RJ_DebugInfo("ListArray '%s' created with initial capacity: %u, size of item: %u", title, initialCapacity, sizeOfItem);
    return RJ_OK;
}

void ListArray_Destroy(ListArray *list)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(list->data);

    free(list->data);
    list->data = NULL;

    list->capacity = 0;
    list->count = 0;
    list->sizeOfItem = 0;

    RJ_DebugInfo("ListArray '%s' destroyed.", list->title);
    list->title[0] = '\0';
}

void ListArray_Resize(ListArray *list, RJ_Size newCapacity)
{
    RJ_DebugAssertNullPointerCheck(list);

    if (newCapacity != 0)
    {
        list->data = realloc(list->data, newCapacity * list->sizeOfItem);
        RJ_DebugAssertNullPointerCheck(list->data);
    }

    RJ_DebugInfo("ListArray '%s' resized from %u to %u.", list->title, list->capacity, newCapacity);

    list->capacity = newCapacity;
    list->count = ListArray_Min(list->count, list->capacity);
}

void *ListArray_Get(const ListArray *list, RJ_Size index)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index trying to access: %u", list->title, list->count, index);

    void *itemLocation = (void *)((char *)list->data + index * list->sizeOfItem);

    return itemLocation;
}

void ListArray_Set(ListArray *list, RJ_Size index, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index: %u", list->title, list->count, index);

    void *targetLocation = ListArray_Get(list, index);

    memcpy(targetLocation, item, list->sizeOfItem);
}

void *ListArray_Add(ListArray *list, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);

    while (list->count + 1 > list->capacity)
    {
        RJ_DebugWarning("ListArray '%s' is full. Resizing it from %u to %u.", list->title, list->capacity, (RJ_Size)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
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
    RJ_DebugAssert(itemCount > 0, "Item count to add to ListArray '%s' must be greater than 0.", list->title);

    while (list->count + itemCount > list->capacity)
    {
        RJ_DebugWarning("ListArray '%s' is full. Resizing it from %u to %u.", list->title, list->capacity, (RJ_Size)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
        ListArray_Resize(list, (RJ_Size)((double)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
    }

    void *targetLocation = (void *)((char *)list->data + list->count * list->sizeOfItem);

    if (item == NULL) // todo inspect https://github.com/omerfuyar/Code-Romeo/issues/40
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
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index trying to access: %u", list->title, list->count, index);

    while (list->count + 1 > list->capacity)
    {
        RJ_DebugWarning("ListArray '%s' is full. Resizing it from %u to %u.", list->title, list->capacity, (RJ_Size)((float)list->capacity * LIST_ARRAY_RESIZE_MULTIPLIER));
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
    RJ_DebugAssert(index < list->count, "Index out of range for ListArray '%s'. List size: %u, index: %u", list->title, list->count, index);

#if LIST_ARRAY_CUT_RESIZE
    if (list->count - 1 > 0 && (double)list->capacity > LIST_ARRAY_RESIZE_MULTIPLIER && list->count - 1 < (RJ_Size)((double)list->capacity / LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT))
    {
        RJ_DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %u to %u.", list->title, LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT, list->capacity, (RJ_Size)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
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
    RJ_DebugAssert(index + itemCount <= list->count, "Index out of range for ListArray '%s'. List size: %u, index: %u, count: %u", list->title, list->count, index, itemCount);
    RJ_DebugAssert(itemCount > 0, "Item count to remove from ListArray '%s' must be greater than 0.", list->title);

#if LIST_ARRAY_CUT_RESIZE
    if (list->count - itemCount > 0 && (double)list->capacity > LIST_ARRAY_RESIZE_MULTIPLIER && list->count - itemCount < (RJ_Size)((double)list->capacity / LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT))
    {
        RJ_DebugWarning("ListArray '%s' is less than 1/%d full. Resizing it from %u to %u.", list->title, LIST_ARRAY_ListArray_Min_DECIMAL_LIMIT, list->capacity, (RJ_Size)((double)list->capacity / LIST_ARRAY_RESIZE_MULTIPLIER));
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
        RJ_DebugWarning("Item not found in ListArray '%s'. Cannot remove.", list->title);
        return;
    }

    ListArray_RemoveAtIndex(list, (RJ_Size)iteListArray_Mindex);
}

void *ListArray_Pop(ListArray *list)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(list->count > 0, "Cannot pop from an empty ListArray '%s'.", list->title);

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

    RJ_DebugWarning("Item not found in ListArray '%s'. Returning -1.", list->title);
    return -1;
}

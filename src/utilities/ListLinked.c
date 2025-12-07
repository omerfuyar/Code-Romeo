//! disable debug info for this file
#undef RJGLOBAL_DEBUG_INFO
#define RJGLOBAL_DEBUG_INFO false

#include "utilities/ListLinked.h"

#define ListLinked_Min(a, b) ((a) < (b) ? (a) : (b))
#define ListLinked_Max(a, b) ((a) > (b) ? (a) : (b))

#pragma region Source Only

/// @brief A node in the linked list.
typedef struct ListLinkedNode
{
    struct ListLinkedNode *next;
    void *data;
} ListLinkedNode;

/// @brief Creator function for ListLinkedNode. Uses memcpy to copy the data.
/// @param sizeOfData Size of the data to be stored in the node.
/// @param data Pointer to the data to store inside the node.
/// @return The created ListLinkedNode struct.
ListLinkedNode *ListLinkedNode_Create(RJGlobal_Size sizeOfData, const void *data)
{
    ListLinkedNode *node = NULL;
    RJGlobal_DebugAssertAllocationCheck(ListLinkedNode, node, 1);
    RJGlobal_DebugAssertAllocationCheck(char, node->data, sizeOfData);

    if (data != NULL)
    {
        RJGlobal_MemoryCopy(node->data, sizeOfData, data);
    }
    else
    {
        RJGlobal_MemorySet(node->data, sizeOfData, 0);
    }

    node->next = NULL;

    return node;
}

/// @brief Destroyer function for ListLinkedNode. Frees the data and the node itself.
/// @param node Pointer to the ListLinkedNode to destroy.
void ListLinkedNode_Destroy(ListLinkedNode *node)
{
    RJGlobal_DebugAssertNullPointerCheck(node);

    node->next = NULL;

    free(node->data);
    free(node);
}

/// @brief Destroyer function for ListLinkedNode. Destroys itself and the next node recursively.
/// @param node Pointer to the head node of the linked list to destroy.
void ListLinkedNode_DestroyAll(ListLinkedNode *node)
{
    RJGlobal_DebugAssertNullPointerCheck(node);

    if (node->next != NULL)
    {
        ListLinkedNode_DestroyAll(node->next);
    }

    ListLinkedNode_Destroy(node);
}

/// @brief Connects the node to nextNode.
/// @param node Node to connect to next.
/// @param nextNode Next node to connect.
void ListLinkedNode_Connect(ListLinkedNode *node, ListLinkedNode *nextNode)
{
    RJGlobal_DebugAssertNullPointerCheck(node);

    node->next = nextNode;
}

/// @brief Connects two linked list nodes if the next is null recursively.
/// @param node Pointer to the current node to set next node of.
/// @param nextNode Pointer to the next node to connect.
void ListLinkedNode_Append(ListLinkedNode *node, ListLinkedNode *nextNode)
{
    RJGlobal_DebugAssertNullPointerCheck(node);
    RJGlobal_DebugAssertNullPointerCheck(nextNode);

    if (node->next == NULL)
    {
        ListLinkedNode_Connect(node, nextNode);
    }
    else
    {
        ListLinkedNode_Append(node->next, nextNode);
    }
}

/// @brief Gets the index of the given item from start index recursively.
/// @param node Node to search.
/// @param sizeOfItem Size of an item in the list.
/// @param data Data to compare with node's data.
/// @param startIndex Index to pass until the item is found. Incremented by one if the data didn't match.
/// @return The index which is incremented from previous calls. Returns -1 if item is not found in the sequence.
long long ListLinkedNode_GetIndexIfMatch(ListLinkedNode *node, RJGlobal_Size sizeOfItem, const void *data, long long startIndex)
{
    if (RJGlobal_MemoryCompare(node->data, sizeOfItem, data) == 0)
    {
        return startIndex;
    }
    else if (node->next == NULL)
    {
        RJGlobal_DebugWarning("Item not found in ListLinked. Returning -1.");
        return -1;
    }
    else
    {
        startIndex++;
        return ListLinkedNode_GetIndexIfMatch(node->next, sizeOfItem, data, startIndex);
    }
}

#pragma endregion Source Only

ListLinked ListLinked_Create(const char *nameOfType, RJGlobal_Size sizeOfItem)
{
    ListLinked list;
    list.count = 0;
    list.sizeOfItem = sizeOfItem;
    list.head = NULL;

    RJGlobal_Size nameOfTypeLength = RJGlobal_StringLength(nameOfType);

    RJGlobal_DebugAssertAllocationCheck(char, list.nameOfType, nameOfTypeLength + 1);
    RJGlobal_MemoryCopy(list.nameOfType, nameOfTypeLength + 1, nameOfType);
    list.nameOfType[nameOfTypeLength] = '\0';

    RJGlobal_DebugInfo("ListLinked '%s' created with size of item: %u", nameOfType, sizeOfItem);
    return list;
}

void ListLinked_Destroy(ListLinked *list)
{
    RJGlobal_DebugAssertNullPointerCheck(list);

    RJGlobal_Size nameOfTypeLength = RJGlobal_StringLength(list->nameOfType);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, ListLinked_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, nameOfTypeLength), list->nameOfType);
    tempTitle[ListLinked_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, nameOfTypeLength)] = '\0';

    if (list->head != NULL)
    {
        ListLinkedNode_DestroyAll(list->head);
    }

    list->head = NULL;

    free(list->nameOfType);
    list->nameOfType = NULL;
    list->count = 0;
    list->sizeOfItem = 0;

    RJGlobal_DebugInfo("ListLinked '%s' destroyed.", tempTitle);
}

void *ListLinked_Get(const ListLinked *list, RJGlobal_Size index)
{
    RJGlobal_DebugAssertNullPointerCheck(list);
    RJGlobal_DebugAssert(index < list->count, "Index out of range. List count : %du, index : %du", list->count, index);

    ListLinkedNode *currentNode = list->head;

    for (RJGlobal_Size i = 0; i < index; i++)
    {
        currentNode = currentNode->next;
    }

    return currentNode->data;
}

void ListLinked_Set(ListLinked *list, RJGlobal_Size index, const void *item)
{
    RJGlobal_DebugAssertNullPointerCheck(list);
    RJGlobal_DebugAssert(index < list->count, "Index out of range. List count : %du, index : %du", list->count, index);

    ListLinkedNode *nodeToSet = ListLinked_Get(list, index);
    RJGlobal_MemoryCopy(nodeToSet->data, list->sizeOfItem, item);
}

void *ListLinked_Add(ListLinked *list, const void *item)
{
    RJGlobal_DebugAssertNullPointerCheck(list);

    ListLinkedNode *newNode = ListLinkedNode_Create(list->sizeOfItem, item);

    if (list->head == NULL)
    {
        list->head = newNode;
    }
    else
    {
        ListLinkedNode_Append(list->head, newNode);
    }

    list->count++;

    return newNode->data;
}

void ListLinked_RemoveAtIndex(ListLinked *list, RJGlobal_Size index)
{
    RJGlobal_DebugAssertNullPointerCheck(list);
    RJGlobal_DebugAssertNullPointerCheck(list->head);
    RJGlobal_DebugAssert(index < list->count, "Index out of range. List count : %du, index : %du", list->count, index);

    if (index == 0)
    {
        ListLinkedNode *oldHead = list->head;
        list->head = oldHead->next;
        ListLinkedNode_Destroy(oldHead);
    }
    else
    {
        ListLinkedNode *previousNodeToRemove = ListLinked_Get(list, index - 1);
        ListLinkedNode *nodeToRemove = previousNodeToRemove->next;
        ListLinkedNode_Connect(previousNodeToRemove, nodeToRemove->next);
        ListLinkedNode_Destroy(nodeToRemove);
    }

    list->count--;
}

void ListLinked_RemoveItem(ListLinked *list, const void *item)
{
    RJGlobal_DebugAssertNullPointerCheck(list);
    RJGlobal_DebugAssertNullPointerCheck(list->head);

    ListLinked_RemoveAtIndex(list, (RJGlobal_Size)ListLinked_IndexOf(list, item));
}

void ListLinked_Clear(ListLinked *list)
{
    RJGlobal_DebugAssertNullPointerCheck(list);

    if (list->head != NULL)
    {
        ListLinkedNode_DestroyAll(list->head);
        list->head = NULL;
    }

    list->count = 0;
}

long long ListLinked_IndexOf(const ListLinked *list, const void *item)
{
    RJGlobal_DebugAssertNullPointerCheck(list);
    RJGlobal_DebugAssertNullPointerCheck(list->head);

    return ListLinkedNode_GetIndexIfMatch(list->head, list->sizeOfItem, item, 0);
}

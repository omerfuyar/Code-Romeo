#include "utilities/ListLinked.h"
#include <string.h>

#pragma region Source Only

/// @brief A node in the linked list.
typedef struct ListLinkedNode
{
    void *data;
    struct ListLinkedNode *next;
} ListLinkedNode;

typedef struct ListLinked
{
    ListLinkedNode *head;
    size_t size;
    size_t sizeOfItem;
} ListLinked;

/// @brief Creator function for ListLinkedNode. Uses memcpy to copy the data.
/// @param sizeOfData Size of the data to be stored in the node.
/// @param data Pointer to the data to store inside the node.
/// @return The created ListLinkedNode struct.
ListLinkedNode *ListLinkedNode_Create(size_t sizeOfData, const void *data)
{
    ListLinkedNode *node = (ListLinkedNode *)malloc(sizeof(ListLinkedNode));
    DebugAssertPointerNullCheck(node);

    node->data = (void *)malloc(sizeOfData);
    DebugAssertPointerNullCheck(node->data);

    memcpy(node->data, data, sizeOfData);

    node->next = NULL;

    return node;
}

/// @brief Destroyer function for ListLinkedNode. Frees the data and the node itself.
/// @param node Pointer to the ListLinkedNode to destroy.
void ListLinkedNode_Destroy(ListLinkedNode *node)
{
    DebugAssertPointerNullCheck(node);

    if (node->data != NULL)
    {
        free(node->data);
    }
    node->data = NULL;
    node->next = NULL;

    free(node);
    node = NULL;
}

/// @brief Destroyer function for ListLinkedNode. Destroys itself and the next node recursively.
/// @param node Pointer to the head node of the linked list to destroy.
void ListLinkedNode_DestroyAll(ListLinkedNode *node)
{
    DebugAssertPointerNullCheck(node);

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
    node->next = nextNode;
}

/// @brief Connects two linked list nodes if the next is null recursively.
/// @param node Pointer to the current node to set next node of.
/// @param nextNode Pointer to the next node to connect.
void ListLinkedNode_Append(ListLinkedNode *node, ListLinkedNode *nextNode)
{
    if (node->next == NULL)
    {
        ListLinkedNode_Connect(node->next, nextNode);
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
long long ListLinkedNode_GetIndexIfMatch(ListLinkedNode *node, size_t sizeOfItem, const void *data, long long startIndex)
{
    if (memcmp(node->data, data, sizeOfItem) == 0)
    {
        return startIndex;
    }
    else if (node->next == NULL)
    {
        DebugWarning("Item not found in ListLinked. Returning -1.");
        return -1;
    }
    else
    {
        startIndex++;
        return ListLinkedNode_GetIndexIfMatch(node->next, sizeOfItem, data, startIndex);
    }
}

#pragma endregion Source Only

ListLinked *ListLinked_Create(size_t sizeOfItem)
{
    ListLinked *list = (ListLinked *)malloc(sizeof(ListLinked));
    DebugAssertPointerNullCheck(list);

    list->size = 0;
    list->sizeOfItem = sizeOfItem;
    list->head = NULL;

    DebugInfo("ListLinked created with size of item: %zu", sizeOfItem);
    return list;
}

void ListLinked_Destroy(ListLinked *list)
{
    DebugAssertPointerNullCheck(list);

    if (list->head != NULL)
    {
        ListLinkedNode_DestroyAll(list->head);
    }

    list->head = NULL;

    free(list);
    list = NULL;

    DebugInfo("ListLinked destroyed.");
}

void *ListLinked_Get(ListLinked *list, size_t index)
{
    DebugAssertPointerNullCheck(list);
    DebugAssert(index < list->size, "Index out of range. List size : %du, index : %du", list->size, index);

    ListLinkedNode *currentNode = list->head;

    for (size_t i = 0; i < index; i++)
    {
        currentNode = currentNode->next;
    }

    return currentNode->data;
}

void ListLinked_Set(ListLinked *list, size_t index, const void *item)
{
    DebugAssertPointerNullCheck(list);
    DebugAssert(index < list->size, "Index out of range. List size : %du, index : %du", list->size, index);

    ListLinkedNode *nodeToSet = ListLinked_Get(list, index);
    memcpy(nodeToSet->data, item, list->sizeOfItem);
}

void ListLinked_Add(ListLinked *list, const void *item)
{
    DebugAssertPointerNullCheck(list);

    ListLinkedNode *newNode = ListLinkedNode_Create(list->sizeOfItem, item);

    if (list->head == NULL)
    {
        list->head = newNode;
    }
    else
    {
        ListLinkedNode_Append(list->head, newNode);
    }

    list->size++;
}

void ListLinked_RemoveAtIndex(ListLinked *list, size_t index)
{
    DebugAssertPointerNullCheck(list);
    DebugAssert(index < list->size, "Index out of range. List size : %du, index : %du", list->size, index);

    if (index == 0)
    {
        list->head = list->head->next;
        ListLinkedNode_Destroy(list->head);
    }
    else
    {
        ListLinkedNode *previousNodeToRemove = ListLinked_Get(list, index - 1);
        ListLinkedNode_Connect(previousNodeToRemove, previousNodeToRemove->next->next);
        ListLinkedNode_Destroy(previousNodeToRemove->next);
    }

    list->size--;
}

void ListLinked_RemoveItem(ListLinked *list, const void *item)
{
    DebugAssertPointerNullCheck(list);

    ListLinked_RemoveAtIndex(list, (size_t)ListLinked_IndexOf(list, item));
}

void ListLinked_Clear(ListLinked *list)
{
    DebugAssertPointerNullCheck(list);

    ListLinkedNode_DestroyAll(list->head);
}

long long ListLinked_IndexOf(ListLinked *list, const void *item)
{
    DebugAssertPointerNullCheck(list);
    DebugAssertPointerNullCheck(list->head);

    return ListLinkedNode_GetIndexIfMatch(list->head, list->sizeOfItem, item, 0);
}

size_t ListLinked_GetSize(ListLinked *list)
{
    DebugAssertPointerNullCheck(list);

    return list->size;
}

size_t ListLinked_GetSizeOfItem(ListLinked *list)
{
    DebugAssertPointerNullCheck(list);

    return list->sizeOfItem;
}

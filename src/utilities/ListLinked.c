//! disable debug info for this file
// #undef RJ_DEBUG_INFO
// #define RJ_DEBUG_INFO false

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
static RJ_Result ListLinkedNode_Create(ListLinkedNode **retNode, RJ_Size sizeOfData, const void *data)
{
    ListLinkedNode *node = NULL;
    RJ_ReturnAllocate(ListLinkedNode, node, 1);
    RJ_ReturnAllocate(char, node->data, sizeOfData);

    *retNode = node;

    if (data != NULL)
    {
        memcpy(node->data, data, sizeOfData);
    }
    else
    {
        memset(node->data, 0, sizeOfData);
    }

    node->next = NULL;

    return RJ_OK;
}

/// @brief Destroyer function for ListLinkedNode. Frees the data and the node itself.
/// @param node Pointer to the ListLinkedNode to destroy.
static void ListLinkedNode_Destroy(ListLinkedNode *node)
{
    RJ_DebugAssertNullPointerCheck(node);

    node->next = NULL;

    free(node->data);
    free(node);
}

/// @brief Destroyer function for ListLinkedNode. Destroys itself and the next node recursively.
/// @param node Pointer to the head node of the linked list to destroy.
static void ListLinkedNode_DestroyAll(ListLinkedNode *node)
{
    RJ_DebugAssertNullPointerCheck(node);

    if (node->next != NULL)
    {
        ListLinkedNode_DestroyAll(node->next);
    }

    ListLinkedNode_Destroy(node);
}

/// @brief Connects the node to nextNode.
/// @param node Node to connect to next.
/// @param nextNode Next node to connect.
static void ListLinkedNode_Connect(ListLinkedNode *node, ListLinkedNode *nextNode)
{
    RJ_DebugAssertNullPointerCheck(node);

    node->next = nextNode;
}

/// @brief Connects two linked list nodes if the next is null recursively.
/// @param node Pointer to the current node to set next node of.
/// @param nextNode Pointer to the next node to connect.
static void ListLinkedNode_Append(ListLinkedNode *node, ListLinkedNode *nextNode)
{
    RJ_DebugAssertNullPointerCheck(node);
    RJ_DebugAssertNullPointerCheck(nextNode);

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
static long long ListLinkedNode_GetIndexIfMatch(ListLinkedNode *node, RJ_Size sizeOfItem, const void *data, long long startIndex)
{
    if (memcmp(node->data, data, sizeOfItem) == 0)
    {
        return startIndex;
    }
    else if (node->next == NULL)
    {
        RJ_DebugWarning("Item not found in ListLinked. Returning -1.");
        return -1;
    }
    else
    {
        startIndex++;
        return ListLinkedNode_GetIndexIfMatch(node->next, sizeOfItem, data, startIndex);
    }
}

#pragma endregion Source Only

ListLinked ListLinked_Create(const char *title, RJ_Size sizeOfItem)
{
    ListLinked list = {0};
    list.count = 0;
    list.sizeOfItem = sizeOfItem;
    list.head = NULL;

    RJ_Size nameOfTypeLength = (RJ_Size)strlen(title);

    if (title == NULL)
    {
        title = "ListLinked";
    }

    size_t titleLength = strlen(title);
    if (titleLength >= LIST_LINKED_MAX_TITLE_LENGTH)
    {
        RJ_DebugWarning("ListLinked title '%s' is longer than the maximum length of %d characters. It will be truncated.", title, LIST_LINKED_MAX_TITLE_LENGTH - 1);
    }

    titleLength = ListLinked_Min(LIST_LINKED_MAX_TITLE_LENGTH - 1, titleLength);

    memcpy(list.title, title, titleLength);
    list.title[titleLength] = '\0';

    RJ_DebugInfo("ListLinked '%s' created with capacity: %u, size of item: %u", list.title, list.count, sizeOfItem);

    return list;
}

void ListLinked_Destroy(ListLinked *list)
{
    RJ_DebugAssertNullPointerCheck(list);

    if (list->head != NULL)
    {
        ListLinkedNode_DestroyAll(list->head);
    }

    list->head = NULL;

    list->count = 0;
    list->sizeOfItem = 0;

    RJ_DebugInfo("ListLinked '%s' destroyed.", list->title);
    list->title[0] = '\0';
}

void *ListLinked_Get(const ListLinked *list, RJ_Size index)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range. List count : %du, index : %du", list->count, index);

    ListLinkedNode *currentNode = list->head;

    for (RJ_Size i = 0; i < index; i++)
    {
        currentNode = currentNode->next;
    }

    return currentNode->data;
}

void ListLinked_Set(ListLinked *list, RJ_Size index, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssert(index < list->count, "Index out of range. List count : %du, index : %du", list->count, index);

    ListLinkedNode *nodeToSet = ListLinked_Get(list, index);
    memcpy(nodeToSet->data, item, list->sizeOfItem);
}

void *ListLinked_Add(ListLinked *list, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);

    ListLinkedNode *newNode = NULL;
    ListLinkedNode_Create(&newNode, list->sizeOfItem, item);

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

void ListLinked_RemoveAtIndex(ListLinked *list, RJ_Size index)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(list->head);
    RJ_DebugAssert(index < list->count, "Index out of range. List count : %du, index : %du", list->count, index);

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
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(list->head);

    ListLinked_RemoveAtIndex(list, (RJ_Size)ListLinked_IndexOf(list, item));
}

void ListLinked_Clear(ListLinked *list)
{
    RJ_DebugAssertNullPointerCheck(list);

    if (list->head != NULL)
    {
        ListLinkedNode_DestroyAll(list->head);
        list->head = NULL;
    }

    list->count = 0;
}

long long ListLinked_IndexOf(const ListLinked *list, const void *item)
{
    RJ_DebugAssertNullPointerCheck(list);
    RJ_DebugAssertNullPointerCheck(list->head);

    return ListLinkedNode_GetIndexIfMatch(list->head, list->sizeOfItem, item, 0);
}

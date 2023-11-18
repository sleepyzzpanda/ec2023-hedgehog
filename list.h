
#ifndef _LIST_H_
#define _LIST_H_
#include <stdbool.h>

#define LIST_SUCCESS 0
#define LIST_FAIL -1

typedef struct Node_s Node;
struct Node_s {
    void* pItem;
    Node* pNext;
    Node* pPrev;
};

enum ListOutOfBounds {
    LIST_OOB_START,
    LIST_OOB_END
};

typedef struct List_s List;
struct List_s{
    Node* pFirstNode;
    Node* pLastNode;
    Node* pCurrentNode;
    int count;
    List* pNextFreeHead;
    enum ListOutOfBounds lastOutOfBoundsReason;
};

#define LIST_MAX_NUM_HEADS 100
#define LIST_MAX_NUM_NODES 1000

List* List_create();
int List_count(List* pList);
void* List_first(List* pList);
void* List_last(List* pList); 
void* List_next(List* pList);
void* List_prev(List* pList);
void* List_curr(List* pList);
int List_insert_after(List* pList, void* pItem);
int List_insert_before(List* pList, void* pItem);
int List_append(List* pList, void* pItem);
int List_prepend(List* pList, void* pItem);
void* List_remove(List* pList);
void List_concat(List* pList1, List* pList2);
typedef void (*FREE_FN)(void* pItem);
void List_free(List* pList, FREE_FN pItemFreeFn);
void* List_trim(List* pList);
typedef bool (*COMPARATOR_FN)(void* pItem, void* pComparisonArg);
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg);

#endif

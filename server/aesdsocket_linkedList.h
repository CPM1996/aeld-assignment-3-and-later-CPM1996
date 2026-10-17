typedef struct linkedListElementStr LinkedListElement;

typedef struct linkedListElementStr
{
    LinkedListElement *prev;
    LinkedListElement *next;
} LinkedListElement; // MUST ALWAYS GO FIRST IN A STRUCT

typedef struct
{
    LinkedListElement *first;
    // LinkedListElement *last;
} LinkedList;

typedef int (* LinkedListCallback)(LinkedListElement *, void *);

int initLinkedList(LinkedList *);
int insertElement(LinkedList *, LinkedListElement *);
int deleteElement(LinkedList *, LinkedListElement *);
int travelList(LinkedList *, LinkedListCallback, void *);
int destroyLinkedList(LinkedList *);

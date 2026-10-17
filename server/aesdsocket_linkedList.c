#include <stdlib.h>

#include "aesdsocket_linkedList.h"

int initLinkedList(LinkedList * linkedList)
{
    linkedList->first = NULL;
    // linkedList->last = NULL;

    return 0;
}

int insertElement(LinkedList * linkedList, LinkedListElement * element)
{
    LinkedListElement *auxElement;

    if(linkedList->first == NULL)
    {
        linkedList->first = element;
        // linkedList->last = element;

        element->prev = NULL;
        element->next = NULL;

        return 0;
    }

    auxElement = linkedList->first;

    element->prev = NULL;
    element->next = auxElement;

    linkedList->first = element;

    return 0;
}

int deleteElement(LinkedList * linkedList, LinkedListElement * element)
{
    LinkedListElement *prevElement, *nextElement;

    prevElement = element->prev;
    nextElement = element->next;

    if(prevElement)
    {
        prevElement->next = nextElement;
    }
    // First element in list
    else
    {
        linkedList->first = nextElement;
    }

    if(nextElement)
    {
        nextElement->prev = prevElement;
    }

    free(element);
    return 0;
}

int travelList(LinkedList *linkedList, LinkedListCallback callback, void *param)
{
    int res = 0;
    LinkedListElement *auxElement;

    for(auxElement = linkedList->first; auxElement != NULL; auxElement = auxElement->next)
    {
        res |= callback(auxElement, param);
    }

    return res;
}

int destroyLinkedList(LinkedList *linkedList)
{
    LinkedListElement *auxElement, *auxNextElement;

    auxElement = linkedList->first;
    while(auxElement != NULL)
    {
        auxNextElement = auxElement->next;
        deleteElement(linkedList, auxElement);
        auxElement = auxNextElement;
    }

    return 0;
}
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include "SortedList.h"

int opt_yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    if (!list ||!element) {
        return;
    }

    SortedListElement_t *cur = list->next;
    while (cur != list && (strcmp(cur->key, element->key) < 0)) {
        cur = cur->next;
    }
    if (opt_yield & INSERT_YIELD){
        sched_yield();
    }
    element->next = cur;
    element->prev = cur->prev;
    element->prev->next = element;
    cur->prev = element;

}

int SortedList_delete(SortedListElement_t *element) {
    if (!element->key) {
        return 1;
    }

    if (element->next->prev != element || element->prev->next != element) {
        return 1;
    }

    element->prev->next = element->next;
    if (opt_yield & DELETE_YIELD){
        sched_yield();
    }
    element->next->prev = element->prev;
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    if (!list || !key) {
        return NULL;
    }

    SortedListElement_t *cur = list->next;
    while (cur != list) {
        if (strcmp(cur->key, key) == 0)
            return cur;
        if (strcmp(cur->key, key) > 0)
            return NULL;
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        cur = cur->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list) {
    int count = 0;
    if (!list) {
        fprintf(stderr, "List head is null\n");
        return -1;
    }
    SortedListElement_t *cur = list->next;
    while (cur != list) {
        if (!cur || cur->prev->next != cur || cur->next->prev != cur){
            return -1;
        }
        count += 1;
        cur = cur->next;
        if (opt_yield & LOOKUP_YIELD){
            sched_yield();
        }
    }
    return count;
}

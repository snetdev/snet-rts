#include <list.h>
#include <stdlib.h>
#include <memfun.h>
#include <stdio.h>

struct list_elem {
  struct list_elem* next;
  struct list_elem* prev;

  void* content;
};

struct list {
  struct list_elem *first;
  struct list_elem *last;
  struct list_elem *current;
};

static void fatal(char* error) {
  fprintf(stderr, "\n\n** FATAL ERROR ** %s\n\n", error);
  exit(1);
}

extern SNetUtilList SNetUtilListCreate() {
  struct list *result = SNetMemAlloc(sizeof(struct list));
  result->first = NULL;
  result->last = NULL;
  result->current = NULL;
  return result;
}

extern void SNetUtilListDestroy(SNetUtilList target) {
  struct list_elem *next = NULL;
  struct list_elem *current = NULL;
  if(target == NULL) return;
  next = target->first;
  while(next != NULL) {
    current = next;
    next = next->next;
    SNetMemFree(current);
  }
  SNetMemFree(target->current);
  SNetMemFree(target);
}

static void link(struct list_elem *pred, struct list_elem* succ) {
  pred->next = succ;
  succ->prev = pred;
}

extern void SNetUtilListAddAfter(SNetUtilList target, void* content) {
  struct list_elem *succ;
  struct list_elem *pred;
  struct list_elem *elem;
 
  if(target == NULL) fatal("target == null in AddAfter!");
  if(target->current == NULL) fatal("target->current == null in AddAfter!");
  elem = SNetMemAlloc(sizeof(struct list_elem));
  elem->content = content;

  if(target->current == NULL) {
    /* list is empty */
    target->first = elem;
    target->last = elem;
    elem->next = NULL;
    elem->prev = NULL;
  } else {
    if(target->current->next == NULL) {
      /* adding to end of list */
      target->last = elem;
      link(target->current, elem);
    } else {
      /* adding somewhere in the list */
      pred = target->current;
      succ = target->current->next;
      link(pred, elem);
      link(elem, succ);
    }
  }
}


extern void SNetUtilListAddBefore(SNetUtilList target, void* content) {
  struct list_elem *succ;
  struct list_elem *pred;
  struct list_elem *elem;

  if(target == NULL) fatal("target == null in AddBefore!");
  if(target->current == NULL) fatal("target->current == null in AddBefore!");

  elem = SNetMemAlloc(sizeof(struct list_elem));
  elem->content = content;
  if(target->current == NULL) {
    /* list is empty */
    target->first = elem;
    target->last = elem;
    elem->next = NULL;
    elem->prev = NULL;
  } else {
    if(target->current->prev == NULL) {
      /* adding to beginning of list */
      target->first = elem;
      link(elem, target->current);
    } else {
      /* adding somewhere in the list */
      pred = target->current->prev;
      succ = target->current;
      link(pred, elem);
      link(elem, succ);
    }
  }
}

extern void SNetUtilListAddBeginning(SNetUtilList target, void* content) {
  struct list_elem *temp;

  if(target == NULL) fatal("target == null in AddBeginning");

  temp = target->current;
  target->current = target->first;
  SNetUtilListAddBefore(target, content);
  target->current = temp;
}

extern void SNetUtilListAddEnd(SNetUtilList target, void* content) {
  struct list_elem *temp = target->current;

  if(target == NULL) fatal("target == null in AddBeginning");

  temp = target->current;
  target->current = target->last;
  SNetUtilListAddAfter(target, content);
  target->current = temp;
}

extern void SNetUtilListGotoBeginning(SNetUtilList target) {
  if(target == NULL) fatal("target == null in GotoBeginning!");

  target->current = target->first;
}

extern void SNetUtilListGotoEnd(SNetUtilList target) {
  if(target == NULL) fatal("target == null in GotoEnd!");

  target->current = target->last;
}

extern void SNetUtilListNext(SNetUtilList target) {
  if(target == NULL) fatal("target == null in ListNext!");
  if(target->current == NULL) fatal("target->current == null in ListNext!");

  if(target->current->next != NULL) {
    target->current = target->current->next;
  }
}

extern void SNetUtilListPrev(SNetUtilList target) {
  if(target == NULL) fatal("target == null in ListPrev!");
  if(target->current == NULL) fatal("target->current == null in ListPrev");

  if(target->current->prev != NULL) {
    target->current = target->current->prev;
  }
}

extern void* SNetUtilListGet(SNetUtilList target) {
  if(target == NULL) fatal("target == null in ListGet");
  if(target->current == NULL) fatal("target->current == null in listGet");

  return target->current->content;
}

extern int SNetUtilListHasNext(SNetUtilList target) {
  if(target == NULL) fatal("target == null in HasNext");
  if(target->current == NULL) fatal("target->current == NULL!");

  return target->current->next != NULL;
}

extern int SNetUtilListHasPrev(SNetUtilList target) {
  if(target == NULL) fatal("target == null in HasPrev");
  if(target->current == NULL) fatal("target->current == null in HasPrev!");

  return target->current->prev != NULL;
}

extern void SNetUtilListDeleteCurrent(SNetUtilList target) {
  struct list_elem* pred;
  struct list_elem* succ;

  if(target == NULL) fatal("target == null in DeleteCurrent!");
  if(target->current == NULL) 
    fatal("target->current == null in DeleteCurrent!");

  pred = target->current->prev;
  succ = target->current->next;
  link(pred, succ);
  SNetMemFree(target->current);
  target->current = succ;
}

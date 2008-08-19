#include <stdlib.h>

#include "list.h"
#include "memfun.h"
#include "debug.h"

/**
 *
 * @file
 *
 * This implements a double linked list.
 *
 * This implements a doubly linked list that supports arbitrary contents. 
 * Furthermore, it is possible to traverse this list, because it keeps track
 * of a current element that can be inspected or removed or you 
 * can add elements
 * before or after it. 
 */

struct list_elem {
  struct list_elem *next;
  struct list_elem *prev;

  void *content;
};

struct list {
  struct list_elem *first;
  struct list_elem *last;
  struct list_elem *current;
};

struct list_iterator {
  struct list *base;
  struct list_elem *current;
};

#define NULLCHECK(VAR) if((VAR) == NULL) { SNetUtilDebugFatal("%s: %s == null", __FUNCTION__, #VAR);}

static void Link(struct list_elem *pred, struct list_elem *succ) {
  pred->next = succ;
  succ->prev = pred;
}

void SNetUtilListDump(snet_util_list_t *target) {
  struct list_elem *current;

  fprintf(stderr, "List %x:\n", (unsigned int) target);
  fprintf(stderr, "first: %x, last: %x\n", 
          (unsigned int) target->first,
          (unsigned int) target->last);
  fprintf(stderr, " current    | prev       | content    | next\n");
  fprintf(stderr, "--------------------------------------------\n");
  
  current = target->first;
  while(current) {
    fprintf(stderr, " %10x | %10x | %10x | %10x\n",
            (unsigned int) current,
            (unsigned int) current->prev,
            (unsigned int) current->content,
            (unsigned int) current->next);
    current = current->next;
  }
  fprintf(stderr, "\n");
}

snet_util_list_t *SNetUtilListIterMoveToBack(snet_util_list_iter_t *target) {
  void *content;
  snet_util_list_t *base_list;

  NULLCHECK(target);

  base_list = target->base;
  content = SNetUtilListIterGet(target);
  base_list = SNetUtilListIterDelete(target);
  base_list = SNetUtilListAddEnd(base_list, content);
  return base_list;
}

/** <!--********************************************************************-->
 *
 * @fn snet_util_list_t *SNetUtilListCreate()
 *
 * @brief returns a new, initialized empty list. 
 * 
 *        This just allocates a new list, initializes it as empty and returns 
 *        it. If we are unable to create this list because memory is full,
 *        this results in some fatal error. 
 *
 * @return an empty list, never NULL.
 *
 *****************************************************************************/
snet_util_list_t *SNetUtilListCreate() {
  struct list *result = SNetMemAlloc(sizeof(snet_util_list_t));
  NULLCHECK(result)
  result->first = NULL;
  result->last = NULL;
  result->current = NULL;
  return result;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListDestroy(snet_util_list_t *target)
 *
 * @brief destroys the list and possible elements that are still in it.
 *
 *        This destroys the given list. Any elements that might be left will be
 *        deallocated as well. The contents of the list will not be touched. 
 *
 * @param target The list to be deallocated. if it is NULL already, nothing
 *        happens. 
 *
 ******************************************************************************/
void SNetUtilListDestroy(snet_util_list_t *target) {
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

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListAddBeginning(snet_util_list_t *target, void *content
 *
 * @brief Adds the content to the beginning of the list
 *
 *      This adds the element to the beginning of the list. The current
 *      element is not affected, that is, it still points to the same element. 
 *
 * @param target the list to add to
 * @param content the new element to add
 *
 *****************************************************************************/
snet_util_list_t *
SNetUtilListAddBeginning(snet_util_list_t *target, void *content) {
  struct list_elem *new_element;

  NULLCHECK(target)

  new_element = SNetMemAlloc(sizeof(struct list_elem));
  new_element->content = content;
  new_element->prev = NULL;

  if(target->first == NULL) {
    /* empty list */
    target->first = new_element;
    target->last = new_element;
    new_element->next = NULL;
  } else {
    /* list with at least 1 element */
    new_element->next = target->first;
    target->first->prev = new_element;
    target->first = new_element;
  }
  return target;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListAddEnd(snet_util_list_t *target, void *content)
 *
 * @brief Adds the content to the end of the list
 *
 *      This adds the element to the end of the list. the current
 *      element is not affected, that is, it still points to the same
 *      element.
 *
 * @param target the list to add to
 * @param content the new element to add
 *
 *****************************************************************************/
snet_util_list_t *SNetUtilListAddEnd(snet_util_list_t *target, void *content) {
  struct list_elem *new_element;

  NULLCHECK(target)

  new_element = SNetMemAlloc(sizeof(struct list_elem));
  new_element->content = content;
  new_element->next = NULL;

  if(target->last == NULL) {
    SNetUtilDebugNotice("AddEnd: empty list");
    /* empty list */
    target->first = new_element;
    target->last = new_element;
    new_element->prev = NULL;
  } else {
    SNetUtilDebugNotice("AddEnd: at least one element");
    /* list with at least one element */
    new_element->prev = target->last;
    target->last->next = new_element;
    target->last = new_element;
  }
  return target;
}

/** <!--********************************************************************-->
 *
 * @fn bool SNetUtilListIsEmpty(snet_util_list_t *target)
 *
 * @brief returns if the list is empty
 *
 * @param target the list to inspect
 *
 * @return true if the list is empty, false otherwise
 *
 *****************************************************************************/
bool SNetUtilListIsEmpty(snet_util_list_t *target) {
    NULLCHECK(target)
    return (target->first == NULL);
}

/** <!--********************************************************************-->
 *
 * @fn int SNetUtilListCount(snet_util_list_t *target)
 *
 * @brief returns the amount of elements in the list
 *    The elements in the list will be counted. If the list is NULL, a fatal
 *    error will be signalled
 *
 * @param target the list 
 *
 * @return the number of elements
 *
 ******************************************************************************/
int SNetUtilListCount(snet_util_list_t *target) {
  struct list_iterator *current_position;
  int counter;

  NULLCHECK(target)

  counter = 0;
  current_position = SNetUtilListFirst(target);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    counter += 1;
    current_position = SNetUtilListIterNext(current_position);
  }
  SNetUtilListIterDestroy(current_position);
  return counter;
}

/**<!--*********************************************************************-->
 *
 * @fn void SNetUtilListDeleteFirst(snet_util_list_t *target)
 *
 * @brief deletes the first element of a list
 *
 *    Delets the first element of the list. If the list is NULL or empty, a 
 *    fatal error is signaled. If the current pointer pointed to the first 
 *    element, it will be undefined after this.
 *
 * @param target the list to manipulate
 *
 *****************************************************************************/
snet_util_list_t *SNetUtilListDeleteFirst(snet_util_list_t *target) {
  struct list_elem *to_delete;

  NULLCHECK(target)
  if(SNetUtilListIsEmpty(target)) {
    SNetUtilDebugFatal("SNetUtilListDeleteFirst: list is empty already!");
  }

  if(target->current == target->first) {
    /* singleton list */
    to_delete = target->first;
    target->first = NULL;
    target->last = NULL;
  } else {
    /* list with at least 2 elements */
    to_delete = target->first;
    target->first = to_delete->next;
    to_delete->next->prev = NULL;
  }
  SNetMemFree(to_delete);
  return target;
}

/**<!--*********************************************************************-->
 *
 * @fn void SNetUtilListGetFirst(snet_util_list_t *target)
 *
 * @brief returns the first element of the list 
 *
 *    This returns the first element of the list. If the list is NULL or
 *    undefined, a fatal error is signaled.
 *
 * @param target the list to inspect
 *
 *****************************************************************************/
void *SNetUtilListGetFirst(snet_util_list_t *target) {
  NULLCHECK(target)
  if(SNetUtilListIsEmpty(target)) {
    SNetUtilDebugFatal("SNetUtilListDeleteFirst: list is empty already!");
  }

  return target->first->content;
}


/* iterator functions */

/* @fn snet_util_list_iter_t SNetUtilListIterAddAfter(snet_util_list_iter_t *target, void *content)
 * @brief adds the given content after the current element of the iterator.
 * @param target iterator with defined current element, not NULL
 * @param content the content to add
 * @return a pointer of the base list of the iterator
 */
snet_util_list_t *
SNetUtilListIterAddAfter(snet_util_list_iter_t *target_iter, void *content) {
  struct list_elem *succ;
  struct list_elem *pred;
  struct list_elem *current_element;
  struct list_elem *new_elem;
  struct list *base_list;


  NULLCHECK(target_iter)
  NULLCHECK(target_iter->base)

  base_list = target_iter->base;
  current_element = target_iter->current;

  new_elem = SNetMemAlloc(sizeof(struct list_elem));
  new_elem->content = content;

  if(base_list->first == NULL && base_list->last == NULL) {
    /* list is empty */
    base_list->first = new_elem;
    base_list->last = new_elem;
    target_iter->current = new_elem;
    new_elem->next = NULL;
    new_elem->prev = NULL;
  } else {
    if(current_element->next == NULL) {
      /* adding to end of list */
      base_list->last = new_elem;
      Link(current_element, new_elem);
      new_elem->next = NULL;
    } else {
      /* adding somewhere in the list */
      pred = current_element;
      succ = current_element->next;
      Link(pred, new_elem);
      Link(new_elem, succ);
    }
  }
  return base_list;
}

/* @fn snet_util_list_t *SNetUtilListIterAddBefore(snet_util_list_t *target, void *content)
 * @brief Adds the new element before the current element of the list
 * @param target the list you want to add the element to
 * @param content the new element to add
 */
snet_util_list_t *
SNetUtilListIterAddBefore(snet_util_list_iter_t *iterator, void *content)
{
  struct list_elem *new_elem;
  struct list_elem *current_element;
  struct list *base_list;

  NULLCHECK(iterator)
  NULLCHECK(iterator->base)
  NULLCHECK(iterator->current)

  base_list = iterator->base;
  current_element = iterator->current;

  new_elem = SNetMemAlloc(sizeof(struct list_elem));
  new_elem->content = content;

  if(base_list->first == NULL && base_list->last == NULL) {
    /* list is empty */
    base_list->first = new_elem;
    base_list->last = new_elem;
    iterator->current = new_elem;
    new_elem->next = NULL;
    new_elem->prev = NULL;
  } else {
    if(current_element->prev == NULL) {
      /* adding to beginning of list */
      base_list->first = current_element;
      Link(new_elem, current_element);
      new_elem->prev = NULL;
    } else {
      /* adding somewhere in the list */
      Link(current_element->prev, new_elem);
      Link(new_elem, current_element);
    }
  }
  return base_list;
}

/* @fn snet_util_list_iterator_t *SNetUtilListFirst(snet_util_list_t *target)
 * @brief creates an iterator on the first element of the list
 * @param target_iter the list iterate over
 * @return a pointer to the new iterator
 */
snet_util_list_iter_t *SNetUtilListFirst(snet_util_list_t *base) {
  struct list_iterator *result;

  NULLCHECK(base)

  result = SNetMemAlloc(sizeof(struct list_iterator));
  result->base = base;
  result->current = base->first;
  return result;
}

/* @fn snet_util_list_iterator_t *SNetUtilListLast(snet_util_list_t *target)
 * @brief creates an iterator on the last element of the list
 * @param target the list to iterate over
 * @return a pointer to the new iterator
 */
snet_util_list_iter_t *SNetUtilListLast(snet_util_list_t *base) {
  struct list_iterator *result;
  NULLCHECK(base)

  result = SNetMemAlloc(sizeof(struct list_iterator));
  result->base = base;
  result->current = base->last;
  return result;
}

/** <!--********************************************************************-->
 *
 * @fn bool SNetUtilListIterCurrentDefined(snet_util_list_iterator_t *target)
 *
 * @brief returns true if current is in some defined state, false otherwise
 *
 *      If the current element is defined, this is true, otherwise, this is
 *      false. You can use this to iterate over all elements of a list by
 *      checking this while calling Next or Prev after initializing the
 *      current element to the beginning or the end of the list.
 *      Doing something like while(hasNext()) will not execute the loop
 *      body for the last or first element of the list, as those elements
 *      have no next / previous element.
 *
 * @return true if the current element is defined
 *
 */
bool SNetUtilListIterCurrentDefined(snet_util_list_iter_t *target) {
  NULLCHECK(target)
  return (target->current != NULL);
}

/** <!--********************************************************************-->
 *
 * @fn snet_util_list_iter_t *SNetUtilListIterRotateFoward(snet_util_list_iter_t *target)
 *
 * @brief sets the current element to the next element, wraps around at the end
 *
 *    This sets the current element to the next element. if the current element
 *    would be undefined after this operation, the current element will be
 *    set to the first element of the list.
 *    Neither target, nor the current element of target can be undefined and
 *    will result in a fatal error.
 *
 * @param target the list to modify
 * @return the modified iterator
 *****************************************************************************/
snet_util_list_iter_t *SNetUtilListIterRotateForward(snet_util_list_iter_t *target)
{
  NULLCHECK(target)
  NULLCHECK(target->current)
  NULLCHECK(target->base)

  target = SNetUtilListIterNext(target);
  if(!SNetUtilListIterCurrentDefined(target)) {
    target->current = target->base->first;
  }
  return target;
}

/** <!--********************************************************************-->
 *
 * @fn snet_util_list_t *SNetUtilListIterDelete(snet_util_list_t *target)
 *
 * @brief Deletes the current element
 *
 *      This removes the current element of the list iterator 
 *      from the list. If the current element is
 *      undefined a fatal error will be signaled.
 *
 * @param the list to modify
 * @return a pointer to the base list
 ***************************************************************************/
snet_util_list_t *SNetUtilListIterDelete(snet_util_list_iter_t *target) {
  struct list_elem* pred;
  struct list_elem* succ;

  struct list* base_list;
  NULLCHECK(target)
  NULLCHECK(target->current)

  base_list = target->base;
  pred = target->current->prev;
  succ = target->current->next;
  SNetUtilListDump(base_list);

  if(pred != NULL && succ != NULL) {
    fprintf(stderr, "middle of list\n");
    /* deleting somewhere in the list */
    Link(pred, succ);
  } else if(pred == NULL && succ != NULL) {
    fprintf(stderr, "first element of list\n");
    /* deleting first element of list */
    base_list->first = succ;
    succ->prev = NULL;
  } else if(pred != NULL &&  succ == NULL) {
    fprintf(stderr, "last\n");
    /* deleting last element of list */
    base_list->last = pred;
    pred->next= NULL;
  } else { /* pred == succ == NULL */
    fprintf(stderr, "singleton\n");
    /* deleting only element of the list */
    base_list->first = NULL;
    base_list->last = NULL;
  }
  SNetMemFree(target->current);
  target->current = succ;
  SNetUtilListDump(base_list);
  return base_list;
}

/* @fn snet_util_list_t *SNetUtilListIterSet(snet_util_list_iter_t *target, void *new_content)
 * @brief sets the content of the current element to the new content
 * @param list the list to manipulate
 * @param new_content the new content for the current element.
 * @return a pointer to the base list
 */
snet_util_list_t *SNetUtilListIterSet(snet_util_list_iter_t *target, void *new_content) {
  NULLCHECK(target)
  NULLCHECK(target->current)
  target->current->content = new_content;
  return target->base;
}

/* @fn snet_util_list_iter_t *SNetUtilListIterNext(snet_util_list_iter_t *target)
 * @brief makes the next element the current element
 * @param target the list iterator to modify
 * @return a pointer to the modified iterator
 */
snet_util_list_iter_t *SNetUtilListIterNext(snet_util_list_iter_t *target) {
  NULLCHECK(target)
  NULLCHECK(target->current)

  target->current = target->current->next;
  return target;
}

/* @fn snet_util_list_iter_t *SNetUtilListIterPrev(snet_util_list_iter_t *target)
 * @brief makes the previous element the current element
 * @param target the list to modify
 * @return a pointer to the modified list iterator
 */
snet_util_list_iter_t *SNetUtilListIterPrev(snet_util_list_iter_t *target) {
  NULLCHECK(target)
  NULLCHECK(target->current)

  target->current = target->current->prev;
  return target;
}

/* @fn void* SNetUtilListIterGet(snet_util_list_iter_t *target)
 * @brief returns the current value.
 * @param target the list iterator to inspect
 * @return the value of the current element.
 */
void* SNetUtilListIterGet(snet_util_list_iter_t *target) {
  NULLCHECK(target)
  NULLCHECK(target->current)
  return target->current->content;
}

/* @fn bool SNetUtilListIterHasNext(snet_util_list_iter_t target)
 * @brief checks if a next element exists.
 * @param target the list iterator to inspect
 * @return true if there is some next element, false otherwise
 */
bool SNetUtilListIterHasNext(snet_util_list_iter_t *target) {
  NULLCHECK(target)
  NULLCHECK(target->current)

  return target->current->next != NULL;
}

/* @fn bool SNetUtilListIterHasPrev(snet_util_list_iter_t *target)
 * @brief returns if a previous element exists
 * @param target the list iterator to inspect
 * @return true if a previous element exists, false otherwise
 */
bool SNetUtilListIterHasPrev(snet_util_list_iter_t *target) {
  NULLCHECK(target)
  NULLCHECK(target->current)
  return target->current->prev != NULL;
}

/* @fn void SNetUtilListIterDestroy(snet_util_list_iter_t *target)
 * @brief destroys the given iterator
 * @param target the iterator to destroy, null is accepted
 */
void SNetUtilListIterDestroy(snet_util_list_iter_t *target) {
  if(target != NULL) {
    SNetMemFree(target);
  }
}

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
 * of a current element that can be inspected or removed or you can add elements
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

/** <!--********************************************************************-->
 *
 * @fn void Link(struct list_elem *pred, struct list_elem*succ)
 * 
 *   @brief establishes a link so pred is the predecessor of succ.
 *
 *   This is just a little helper function that links pred and succ together.
 *   As this is a doubly linked list, we have to set next and prev-pointers
 *   so pred really is the predecessor and succ really is the successor of 
 *   pred.
 *
 * @param pred this is any non-null element of the list
 * @param succ this is any non-null element of the list
 *
 *****************************************************************************/
static void Link(struct list_elem *pred, struct list_elem *succ) {
  pred->next = succ;
  succ->prev = pred;
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
  if(result == NULL) {
    SNetUtilDebugFatal("Could not create list, MemAlloc returned 0.");
  }
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
 * @fn void SNetUtilListAddAfter(snet_util_list_t *target, void* content) 
 *
 * @brief Adds the content in a new list_element after the current element
 *
 *      This list has a current element. The new content will be added to 
 *      the list after the current element. If the list is empty, the new
 *      element will be added without the need to set the current element.
 *
 * @param target the list to add the element to
 * @param content the new content to add.
 *
 *****************************************************************************/
void SNetUtilListAddAfter(snet_util_list_t *target, void *content) {
  struct list_elem *succ;
  struct list_elem *pred;
  struct list_elem *elem;
 
  if(target == NULL) { 
    SNetUtilDebugFatal("target == null in AddAfter!"); 
  }
  elem = SNetMemAlloc(sizeof(struct list_elem));
  elem->content = content;

  if(target->first == NULL && target->last == NULL) {
    /* list is empty */
    target->first = elem;
    target->last = elem;
    target->current = elem;
    elem->next = NULL;
    elem->prev = NULL;
  } else {
    if(target->current->next == NULL) {
      /* adding to end of list */
      target->last = elem;
      Link(target->current, elem);
      elem->next = NULL;
    } else {
      /* adding somewhere in the list */
      pred = target->current;
      succ = target->current->next;
      Link(pred, elem);
      Link(elem, succ);
    }
  }
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListAddBefore(snet_util_list_t *target, void *content)
 *
 * @brief Adds the new element before the current element of the list
 *
 *      The content will be added to an element before the current element
 *      in the list. If the list was empty before, the element will be added 
 *      correctly. 
 *
 * @param target the list you want to add the element to
 * @param content the new element to add
 *
 ******************************************************************************/
void SNetUtilListAddBefore(snet_util_list_t *target, void *content) {
  struct list_elem *succ;
  struct list_elem *pred;
  struct list_elem *elem;

  if(target == NULL) {
    SNetUtilDebugFatal("target == null in AddBefore!");
  }
  elem = SNetMemAlloc(sizeof(struct list_elem));
  elem->content = content;
  if(target->first == NULL && target->last == NULL) {
    /* list is empty */
    target->first = elem;
    target->last = elem;
    target->current = elem;
    elem->next = NULL;
    elem->prev = NULL;
  } else {
    if(target->current->prev == NULL) {
      /* adding to beginning of list */
      target->first = elem;
      Link(elem, target->current);
      elem->prev = NULL;
    } else {
      /* adding somewhere in the list */
      pred = target->current->prev;
      succ = target->current;
      Link(pred, elem);
      Link(elem, succ);
    }
  }
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
void SNetUtilListAddBeginning(snet_util_list_t *target, void *content) {
  struct list_elem *temp;

  if(target == NULL) {
    SNetUtilDebugFatal("target == null in AddBeginning");
  }

  temp = target->current;
  target->current = target->first;
  SNetUtilListAddBefore(target, content);
  target->current = temp;
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
void SNetUtilListAddEnd(snet_util_list_t *target, void *content) {
  struct list_elem *temp;

  if(target == NULL) {
    SNetUtilDebugFatal("target == null in AddBeginning");
  }

  temp = target->current;
  target->current = target->last;
  SNetUtilListAddAfter(target, content);
  target->current = temp;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListGotoBeginning(snet_util_list_t *target)
 *
 * @brief sets the current element to the first element in the list
 *
 *      This sets the current element to the beginning of the list. If the
 *      list is empty, current will be undefined.
 *
 * @param target the list to modify
 *
 *****************************************************************************/
void SNetUtilListGotoBeginning(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == null in GotoBeginning!");
  }

  target->current = target->first;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilLIstGotoEnd(snet_util_list_t *target)
 *
 * @brief moves the current pointer to the end of the list
 *
 *      This moves the current element to the last element of the list. If the
 *      list is empty, the current element will be undefined afterwards.
 *
 * @param target the list to modify
 *
 *****************************************************************************/
void SNetUtilListGotoEnd(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == null in GotoEnd!");
  }

  target->current = target->last;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListNext(snet_util_list_t *target)
 *
 * @brief makes the next element the current element 
 *
 *        This function sets the current element to the next element of the old
 *        current element. If the current element is the last element in 
 *        the list, the current element goes in some undefined state. Further
 *        calls to SNeTUtilListNext will keep the current element in this 
 *        undefined state. It is not safe to call AddBefore or similar functions
 *        while we are in this undefined state. 
 *
 * @param target the list to modify
 *
 ******************************************************************************/
void SNetUtilListNext(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == null in ListNext!");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("target->current == null in ListNext!");
  }

  if(target->current != NULL) {
    target->current = target->current->next;
  }
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListPrev(snet_util_list *target)
 *
 * @brief makes the previous element the current element
 * 
 *      This function sets the current element to the previous element of the
 *      old current element. If the current element is the first element of the
 *      list, the current element goes into some undefined state. Further calls
 *      to Prev with the current element in the undefined state keeps the 
 *      current element in the undefined state. It is not safe to call functions
 *      like AddAfter or similar functions with the current element in the 
 *      undefined state.
 *
 * @param target the list to modify
 *
 *****************************************************************************/
void SNetUtilListPrev(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == null in ListPrev!");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("target->current == null in ListPrev");
  }

  if(target->current != NULL) {
    target->current = target->current->prev;
  }
}

/** <!--********************************************************************-->
 *
 * @fn void* SNetUtilListGet(snet_util_list_t target)
 *
 * @brief returns the current value.
 *
 *      This function returns the value of the element current points to. If 
 *      the list is empty and current is thus undefined, a fatal error will 
 *      be signaled.
 *
 * @param target the list to inspect
 *
 * @return the value of the current element.
 *
 *****************************************************************************/
void* SNetUtilListGet(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == null in ListGet");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("target->current == null in listGet");
  }

  return target->current->content;
}

/** <!--********************************************************************-->
 *
 * @fn bool SNetUtilHasNext(snet_util_list_t target)
 *
 * @brief checks if a next element exists.
 *
 * @param target the list to inspect
 *
 * @return true if there is some next element, false otherwise
 *
 *****************************************************************************/
bool SNetUtilListHasNext(snet_util_list_t *target) {
  if(target == NULL) { 
    SNetUtilDebugFatal("target == null in HasNext");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("target->current == NULL!");
  }

  return target->current->next != NULL;
}

/** <!--********************************************************************-->
 *
 * @fn bool SNetUtilListHasPrev(snet_util_list_t *target)
 *
 * @brief returns if a previous element exists
 *
 * @param target the list to inspect
 *
 * @return true if a previous element exists, false otherwise
 *
 *****************************************************************************/
bool SNetUtilListHasPrev(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == null in HasPrev");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("target->current == null in HasPrev!");
  }

  return target->current->prev != NULL;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListSet(snet_util_list_t *target, void *new_content)
 *
 * @brief sets the content of the current element to the new content
 *
 *    If the current element is defined, this sets it's content to the new
 *    content. Otherwise, some fatal error will be signalled.
 *
 * @param list the list to manipulate
 * @param new_content the new content for the current element.
 ******************************************************************************/
void SNetUtilListSet(snet_util_list_t *target, void *new_content) {
  if(target == NULL) {
    SNetUtilDebugFatal("target == NULL in SNetUtilListSet!");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("current element of target list is undefined in"
                       " SNetUtilListSet!");
  }
  target->current->content = new_content;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListDelete(snet_util_list_t *target)
 *
 * @brief Deletes the current element
 *
 *      This removes the current element from the list. If the list is empty - 
 *      and thus the current element undefined - a fatal error will be 
 *      signaled.
 *
 * @param the list to modify
 *
 ***************************************************************************/
void SNetUtilListDelete(snet_util_list_t *target) {
  struct list_elem* pred;
  struct list_elem* succ;

  if(target == NULL) {
    SNetUtilDebugFatal("target == null in SNetUtilListDelete!");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("target->current == null in SNetUtilListDelete!");
  }
  pred = target->current->prev;
  succ = target->current->next;
  if(pred != NULL && succ != NULL) {
    Link(pred, succ);
  } else if(pred == NULL && succ != NULL) {
    // deleting first element of list
    target->first = succ;
    succ->prev = NULL;
  } else if(pred != NULL &&  succ == NULL) {
    // deleting last element of list
    target->last = pred;
    pred->next= NULL;
  } else { //pred == succ == NULL
    // deleting only element of the list
    target->first = NULL;
    target->last = NULL;
  }
  SNetMemFree(target->current);
  target->current = succ;
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
    return (target->first == NULL);
}

/** <!--********************************************************************-->
 *
 * @fn bool SNetUtilListCurrentDefined(snet_util_list *target)
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
bool SNetUtilListCurrentDefined(snet_util_list_t *target) {
    return (target->current != NULL);
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
  struct list_elem *temp;
  int counter;

  if(target == NULL) {
    SNetUtilDebugFatal("SnetUtilListCount: target == NULL!");
  }

  temp = target->current;

  counter = 0;
  for(SNetUtilListGotoBeginning(target);
      SNetUtilListCurrentDefined(target);
      SNetUtilListNext(target)) {
    counter += 1;
  }
  target->current = temp;
  return counter;
}

/** <!--********************************************************************-->
 *
 * @fn void SNetUtilListRotateFoward(snet_util_list_t *target)
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
 *
 *****************************************************************************/
void SNetUtilListRotateForward(snet_util_list_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilListRotateForward: target == NULL");
  }
  if(target->current == NULL) {
    SNetUtilDebugFatal("SnetUtilListRotateForward: current element undefined");
  }

  SNetUtilListNext(target);
  if(!SNetUtilListCurrentDefined(target)) {
    SNetUtilListGotoBeginning(target);
  }
}

#include <stdlib.h>

#include "stack.h"
#include "list.h"
#include "debug.h"
#include "memfun.h"

#define NULLCHECK(STACK,VAR) if((VAR) == NULL) {\
                        SNetUtilDebugFatal("%s on %x: %s == null",\
                        __FUNCTION__, (unsigned int) (STACK), #VAR);}

/**
 *
 * @file stack.c
 *
 * Implememnts a stack with the standard functions and some deep inspection.
 *
 *    (Remark: In this description, I will skip the SNetUtilStack-Prefix of the
 * functions as it is clear that this prefix is necessary.)
 *    This module implements a stack. This stack supports the usual functions 
 * a stack must provide - IsEmpty, Push, Pop, Peek - with their regular 
 * behaviour (In this stack implementation, pop removes and discards the
 * topmost element instead of removing it. This was chosen as the current 
 * major usage, the iteration counters, mostly peek at the topmost element
 * without removing it. On the other hand, the topmost element is removed 
 * rarely - if a record leaves a feedback star).
 *    Furthermore, this stack provides some deep inspection methods. 
 * These methods allow to look at every element in the stack using some 
 * simple iterator interface, that is, GotoTop, GotoBottom, Up, Down and Get. 
 * This is necessary, because we need to map a certain stack to the state of
 * a syncrocell in the feedback star. Of course, it would be possible to 
 * implement this deep inspection with two stacks and the basic methods of
 * the stack, but I consider that really cumbersome.
 *
 *****************************************************************************/
/* Single linked list structure.
 * We cannot use the UtilList here, because if we did, we had
 * to keep the counting integers on the heap. This way, we can
 * store the ints directly.
 */
struct stack_elem {
  int data;
  struct stack_elem *next;
  struct stack_elem *prev;
};

/* invariants:
 * If foo is a stack, and foo->top is NULL then foo->bottom
 * must be NULL as well.
 * If foo is a stack and foo->bottom is NULL then foo->top
 * must be NULL as well.
 * (in those two cases, the stack is empty)
 *
 * If foo is a stack, and foo->top == foo->bottom, then
 * foo->top->next must be null. In this case, the stack contains
 * exactly one element.
 */
struct stack {
  struct stack_elem *top;

  /* We need a pointer to the last element of the stack because
   * within the SNetUtilTree, we insert stacks with the bottom
   * of the stack being the first element to be considered (and
   * we do not want to walk all through the stack all the time
   * in order to get to the end;
   */
  struct stack_elem *bottom;
};

enum iterator_direction { UP, DOWN };

struct stack_iterator {
  struct stack *base; /* base provides the elements */

  /* if we start at the bottom, we want to iterate towards the top of stack,
   * if we start at the tos, we want to iterate towards the bottom of the stack.
   * this is stored here.
   */
  enum iterator_direction direction;

  /* stores the current stack element. if this is NULL, the iterator is
   * considered finished, that is, it walked through the entire stack
   */
  struct stack_elem *current;
};
/**
 *
 * @name Stack Management
 *
 * <!--
 * snet_util_stack_t *SNetUtilStackCreate() : creates a new stack
 * void SNetUtilStackDestroy(snet_util_stack *target) : destroys a given stack
 * -->
 *
 * This functions create and destroy stacks
 *
 *****************************************************************************/
/*@{*/

/**<!--*********************************************************************-->
 *
 * @fn snet_util_stack_t *SNetUtilStackCreate()
 *
 * @brief creates a new stack
 *
 * @return an empty stack.
 *
 *****************************************************************************/
snet_util_stack_t *SNetUtilStackCreate() {
  snet_util_stack_t *result;

  result = SNetMemAlloc(sizeof(snet_util_stack_t));
  result->top = NULL;
  result->bottom = NULL;
  return result;
}

/**<!--*********************************************************************-->
 *
 * @fn void SnetUtilStackDestroy(snet_util_stack_t *target)
 *
 * @brief this destroys the stack and frees all memory. content is not touched
 *
 *    This frees the given stack. If the stack is non-empty, contained integers
 *    will be discarded silently.
 *    If the target is NULL, a fatal error will be signaled.
 *
 * @param target the stack to kill
 *
 *****************************************************************************/
void SNetUtilStackDestroy(snet_util_stack_t *target) {
  struct stack_elem *current;
  struct stack_elem *next;

  NULLCHECK(target, target)

  /* free all elements of the stack*/
  if(target->top == NULL) {
    /* nothing to do, stack is empty */
  } else if(target->top == target->bottom) {
    /* singleton stack */
    SNetMemFree(target->top);
  } else {
    /* multi element stack */
    current = target->top;
    while(current != NULL) {
      next = current->next;
      SNetMemFree(current);
      current = next;
    }
  }
  SNetMemFree(target);
}

/*@}*/


/**
 * @name Standard functions
 *
 * <!--
 * bool SNetUtilStackIsEmpty(snet_util_stack_t target) : returns true
 *    if the stack is empty
 * void SNetUtilStackPush(snet_util_stack_t *target, void *content) :
 *    pushes the new element on the stack
 * void SNetUtilStackPop(snet_util_stack_t *target, void *content) :
 *    removes the topmost element from the stack
 * void *SNetUtilStackPeek(snet_util_stack_t *target) :
 *    returns the content of the topmost element of the stack and keeps
 *    the element
 * snet_util_stack_t*SNetUtilStackSet(snet_util_stack_t *target, int new_value):
 *    sets the top of stack to the given element
 * -->
 *
 *  This implements the standard stack functions, that is, push, pop, 
 *  isEmpty and peek. Peek just inspects the topmost element of the stack,
 *  in order to retrieve its content and remove it, you have to call
 *  peek and pop in this order. 
 *
 *****************************************************************************/
/*@{*/

/**<!--*********************************************************************-->
 *
 * @fn bool SNetUtilStackIsEmpty(snet_util_stack_t *target)
 *
 * @brief returns true if the stack is empty, false otherwise
 *
 *    This checks if the stack is empty. if the given stack is NULL, some fatal
 *    error is signaled.
 *
 * @param target the stack to examine
 *
 * @return true or false
 *****************************************************************************/
bool SNetUtilStackIsEmpty(snet_util_stack_t *target) {
  NULLCHECK(target, target)
  return (target->top == NULL);
}

/**<!--*********************************************************************-->
 *
 * @fn snet_util_stack_t *SNetUtilStackPush(snet_util_stack_t *target,
 *                                                       int content)
 *
 * @brief pushes the element on the given stack
 *
 *    This pushes the given content onto the stack. If the stack is NULL,
 *    some fatal error is signaled.
 *
 * @param target the stack to push to
 * @param content the element to push
 * @return a pointer to the stack
 *
 *****************************************************************************/
snet_util_stack_t *SNetUtilStackPush(snet_util_stack_t *target, int content) {
  struct stack_elem *new_elem;

  NULLCHECK(target,target)

  new_elem = SNetMemAlloc(sizeof(struct stack_elem));
  new_elem->data = content;

  if(target->top == NULL) {
    /* empty stack */
    new_elem->next = NULL;
    new_elem->prev = NULL;
    target->top = new_elem;
    target->bottom = new_elem;
  } else {
    /* singleton stack and multi element stack handled equally*/
    new_elem->next = target->top;
    target->top->prev = new_elem;
    target->top = new_elem;
  }
  return target;
}

/**<!--*********************************************************************-->
 *
 * @fn snet_util_stack_t *SNetUtilStackPop(snet_util_stack_t *target)
 *
 * @brief removes the topmost element of the stack
 *
 *    This removes the topmost element of the stack and discards its content.
 *    If the given stack is NULL or empty an error is signalled
 *
 * @param target the stack to modify
 * @return a pointer to the stack
 *
 *****************************************************************************/
snet_util_stack_t *SNetUtilStackPop(snet_util_stack_t *target) {
  struct stack_elem *to_delete;

  NULLCHECK(target, target)
  if(SNetUtilStackIsEmpty(target)) {
    SNetUtilDebugFatal("SNetUtilStackPop: target is empty!");
  }

  /* delete element */
  /* tos-> top != NULL and tos->bottom != NULL */
  to_delete = target->top;
  if(target->top == target->bottom) {
    /* singleton stack */
    target->top = NULL;
    target->bottom = NULL;
  } else {
    /* multi element stack */
    target->top = to_delete->next;
    to_delete->next->prev = NULL;
  }
  SNetMemFree(to_delete);
  return target;
}

/**<!--*********************************************************************-->
 *
 * @fn void *SNetUtilStackPeek(snet_util_stack_t *target)
 *
 * @brief returns the content of the topmost element on the stack. if the stack
 *      is NULL or empty, signals some fatal error.
 *
 * @param target the stack to inspect
 *
 * @return the topmost element of the stack.
 *
 *****************************************************************************/
int SNetUtilStackPeek(snet_util_stack_t *target) {
  NULLCHECK(target, target)
  if(SNetUtilStackIsEmpty(target)) {
    SNetUtilDebugFatal("SnetUtilStackPeek(%x): empty stack",
      (unsigned int) target);
  }
  return target->top->data;
}

/*
 * @fn snet_util_stack_t *SNetUtilStackSet(snet_util_stack_t *target,
 *                                                       int new_value
 * @brief sets the top of stack to the new value
 * @param target the stack to manipulate
 * @param new_value the new value to set to
 * @return the modified stack
 */
snet_util_stack_t *SNetUtilStackSet(snet_util_stack_t *target, int new_value) {
  NULLCHECK(target, target)
  NULLCHECK(target, target->top)

  target->top->data = new_value;
  return target;
}
/*@}*/

/**
 * @name Iterator functions
 *
 * This implements functions that allow the inspection of all elements of the
 * stack
 */
/*@{*/

/* @fn void SNetUtilStackIterDestroy(snet_util_stack_iter_t *target)
 * @brief destroys the iterator
 * @param target the iterator to kill
 */
void SNetUtilStackIterDestroy(snet_util_stack_iterator_t *target) {
  NULLCHECK(target, target)
  SNetMemFree(target);
}
/**
 * @fn snet_util_stack_iterator_t *SNetUtilStackTop(snet_util_stack_t *target)
 *
 * @brief Initializes the iterator to_init so points to the top of target and
 *        will iterate towards the bottom of target
 *
 * @param target the stack to iterate over - NULL will cause a fatal error
 * @param to_init the iterator to initialize - NULL will cause a fatal error
 * @return the intialized iterator
 */
snet_util_stack_iterator_t *SNetUtilStackTop(snet_util_stack_t *target) {
  struct stack_iterator *result;
  NULLCHECK(target, target)

  result = SNetMemAlloc(sizeof(struct stack_iterator));
  result->base = target;
  result->direction = DOWN;
  result->current = target->top;

  return result;
}

/**
 * @fn snet_util_stack_iterator_t *
 *    SNetUtilStackBottom(snet_util_stack_t *target)
 *
 * @brief initializes the given iterator to_init so it points
 *    to the lowest element of the stack and iterates upwards
 *
 * @param target the stack to iterate over, NULL triggers a fatal error
 * @param to_init the iterator to intialize. NULL triggers a fatal error
 * @return returns the initialized iterator
 */
snet_util_stack_iterator_t *SNetUtilStackBottom(snet_util_stack_t *target) {
  struct stack_iterator *result;
  NULLCHECK(target, target)

  result = SNetMemAlloc(sizeof(struct stack_iterator));
  result->base = target;
  result->direction = UP;
  result->current = target->bottom;
  return result;
}

/**
 * @fn snet_util_stack_iterator_t *
 *    SNetUtilStackIterNext(snet_util_stack_iterator_t *target)
 * @brief sets the current element to the next element
 * @param target the iterator to manipulate
 * @return the manipulated iterator
 */
snet_util_stack_iterator_t *
    SNetUtilStackIterNext(snet_util_stack_iterator_t *target) {
  NULLCHECK(target, target)

  if(target->current != NULL) {
    switch(target->direction) {
      case UP:
        target->current = target->current->next;
      break;

      case DOWN:
        target->current = target->current->prev;
      break;
    }
  }
  return target;
}

/**
 * @fn bool SNetUtilStackIterCurrDefined(snet_util_stack_iterator_t *target)
 * @brief returns if the current element of the iterator is defined
 * @param target the iterator to inspect
 * @return true if the current element is defined, false otherwise
 */
bool SNetUtilStackIterCurrDefined(snet_util_stack_iterator_t *target) {
  NULLCHECK(target, target)

  return target->current != NULL;
}
/**
 * @fn int SNetUtilStackIterGet(snet_util_stack_iterator_t *target)
 * @brief return the data in the current element. signals a fatal error if
 *        it's undefined
 * @param target the iterator to inspect, NULL or current undefined results in
 *        a fatal error
 * @return the data of he current element
 */
int SNetUtilStackIterGet(snet_util_stack_iterator_t *target) {
  NULLCHECK(target, target)
  NULLCHECK(target, target->current)

  return target->current->data;
}
/*@}*/

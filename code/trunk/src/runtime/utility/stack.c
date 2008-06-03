#include <stdlib.h>

#include "stack.h"
#include "list.h"
#include "debug.h"
#include "memfun.h"

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
struct stack {
  snet_util_list_t *data;
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

  return result;
}

/**<!--*********************************************************************-->
 *
 * @fn void SnetUtilStackDestroy(snet_util_stack_t *target)
 *
 * @brief this destroys the stack and frees all memory. content is not touched
 *
 *    This frees the given stack. If the stack is non-empty, elements will
 *    be discarded silently. The contents will not be touched. 
 *    If the target is NULL, a fatal error will be signaled. 
 *
 * @param target the stack to kill
 *
 *****************************************************************************/
void SNetUtilStackDestroy(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilStackDestroy: target==NULL");
  }

  SNetUtilListDestroy(target->data);
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
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilStackIsEmpty: target == NULL");
  }
  return SNetUtilListIsEmpty(target->data);
}

/**<!--*********************************************************************-->
 *
 * @fn void SNetUtilStackPush(snet_util_stack_t *target, void *content)
 *
 * @brief pushes the element on the given stack
 *
 *    This pushes the given content onto the stack. If the stack is NULL,
 *    some fatal error is signaled. 
 *
 * @param target the stack to push to
 * @param content the element to push
 *
 *****************************************************************************/
void SNetUtilStackPush(snet_util_stack_t *target, void *content) {
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilStackPush: target == NULL");
  }

  SNetUtilListAddBeginning(target->data, content);
}

/**<!--*********************************************************************-->
 *
 * @fn void SNetUtilStackPop(snet_util_stack_t *target)
 *
 * @brief removes the topmost element of the stack
 *
 *    This removes the topmost element of the stack and discards its content. 
 *    If the given stack is NULL or empty an error is signalled
 *
 * @param target the stack to modify
 *
 *****************************************************************************/
void SNetUtilStackPop(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilStackPop: target == null");
  }
  if(SNetUtilStackIsEmpty(target)) {
    SNetUtilDebugFatal("SNetUtilStackPop: target is empty!");
  }
  SNetUtilListDeleteFirst(target->data);
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
void *SNetUtilStackPeek(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SnetUtilStackPeek: target == NULL");
  }
  if(SNetUtilStackIsEmpty(target)) {
    SNetUtilDebugFatal("SnetUtilStackPeek: empty stack");
  }
  return SNetUtilListGetFirst(target->data);
}
/*@}*/

/**
 *
 * @name Deep Inspection Functions
 *
 * <!--
 * void SNetUtilStackGotoTop(snet_util_stack_t *target) : makes the topmost
 *    element of the stack the current element for inspection.
 * void SNetUtilStackGotoBottom(snet_util_stack_t *target) : makes the deepest
 *    element in the stack the current element for inspection
 * void SNetUtilStackUp(snet_util_stack_t *target) : Makes the element above
 *    the old current element the new current element.
 * void SNetUtilStackDown(snet_util_stack_t *target) : Makes the element below
 *    the old current element the new current element.
 * void *SNetUtilStackGet(snet_util_stack_t *target) : Returns the value of
 *    the current stack element.
 * bool SNetUtilStackCurrentDefined(snet_util_stack_t *target) : Returns true
 *    if the current element is defined, false otherwise. 
 */
/*@{*/
/**<!--*********************************************************************-->
 *
 * @fn void SNetUtilStackGotoTop(snet_util_stack_t *target)
 *
 * @brief makes the topmost element of the stack the current element. 
 *
 *    This makes the topmost element of the stack the currently inspected 
 *    element regardless of the current elements state. 
 *    If the given stack is NULL, a fatal error is signaled.
 *
 * @param target the stack to manipulate, must not be NULL
 *
 *****************************************************************************/
void SNetUtilStackGotoTop(snet_util_stack_t *target) {
  if(target == NULL) { 
    SNetUtilDebugFatal("SNetUtilStackGotoTop: target == NULL");
  }
  SNetUtilListGotoBeginning(target->data);
}

/**<!--**********************************************************************-->
 *
 * @fn SneTUtilStackGotoBottom(snet_util_stack *target)
 *
 * @brief makes the deepest element in the stack the current element.
 *
 *    This makes the deepest element of the stack the current element, 
 *    regardless of the current elements state. 
 *    target must not be null, a fatal error will be signaled otherwise.
 *
 * @param target the stack to manipulate
 *
 ******************************************************************************/
void SnetUtilStackGotoBottom(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SnetUtilStackGotoBottom: target == NULL");
  }
  SNetUtilListGotoEnd(target->data);
}

/**<!--**********************************************************************-->
 *
 * @fn void SnetUtilStackUp(snet_util_stack_t *target)
 *
 * @brief makes the element above the old current element the current element
 *
 *         This makes the element above the old current element the new current 
 *    element. If the current element is the topmost element in the stack, the
 *    current element will be undefined after this operation. If the current 
 *    element was undefined already, it will stay undefined. (Thus, GotoTop(s);
 *    Up(s) implies CurrentDefined(s) == False)
 *         Furthermore, the target stack must not be NULL, a fatal error will
 *    be signaled otherweise.
 *
 * @param target the stack to manipulate, must not be null.
 *
 ******************************************************************************/
void SNetUtilStackUp(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilStackUp: target == NULL");
  }
  SNetUtilListPrev(target->data);
}

/**<!--**********************************************************************-->
 *
 * @fn void SNetUtilStackDown(snet_util_stack_t *target)
 *
 * @brief makes the element below the old current element the current element
 *
 *      This makes the element below the old current element the new current
 * element. If the current element is the lowest element on the stack, the
 * new current element will be undefined. If the current element was undefined
 * before, the current element will be undefined. (Thus, GotoBottom(s); Down(s)
 * implies CurrentDefined(s) == False.
 *      Furthermore, the target stack must not be NULL. If it is NULL, a fatal
 * error is signaled.
 *
 * @param target the stack to manipulate. Must not be NULL.
 *
 ******************************************************************************/
void SNetUtilStackDown(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SnetUtilStackDown: target == NULL");
  }
  SNetUtilListNext(target->data);
}

/**<!--**********************************************************************-->
 *
 * @fn void *SNetUtilStackGet(snet_util_stack_t *target) 
 *
 * @brief Returns the content of the current element. 
 *
 *    This returns the content of the current element. The target stack must 
 * not be NULL or empty and the current element must be defined, otherwise, 
 * according fatal errors will be signaled.
 *    
 * @param target the stack to inspect
 *
 * @return the content of the current element
 *
 ******************************************************************************/
void *SNetUtilStackGet(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SNetUtilStackGet: target == NULL!");
  }
  if(SNetUtilStackIsEmpty(target)) {
    SNetUtilDebugFatal("SNetUtilStackGet: Stack is empty!");
  }
  if(!SNetUtilStackCurrentDefined(target)) {
    SNetUtilDebugFatal("SnetUtilStackGet: current element is undefined!");
  }
  return SNetUtilListGet(target->data);
}

/**<!--**********************************************************************-->
 *
 * @fn bool SNetUtilStackCurrentDefined(snet_util_stack_t *target)
 *
 * @brief returns true if the current element is defined
 *
 *      This checks if the current element is defined. If it is, all is good. 
 * If it is undefined, it is unsafe to call Get for example. 
 *      Furthermore, the target must not be NULL, a fatal error will be 
 * signalled otherwise.      
 *
 * @param target the stack to inspect
 *
 * @return true if the current element is defined, false otherwise
 *
 ******************************************************************************/
bool SNetUtilStackCurrentDefined(snet_util_stack_t *target) {
  if(target == NULL) {
    SNetUtilDebugFatal("SnetUtilStackCurrentDefined: target == NULL");
  }
  return SNetUtilListCurrentDefined(target->data);
}

/*@}*/

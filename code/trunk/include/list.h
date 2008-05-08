/*
 * This provides a doubly linked list with void* context.
 */
#ifndef LIST_H

typedef struct list *SNetUtilList;

/*
 * Creates an empty list.
 */
extern SNetUtilList SNetUtilListCreate();

/*
 * Destroys a list. The argument can be any element of the
 * list.
 */
extern void SNetUtilListDestroy(SNetUtilList target);

/*
 * target->current has to point to an element of this list.
 * an element with content content will be added after this
 * element. target->current remains unchanged.
 */
extern void SNetUtilListAddAfter(SNetUtilList pred, void* content);

/*
 * adds an element before the element that target->current points to.
 * see AddAfter()
 */
extern void SNetUtilListAddBefore(SNetUtilList succ, void* content);

/*
 * adds an element to the beginning. target->current does not matter.
 */
extern void SNetUtilListAddBeginning(SNetUtilList target, void* content);

/*
 * adds an element to the end. target->current does not matter
 */

extern void SNetUtilListAddEnd(SNetUtilList target, void* content);

/*
 * sets the current element to the first element in the list.
 */
extern void SNetUtilListGotoBeginning(SNetUtilList target);

/*
 * sets the current element to the last element in the list.
 */
extern void SNetUtilListGotoEnd(SNetUtilList target);

/*
 * sets the element to the next element in list; if the current
 * element is the last element in list, nothing will happen
 */
extern void SNetUtilListNext(SNetUtilList target);

/*
 * sets the element to the previous element in list; if the current
 * element is the first element in list, nothing will happen
 */
extern void SNetUtilListPrev(SNetUtilList target);

/*
 * gets the current element from the list
 */
extern void* SNetUtilListGet(SNetUtilList target); 

/*
 * returns if a next element exists
 */
extern int SNetUtilListHasNext(SNetUtilList target); 

/*
 * returns if a previous element exists
 */
extern int SNetUtilListHasPrev(SNetUtilList target);

/*
 * deletes the element at the current position and sets
 * target->current to target->current->next (this is an arbitrary decision)
 */
extern void SNetUtilListDeleteCurrent(SNetUtilList target);

#endif

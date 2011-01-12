
/**
 * Functions for maintaining a list of stream descriptors
 */

#include <stdlib.h>
#include <assert.h>

#include "stream.h"


/**
 * An iterator for a stream descriptor list
 */
struct lpel_stream_iter_t {
  lpel_stream_desc_t *cur;
  lpel_stream_desc_t *prev;
  lpel_stream_list_t *list;
};



/**
 * Append a stream descriptor to a stream descriptor list
 *
 * @param lst   the stream descriptor list
 * @param node  the stream descriptor to be appended
 *
 * @pre   it is NOT safe to append while iterating, i.e. if an SD should
 *        be appended while the list is iterated through, StreamIterAppend()
 *        must be used.
 */
void LpelStreamListAppend( lpel_stream_list_t *lst, lpel_stream_desc_t *node)
{
  if (*lst  == NULL) {
    /* list is empty */
    *lst = node;
    node->next = node; /* selfloop */
  } else { 
    /* insert stream between last element=*lst
       and first element=(*lst)->next */
    node->next = (*lst)->next;
    (*lst)->next = node;
    *lst = node;
  }
}


/**
 * Remove a stream descriptor from a stream descriptor list
 *
 * @return 0 on success, -1 if node is not contained in the list
 * @note  O(n) operation
 */
int LpelStreamListRemove( lpel_stream_list_t *lst, lpel_stream_desc_t *node)
{
  lpel_stream_desc_t *prev, *cur;
  assert( *lst != NULL);

  prev = *lst;
  do {
    cur = prev->next;
    prev = cur;
  } while (cur != node && prev != *lst);

  if (cur != node) return -1;

  if ( prev == cur) {
    /* self-loop */
    *lst = NULL;
  } else {
    prev->next = cur->next;
    cur->next = NULL;
    /* fix list handle if necessary */
    if (*lst == cur) {
      *lst = prev;
    }
  }
  return 0;
}


/**
 * Test if a stream descriptor list is empty
 *
 * @param lst   stream descriptor list
 * @return      1 if the list is empty, 0 otherwise
 */
int LpelStreamListIsEmpty( lpel_stream_list_t *lst)
{
  return (*lst == NULL);
}


/**
 * Create a stream iterator
 * 
 * Creates a stream descriptor iterator for a given stream descriptor list,
 * and, if the stream descriptor list is not empty, initialises the iterator
 * to point to the first element such that it is ready to be used.
 * If the list is empty, it must be resetted with StreamIterReset()
 * before usage.
 *
 * @param lst   list to create an iterator from, can be NULL to only allocate
 *              the memory for the iterator
 * @return      the newly created iterator
 */
lpel_stream_iter_t *LpelStreamIterCreate( lpel_stream_list_t *lst)
{
  lpel_stream_iter_t *iter =
    (lpel_stream_iter_t *) malloc( sizeof( lpel_stream_iter_t));

  if (lst) {
    iter->prev = *lst;
    iter->list = lst;
  }
  iter->cur = NULL;
  return iter;
}


/**
 * Destroy a stream descriptor iterator
 *
 * Free the memory for the specified iterator.
 *
 * @param iter  iterator to be destroyed
 */
void LpelStreamIterDestroy( lpel_stream_iter_t *iter)
{
  free(iter);
}


/**
 * Initialises the stream list iterator to point to the first element
 * of the stream list.
 *
 * @param lst   list to be iterated through
 * @param iter  iterator to be resetted
 * @pre         The stream list is not empty, i.e. *lst != NULL
 */
void LpelStreamIterReset( lpel_stream_list_t *lst, lpel_stream_iter_t *iter)
{
  assert( lst != NULL);
  iter->prev = *lst;
  iter->list = lst;
  iter->cur = NULL;
}


/**
 * Test if there are more stream descriptors in the list to be
 * iterated through
 *
 * @param iter  the iterator
 * @return      1 if there are stream descriptors left, 0 otherwise
 */
int LpelStreamIterHasNext( lpel_stream_iter_t *iter)
{
  return (*iter->list != NULL) &&
    ( (iter->cur != *iter->list) || (iter->cur == NULL) );
}


/**
 * Get the next stream descriptor from the iterator
 *
 * @param iter  the iterator
 * @return      the next stream descriptor
 * @pre         there must be stream descriptors left for iteration,
 *              check with StreamIterHasNext()
 */
lpel_stream_desc_t *LpelStreamIterNext( lpel_stream_iter_t *iter)
{
  assert( LpelStreamIterHasNext(iter) );

  if (iter->cur != NULL) {
    /* this also does account for the state after deleting */
    iter->prev = iter->cur;
    iter->cur = iter->cur->next;
  } else {
    iter->cur = iter->prev->next;
  }
  return iter->cur;
}

/**
 * Append a stream descriptor to a list while iterating
 *
 * Appends the SD at the end of the list.
 * [Uncomment the source to insert the specified SD
 * after the current SD instead.]
 *
 * @param iter  iterator for the list currently in use
 * @param node  stream descriptor to be appended
 */
void LpelStreamIterAppend( lpel_stream_iter_t *iter,
    lpel_stream_desc_t *node)
{
#if 0
  /* insert after cur */
  node->next = iter->cur->next;
  iter->cur->next = node;

  /* if current node was last node, update list */
  if (iter->cur == *iter->list) {
    *iter->list = node;
  }

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
#else
  /* insert at end of list */
  node->next = (*iter->list)->next;
  (*iter->list)->next = node;

  /* if current node was first node */
  if (iter->prev == *iter->list) {
    iter->prev = node;
  }
  *iter->list = node;

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
#endif
}

/**
 * Remove the current stream descriptor from list while iterating through
 *
 * @param iter  the iterator
 * @pre         iter points to valid element
 *
 * @note StreamIterRemove() may only be called once after
 *       StreamIterNext(), as the current node is not a valid
 *       list node anymore. Iteration can be continued though.
 */
void LpelStreamIterRemove( lpel_stream_iter_t *iter)
{
  /* handle case if there is only a single element */
  if (iter->prev == iter->cur) {
    assert( iter->prev == *iter->list );
    iter->cur->next = NULL;
    *iter->list = NULL;
  } else {
    /* remove cur */
    iter->prev->next = iter->cur->next;
    /* cur becomes invalid */
    iter->cur->next = NULL;
    /* if the first element was deleted, clear cur */
    if (*iter->list == iter->prev) {
      iter->cur = NULL;
    } else {
      /* if the last element was deleted, correct list */
      if (*iter->list == iter->cur) {
        *iter->list = iter->prev;
      }
      iter->cur = iter->prev;
    }
  }
}



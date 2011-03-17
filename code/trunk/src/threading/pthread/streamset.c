
/**
 * Functions for maintaining a set of stream descriptors
 */

#include <stdlib.h>
#include <assert.h>

#include "threading.h"

#include "stream.h"

/*
 * n is a pointer to a stream descriptor
 */
#define NODE_NEXT(n)  (n)->next


/**
 * An iterator for a stream descriptor set
 */
struct snet_stream_iter_t {
  snet_stream_desc_t *cur;
  snet_stream_desc_t *prev;
  snet_streamset_t *set;
};



/**
 * Put a stream descriptor to a stream descriptor set
 *
 * @param set   the stream descriptor set
 * @param node  the stream descriptor to be appended
 *
 * @pre   it is NOT safe to put a descriptor to the set while iterating,
 *        use  StreamIterAppend() instead.
 */
void SNetStreamsetPut( snet_streamset_t *set, snet_stream_desc_t *node)
{
  if (*set  == NULL) {
    /* set is empty */
    *set = node;
    NODE_NEXT(node) = node; /* selfloop */
  } else { 
    /* insert stream between last element=*set
       and first element=(*set)->next */
    NODE_NEXT(node) = NODE_NEXT(*set);
    NODE_NEXT(*set) = node;
    *set = node;
  }
}


/**
 * Remove a stream descriptor from a stream descriptor set
 *
 * @return 0 on success, -1 if node is not contained in the set
 * @note  O(n) operation
 */
int SNetStreamsetRemove( snet_streamset_t *set, snet_stream_desc_t *node)
{
  snet_stream_desc_t *prev, *cur;
  assert( *set != NULL);

  prev = *set;
  do {
    cur = NODE_NEXT(prev);
    prev = cur;
  } while (cur != node && prev != *set);

  if (cur != node) return -1;

  if ( prev == cur) {
    /* self-loop */
    *set = NULL;
  } else {
    NODE_NEXT(prev) = NODE_NEXT(cur);
    NODE_NEXT(cur) = NULL;
    /* fix set handle if necessary */
    if (*set == cur) {
      *set = prev;
    }
  }
  return 0;
}


/**
 * Test if a stream descriptor set is empty
 *
 * @param set   stream descriptor set
 * @return      1 if the set is empty, 0 otherwise
 */
int SNetStreamListIsEmpty( snet_streamset_t *set)
{
  return (*set == NULL);
}


/**
 * Create a stream iterator
 * 
 * Creates a stream descriptor iterator for a given stream descriptor set,
 * and, if the stream descriptor set is not empty, initialises the iterator
 * to point to the first element such that it is ready to be used.
 * If the set is empty, it must be resetted with StreamIterReset()
 * before usage.
 *
 * @param set   set to create an iterator from, can be NULL to only allocate
 *              the memory for the iterator
 * @return      the newly created iterator
 */
snet_stream_iter_t *SNetStreamIterCreate( snet_streamset_t *set)
{
  snet_stream_iter_t *iter =
    (snet_stream_iter_t *) malloc( sizeof( snet_stream_iter_t));

  if (set) {
    iter->prev = *set;
    iter->set = set;
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
void SNetStreamIterDestroy( snet_stream_iter_t *iter)
{
  free(iter);
}


/**
 * Initialises the stream set iterator to point to the first element
 * of the stream set.
 *
 * @param iter  iterator to be resetted
 * @param set   set to be iterated through
 * @pre         The stream set is not empty, i.e. *set != NULL
 */
void SNetStreamIterReset(snet_stream_iter_t *iter, snet_streamset_t *set)
{
  assert( set != NULL);
  iter->prev = *set;
  iter->set = set;
  iter->cur = NULL;
}


/**
 * Test if there are more stream descriptors in the set to be
 * iterated through
 *
 * @param iter  the iterator
 * @return      1 if there are stream descriptors left, 0 otherwise
 */
int SNetStreamIterHasNext( snet_stream_iter_t *iter)
{
  return (*iter->set != NULL) &&
    ( (iter->cur != *iter->set) || (iter->cur == NULL) );
}


/**
 * Get the next stream descriptor from the iterator
 *
 * @param iter  the iterator
 * @return      the next stream descriptor
 * @pre         there must be stream descriptors left for iteration,
 *              check with StreamIterHasNext()
 */
snet_stream_desc_t *SNetStreamIterNext( snet_stream_iter_t *iter)
{
  assert( SNetStreamIterHasNext(iter) );

  if (iter->cur != NULL) {
    /* this also does account for the state after deleting */
    iter->prev = iter->cur;
    iter->cur = NODE_NEXT(iter->cur);
  } else {
    iter->cur = NODE_NEXT(iter->prev);
  }
  return iter->cur;
}

/**
 * Append a stream descriptor to a set while iterating
 *
 * Appends the SD at the end of the set.
 * [Uncomment the source to insert the specified SD
 * after the current SD instead.]
 *
 * @param iter  iterator for the set currently in use
 * @param node  stream descriptor to be appended
 */
void SNetStreamIterAppend( snet_stream_iter_t *iter,
    snet_stream_desc_t *node)
{
#if 0
  /* insert after cur */
  NODE_NEXT(node) = NODE_NEXT(iter->cur);
  NODE_NEXT(iter->cur) = node;

  /* if current node was last node, update set */
  if (iter->cur == *iter->set) {
    *iter->set = node;
  }

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
#else
  /* insert at end of set */
  NODE_NEXT(node) = NODE_NEXT(*iter->set);
  NODE_NEXT(*iter->set) = node;

  /* if current node was first node */
  if (iter->prev == *iter->set) {
    iter->prev = node;
  }
  *iter->set = node;

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
#endif
}

/**
 * Remove the current stream descriptor from set while iterating through
 *
 * @param iter  the iterator
 * @pre         iter points to valid element
 *
 * @note StreamIterRemove() may only be called once after
 *       StreamIterNext(), as the current node is not a valid
 *       set node anymore. Iteration can be continued though.
 */
void SNetStreamIterRemove( snet_stream_iter_t *iter)
{
  /* handle case if there is only a single element */
  if (iter->prev == iter->cur) {
    assert( iter->prev == *iter->set );
    NODE_NEXT(iter->cur) = NULL;
    *iter->set = NULL;
  } else {
    /* remove cur */
    NODE_NEXT(iter->prev) = NODE_NEXT(iter->cur);
    /* cur becomes invalid */
    NODE_NEXT(iter->cur) = NULL;
    /* if the first element was deleted, clear cur */
    if (*iter->set == iter->prev) {
      iter->cur = NULL;
    } else {
      /* if the last element was deleted, correct set */
      if (*iter->set == iter->cur) {
        *iter->set = iter->prev;
      }
      iter->cur = iter->prev;
    }
  }
}



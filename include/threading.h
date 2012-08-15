#ifndef _THREADING_H_
#define _THREADING_H_

#include "distribution.h" //XXX dependency?
#include "locvec.h"
#include "moninfo.h"

/******************************************************************************
 *
 * Threading Backends for the Multithreaded S-Net Runtime System
 *
 * This document describes the interface for the threading backend of the
 * multithreaded S-Net runtime system. It includes:
 *
 * (1) general initialization and shutdown,
 * (2) supporting calls to signal events,
 * (3) entity (task) creation, and
 * (4) stream operations and management
 *
 *
 * Author: Daniel Prokesch <dlp@snet-home.org>
 * Author: Stefan Kok
 * Date:   15/03/2011
 *
 *****************************************************************************/

typedef enum {
  ENTITY_box,
  ENTITY_parallel,
  ENTITY_star,
  ENTITY_split,
  ENTITY_fbcoll,
  ENTITY_fbdisp,
  ENTITY_fbbuf,
  ENTITY_sync,
  ENTITY_filter,
  ENTITY_nameshift,
  ENTITY_collect,
  ENTITY_other
} snet_entity_t;

typedef void (*snet_taskfun_t)(void*);

/*****************************************************************************
 * (1) Initialization and Shutdown
 ****************************************************************************/


/**
 * Initialize the threading backend
 *
 * @return 0 on success
 */
int SNetThreadingInit(int argc, char **argv);


const char *SNetThreadingGetName();


/**
 * Stop the threading backend.
 *
 * This function blocks the calling (main) thread until the entity threads
 * have finished and it is safe to shutdown.
 *
 * @return 0 on success
 */
int SNetThreadingStop(void);




/**
 * Shutdown the threading backend.
 * Upon calling this function, all entity threads
 * must have terminated.
 *
 * @return 0 on success
 */
int SNetThreadingCleanup(void);


/*****************************************************************************
 * (2) Supporting calls to signal events
 ****************************************************************************/


/**
 * Signal a monitoring event
 *
 * @param ent       the entity generating the event
 * @param moninfo   the monitoring info, can be NULL
 * @post  if moninfo != NULL, moninfo is destroyed
 */
void SNetThreadingEventSignal(snet_moninfo_t *moninfo);



/*****************************************************************************
 * (3) Entity creation
 *
 * All entities are detached (not joinable).
 *
 * An entity thread has following signature:
 *
 *   void EntityThread(void *arg);
 *
 ****************************************************************************/





/**
 * Spawn a new thread
 *
 * @param ent     the entity to spawn
 *
 * @return 0 on success
 */
void SNetThreadingSpawn(snet_entity_t ent, int loc, snet_locvec_t *locvec,
                        const char *name, snet_taskfun_t f, void *arg);

void SNetThreadingRespawn(snet_taskfun_t);

/**
 * Let the current entity thread/task give up execution
 */
void SNetThreadingYield(void);





/*****************************************************************************
 * (4) Stream handling
 ****************************************************************************/

/**
 * Stream
 */
//typedef struct snet_stream_t snet_stream_t;
struct snet_stream_t;


/**
 * Stream descriptor
 */
typedef struct snet_stream_desc_t snet_stream_desc_t;



/**
 * Create a stream with a given capacity
 *
 * @param capacity  the number of items that can be
 *                  stored in the stream
 * @return a newly created stream
 */
snet_stream_t *SNetStreamCreate(int capacity);



/**
 * Set the source location vector of a stream
 *
 * @param s   the stream
 * @param lv  the location vector
 */
void SNetStreamSetSource(snet_stream_t *s, snet_locvec_t *lv);


/**
 * Get the source location vector of a stream
 *
 * @param s   the stream
 * @return  the location vector
 */
snet_locvec_t *SNetStreamGetSource(snet_stream_t *s);



/**
 * Register a callback function for the stream that is called
 * by the consumer after every read, with a specified argument.
 *
 * @param s         the stream
 * @param callback  the callback function
 * @param cbarg     the argument that is passed to callback
 *
 * @note IMPORTANT: Make sure you register the callback before you
 *       need it, i.e., before the consumer reads from it!
 */
void SNetStreamRegisterReadCallback(snet_stream_t *s,
    void (*callback)(void *), void *cbarg);


/**
 * Get the callback argument from a stream descriptor
 * @param sd    the stream descriptor
 * @return  the callabck argument registered previously
 *          with SNetStreamRegisterReadCallback
 */
void *SNetStreamGetCallbackArg(snet_stream_desc_t *sd);



/**
 * Open a stream for usage within an entity
 *
 * @param stream  the stream to open
 * @param mode    either 'r' for reading or 'w' for writing
 *
 * @return a stream descriptor for further usage of the stream
 */
snet_stream_desc_t *SNetStreamOpen(snet_stream_t *stream, char mode);


/**
 * Close a stream, and destroy it if necessary
 *
 * @param sd  the stream descriptor of the open stream
 * @param destroy_stream  if != 0, destroy the underlying
 *                        snet_stream_t
 */
void SNetStreamClose(snet_stream_desc_t *sd, int destroy_stream);




/**
 * Replace the underlying stream by another one
 * @param sd          the stream descriptor for which to replace
 *                    the underlying stream
 * @param new_stream  the new stream
 *
 * @pre sd is opened for reading
 * @post the old stream of sd is destroyed and free'd
 */
void SNetStreamReplace(snet_stream_desc_t *sd, snet_stream_t *new_stream);


/**
 * Get the stream opened from a stream descriptor
 * @param sd  stream descriptor
 * @return  the stream opened by the stream descriptor
 */
snet_stream_t *SNetStreamGet(snet_stream_desc_t *sd);


/**
 * Read an item from the stream, consuming
 *
 * @param sd  stream descriptor
 *
 * @pre sd is opened for reading
 *
 * @return the item in the buffer
 */
void *SNetStreamRead(snet_stream_desc_t *sd);


/**
 * Return the next item in the stream, non-consuming
 *
 * @pre sd is opened for reading
 *
 * @return NULL if the stream does not contain items
 */
void *SNetStreamPeek(snet_stream_desc_t *sd);





/**
 * Write an item to the stream
 *
 * @param sd  stream descriptor
 * @param item  item to write
 * @pre sd is opened for writing
 */
void SNetStreamWrite(snet_stream_desc_t *sd, void *item);


/**
 * Try to write an item to the stream, non-blocking:
 * If the buffer is full, the function returns immediately
 * indicating failure
 *
 * @param sd  stream descriptor
 * @param item  item to write
 * @pre sd is opened for writing
 *
 * @return 0 if successful, -1 if buffer is full
 */
int SNetStreamTryWrite(snet_stream_desc_t *sd, void *item);




/**
 * StreamSets
 *
 * A stream set is a handle to a stream descriptor
 *
 * Initialization is done via:
 *   snet_streamset_t myset = NULL;
 */
typedef snet_stream_desc_t *snet_streamset_t;


/**
 * Poll a set of streams.
 *
 * This function returns a stream descriptor for a stream that contains
 * available data. If no stream contains data, this function blocks
 * until data is written to one of the streams contained in the stream set.
 *
 * @param set   the set of streams to be polled
 * @pre         all streams in the set are opened for reading
 *
 * @return  a stream descriptor for a stream containing data
 */
snet_stream_desc_t *SNetStreamPoll(snet_streamset_t *set);






/**
 * Put the stream descriptor sd into the streamset set
 *
 * @param set   the streamset
 * @param sd    the stream descriptor
 *
 */
void SNetStreamsetPut(snet_streamset_t *set, snet_stream_desc_t *sd);



/**
 * Remove a stream descriptor sd from the streamset set
 *
 * @param set   the streamset
 * @param sd    the stream descriptor
 *
 * @return 0 on success, -1 if sd was not contained in set
 */
int SNetStreamsetRemove(snet_streamset_t *set, snet_stream_desc_t *sd);



/*
 * NOTE:
 *   Streamsets need not to provide a function for checking membership,
 *   as a set only contains local stream descriptors of an entity anyway.
 */



/**
 * Iterators
 *
 * It is also possible to iterate over a streamset.
 * Therefore, first an iterator has to be obtained.
 */
typedef struct snet_stream_iter_t snet_stream_iter_t;


/**
 * Create an iterator for the streamset set.
 *
 * @param set   streamset which to iterate over.
 *              Can be NULL, in this case, only the memory is allocated.
 * @return pointer to a newly allocated iterator
 */
snet_stream_iter_t *SNetStreamIterCreate(snet_streamset_t *set);


/**
 * Destroy a given iterator for a stream set
 *
 * @param iter  the iterator to destroy
 */
void SNetStreamIterDestroy(snet_stream_iter_t *iter);


/**
 * Reset an already existing iterator for a given streamset
 * This enables resuse of an iterator that has been created before.
 *
 * @param iter  the iterator to reset
 * @param set   the streamset for which the iterator is reset
 */
void SNetStreamIterReset(snet_stream_iter_t *iter, snet_streamset_t *set);



/**
 * Test if there are more stream descriptors in the streamset to be
 * iterated through
 *
 * @param iter  the iterator
 * @return      != 0 if there are stream descriptors left, 0 otherwise
 */
int SNetStreamIterHasNext(snet_stream_iter_t *iter);



/**
 * Get the next stream descriptor from the iterator
 *
 * @param iter  the iterator
 * @return      the next stream descriptor
 * @pre         there must be stream descriptors left for iteration,
 *              check with SNetStreamIterHasNext()
 */
snet_stream_desc_t *SNetStreamIterNext( snet_stream_iter_t *iter);



/**
 * Pattern for iteration (foreach s in set):
 *
 * it = IterCreate(set) or IterReset(it,set)
 * while(IterHasNext(it) {
 *   sd_t s = IterNext(it);
 *   ... do whatever you like with s ...
 * }
 *
 */




/*
 * Streamsets together with their iterators need to be desiged in a way
 * such that they allow adding and removing elements during iteration
 * as follows:
 */


/**
 * Append a stream descriptor to a streamset while iterating
 *
 * @param iter  iterator for the list currently in use
 * @param sd    stream descriptor to be appended
 */
void SNetStreamIterAppend( snet_stream_iter_t *iter,
    snet_stream_desc_t *node);


/**
 * Remove the current stream descriptor from streamset
 * while iterating through
 *
 * @param iter  the iterator
 * @pre         iter points to valid element
 *
 * @note SNetStreamIterRemove() may only be called once after
 *       SNetStreamIterNext().
 */
void SNetStreamIterRemove( snet_stream_iter_t *iter);



#endif /* _THREADING_H_ */

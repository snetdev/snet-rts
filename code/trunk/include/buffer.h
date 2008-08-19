/*
 * $Id$
 */

/*
 * buffer.h
 * This implements a buffer and its accessor functions.
 */



#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <semaphore.h>
#include "bool.h"
#include "stream_layer.h"
#include "extended_semaphore.h"
typedef enum bufmessage snet_buffer_msg_t;
typedef struct buffer  snet_buffer_t;

enum bufmessage
{
	BUF_fatal = -1,
	BUF_success,
	BUF_blocked,
	BUF_full,
	BUF_empty
};


extern bool SNetBufIsEmpty(snet_buffer_t *buf);
extern void SNetBufBlockUntilDataReady(snet_buffer_t *buf);
extern void SNetBufBlockUntilHasSpace(snet_buffer_t *buf);
extern void SNetBufClaim(snet_buffer_t *buf);
extern void SNetBufRelease(snet_buffer_t *buf);

/*
 * Allocates memory for the buffer and initializes
 * the datastructure. 
 * This function creates a buffer with 'sizes'
 * spaces for pointers to elements. 
 * RETURNS: pointer to the buffer on success or NULL
 *          otherwise. 
 */

extern snet_buffer_t *SNetBufCreate( unsigned int size, pthread_mutex_t *lock); 
// TODO: add extern to all functions




/*
 * Puts the pointer given as second parameter in the buffer.
 * This function will block until succeeded.
 * Possible reasons for blocking are:
 * - buffer is in use by another thread
 * - no space left in the buffer  
 * RETURNS: pointer to the modified buffer.
 */

extern snet_buffer_t *SNetBufPut( snet_buffer_t *buf, void* elem);




/*
 * Returns an element of the buffer. 
 * This function will block until succeeded.
 * Possible reasons for blocking are:
 * - buffer is in use by another thread
 * - no elements in the buffer
 * RETURNS: pointer to the element.  
 */

extern void *SNetBufGet( snet_buffer_t *buf	); // TODO: returns element, not buffer




/*
 * Puts the pointer given as second parameter to the buffer if possible.
 * This function will NOT block (returns immediately).
 * The function needs a third parameter to let the caller know if
 * the element was stored in the buffer or not. Possible values for
 * msg are:
 * - BUF_SUCCESS: element was stored in the buffer
 * - BUF_BLOCKED: buffer was blocked by another thread,
 *                element has not been added.
 * - BUF_FULL:    no space left in the buffer, element
 *                has not been added.               
 * RETURNS: pointer to the (modified) buffer.
 */

extern snet_buffer_t *SNetBufTryPut( snet_buffer_t *buf, void* elem, snet_buffer_msg_t *msg); 




/*
 * Returns an element from the buffer if possible.
 * This function will NOT block (returns immediately).
 * The function needs a second parameter to let the caller know
 * what happened.
 * Possible values for msg are:
 * - BUF_SUCCESS: element is returned
 * - BUF_BLOCKED: buffer was blocked by another thread,
 *                no element is returned.
 * - BUF_EMPTY:   no elements in the buffer, no
 *                element is returned.               
 * RETURNS: an element or NULL pointer.
 */

extern void *SNetBufTryGet( snet_buffer_t *buf, snet_buffer_msg_t *msg);



/*
 * Returns element from the buffer _without_ deleting it.
 * 
 * RETURNS: an element or NULL
 */
 
extern void *SNetBufShow( snet_buffer_t *buf);




/*
 * RETURNS: the overall capacity of the buffer.
 */

extern unsigned int SNetBufGetCapacity( snet_buffer_t *bf);



/*
 * This function does not lock the buffer!
 * RETURNS: the remaining capacity.
 */

extern unsigned int SNetGetSpaceLeft( snet_buffer_t *bf);





/*
 * Replaces the internal mutex and the condition variable,
 * which is used to signal that there is a new item (SNetBufPut) available,
 * with the mutex and cond.var. of the dispatcher.
 */

extern void SNetBufRegisterDispatcher( snet_buffer_t *buf, snet_ex_sem_t *sem);




/*
 * This function blocks, until the buffer is empty
 */

extern void SNetBufBlockUntilEmpty( snet_buffer_t *bf);





/*
 * Deletes the buffer. All memory is free'd.
 * The caller has to assure that there are no
 * blocked threads left waiting to put/get elements
 * or undefined behaviour will arise.
 * RETURNS: nothing
 */

extern void SNetBufDestroy( snet_buffer_t *bf);

extern void SNetBufDestroyByDispatcher( snet_buffer_t *bf);


#endif


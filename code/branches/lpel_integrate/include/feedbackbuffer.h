/*
 * $Id$
 */

/*  
 * buffer.h
 * This implements a buffer and its accessor functions.   
 */



#ifndef FEEDBACKBUFFER_H
#define FEEDBACKBUFFER_H

#include <pthread.h>
#include <semaphore.h>
#include "bool.h"
#include "extended_semaphore.h"
typedef enum fbckbufmessage snet_fbckbuffer_msg_t;
typedef struct fbckbuffer snet_fbckbuffer_t;

enum fbckbufmessage
{
	FBCKBUF_fatal = -1,
	FBCKBUF_success,
	FBCKBUF_blocked,
	FBCKBUF_empty
};




/*
 * Allocates memory for the buffer and initializes
 * the datastructure. 
 * RETURNS: pointer to the buffer on success or NULL
 *          otherwise. 
 */

extern snet_fbckbuffer_t *SNetFbckBufCreate();

extern bool SNetFbckBufIsEmpty(snet_fbckbuffer_t *buf);

extern void SNetFbckBufBlockUntilDataReady(snet_fbckbuffer_t *buf);


/*
 * Puts the pointer given as second parameter in the buffer.
 * This function will block until succeeded.
 * Possible reasons for blocking are:
 * - buffer is in use by another thread
 * RETURNS: pointer to the modified buffer.
 */

extern snet_fbckbuffer_t *SNetFbckBufPut( snet_fbckbuffer_t *buf, void* elem);




/*
 * Returns an element of the buffer. 
 * This function will block until succeeded.
 * Possible reasons for blocking are:
 * - buffer is in use by another thread
 * - no elements in the buffer
 * RETURNS: pointer to the element.  
 */

extern void *SNetFbckBufGet( snet_fbckbuffer_t *buf); 


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

extern void *SNetFbckBufTryGet( snet_fbckbuffer_t *buf, snet_fbckbuffer_msg_t *msg);



/*
 * Returns element from the buffer _without_ deleting it.
 * 
 * RETURNS: an element or NULL
 */
 
extern void *SNetFbckBufShow( snet_fbckbuffer_t *buf);




/*
 * Replaces the internal mutex and the condition variable,
 * which is used to signal that there is a new item (SNetBufPut) available,
 * with the mutex and cond.var. of the dispatcher.
 */

extern void SNetFbckBufRegisterDispatcher( snet_fbckbuffer_t *buf, snet_ex_sem_t *sem);




/*
 * This function blocks, until the buffer is empty
 */

extern void SNetFbckBufBlockUntilEmpty( snet_fbckbuffer_t *bf);





/*
 * Deletes the buffer. All memory is free'd.
 * The caller has to assure that there are no
 * blocked threads left waiting to put/get elements
 * or undefined behaviour will arise.
 * RETURNS: nothing
 */

extern void SNetFbckBufDestroy( snet_fbckbuffer_t *bf);

extern void SNetFbckBufDestroyByDispatcher( snet_fbckbuffer_t *bf);


#endif


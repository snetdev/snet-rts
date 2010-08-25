/*
 * $Id$
 */

#ifndef _SNET_BUFFER_H_
#define _SNET_BUFFER_H_


/*
 * buffer.h
 * This implements a buffer and its accessor functions.
 */




#include <pthread.h>
#include <semaphore.h>
#include "bool.h"
#include "stream_layer.h"
#include "extended_semaphore.h"

typedef struct buffer  snet_buffer_t;

extern unsigned int SNetBufSize( snet_buffer_t *bf);
extern bool SNetBufIsEmpty(snet_buffer_t *buf);

/*
 * Allocates memory for the buffer and initializes
 * the datastructure. 
 * This function creates a buffer with 'sizes'
 * spaces for pointers to elements. 
 * RETURNS: pointer to the buffer on success or NULL
 *          otherwise. 
 */

extern snet_buffer_t *SNetBufCreate( unsigned int size, pthread_mutex_t *lock); 


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
 * Returns element from the buffer _without_ deleting it.
 * 
 * RETURNS: an element or NULL
 */
 
extern void *SNetBufShow( snet_buffer_t *buf);


extern int SNetBufRegisterDispatcher( snet_buffer_t *buf, snet_ex_sem_t *sem);

extern int SNetBufUnregisterDispatcher( snet_buffer_t *buf, snet_ex_sem_t *sem);


/*
 * Deletes the buffer. All memory is free'd.
 * The caller has to assure that there are no
 * blocked threads left waiting to put/get elements
 * or undefined behaviour will arise.
 * RETURNS: nothing
 */

extern void SNetBufDestroy( snet_buffer_t *bf);


extern bool SNetBufGetFlag(snet_buffer_t *buf);
  
extern void SNetBufSetFlag(snet_buffer_t *buf, bool flag);

#endif /* _SNET_BUFFER_H_ */


/* $Id$ */

/** <!-- ****************************************************************** -->
* @file threading.h
*
* @brief Thread Handling for S-Net Runtime System.
*
* This implements thread handling for the S-Net Runtime System. Currently,
* each thread is a separate pthread. Stack sizes of new threads are 
* determined by an entity_id. For stack sizes see threading.c.
*
******************************************************************************/

#ifndef _threading_h_
#define _threading_h_

/**
 * snet_entity_id_t type
 */
typedef enum {
	ENTITY_parallel_nondet,
	ENTITY_parallel_det,
	ENTITY_star_nondet,
	ENTITY_star_det,
	ENTITY_split_nondet,
	ENTITY_split_det,
	ENTITY_box,
	ENTITY_sync,
	ENTITY_filter,
	ENTITY_collect_nondet,
	ENTITY_collect_det,
	ENTITY_none
} snet_entity_id_t;
 


/** <!-- ****************************************************************** -->
* @brief Spawns a new thread
*
* This functions spawns a pthread. The stack size of the thread is determined
* by the ID of the component. For default stack size set ID to ENTITY_none.
*
* @param fun     Function (component) to be executed by the new thread
* @param fun_arg Argument to the function
* @param id      ID of component 
* 
******************************************************************************/

extern void SNetThreadCreate( void *(*fun)(void*),
                              void *fun_arg,
  	          snet_entity_id_t id);


#endif /* _threading_h_ */					 


#ifdef DISTRIBUTED_SNET

/** <!--********************************************************************-->
 * $Id$
 *
 * @file fun.c
 *
 * @brief Implements system to map S-Net startup-functions to IDs.
 *
 *        These IDs are used to name functions so that different
 *        in remote operation between nodes.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 13.1.2009
 *
 *****************************************************************************/

#include <string.h>

#include "fun.h"
#include "memfun.h"

/** <!--********************************************************************-->
 *
 * @name Fun
 *
 * <!--
 * int SNetDistFunRegisterLibrary(const char *libname, snet_fun2id_fun_t fun2id, snet_id2fun_fun_t id2fun) : Registers a library
 * void SNetDistFunCleanup() : Frees all recources hold by the libraries.
 * int SNetDistFunFun2ID(snet_startup_fun_t fun, snet_fun_id_t *id) : Map function to id.
 * int SNetDistFunID2Fun(snet_fun_id_t *id, snet_startup_fun_t *fun) : Map id to function.
 * -->
 *
 *****************************************************************************/
/*@{*/

/** @struct fun_table_entry_t;
 *
 *   @brief Type to hold a library data.
 *
 */

typedef struct fun_table_entry {
  char libname[SNET_LIBNAME_MAX_LENGTH];  /**< Name if the library. */
  snet_fun2id_fun_t fun2id;               /**< A function to map fun -> id. */
  snet_id2fun_fun_t id2fun;               /**< A function to map id -> fun. */
  struct fun_table_entry *next;           /**< Next library. */
} fun_table_entry_t;

static fun_table_entry_t *fun_table = NULL;


/** <!--********************************************************************-->
 *
 * @fn  int SNetDistFunRegisterLibrary(const char *libname, snet_fun2id_fun_t fun2id, snet_id2fun_fun_t id2fun) 
 *
 *   @brief Registers new library to ID system.
 *
 *          After a succesfull call the startup-functions of the library
 *          can be mapped into IDs. 
 *
 *          Name of the library must be at most SNET_LIBNAME_MAX_LENGTH - 1 
 *          characters.
 *
 *   @param libname  Name of the library.
 *   @param fun2id   Function to map startup-function to id.
 *   @param id2fun   Function to map id to startup-function.
 *
 *   @return         SNET_FUN_SUCCESS in case of success, SNET_FUN_ERROR otherwise
 *
 ******************************************************************************/

int SNetDistFunRegisterLibrary(const char *libname, snet_fun2id_fun_t fun2id, snet_id2fun_fun_t id2fun) 
{
  fun_table_entry_t *new;

  new = SNetMemAlloc(sizeof(fun_table_entry_t));

  if(strlen(libname) == SNET_LIBNAME_MAX_LENGTH - 1) {
    return SNET_FUN_ERROR;
  }

  strcpy(new->libname, libname);
  new->fun2id = fun2id;
  new->id2fun = id2fun;
  new->next = fun_table;
  fun_table = new;

  return SNET_FUN_SUCCESS;
}


/** <!--********************************************************************-->
 *
 * @fn  void SNetDistFunCleanup() 
 *
 *   @brief Frees resources allocated by calls to SNetDistFunRegisterLibrary.
 *
 ******************************************************************************/

void SNetDistFunCleanup() 
{
  fun_table_entry_t *next;

  while(fun_table != NULL) {  
    next = fun_table;
    fun_table = fun_table->next;

    SNetMemFree(next);
  }
}


/** <!--********************************************************************-->
 *
 * @fn  int SNetDistFunFun2ID(snet_startup_fun_t fun, snet_fun_id_t *id) 
 *
 *   @brief  Maps functions to IDs.
 *
 *
 *   @note  Only functions that belong to regitered library can be mapped!
 *
 *   @param fun  Pointer to function to be mapped.
 *   @param id   Address into which the id is written.
 *
 *   @return     SNET_FUN_SUCCESS in case of success, SNET_FUN_UNKNOWN otherwise
 *
 ******************************************************************************/
int SNetDistFunFun2ID(snet_startup_fun_t fun, snet_fun_id_t *id) 
{
  fun_table_entry_t *next = fun_table;
  int fun_id;

  while(next != NULL) {
    fun_id = next->fun2id(fun);
    if(fun_id != -1) {
      strcpy(id->lib, next->libname);
      id->id = fun_id;
      return SNET_FUN_SUCCESS;
    }

    next = next->next;
  }
  
  return SNET_FUN_UNKNOWN;
}


/** <!--********************************************************************-->
 *
 * @fn  int SNetDistFunID2Fun(snet_fun_id_t *id, snet_startup_fun_t *fun) 
 *
 *   @brief  Maps IDs to functions.
 *
 *   @note  Only functions that belong to regitered library can be mapped!
 *
 *   @param id   ID of the function.
 *   @param fun  Address into which the function pointer is written.
 *
 *   @return     SNET_FUN_SUCCESS in case of success, SNET_FUN_UNKNOWN otherwise
 *
 ******************************************************************************/

int SNetDistFunID2Fun(snet_fun_id_t *id, snet_startup_fun_t *fun) 
{
  fun_table_entry_t *next = fun_table;
  snet_startup_fun_t temp_fun;
  
  while(next != NULL) {
    if(strcmp(next->libname, id->lib) == 0) {
      temp_fun = next->id2fun(id->id);
      if(temp_fun != NULL) {
	*fun = temp_fun;
	return SNET_FUN_SUCCESS;
      }
      return SNET_FUN_ERROR;
    }
    next = next->next;
  }
  
  return SNET_FUN_UNKNOWN;
}

/*@}*/

#endif /* DISTRIBUTED_SNET */

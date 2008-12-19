#ifdef DISTRIBUTED_SNET
#include <string.h>

#include "fun.h"
#include "memfun.h"

/* TODO: 
 *
 * - The same system could later be used to save and transfer libraries to other nodes! 
 */

typedef struct fun_table_entry {
  char libname[SNET_LIBNAME_MAX_LENGTH];
  snet_fun2id_fun_t fun2id;
  snet_id2fun_fun_t id2fun;
  struct fun_table_entry *next;
} fun_table_entry_t;

static fun_table_entry_t *fun_table = NULL;


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

void SNetDistFunCleanup() 
{
  fun_table_entry_t *next;

  while(fun_table != NULL) {  
    next = fun_table;
    fun_table = fun_table->next;

    SNetMemFree(next);
  }
}


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
#endif /* DISTRIBUTED_SNET */

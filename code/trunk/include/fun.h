#ifndef _SNET_FUN_H_
#define _SNET_FUN_H_

#include "snettypes.h"

#define SNET_LIBNAME_MAX_LENGTH 32

#define SNET_FUN_SUCCESS 0
#define SNET_FUN_UNKNOWN -1
#define SNET_FUN_ERROR -2

typedef struct {
  char lib[SNET_LIBNAME_MAX_LENGTH];
  int fun_id;
} snet_fun_id_t;

typedef int (*snet_fun2id_fun_t)(snet_startup_fun_t);
typedef snet_startup_fun_t (*snet_id2fun_fun_t)(int);

/* TODO: The same system could later be used to transfer libraries to other nodes! */

int SNetDistFunRegisterLibrary(const char *libname, 
			       snet_fun2id_fun_t fun2id, 
			       snet_id2fun_fun_t id2fun);

void SNetDistFunCleanup();

int SNetDistFunFun2ID(snet_startup_fun_t fun, snet_fun_id_t *id);

int SNetDistFunID2Fun(snet_fun_id_t *id, snet_startup_fun_t *fun);

#endif /* _SNET_FUN_H_ */

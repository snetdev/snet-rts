#ifndef INTERFACE_FUNCTIONS_HEADER
#define INTERFACE_FUNCTIONS_HEADER

#include <stdio.h>
#include "bool.h"
#include "stdlib.h"
#include "pthread.h"
#include "record.h"

typedef struct {
  int id;
  void (*freefun)( void*);
  void* (*copyfun)( void*);
  int (*serialisefun)( FILE *, void*);
  void* (*deserialisefun)( FILE *);
  int (*encodefun)( FILE *, void*);
  void* (*decodefun)( FILE *);
} snet_global_interface_functions_t;

typedef struct {
  int num;
  snet_global_interface_functions_t **interface;
} snet_global_info_structure_t;

/* hkr: TODO: consider if this can be removed *somehow*? */
extern snet_global_info_structure_t *snet_global;

#ifdef DBG_RT_TRACE_THREAD_CREATE
  int tcount = 0;
  pthread_mutex_t *t_count_mtx;
  
  int SNetGlobalGetThreadCount();
  void SNetGlobalIncThreadCount();
  void SNetGlobalLockThreadMutex();
  void SNetGlobalUnlockThreadMutex();
#endif

bool SNetGlobalRegisterInterface( int id, 
				  void (*freefun)( void*),
				  void* (*copyfun)( void*), 
				  int (*serialisefun)( FILE *, void*),
				  void* (*deserialisefun)( FILE *),
				  int (*encodefun)( FILE *, void*),
				  void* (*decodefun)( FILE *));


void SNetGlobalSetFreeFun(snet_global_interface_functions_t *f, 
                        void (*freefun) (void*));
void* SnetGlobalGetFreeFun(snet_global_interface_functions_t *f);

void SNetGlobalSetCopyFun(snet_global_interface_functions_t *f,
                        void* (*copyfun) (void *));
void* SNetGlobalGetCopyFun(snet_global_interface_functions_t *f);

void SNetGlobalSetSerializationFun(snet_global_interface_functions_t *f,
                        int (*serialisefun) (FILE *, void*));
void* SNetGlobalGetSerializationFun(snet_global_interface_functions_t *f);

void SNetGlobalSetDeserializationFun(snet_global_interface_functions_t *f,
                        void* (*deserialisefun) (FILE *));

void *SNetGlobalGetDeserializationFun(snet_global_interface_functions_t *f);

void SNetGlobalSetEncodingFun(snet_global_interface_functions_t *f,
			      int (*encodefun) (FILE *, void*));
void* SNetGlobalGetEncodingFun(snet_global_interface_functions_t *f);

void SNetGlobalSetDecodingFun(snet_global_interface_functions_t *f,
			      void* (*decodefun) (FILE *));

void *SNetGlobalGetDecodingFun(snet_global_interface_functions_t *f);

void *SNetGetFreeFunFromRec(snet_record_t *rec);
void *SNetGetCopyFunFromRec(snet_record_t *rec);
void *SNetGetSerializationFunFromRec(snet_record_t *rec);
void *SNetGetDeserializationFunFromRec(snet_record_t *rec);
void *SNetGetEncodingFunFromRec(snet_record_t *rec);
void *SNetGetDecodingFunFromRec(snet_record_t *rec);

void SNetGlobalSetId(snet_global_interface_functions_t *f, int id);
int SNetGlobalGetId(snet_global_interface_functions_t *f);

snet_global_interface_functions_t *SNetGlobalGetInterface(int id);
bool SNetGlobalRuntimeInitialised();

bool SNetGlobalInitialise();
void SNetGlobalDestroy();

void *SNetGetCopyFun(int id);
void *SNetGetFreeFun(int id);
void *SNetGetSerializationFun(int id);
void *SNetGetDeserializationFun(int id);
void *SNetGetEncodingFun(int id);
void *SNetGetDecodingFun(int id);
#endif

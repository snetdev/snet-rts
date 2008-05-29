/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   4.4.2008
 * -----
 *
 * Description:
 * ------------
 *
 * SNet observer implementation
 *
 *******************************************************************************/


#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <observers.h>
#include <snetentities.h>
#include <record.h>
#include <typeencode.h>
#include <buffer.h>
#include <memfun.h>
#include <bool.h>
#include <constants.h>
#include "dispatcher.h"

/* These are used to provide instance specific IDs for observers */
static int id_pool = 0;
static pthread_mutex_t id_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static snetin_label_t *labels = NULL;
static snetin_interface_t *interfaces = NULL;

/* */
typedef struct {
  snet_buffer_t *inbuf;  // Buffer for incoming records
  snet_buffer_t *outbuf; // Buffer for outgoibg records
  int id;                // Instance specific ID of this observer
  int fdesc;             // File descriptor provided by the dispatcher at register time
  const char *code;      // Code attribute
  const char *position;  // Code attribute
  char type;             // Type of the observer (before/after)
  bool interactive;      // Is this observer interactive
  char data_level;       // Data level used
}observer_handle_t;

/* This function produces observer message from record. 
 *
 * TODO: 
 * - Checks for buf overflow
 * - Other record types than data and terminate
 *
 */

static int recToBuffer(observer_handle_t *handle, snet_record_t *rec, char *buf, int buflen){
  int k, i;
  char *label = NULL;
  char *interface = NULL;

  for(i = 0; i < buflen; i++){
    buf[i] = '\0';
  }

  sprintf(buf + strlen(buf),"<?xml version=\"1.0\"?>");
  sprintf(buf + strlen(buf),"<data xmlns=\"snet-home.org\">");
  sprintf(buf + strlen(buf),"<observer oid=\"%d\" position=\"%s\" type=\"", handle->id, handle->position);

  if(handle->type == SNET_OBSERVERS_TYPE_BEFORE) {
    sprintf(buf + strlen(buf),"before\" ");
  } else if(handle->type == SNET_OBSERVERS_TYPE_AFTER){
    sprintf(buf + strlen(buf),"after\" ");
  }

  if(handle->code != NULL){
    sprintf(buf + strlen(buf),"code=\"%s\" ", handle->code);
  }

  sprintf(buf + strlen(buf),">");
  
  switch(SNetRecGetDescriptor( rec)) {

  case REC_data:
    sprintf(buf + strlen(buf),"<record type=\"data\" >");

    /* fields */
    for(k=0; k<SNetRecGetNumFields( rec); k++) {
      i = SNetRecGetFieldNames( rec)[k];
      char *data = NULL;
      
      int id = SNetRecGetInterfaceId(rec);
      
      int (*fun)(void *, char **) = SNetGetSerializationFun(id);
      
      int len = fun(SNetRecGetField(rec, i), &data);
      
      if((label = SNetInIdToLabel(labels, i)) != NULL){
	int l = 0;
	if(handle->data_level == SNET_OBSERVERS_DATA_LEVEL_FULL 
	   && (interface = SNetInIdToInterface(interfaces, id)) != NULL){
	  sprintf(buf + strlen(buf),"<field label=\"%s\" interface=\"%s\" >", label, interface);
	  
	  int ind = strlen(buf);
	  for(l = 0; l < len; l++){
	    buf[ind + l] = data[l];
	  }
	  sprintf(buf + strlen(buf),"</field>");
	}else{
	  sprintf(buf + strlen(buf),"<field label=\"%s\" />", label);
	}
      }
      
      SNetMemFree(data);	 
      SNetMemFree(label);
    }

    /* tags */
    for(k=0; k<SNetRecGetNumTags( rec); k++) {
      i = SNetRecGetTagNames( rec)[k];
      
      if((label = SNetInIdToLabel(labels, i)) != NULL){
	if(handle->data_level == SNET_OBSERVERS_DATA_LEVEL_NONE) {
	  sprintf(buf + strlen(buf),"<tag label=\"%s\" />", label);
	}else {
	  sprintf(buf + strlen(buf),"<tag label=\"%s\" >%d</tag>", label, SNetRecGetTag(rec, i));	   
	}
      }
      
      SNetMemFree(label);
    }

    /* btags */
    for(k=0; k<SNetRecGetNumBTags( rec); k++) {
      i = SNetRecGetBTagNames( rec)[k];
      
      if((label = SNetInIdToLabel(labels, i)) != NULL){
	if(handle->data_level == SNET_OBSERVERS_DATA_LEVEL_NONE) {
	sprintf(buf + strlen(buf),"<btag label=\"%s\" />", label); 
	}else {
	sprintf(buf + strlen(buf),"<btag label=\"%s\" >%d</btag>", label, SNetRecGetBTag(rec, i));
	}
      }
      
      SNetMemFree(label);
    }
    sprintf(buf + strlen(buf),"</record>");
    break;
  case REC_sync:
    sprintf(buf + strlen(buf),"<record type=\"sync\" />");
  case REC_collect: 
    sprintf(buf + strlen(buf),"<record type=\"collect\" />");
  case REC_sort_begin: 
    sprintf(buf + strlen(buf),"<record type=\"sort_begin\" />");
  case REC_sort_end: 
    sprintf(buf + strlen(buf),"<record type=\"sort_end\" />");
  case REC_terminate:
    sprintf(buf + strlen(buf),"<record type=\"terminate\" />");
    break;
  default:
    break;
  }
  sprintf(buf + strlen(buf),"</observer></data>");

  return strlen(buf);;
}


#define BUF_SIZE 4096

/* Function used by the observer thread. */
static void *ObserverBoxThread( void *hndl) {

  observer_handle_t *hnd = (observer_handle_t*)hndl; 
  char buf[BUF_SIZE];
  snet_record_t *rec = NULL;

  bool isTerminated = false;
  int len = 0;
 
  /* Do until terminate record is processed. */
  while(!isTerminated){ 
    rec = SNetBufGet(hnd->inbuf);
    if(rec != NULL) {
      
      /* Produce message. */
      len = 0;
      len = recToBuffer(hnd, rec, buf, BUF_SIZE);
      
      /* Send message. */
      SNetDispatcherSend(hnd->id, hnd->fdesc, buf, len, hnd->interactive);
      
      if(SNetRecGetDescriptor(rec) == REC_terminate){
	isTerminated = true;
      }
      
      /* Pass the record to next component. */
      SNetBufPut(hnd->outbuf, rec);
    }
  } 

  /* Unregister observer */
  SNetDispatcherRemove(hnd->id, hnd->fdesc);

  SNetBufBlockUntilEmpty(hnd->outbuf);
  SNetBufDestroy(hnd->outbuf);

  SNetMemFree(hnd);

  return NULL;
}

#undef BUF_SIZE

/* Function used to initialize a observer component. */
snet_buffer_t *SNetObserverBox(snet_buffer_t *inbuf, const char *addr, int port, bool interactive, 
			       const char *position, char type, char data_level, const char *code)
{
  pthread_t box_thread;
  observer_handle_t *hnd;

  hnd = SNetMemAlloc(sizeof(observer_handle_t));
  if(hnd == NULL){
    SNetMemFree(hnd);
    return inbuf;
  }

  hnd->inbuf = inbuf;
  hnd->outbuf  = SNetBufCreate( BUFFER_SIZE);

  pthread_mutex_lock(&id_pool_mutex);
  hnd->id = id_pool++;
  pthread_mutex_unlock(&id_pool_mutex);

  /* Register observer */
  hnd->fdesc = SNetDispatcherAdd(hnd->id, addr, port, interactive);

  /* If observer couldn't be registered, it can be ignored! */
  if(hnd->fdesc == -1){
    SNetMemFree(hnd);
    return inbuf;
  }

  hnd->type = type;

  hnd->interactive = interactive;
  hnd->position = position;
  hnd->data_level = data_level;
  hnd->code = code;

  pthread_create(&box_thread, NULL, ObserverBoxThread, (void*)hnd);
  pthread_detach(box_thread);

  return(hnd->outbuf);
}

/* This function is used to initialize the observer system. */
int SNetObserverInit(snetin_label_t *labs, snetin_interface_t *interfs) {
  labels = labs;
  interfaces = interfs;  

  /* Nothing to do but start the dispatcher. */
  return SNetDispatcherInit();
}

/* This function is used to destroy te observer system. */
void SNetObserverDestroy() {
  SNetDispatcherDestroy();

  pthread_mutex_destroy(&id_pool_mutex);

  return;
}







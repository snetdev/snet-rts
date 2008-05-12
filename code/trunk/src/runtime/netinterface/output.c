/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   10.1.2008
 * -----
 *
 * Description:
 * ------------
 *
 * Output functions for S-NET network interface.
 *
 *******************************************************************************/

#include <stdio.h>
#include <memfun.h>
#include <pthread.h>
#include <output.h>
#include <snetentities.h>
#include <label.h>
#include <interface.h>

/* Thread to do the output */
static pthread_t thread; 
static snetin_label_t *labels = NULL;
static snetin_interface_t *interfaces = NULL;


/* This function prints records to stdout */
static void printRec(snet_record_t *rec)
{
  int k = 0;
  int i = 0;
  char *label = NULL;
  printf("<data mode=\"textual\" xmlns=\"snet-home.org\">");
  if( rec != NULL) {
    switch( SNetRecGetDescriptor( rec)) {
    case REC_data:
      printf("<record type=\"data\" >");

       /* Fields */
       for( k=0; k<SNetRecGetNumFields( rec); k++) {
	 i = SNetRecGetFieldNames( rec)[k];
	 char *data = NULL;

	 int id = SNetRecGetInterfaceId(rec);

	 int (*fun)(void *, char **) = SNetGetSerializationFun(id);

	 int len = fun(SNetRecGetField(rec, i), &data);

	 if((label = SNetInIdToLabel(labels, i)) != NULL){
	   int l = 0;
	   printf("<field label=\"%s\" interface=\"%s\">", label, 
	   	  SNetInIdToInterface(interfaces, id));
	   for(l = 0; l < len; l++){
	     putchar(data[l]);
	   }
	   printf("</field>");
	 }else{
	   /* Error: unknown label! */
	 }
	 
	 SNetMemFree(data);	 
	 SNetMemFree(label);
       }

       /* Tags */
       for( k=0; k<SNetRecGetNumTags( rec); k++) {
	 i = SNetRecGetTagNames( rec)[k];

	 if((label = SNetInIdToLabel(labels, i)) != NULL){
	   printf("<tag label=\"%s\">%d</tag>", label, SNetRecGetTag(rec, i));	   
	 }else{
	   /* Error: unknown label! */
	 }

	 SNetMemFree(label);
       }

       /* BTags */
       for( k=0; k<SNetRecGetNumBTags( rec); k++) {
	 i = SNetRecGetBTagNames( rec)[k];

	 if((label = SNetInIdToLabel(labels, i)) != NULL){
	   printf("<btag label=\"%s\">%d</btag>", label, SNetRecGetBTag(rec, i)); 
	 }else{
	   /* Error: unknown label! */
	 }

	 SNetMemFree(label);
       }
       printf("</record>");
       break;
    case REC_sync: /* TODO: What additional data is needed? */
      printf("<record type=\"sync\" />");
    case REC_collect: /* TODO: What additional data is needed? */
      printf("<record type=\"collect\" />");
    case REC_sort_begin: /* TODO: What additional data is needed? */
      printf("<record type=\"sort_begin\" />");
    case REC_sort_end: /* TODO: What additional data is needed? */
      printf("<record type=\"sort_end\" />");
    case REC_terminate:
      printf("<record type=\"terminate\" />");
      break;
    default:
      break;
    }
  }
  printf("</data>\n");
}

/* This is output function for the output thread */
static void *doOutput(void* data)
{
  snet_buffer_t * out_buf = (snet_buffer_t *)data;

  snet_record_t *rec = NULL;
  if(out_buf != NULL){
    while(1){
      rec = SNetBufGet(out_buf);
      if(rec != NULL) {
      	printRec(rec);
	if(SNetRecGetDescriptor(rec) == REC_terminate){
	  SNetRecDestroy(rec);
	  break;
	}
	SNetRecDestroy(rec);
      }
    }
  }
  return NULL;
}

int SNetInOutputInit(snetin_label_t *labs, snetin_interface_t *interfs, snet_buffer_t *in_buf){
  if(in_buf == NULL || pthread_create(&thread, NULL, (void *)doOutput, (void *)in_buf) == 0){
    labels = labs;
    interfaces = interfs;
    return 0;
  }
  /* error */
  return 1;
}

int SNetInOutputDestroy(){
  if(pthread_join(thread, NULL) == 0){
    return 0;
  }
  /* Error */
  return 1;
}

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

struct output{
  /* Label mappings to use for output */
  snetin_label_t *labels;
  
  /* Interface mappings to use for output */
  snetin_interface_t *interfaces;
  
  /* Thread to do the output */
  pthread_t thread; 
  
}output;

/* This function prints records to stdout */
static void printRec(snet_record_t *rec)
{
  int k = 0;
  int i = 0;
  char *label = NULL;
  printf("<data mode=\"textual\" xmlns=\"snet.feis.herts.ac.uk\">");
  if( rec != NULL) {
    switch( SNetRecGetDescriptor( rec)) {
    case REC_data:
      printf("<record type=\"data\" >");
       for( k=0; k<SNetRecGetNumFields( rec); k++) {
	 i = SNetRecGetFieldNames( rec)[k];
	 char *data = NULL;

	 int id = SNetRecGetInterfaceId(rec);

	 int (*fun)(void *, char **) = SNetGetSerializationFun(id);

	 int len = fun(SNetRecGetField(rec, i), &data);

	 if((label = SNetInSearchLabelByIndex(output.labels, i)) != NULL){
	   int l = 0;
	   printf("<field label=\"%s\" interface=\"%s\">", label, 
	   	  SNetInIdToInterface(output.interfaces, id));
	   for(l = 0; l < len; l++){
	     putchar(data[l]);
	   }
	   printf("</field>");
	 }else{
	   // TODO: Error, unknown field!
	 }
	 
	 SNetMemFree(data);	 
	 SNetMemFree(label);
       }
       for( k=0; k<SNetRecGetNumTags( rec); k++) {
	 i = SNetRecGetTagNames( rec)[k];

	 if((label = SNetInSearchLabelByIndex(output.labels, i)) != NULL){
	   printf("<tag label=\"%s\">%d</tag>", label, SNetRecGetTag(rec, i));	   
	 }else{
	   // TODO: Error, unknown tag!
	 }

	 SNetMemFree(label);
       }
       for( k=0; k<SNetRecGetNumBTags( rec); k++) {
	 i = SNetRecGetBTagNames( rec)[k];

	 if((label = SNetInSearchLabelByIndex(output.labels, i)) != NULL){
	   printf("<btag label=\"%s\">%d</btag>", label, SNetRecGetBTag(rec, i)); 
	 }else{
	   // TODO: Error, unknown btag!
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

void SNetInOutputInit(snetin_label_t *labels, snetin_interface_t *interfaces){
  output.labels = labels;
  output.interfaces = interfaces;
}

int SNetInOutputBegin(snet_buffer_t *in_buf){
  if(pthread_create(&output.thread, NULL, (void *)doOutput, (void *)in_buf) == 0){
    return 0;
  }
  // error
  return 1;
}

int SNetInOutputBlockUntilEnd(){
  if(pthread_join(output.thread, NULL) == 0){
    return 0;
  }
  //error
  return 1;
}

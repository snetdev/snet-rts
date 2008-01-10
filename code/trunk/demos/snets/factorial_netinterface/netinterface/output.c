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

/* Label mappings to use for output */
label_t *output_labels = NULL;

/* Interface mappings to use for output */
interface_t *output_interfaces = NULL;

/* Thread to do the output */
pthread_t thread; 

/* This function prints records to stdout */
static void outputRec(label_t * labels, interface_t *interfaces, snet_buffer_t *b, snet_record_t *rec)
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

	 data = serialize(interfaces, id, SNetRecGetField(rec, i));
    
	 if((label = searchLabelByIndex(labels, i)) != NULL){
	   printf("<field label=\"%s\" interface=\"%s\">%s</field>", label, idToInterface(interfaces, id), data);
	 }

	 SNetMemFree(data);	 
	 SNetMemFree(label);
       }
       for( k=0; k<SNetRecGetNumTags( rec); k++) {
	 i = SNetRecGetTagNames( rec)[k];
	 if((label = searchLabelByIndex(labels, i)) != NULL){
	   printf("<tag label=\"%s\">%d</tag>", label, SNetRecGetTag(rec, i));	   
	 }
	 SNetMemFree(label);
       }
       for( k=0; k<SNetRecGetNumBTags( rec); k++) {
	 i = SNetRecGetBTagNames( rec)[k];
	 if((label = searchLabelByIndex(labels, i)) != NULL){
	   printf("<btag label=\"%s\">%d</btag>", label, SNetRecGetBTag(rec, i)); 
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
static void *output(void* data)
{
  snet_buffer_t * out_buf = (snet_buffer_t *)data;

  snet_record_t *rec = NULL;
  if(out_buf != NULL){
    while(1){
      rec = SNetBufGet(out_buf);
      if(rec != NULL) {
      	outputRec(output_labels, output_interfaces, out_buf, rec);
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

void initOutput(label_t *labels, interface_t *interfaces){
  output_labels = labels;
  output_interfaces = interfaces;

}

int startOutput(snet_buffer_t *in_buf){
  if(pthread_create(&thread, NULL, (void *)output, (void *)in_buf) != 0){
    return 1;
    // error
  }
  return 0;
}

int blockUntilEndOfOutput(){
  if(pthread_join(thread, NULL) != 0){
    //Error!
    return 1;
  }
  return 0;
}

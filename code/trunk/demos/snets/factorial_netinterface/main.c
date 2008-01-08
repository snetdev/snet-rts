#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>
#include "parser.h"
#include "label.h"

/*** This might change according to the interface */
#include <C2SNet.h>
/**************************************************/

/*** Is the file name always the same? */
#include <myfuns.h>
/*********************************/

/*** THIS NEEDS TO BE GENERATED (directly to factorial.h ?)*/
#include "factorial.h"
/*
#define NUMBER_OF_LABELS 9

char *static_labels[NUMBER_OF_LABELS] = {"F_none", "F__factorial__x", 
					  "F__factorial__p","F__factorial__xx",
					  "F__factorial__r","F__factorial__rr",
					  "T__factorial__T","T__factorial__F", 
					  "T__factorial__stop"};
*/

#define NUMBER_OF_LABELS 6

char *static_labels[NUMBER_OF_LABELS] = {"F_none", "F__factorial__x", 
					 "F__factorial__p","T__factorial__T",
					 "T__factorial__F","T__factorial__stop"};

/*********************************/


/* Current problems and TODO:
 *
 * - how the extra names should be deleted?
 *   - ref counting
 * - how and where this code should be generated?
 *   - compiler seems to be obvious choice
 * - error conditions?
 *   - parser.y, main.c
 * - other control records than terminate?
 *   - what additional data is needed? (or are these needed at all?)
 * - serialization/deserialization functions?
 *   - are they ok
 *   - should they be added to runtime like copy/free functions
 * - how should the different interfaces be handled?
 *   - switch clauses checking the interface
 *
 */

label_t *global_labels = NULL;

/* This could be put in another file */
void printRec(label_t * labels, snet_buffer_t *b, snet_record_t *rec)
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
	 if(i < labels->number_of_labels){
	   /*** This might change according to the interface */
	   data = myserialize(C2SNet_cdataGetData(SNetRecGetField(rec, i)));
	   /**************************************************/
	 }
	 else{
	   data = (char *)C2SNet_cdataGetData(SNetRecGetField(rec, i));
	 }
	 
	 if((label = searchLabelByIndex(labels, i)) != NULL){
	   printf("<field label=\"%s\" interface=\"C2SNet\">%s</field>", label, data);
	 }
       	 if(i < labels->number_of_labels){
	   SNetMemFree(data);
	 }
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

void initialize() {

  SNetGlobalInitialise();

  /*** This might change according to the interface */
  C2SNet_init( 0);
  /**************************************************/

  global_labels = initLabels(static_labels, NUMBER_OF_LABELS);

}



void cleanup() {
  freeLabels(global_labels);
}

void *output(void* data)
{
  snet_buffer_t * out_buf = (snet_buffer_t *)data;

  snet_record_t *rec = NULL;
  if(out_buf != NULL){
    while(1){
      rec = SNetBufGet(out_buf);
      if(rec != NULL) {
      	printRec(global_labels, out_buf, rec);
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


int main(int argc, char* argv[])
{
  int inBufSize = 10;

  snet_buffer_t *in_buf = SNetBufCreate(inBufSize);
  snet_buffer_t *out_buf = NULL;

  pthread_t thread; 

  initialize();

  /*** THIS NEEDS TO BE GENERATED */
  out_buf = SNet__factorial___factorial(in_buf);
  /********************************/
  
  if(pthread_create(&thread, NULL, (void *)output, (void *)out_buf) != 0){
    return 1;
    // error
  }

  /*** Are the function names always the same? */
  parserInit(in_buf, global_labels, &mydeserialize, &myfree, &mycopy);
  /*********************************************/

  int i = PARSE_CONTINUE;
  while(i != PARSE_TERMINATE) {
    i = parserParse();
  }
  
  if(in_buf != NULL){
    SNetBufBlockUntilEmpty(in_buf);
    SNetBufDestroy(in_buf);
  }
  
  parserDelete();

  if(pthread_join(thread, NULL) != 0){
    //Error!
    return 1;
  }

  cleanup();          
  
  return 0;
}

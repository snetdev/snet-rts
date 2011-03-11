/*******************************************************************************
 *
 * $Id: output.c 2891 2010-10-27 14:48:34Z dlp $
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
#include <pthread.h>
#include <unistd.h>

#include "memfun.h"
#include "output.h"
#include "record_p.h"
#include "snetentities.h"
#include "label.h"
#include "interface.h"
#include "bool.h"
#include "debug.h"

#include "lpelif.h"
#include "distribution.h"


typedef struct { 
  FILE *file;
  snetin_label_t *labels;
  snetin_interface_t *interfaces;
  lpel_stream_t *buffer;
} handle_t;


/* This function prints records to stdout */
static void printRec(snet_record_t *rec, handle_t *hnd)
{
  int k = 0;
  int i = 0;
  char *label = NULL;
  char *interface = NULL;
  snet_record_mode_t mode;

  /* Change this to redirect the output! */

  if( rec != NULL) {

    fprintf(hnd->file, "<?xml version=\"1.0\" ?>");

    switch( SNetRecGetDescriptor( rec)) {
    case REC_data:

      mode = SNetRecGetDataMode(rec);
      
      if(mode == MODE_textual) {
	fprintf(hnd->file, "<record xmlns=\"snet-home.org\" type=\"data\" mode=\"textual\" >");
      }else {
	fprintf(hnd->file, "<record xmlns=\"snet-home.org\" type=\"data\" mode=\"binary\" >");
      }

      /* Fields */
      for( k=0; k<SNetRecGetNumFields( rec); k++) {
	i = SNetRecGetFieldNames( rec)[k];
	
	int id = SNetRecGetInterfaceId(rec);
	
	if((label = SNetInIdToLabel(hnd->labels, i)) != NULL){
	  if((interface = SNetInIdToInterface(hnd->interfaces, id)) != NULL) {
	    fprintf(hnd->file, "<field label=\"%s\" interface=\"%s\">", label, 
		    interface);
	    
	    if(mode == MODE_textual) { 
	      SNetInterfaceGet(id)->serialisefun(hnd->file, SNetRecGetField(rec, i));
	    }else {
	      SNetInterfaceGet(id)->encodefun(hnd->file, SNetRecGetField(rec, i));
	    }
	    
	    fprintf(hnd->file, "</field>");
	    SNetMemFree(interface);
	  }

	  SNetMemFree(label);
	}else{
	  SNetUtilDebugFatal("Unknown field %d at output!", i);
	}
	
      }
      
       /* Tags */
      for( k=0; k<SNetRecGetNumTags( rec); k++) {
	i = SNetRecGetTagNames( rec)[k];
	
	if((label = SNetInIdToLabel(hnd->labels, i)) != NULL){
	  fprintf(hnd->file, "<tag label=\"%s\">%d</tag>", label, SNetRecGetTag(rec, i));	   
	}else{
	  SNetUtilDebugFatal("Unknown tag %d at output!", i);
	}
	
	SNetMemFree(label);
      }
      
      /* BTags */
      for( k=0; k<SNetRecGetNumBTags( rec); k++) {
	i = SNetRecGetBTagNames( rec)[k];
	
	if((label = SNetInIdToLabel(hnd->labels, i)) != NULL){
	  fprintf(hnd->file, "<btag label=\"%s\">%d</btag>", label, SNetRecGetBTag(rec, i)); 
	}else{
	  SNetUtilDebugFatal("Unknown binding tag %d at output!", i);
	}
	
	SNetMemFree(label);
      }
      fprintf(hnd->file, "</record>");
      break;
    case REC_sync: 
      SNetUtilDebugFatal("REC_synch in output! This should not happen.");
      break;
    case REC_collect: 
      SNetUtilDebugFatal("REC_collect in output! This should not happen.");
      //SNetUtilDebugFatal("Output of REC_collect not yet implemented!");
      //fprintf(hnd->file, "<record type=\"collect\" />");
      break;
    case REC_sort_end:
      SNetUtilDebugFatal("REC_sort_end in output! This should not happen.");
      //SNetUtilDebugFatal("Output of REC_sort_end not yet implemented!");
      //fprintf(hnd->file, "<record type=\"sort_end\" />");
      break;
    case REC_trigger_initialiser:
      SNetUtilDebugFatal("REC_trigger_initializer in output! This should not happen.");
      //SNetUtilDebugFatal("Output of REC_trigger_initializer not yet implemented!");
      //fprintf(hnd->file, "<record type=\"trigger_initialiser\" />");
      break;
    case REC_terminate:
      fprintf(hnd->file, "<record type=\"terminate\" />");
      break;
    default:
      break;
    }
  }

  fflush(hnd->file);
}

/**
 * This is the task doing the global output
 */
static void GlobOutputTask( lpel_task_t *self, void* data)
{
  bool terminate = false;
  handle_t *hnd = (handle_t *)data;
  snet_record_t *rec = NULL;

  if(hnd->buffer != NULL) {
    lpel_stream_desc_t *instream = LpelStreamOpen( self, hnd->buffer, 'r');

    while(!terminate){
      rec = LpelStreamRead( instream);
      if(rec != NULL) {
        switch(SNetRecGetDescriptor(rec)) {
        case REC_sync:
          {
            lpel_stream_t *newstream = (lpel_stream_t*) SNetRecGetStream(rec);
            LpelStreamReplace( instream, newstream);
            hnd->buffer = newstream;
          }
          break;
        case REC_data:
        case REC_collect:
        case REC_sort_end:
        case REC_trigger_initialiser:
          printRec(rec, hnd);
          break;
        case REC_terminate:
          printRec(rec, hnd);
          terminate = true;
          SNetDistribStop();
          break;
        default:
          break;
        }

        SNetRecDestroy(rec);
      }
    }

    fflush(hnd->file);
    fprintf(hnd->file, "\n");

    LpelStreamClose( instream, true);
  }
  SNetMemFree(hnd);

  /* signal termination to workers */
  LpelWorkerTerminate();
}

void SNetInOutputInit(FILE *file,
                     snetin_label_t *labels,
                     snetin_interface_t *interfaces,
                     snet_stream_t *out_buf
  )
{
  handle_t *hnd = SNetMemAlloc(sizeof(handle_t));

  hnd->file = file;
  hnd->labels = labels;
  hnd->interfaces = interfaces;
  hnd->buffer = (lpel_stream_t *) out_buf;

  /* create a joinable wrapper thread */
  SNetLpelIfSpawnWrapper( GlobOutputTask, (void*)hnd, "glob_output");
}
